#include "Customizer.h"
#include "Editor.h"
#include "Performance.h"
#include "Processor.h"
#include "SampleView.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
static bool inAudio = false;
bool imageDiffers(const juce::Image &, const juce::Image &);
void *operator new(std::size_t n)
{
    if (inAudio)
        throw std::runtime_error("processor audio allocation");
    if (auto *p = std::malloc(n))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
#include <stdexcept>
void check(bool ok, const char *m)
{
    if (!ok)
        throw std::runtime_error(m);
}
juce::MemoryBlock makeAudio(bool mono, bool aiff)
{
    juce::MemoryBlock bytes;
    auto out = std::make_unique<juce::MemoryOutputStream>(bytes, false);
    std::unique_ptr<juce::AudioFormatWriter> writer;
    if (aiff)
    {
        juce::AiffAudioFormat f;
        writer.reset(f.createWriterFor(out.release(), 48000, mono ? 1 : 2, 16, {}, 0));
    }
    else
    {
        juce::WavAudioFormat f;
        writer.reset(f.createWriterFor(out.release(), 48000, mono ? 1 : 2, 16, {}, 0));
    }
    juce::AudioBuffer<float> b(mono ? 1 : 2, 48000);
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample(c, i,
                        .5f * std::exp(-i / 12000.f) *
                            std::sin(float(i * 440 * 6.283185307 / 48000)));
    check(writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples()), "write fixture");
    writer.reset();
    return bytes;
}
float process(RastroProcessor &p, int channels, int on = -1, int off = -1)
{
    juce::AudioBuffer<float> b(channels, 512);
    juce::MidiBuffer midi;
    if (on >= 0)
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), on);
    if (off >= 0)
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), off);
    inAudio = true;
    p.processBlock(b, midi);
    inAudio = false;
    float e = 0;
    for (int c = 0; c < channels; ++c)
        for (int i = 0; i < 512; ++i)
        {
            float v = b.getSample(c, i);
            check(std::isfinite(v) && std::abs(v) <= 1, "bounded audio");
            if (on > 0 && i < on)
                check(v == 0, "sample accurate note on");
            e += v * v;
        }
    return e;
}
juce::MouseEvent mouse(juce::Component &c, juce::Point<float> point, int clicks = 1)
{
    auto time = juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(),
            point,
            juce::ModifierKeys::leftButtonModifier,
            1,
            0,
            0,
            0,
            0,
            &c,
            &c,
            time,
            point,
            time,
            clicks,
            false};
}
void sampleEditingTests()
{
    for (auto filename : {"mono.mp3", "stereo.mp3"})
    {
        RastroProcessor p;
        p.prepareToPlay(48000, 512);
        auto file = juce::File(RASTRO_TEST_DATA_DIR).getChildFile(filename);
        check(p.loadFile(file), "MP3 imported through the instrument");
        auto *sample = p.source.load();
        check(sample && sample->size() > 8000, "decoded MP3 source");
        p.set("start", .15f);
        p.set("end", .85f);
        p.setPlaybackMode(4);
        p.equalSlices(5);
        p.set("slice2", .37f);
        check(process(p, 2, 15) > 0, "MP3 MIDI slice playback");
        juce::MemoryBlock state;
        p.getStateInformation(state);
        RastroProcessor restored;
        restored.prepareToPlay(48000, 512);
        restored.setStateInformation(state.getData(), (int)state.getSize());
        check(restored.sampleName() == filename && restored.source.load()->size() == sample->size(),
              "MP3 bytes restore without original file");
        check(restored.source.load()->data == sample->data,
              "MP3 restoration decodes identical PCM offline");
        check(std::abs(restored.value("start") - .15f) < 1e-6 &&
                  std::abs(restored.value("end") - .85f) < 1e-6 &&
                  restored.value("playMode") == 4 && restored.value("sliceCount") == 5 &&
                  std::abs(restored.value("slice2") - .37f) < 1e-6,
              "trim/mode/manual slices state roundtrip");
        restored.setPlaybackMode(2);
        restored.previewRequest = 1;
        check(process(restored, 2) > 0 && restored.engine.previewActive,
              "selected-region preview works independently of Reverse/Slice mode");
        restored.previewRequest = -1;
        for (int i = 0; i < 12; ++i)
            process(restored, 2);
        check(!restored.engine.previewActive, "preview stops/releases");
        restored.setPlaybackMode(0);
        restored.set("start", .1f);
        restored.set("end", .9f);
        std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
        SampleView *view = nullptr;
        for (auto *child : editor->getChildren())
            if (auto *candidate = dynamic_cast<SampleView *>(child))
                view = candidate;
        check(view != nullptr, "interactive waveform exists");
        auto point = [&](float v)
        { return juce::Point<float>(8 + v * (view->getWidth() - 16), 15); };
        view->mouseDown(mouse(*view, point(.1f)));
        view->mouseDrag(mouse(*view, point(.2f)));
        view->mouseUp(mouse(*view, point(.2f)));
        check(std::abs(restored.value("start") - .2f) < .001f, "direct start trim drag");
        view->mouseDown(mouse(*view, point(.9f)));
        view->mouseDrag(mouse(*view, point(.75f)));
        view->mouseUp(mouse(*view, point(.75f)));
        check(std::abs(restored.value("end") - .75f) < .001f, "direct end trim drag");
        restored.setPlaybackMode(4);
        restored.equalSlices(4);
        float cut = .2f + (.75f - .2f) * .25f;
        float target = .2f + (.75f - .2f) * .3f;
        view->mouseDown(mouse(*view, point(cut)));
        view->mouseDrag(mouse(*view, point(target)));
        view->mouseUp(mouse(*view, point(target)));
        check(std::abs(restored.value("slice1") - .3f) < .002f, "manual slice marker drag");
        auto *drop = dynamic_cast<juce::FileDragAndDropTarget *>(editor.get());
        check(drop && drop->isInterestedInFileDrag({file.getFullPathName()}), "MP3 drop accepted");
        check(!drop->isInterestedInFileDrag({"/tmp/image.png"}), "non-audio drop rejected");
        auto other =
            juce::File(RASTRO_TEST_DATA_DIR)
                .getChildFile(juce::String(filename) == "mono.mp3" ? "stereo.mp3" : "mono.mp3");
        drop->filesDropped({other.getFullPathName()}, 50, 50);
        for (int i = 0; i < 400 && restored.sampleName() != other.getFileName(); ++i)
            juce::Thread::sleep(5);
        check(restored.sampleName() == other.getFileName(),
              "file drop completes asynchronous import");
    }
}
void recordingImportTests()
{
    using E = rastro::ImportError;
    for (auto filename :
         {"voice-aac-mono.m4a", "voice-aac-mono48.m4a", "voice-aac-stereo.m4a",
          "voice-alac-mono16.m4a", "voice-alac-mono24.m4a", "voice-alac-stereo16.m4a"})
    {
        auto file = juce::File(RASTRO_TEST_DATA_DIR).getChildFile(filename);
        juce::MemoryBlock bytes;
        check(file.loadFileAsData(bytes), "M4A codec fixture available");
        auto info = rastro::inspectM4a(bytes.getData(), bytes.getSize());
        check(info.error == E::none && (info.aac != info.alac),
              "rendered AAC/ALAC container inspected");
        RastroProcessor p;
        p.prepareToPlay(48000, 512);
        check(p.loadFile(file), "M4A imported offline from file");
        auto *sample = p.source.load();
        check(sample && sample->size() > 10000 && sample->size() < 18000, "M4A duration decoded");
        float peak = 0;
        for (float v : sample->waveform)
        {
            check(std::isfinite(v), "M4A waveform finite");
            peak = std::max(peak, v);
        }
        check(peak > .08f && peak < .6f, "M4A nonempty waveform with bounded level");
        for (auto &channel : sample->data)
            for (float v : channel)
                check(std::isfinite(v) && std::abs(v) < .6f, "M4A decoded PCM finite and bounded");
        p.set("drywet", 0);
        check(process(p, 2, 47) > 0, "M4A MIDI playback and sample accurate onset");
        process(p, 2, -1, 30);
        for (int i = 0; i < 40; ++i)
            process(p, 2);
        check(process(p, 2) < 1e-9f, "One Shot recording reaches end after note-off");
        p.previewRequest = 1;
        check(process(p, 2) > 0 && p.engine.previewActive, "M4A audition");
        p.previewRequest = -1;
        for (int i = 0; i < 12; ++i)
            process(p, 2);
        check(!p.engine.previewActive, "M4A audition stops");
        p.set("start", .1f);
        p.set("end", .9f);
        p.setPlaybackMode(1);
        juce::MemoryBlock state;
        p.getStateInformation(state);
        RastroProcessor restored;
        restored.prepareToPlay(48000, 512);
        restored.setStateInformation(state.getData(), (int)state.getSize());
        check(restored.source.load() && restored.source.load()->data == sample->data,
              "embedded M4A restores identical PCM offline without original file path");
        check(restored.sampleName() == filename && restored.value("playMode") == 1 &&
                  std::abs(restored.value("start") - .1f) < 1e-6f,
              "M4A trim/mode/name session roundtrip");
        check(process(restored, 2, 0) > 0, "restored M4A MIDI");
        check(p.importData(bytes.getData(), bytes.getSize() / 2, "truncated.m4a") == E::corrupt,
              "truncated M4A atom rejected");
        check(p.source.load() == sample, "failed recording import preserves current sample");
        auto bad = bytes;
        auto *data = static_cast<char *>(bad.getData());
        const char *codec = info.aac ? "mp4a" : "alac";
        bool patched = false;
        for (size_t i = 4; i + 4 < bad.getSize(); ++i)
            if (std::memcmp(data + i, codec, 4) == 0)
            {
                std::memcpy(data + i, "zzzz", 4);
                patched = true;
                break;
            }
        check(patched &&
                  rastro::inspectM4a(bad.getData(), bad.getSize()).error == E::incompatibleCodec,
              "unknown M4A codec distinguished from corrupt container");
        bad = bytes;
        std::memcpy(static_cast<char *>(bad.getData()) + 8, "qt  ", 4);
        check(rastro::inspectM4a(bad.getData(), bad.getSize()).error == E::editableMovie,
              "QuickTime branded file prompts rendered audio even if renamed M4A");
        check(p.importData(bytes.getData(), bytes.getSize(), "editable.mov") == E::editableMovie,
              "editable QuickTime is not decoded");
        check(p.importMessage(E::editableMovie).contains(".m4a"),
              "QuickTime message explains rendered M4A export");
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        auto *drop = dynamic_cast<juce::FileDragAndDropTarget *>(editor.get());
        check(drop && drop->isInterestedInFileDrag({file.getFullPathName()}), "M4A drag accepted");
        check(drop->isInterestedInFileDrag({"/tmp/voice-editable.mov"}),
              "QuickTime drag reaches diagnostic");
        check(!drop->isInterestedInFileDrag({file.getFullPathName(), file.getFullPathName()}),
              "multiple-file drop rejected");
        auto *ui = dynamic_cast<RastroEditor *>(editor.get());
        auto baseline = makeAudio(true, false);
        check(p.loadData(baseline.getData(), baseline.getSize(), "previous.wav"),
              "previous UI waveform loaded");
        ui->synchroniseUi();
        auto refreshes = ui->uiMetrics().sampleRefreshes;
        auto *beforePicker = p.source.load();
        // Same worker entry point used after the native chooser returns a selected path.
        check(!p.loadData(nullptr, 0, "empty.wav"), "empty import fails");
        p.importFileAsync(file, [](E) {});
        for (int i = 0; i < 2000 && p.source.load() == beforePicker; ++i)
            juce::Thread::sleep(5);
        check(p.source.load() != beforePicker && p.source.load()->data == sample->data,
              "chooser selected path uses successful asynchronous import");
        auto *fromChooser = p.source.load();
        ui->synchroniseUi();
        check(ui->uiMetrics().sampleRefreshes == refreshes + 1,
              "imported waveform/name refresh at rest without requiring MIDI or full UI repaint");
        drop->filesDropped({file.getFullPathName()}, 40, 40);
        for (int i = 0; i < 2000 && p.source.load() == fromChooser; ++i)
            juce::Thread::sleep(5);
        check(p.source.load() != fromChooser && p.sampleName() == filename,
              "M4A drag/drop worker completes");
        check(p.source.load()->data == sample->data,
              "drop and chooser backend decode identical PCM");
    }
    RastroProcessor p;
    check(p.importFile(juce::File("/tmp/missing-recording.m4a")) == E::unreadable,
          "missing file diagnosed");
    check(p.importFile(juce::File("/tmp/editable.qt")) == E::editableMovie,
          "QuickTime extension diagnosed before read");
    unsigned char malformed[24]{};
    malformed[0] = 0xff;
    check(rastro::inspectM4a(malformed, sizeof malformed).error == E::corrupt,
          "oversized atom rejected without overread");
    std::cout
        << "PASS import: AAC-LC mono/stereo, ALAC mono16/mono24/stereo16, offline PCM/state, MIDI, "
           "preview, waveform, asynchronous drop, corrupt/codec/QuickTime errors\n";
}

void hardwareAndPitchTests()
{
    auto bytes = makeAudio(true, false);
    for (int mode = 0; mode < 5; ++mode)
    {
        RastroProcessor p;
        p.prepareToPlay(48000, 512);
        check(p.loadData(bytes.getData(), bytes.getSize(), "deck.wav"), "deck fixture");
        p.setPlaybackMode(mode);
        p.set("pitchMode", 1);
        p.set("gFeedback", .9f);
        SampleDeck deck(p);
        deck.setSize(176, 140);
        auto still = deck.motion().angle;
        deck.update(2);
        check(!deck.motion().playing && deck.motion().angle == still,
              "deck stays still without notes");
        process(p, 2, 0);
        deck.update(2);
        check(deck.motion().playing && deck.motion().angle != still && deck.motion().level > 0,
              "MIDI/audio drives reels and needle");
        auto full = deck.motion().angle;
        process(p, 2);
        deck.update(1);
        check(deck.motion().angle - full > 0 && deck.motion().angle - full < .041f,
              "reduced motion turns reels more slowly");
        auto frozen = deck.motion().angle;
        process(p, 2);
        deck.update(0);
        check(deck.motion().angle == frozen,
              "motion off freezes reels while meter stays operational");
        process(p, 2, -1, 0);
        for (int i = 0; i < 7; ++i)
        {
            process(p, 2);
            deck.update(2);
        }
        check(deck.motion().playing == (mode == 0),
              "deck obeys actual One Shot versus gated note-off semantics");
        for (int i = 0; i < 120; ++i)
        {
            process(p, 2);
            deck.update(2);
        }
        check(!deck.motion().playing,
              "reels stop when voices finish, independently of effect tails");
        auto stopped = deck.motion().angle;
        for (int i = 0; i < 12; ++i)
        {
            process(p, 2);
            deck.update(2);
        }
        check(deck.motion().angle == stopped, "no decorative reel spin after playback");
        p.previewRequest = 1;
        process(p, 2);
        deck.update(2);
        check(deck.motion().playing, "audition drives reels too");
        for (int i = 0; i < 20; ++i)
            deck.update(2);
        check(!deck.motion().playing, "host callback pause stops stale reel animation");
        for (int i = 0; i < 180; ++i)
        {
            float previous = deck.motion().level;
            bool changed = deck.update(2);
            if (previous > 0 && deck.motion().level == 0)
                check(changed,
                      "meter repaints its final silent readout rather than retaining stale dBFS");
        }
        check(deck.motion().level == 0 && !deck.update(2),
              "meter settles without endless idle repaints");
        auto held = deck.motion().angle;
        deck.resume();
        deck.update(2);
        check(!deck.motion().playing && deck.motion().angle == held,
              "reopening the panel waits for fresh audio blocks");
    }
    RastroProcessor p;
    p.prepareToPlay(48000, 512);
    check(p.loadData(bytes.getData(), bytes.getSize(), "pitch.wav"), "pitch state fixture");
    p.set("pitchMode", 1);
    p.set("cosmicArcade", 1);
    p.set("colourSet", 4);
    p.mutate();
    check(p.value("pitchMode") == 1, "mutation preserves pitch choice");
    juce::MemoryBlock state;
    p.getStateInformation(state);
    RastroProcessor restored;
    restored.setStateInformation(state.getData(), (int)state.getSize());
    check(restored.settings().pitchMode == 1 && restored.value("cosmicArcade") == 1 &&
              restored.value("colourSet") == 4,
          "pitch and Cosmic Arcade persist without renormalizing colourSet");
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), (int)state.getSize());
    auto tree = juce::ValueTree::fromXml(*xml);
    for (auto id : {"pitchMode", "cosmicArcade"})
        tree.removeChild(tree.getChildWithProperty("id", id), nullptr);
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), state);
    restored.setStateInformation(state.getData(), (int)state.getSize());
    check(restored.settings().pitchMode == 0 && restored.value("cosmicArcade") == 0 &&
              restored.value("colourSet") == 4,
          "older sessions default to REPITCH and retain previous palettes");
    std::cout << "PASS hardware: actual playback, note-off, preview, idle/pause/reduced/off; pitch "
                 "state/migration and Cosmic Arcade\n";
}

void characterStateTests()
{
    RastroProcessor p;
    p.prepareToPlay(48000, 512);
    auto sample = makeAudio(false, false);
    check(p.loadData(sample.getData(), sample.getSize(), "character.wav"), "character sample");
    p.set("order", 3);
    auto initialOrder = p.settings().order;
    p.moveModule(5, 0);
    check(p.settings().order[0] == 5 && p.settings().order[1] == initialOrder[0],
          "move CRUSH across all six slots");
    p.moveModule(0, 5);
    check(p.settings().order == initialOrder, "moving back restores full order");
    p.set("playX", .8f);
    p.set("playY", .65f);
    p.set("animation", 1);
    const auto &visualIds = visuals::visualIds;
    for (auto id : visualIds)
        p.set(id, 2);
    p.set("wDepth", .7f);
    p.set("dCharacter", .3f);
    p.set("kAmount", .55f);
    p.cc[3] = 71;
    p.cc[4] = 72;
    juce::MemoryBlock full;
    p.getStateInformation(full);
    RastroProcessor restored;
    restored.prepareToPlay(48000, 512);
    restored.setStateInformation(full.getData(), (int)full.getSize());
    check(restored.settings().order == initialOrder && restored.cc[3] == 71 && restored.cc[4] == 72,
          "six slot order and XY CC state");
    check(std::abs(restored.value("playX") - .8f) < 1e-6 && restored.value("animation") == 1 &&
              std::abs(restored.value("wDepth") - .7f) < 1e-6,
          "XY, motion preference and new effects state");
    for (auto id : visualIds)
        check(restored.value(id) == p.value(id), "visual customization restored independently");
    // Visual choices must leave the rendered sound bit-identical.
    juce::AudioBuffer<float> a(2, 512), b(2, 512);
    juce::MidiBuffer ma, mb;
    ma.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
    mb.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
    for (auto id : visualIds)
        restored.set(id, 0);
    p.processBlock(a, ma);
    restored.processBlock(b, mb);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            check(a.getSample(c, i) == b.getSample(c, i), "visuals do not change audio");
    check(std::abs(p.engine.noteVelocity.load() - .8f) < .01f, "velocity telemetry");
    // Only active/present modules mutate; one exact snapshot survives session restore.
    restored.set("gOn", 0);
    restored.set("dPresent", 0);
    restored.set("mutation", 1);
    auto old = restored.settings();
    auto originalSource = restored.source.load();
    std::array<std::array<float, 4>, 6> previous{};
    for (int m = 0; m < 6; ++m)
        for (int i = 0; i < 4; ++i)
            if (auto id = rastro::controlIds[m][i])
                previous[m][i] = restored.value(id);
    restored.mutate();
    check(restored.canUndoMutation(), "undo available");
    for (int m = 0; m < 6; ++m)
    {
        float delta = 0;
        for (int i = 0; i < 4; ++i)
            if (auto id = rastro::controlIds[m][i])
                delta += std::abs(previous[m][i] - restored.value(id));
        check(old.enabled[m] ? delta > .05f : delta == 0, "only active effects mutate audibly");
    }
    check(restored.source.load() == originalSource && restored.value("root") == old.root &&
              restored.value("start") == old.start && restored.value("end") == old.end &&
              restored.value("level") == old.level,
          "mutation preserves sample, tuning, trims, output");
    check(restored.settings().order == old.order && restored.value("playX") == old.playX &&
              restored.value("playY") == old.playY,
          "mutation preserves order and PLAY");
    juce::MemoryBlock mutated;
    restored.getStateInformation(mutated);
    RastroProcessor undo;
    undo.prepareToPlay(48000, 512);
    undo.setStateInformation(mutated.getData(), (int)mutated.getSize());
    check(undo.canUndoMutation(), "undo saved with session");
    undo.undoMutation();
    for (int m = 0; m < 6; ++m)
        for (int i = 0; i < 4; ++i)
            if (auto id = rastro::controlIds[m][i])
                check(std::abs(previous[m][i] - undo.value(id)) < 1e-6, "exact mutation undo");
    check(!undo.canUndoMutation(), "single undo consumed");
    // Build a genuine v1-shaped tree, with all appended parameters/properties absent.
    auto xml = juce::AudioProcessor::getXmlFromBinary(full.getData(), (int)full.getSize());
    auto tree = juce::ValueTree::fromXml(*xml);
    tree.removeProperty("stateVersion", nullptr);
    tree.removeProperty("cc3", nullptr);
    tree.removeProperty("cc4", nullptr);
    for (int i = 1; i < 16; ++i)
        tree.removeChild(tree.getChildWithProperty("id", "slice" + juce::String(i)), nullptr);
    for (auto id : {"playMode", "sliceCount"})
        tree.removeChild(tree.getChildWithProperty("id", id), nullptr);
    tree.removeChild(tree.getChildWithName("MutationUndo"), nullptr);
    const char *newIds[]{"rackOrder", "playX",  "playY",      "animation", "wDepth",   "wRate",
                         "wMix",      "dLevel", "dCharacter", "dMix",      "kAmount",  "kMix",
                         "wOn",       "dOn",    "kOn",        "wPresent",  "dPresent", "kPresent"};
    for (auto id : newIds)
    {
        auto node = tree.getChildWithProperty("id", id);
        tree.removeChild(node, nullptr);
    }
    juce::MemoryBlock legacy;
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), legacy);
    // Restore into an already edited instance, not just a fresh one.
    restored.setStateInformation(legacy.getData(), (int)legacy.getSize());
    check(restored.value("wOn") == 0 && restored.value("dOn") == 0 && restored.value("kOn") == 0,
          "v1 sessions bypass all new modules");
    check(restored.value("playX") == .5f && restored.value("playY") == .5f &&
              !restored.canUndoMutation(),
          "v1 state resets appended controls");
    check(restored.settings().order == initialOrder && restored.source.load() != nullptr,
          "v1 order and sample preserved");
    // Learn both XY axes while there is no editor, under the allocation trap.
    for (int axis = 3; axis < 5; ++axis)
    {
        restored.learn = axis;
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 30 + axis, 100 - axis), 50);
        inAudio = true;
        restored.processBlock(buffer, midi);
        inAudio = false;
        restored.applyCC();
        check(restored.cc[axis] == 30 + axis, "XY CC learns");
        check(std::abs(restored.value(rastro::macroIds[axis]) - (100 - axis) / 127.f) < 1e-6,
              "XY CC reaches parameter");
    }
    // All visual states are tied to audio telemetry and can be frozen independently of sound.
    RastroAvatar avatar(p);
    avatar.setSize(280, 220);
    avatar.update(0);
    auto off = avatar.createComponentSnapshot(avatar.getLocalBounds());
    process(p, 2, 0);
    for (int i = 0; i < 12; ++i)
    {
        process(p, 2);
        avatar.update(2);
    }
    auto moving = avatar.createComponentSnapshot(avatar.getLocalBounds());
    auto different = [](const juce::Image &a, const juce::Image &b)
    {
        for (int y = 0; y < a.getHeight(); ++y)
            for (int x = 0; x < a.getWidth(); ++x)
                if (a.getPixelAt(x, y) != b.getPixelAt(x, y))
                    return true;
        return false;
    };
    check(different(off, moving), "avatar responds to actual audio/MIDI/XY telemetry");
    avatar.update(0);
    auto frozen = avatar.createComponentSnapshot(avatar.getLocalBounds());
    process(p, 2);
    avatar.update(0);
    check(!different(frozen, avatar.createComponentSnapshot(avatar.getLocalBounds())),
          "animation off remains static while audio runs");
    PlayPad pad(p);
    pad.setSize(400, 240);
    pad.center();
    auto event = mouse(pad, {374, 30});
    pad.mouseDown(event);
    pad.mouseDrag(event);
    pad.mouseUp(event);
    check(p.value("playX") > .99f && p.value("playY") > .99f, "mouse drives XY axes");
    pad.mouseDoubleClick(mouse(pad, {200, 120}, 2));
    check(p.value("playX") == .5f && p.value("playY") == .5f, "double click centers");
    pad.keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
    check(p.value("playX") > .5f, "keyboard XY accessibility");
}
juce::Component *findComponent(juce::Component &root, const juce::String &id)
{
    if (root.getComponentID() == id)
        return &root;
    for (auto *child : root.getChildren())
        if (auto *found = findComponent(*child, id))
            return found;
    return nullptr;
}
bool imageDiffers(const juce::Image &a, const juce::Image &b)
{
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
            if (a.getPixelAt(x, y) != b.getPixelAt(x, y))
                return true;
    return false;
}
void pumpUi(int ms = 25)
{
    struct Stop : juce::Timer
    {
        void timerCallback() override
        {
            stopTimer();
            juce::MessageManager::getInstance()->stopDispatchLoop();
        }
    } stop;
    stop.startTimer(ms);
    juce::MessageManager::getInstance()->runDispatchLoop();
}
void clickUi(juce::Component &root, const juce::String &id)
{
    auto *button = dynamic_cast<juce::TextButton *>(findComponent(root, id));
    check(button && button->isShowing() && button->isEnabled(),
          "visible enabled button for interaction");
    auto point = button->getLocalBounds().toFloat().getCentre();
    auto inRoot = root.getLocalPoint(button, point.toInt());
    check(root.getComponentAt(inRoot) == button, "button hit target unobscured");
    static_cast<juce::Component *>(button)->mouseDown(mouse(*button, point));
    static_cast<juce::Component *>(button)->mouseUp(mouse(*button, point));
}
void checkUiLayout(juce::Component &root)
{
    std::vector<juce::Component *> leaves;
    std::function<void(juce::Component &)> collect = [&](juce::Component &c)
    {
        if (!c.isShowing() || !c.isEnabled())
            return;
        if (dynamic_cast<juce::Button *>(&c) || dynamic_cast<juce::Slider *>(&c) ||
            dynamic_cast<juce::ComboBox *>(&c))
        {
            auto r = root.getLocalArea(&c, c.getLocalBounds());
            check(root.getLocalBounds().contains(r), "interactive control within window");
            check(r.getHeight() >= 24 && r.getWidth() >= 24, "minimum 24 point hit target");
            leaves.push_back(&c);
            return;
        }
        for (auto *child : c.getChildren())
            collect(*child);
    };
    collect(root);
    for (size_t i = 0; i < leaves.size(); ++i)
        for (size_t j = 0; j < i; ++j)
        {
            auto a = root.getLocalArea(leaves[i], leaves[i]->getLocalBounds()),
                 b = root.getLocalArea(leaves[j], leaves[j]->getLocalBounds());
            if (a.intersects(b))
            {
                std::cerr << "Overlap: " << leaves[i]->getComponentID() << " " << a.toString()
                          << " / " << leaves[j]->getComponentID() << " " << b.toString()
                          << std::endl;
                check(false, "interactive controls do not overlap");
            }
        }
}
void saveUi(juce::Component &c, const char *name, float scale = 1)
{
    auto out = juce::File::getCurrentWorkingDirectory().getChildFile(name).createOutputStream();
    check(out != nullptr, "snapshot output");
    out->setPosition(0);
    out->truncate();
    auto image = c.createComponentSnapshot(c.getLocalBounds(), true, scale);
    check(image.getWidth() == juce::roundToInt(c.getWidth() * scale), "snapshot HiDPI pixel size");
    juce::PNGImageFormat png;
    check(png.writeImageToStream(image, *out), "UI snapshot written");
}
void visualStageTests()
{
    RastroProcessor p;
    p.prepareToPlay(48000, 512);
    auto source = makeAudio(false, false);
    check(p.loadData(source.getData(), source.getSize(), "visual.wav"), "visual fixture");
    p.set("wDepth", 1);
    p.set("wRate", 1);
    p.set("wMix", 1);
    RastroAvatar avatar(p);
    avatar.setSize(300, 300);
    auto luminance = [](juce::Colour c)
    {
        auto linear = [](float v)
        { return v <= .04045f ? v / 12.92f : std::pow((v + .055f) / 1.055f, 2.4f); };
        return .2126f * linear(c.getFloatRed()) + .7152f * linear(c.getFloatGreen()) +
               .0722f * linear(c.getFloatBlue());
    };
    for (int theme = 0; theme < 4; ++theme)
        for (int palette = 0; palette < 4; ++palette)
            for (int bg = 0; bg <= 4; ++bg)
            {
                p.set("visualTheme", (float)theme);
                p.set("visualPalette", (float)palette);
                p.set("visualBackground", (float)bg);
                auto pal = visuals::palette(p);
                for (auto surface : {pal.bg, pal.surface, pal.button})
                {
                    check((luminance(pal.text) + .05f) / (luminance(surface) + .05f) > 7.f,
                          "primary contrast across coordinated surfaces");
                    check((luminance(pal.muted) + .05f) / (luminance(surface) + .05f) > 4.5f,
                          "secondary contrast across coordinated surfaces");
                }
            }
    for (int set = 1; set <= 8; ++set)
        for (int bg = 0; bg <= 4; ++bg)
            for (int ac = 0; ac <= 4; ++ac)
            {
                p.set("colourSet", (float)set);
                p.set("visualBackground", (float)bg);
                p.set("visualAccent", (float)ac);
                auto pal = visuals::palette(p);
                for (auto surface : {pal.bg, pal.surface, pal.button})
                {
                    check(visuals::contrast(pal.text, surface) > 7.f,
                          "new palettes primary ink contrast");
                    check(visuals::contrast(pal.muted, surface) > 4.5f,
                          "new palettes secondary ink contrast");
                }
                check(visuals::contrast(pal.control, pal.button) > 4.5f,
                      "new palette control contrast");
            }
    p.set("colourSet", 0);
    p.set("visualAccent", 0);
    p.set("visualTheme", 0);
    p.set("visualPalette", 0);
    p.set("visualBackground", 0);
    for (int i = 0; i < 150; ++i)
    {
        process(p, 2);
        avatar.update(2);
    }
    auto still = avatar.createComponentSnapshot(avatar.getLocalBounds());
    for (int i = 0; i < 60; ++i)
    {
        process(p, 2);
        avatar.update(2);
    }
    check(!imageDiffers(still, avatar.createComponentSnapshot(avatar.getLocalBounds())),
          "no phantom WOBBLE dance at rest");
    // Direct X/Y response independent of downstream FX, at fixed real audio energy.
    p.engine.meter = .25f;
    p.engine.visualX = .1f;
    p.engine.visualY = .1f;
    for (int i = 0; i < 30; ++i)
        avatar.update(2);
    auto low = avatar.createComponentSnapshot(avatar.getLocalBounds());
    p.engine.visualY = .9f;
    for (int i = 0; i < 30; ++i)
        avatar.update(2);
    auto high = avatar.createComponentSnapshot(avatar.getLocalBounds());
    check(imageDiffers(low, high), "PLAY Y directly changes posture/expression");
    p.engine.visualX = .9f;
    for (int i = 0; i < 30; ++i)
        avatar.update(2);
    check(imageDiffers(high, avatar.createComponentSnapshot(avatar.getLocalBounds())),
          "PLAY X directly changes direction");
    bool crouched = false, jumped = false;
    float previous = 0;
    p.engine.meter = 0;
    p.engine.noteVelocity = 1;
    p.engine.noteSerial.fetch_add(1);
    for (int i = 0; i < 120; ++i)
    {
        if (i > 10 && i < 30)
            p.engine.noteSerial.fetch_add(4); // rapid burst must not freeze one parity/pose
        avatar.update(2);
        auto motion = avatar.motion();
        crouched |= motion.lift < 0;
        jumped |= motion.lift > .05f;
        check(std::isfinite(motion.swing) && std::abs(motion.swing) <= 1 &&
                  std::abs(motion.lift) <= 1,
              "bounded musical springs");
        check(std::abs(motion.lift - previous) < .7f, "smooth impact and recovery");
        previous = motion.lift;
    }
    check(crouched && jumped, "anticipation followed by impact");
    for (int i = 0; i < 180; ++i)
        avatar.update(2);
    check(avatar.motion().activity == 0 && std::abs(avatar.motion().lift) < .001f,
          "motion recovers to rest");
    std::unique_ptr<RastroEditor> editor(new RastroEditor(p));
    editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);
    editor->setVisible(true);
    editor->getPeer()->handleFocusGain();
    auto button = [&](const char *id)
    {
        auto *b = dynamic_cast<juce::TextButton *>(findComponent(*editor, id));
        check(b != nullptr, "button exists");
        return b;
    };
    editor->synchroniseUi();
    checkUiLayout(*editor);
    auto *macro = findComponent(*editor, "parameter_presence");
    check(macro && macro->getHeight() > 120, "large actual macro control");
    p.set("colourSet", 1);
    p.set("avatarAccessory", 1);
    p.set("avatarInstrument", 1);
    p.set("avatarClothes", 1);
    p.set("avatarGlasses", 1);
    p.set("knobDesign", 2);
    editor->synchroniseUi();
    for (int m = 0; m < rastro::moduleCount; ++m)
    {
        auto *slider = dynamic_cast<juce::Slider *>(
            findComponent(*editor, "rackPrimarySlider_" + juce::String(m)));
        check(slider && slider->isShowing(),
              "each rack module has its main control directly accessible");
        float original = p.value(rastro::controlIds[m][0]);
        slider->setValue(.43, juce::sendNotificationSync);
        check(std::abs(p.value(rastro::controlIds[m][0]) - .43f) < .001f,
              "rack main control attached to real DSP parameter");
        p.set(rastro::controlIds[m][0], original);
    }
    auto originalOrder = p.settings().order;
    auto *from = findComponent(*editor, "rack_5"), *to = findComponent(*editor, "rack_0");
    auto target = from->getLocalPoint(to, juce::Point<int>(18, 12)).toFloat();
    auto drag = mouse(*from, {18, 12}).withNewPosition(target);
    from->mouseDrag(drag);
    from->mouseUp(drag);
    editor->synchroniseUi();
    check(p.settings().order[0] == 5 && from->getX() == 20,
          "rack drag order matches actual DSP order");
    p.moveModule(0, 5);
    editor->synchroniseUi();
    check(p.settings().order == originalOrder,
          "rack can be reordered back without losing settings");
    auto *pitch = dynamic_cast<juce::ComboBox *>(findComponent(*editor, "pitchMode"));
    check(pitch && pitch->getTooltip().contains("Slice"),
          "pitch control has separate explanatory tooltip");
    pitch->setSelectedId(2, juce::sendNotificationSync);
    check(p.value("pitchMode") == 1 && p.value("playMode") == 0,
          "pitch mode changes independently of playback mode");
    pitch->setSelectedId(1, juce::sendNotificationSync);
    auto *deck = findComponent(*editor, "sampleDeck"),
         *wave = findComponent(*editor, "sampleWaveform");
    check(deck && deck->getWidth() >= 156 && !deck->getBounds().intersects(wave->getBounds()),
          "prominent deck leaves waveform usable");
    for (int style = 1; style <= 4; ++style)
    {
        p.set("knobDesign", (float)style);
        editor->synchroniseUi();
        saveUi(*editor, ("rastro-hardware-" + juce::String(style) + ".png").toRawUTF8());
    }
    p.set("knobDesign", 2);
    editor->synchroniseUi();
    saveUi(*editor, "rastro-final-standard.png");
    for (int set = 1; set <= 8; ++set)
    {
        p.set("colourSet", (float)set);
        editor->synchroniseUi();
        checkUiLayout(*editor);
        auto pal = visuals::palette(p);
        for (auto id : {"language", "playbackMode", "animationMode"})
        {
            if (auto *combo = dynamic_cast<juce::ComboBox *>(findComponent(*editor, id)))
                for (auto *child : combo->getChildren())
                    if (auto *label = dynamic_cast<juce::Label *>(child))
                        check(visuals::contrast(label->findColour(juce::Label::textColourId),
                                                pal.button) > 4.5f,
                              "actual combo label ink updates for light and dark palettes");
        }
        saveUi(*editor, ("rastro-palette-" + juce::String(set) + ".png").toRawUTF8());
    }
    p.set("cosmicArcade", 1);
    editor->synchroniseUi();
    checkUiLayout(*editor);
    auto cosmic = visuals::palette(p);
    for (auto surface : {cosmic.bg, cosmic.surface, cosmic.button})
    {
        check(visuals::contrast(cosmic.text, surface) > 7 &&
                  visuals::contrast(cosmic.muted, surface) > 4.5f,
              "Cosmic Arcade retains legible primary/secondary ink");
    }
    saveUi(*editor, "rastro-final-cosmic.png");
    p.set("cosmicArcade", 0);
    p.set("colourSet", 5);
    p.set("playX", .79f);
    p.set("playY", .72f);
    for (int i = 0; i < 12; ++i)
    {
        process(p, 2, i % 3 == 0 ? 0 : -1);
        editor->synchroniseUi();
    }
    saveUi(*editor, "rastro-final-performance.png");
    p.set("playX", .5f);
    p.set("playY", .5f);
    p.engine.allOff();
    for (int i = 0; i < 200; ++i)
        process(p, 2);
    for (auto id : visuals::visualIds)
        p.set(id, 0);
    editor->synchroniseUi();
    clickUi(*editor, "compactMode");
    editor->synchroniseUi();
    check(editor->getHeight() == 700 && p.value("uiCompact") == 1,
          "compact mode selected and stored");
    checkUiLayout(*editor);
    check(macro->getHeight() >= 80, "compact keeps macro knobs large");
    saveUi(*editor, "rastro-final-compact.png");
    clickUi(*editor, "customize");
    editor->synchroniseUi();
    auto *sheet = dynamic_cast<VisualCustomizer *>(findComponent(*editor, "visualCustomizer"));
    check(sheet && sheet->isVisible(), "customizer opens with real hit test");
    check(!macro->isEnabled(), "covered controls excluded from keyboard interaction");
    auto focus = sheet->createKeyboardFocusTraverser();
    check(focus->getDefaultComponent(sheet) != nullptr, "sheet keyboard focus traversal");
    checkUiLayout(*editor);
    clickUi(*editor, "paletteChoice2");
    editor->synchroniseUi();
    check(p.value("colourSet") == 3, "whole palette selection");
    check(sheet->appliedPreview().isValid(), "COLOURS contains a real instrument snapshot");
    auto applied = sheet->appliedPreview();
    clickUi(*editor, "paletteChoice3");
    editor->synchroniseUi();
    check(imageDiffers(applied, sheet->appliedPreview()),
          "instrument snapshot updates with palette");
    clickUi(*editor, "undoVisuals");
    editor->synchroniseUi();
    clickUi(*editor, "paletteChoice8");
    editor->synchroniseUi();
    check(p.value("cosmicArcade") == 1, "Cosmic Arcade is selectable in COLOURS");
    saveUi(*editor, "rastro-final-cosmic-customizer.png");
    clickUi(*editor, "undoVisuals");
    editor->synchroniseUi();
    check(p.value("cosmicArcade") == 0 && p.value("colourSet") == 3,
          "Cosmic Arcade choice can be undone");
    saveUi(*editor, "rastro-final-colours.png");
    clickUi(*editor, "undoVisuals");
    check(p.value("colourSet") == 0, "palette undo exact");
    clickUi(*editor, "visualTab1");
    editor->synchroniseUi();
    checkUiLayout(*editor);
    auto before = editor->createComponentSnapshot(editor->getLocalBounds());
    for (int i = 0; i < 4; ++i)
    {
        clickUi(*editor, "knobChoice" + juce::String(i));
        editor->synchroniseUi();
        check(p.value("knobDesign") == i + 1, "knob thumbnail applies live style");
    }
    check(imageDiffers(before, editor->createComponentSnapshot(editor->getLocalBounds())),
          "interactive slider preview changes");
    auto *preview = dynamic_cast<juce::Slider *>(findComponent(*editor, "liveKnobPreview"));
    float soundBefore = p.value("presence");
    preview->setValue(23);
    check(p.value("presence") == soundBefore, "preview gesture never changes audio parameter");
    saveUi(*editor, "rastro-final-knobs.png");
    auto metrics = editor->uiMetrics();
    for (int i = 0; i < 24; ++i)
        editor->synchroniseUi();
    auto after = editor->uiMetrics();
    check(after.layouts == metrics.layouts && after.deckUpdates == metrics.deckUpdates &&
              after.mainAvatar == metrics.mainAvatar &&
              after.previewAvatar == metrics.previewAvatar,
          "knobs sheet has zero background avatar/layout steps");
    clickUi(*editor, "visualTab2");
    checkUiLayout(*editor);
    for (auto id : {"avatarHair", "avatarHorns", "avatarClothes", "avatarGlasses", "avatarTail",
                    "avatarWings", "avatarInstrument", "avatarColour"})
    {
        auto *combo = dynamic_cast<juce::ComboBox *>(findComponent(*editor, id));
        check(combo && combo->isShowing(), "part selector on correct page");
        combo->setSelectedId(3, juce::sendNotificationSync);
    }
    editor->synchroniseUi();
    saveUi(*editor, "rastro-final-grooveiro.png");
    metrics = editor->uiMetrics();
    for (int i = 0; i < 24; ++i)
        editor->synchroniseUi();
    after = editor->uiMetrics();
    check(after.deckUpdates == metrics.deckUpdates && after.mainAvatar == metrics.mainAvatar &&
              after.previewAvatar == metrics.previewAvatar + 24 && after.layouts == metrics.layouts,
          "Grooveiro sheet steps only its visible avatar");
    // Exercise mixed parts, rather than only a single all-options-on mascot.
    for (int body = 0; body < 3; ++body)
        for (int eyes = 0; eyes < 3; ++eyes)
            for (int accessory = 0; accessory < 3; ++accessory)
            {
                p.set("avatarBody", (float)body);
                p.set("avatarEyes", (float)eyes);
                p.set("avatarAccessory", (float)accessory);
                auto image = sheet->createComponentSnapshot(sheet->getLocalBounds());
                check(image.isValid(), "27 mascot part combinations render");
            }
    p.set("avatarBody", 0);
    p.set("avatarEyes", 0);
    p.set("avatarAccessory", 0);
    editor->synchroniseUi();
    // Revert returns all opening choices without touching sound, then can itself be undone.
    clickUi(*editor, "revertVisuals");
    check(p.value("knobDesign") == 0 && p.value("avatarHair") == 0, "opening appearance restored");
    clickUi(*editor, "undoVisuals");
    check(p.value("knobDesign") == 4 && p.value("avatarHair") == 2, "revert itself undoable");
    float depth = p.value("wDepth");
    auto *sample = p.source.load();
    juce::MemoryBlock state;
    p.getStateInformation(state);
    RastroProcessor restored;
    restored.setStateInformation(state.getData(), (int)state.getSize());
    for (auto id : visuals::visualIds)
        check(restored.value(id) == p.value(id), "all appearance fields persist");
    check(restored.value("uiCompact") == 1, "compact preference persists");
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), (int)state.getSize());
    auto oldTree = juce::ValueTree::fromXml(*xml);
    for (auto id :
         {"pitchMode", "cosmicArcade", "colourSet", "knobDesign", "uiCompact", "visualBackground",
          "visualAccent", "visualControls", "avatarColour", "avatarHair", "avatarHorns",
          "avatarClothes", "avatarGlasses", "avatarTail", "avatarWings", "avatarInstrument"})
        oldTree.removeChild(oldTree.getChildWithProperty("id", id), nullptr);
    juce::MemoryBlock oldState;
    juce::AudioProcessor::copyXmlToBinary(*oldTree.createXml(), oldState);
    restored.setStateInformation(oldState.getData(), (int)oldState.getSize());
    check(restored.value("knobDesign") == 0 && restored.value("avatarHair") == 0 &&
              restored.value("uiCompact") == 0,
          "pre-customizer v2 session supplies explicit new defaults");
    check(restored.value("visualPalette") == p.value("visualPalette"),
          "old visual choices retained");
    clickUi(*editor, "resetVisuals");
    for (auto id : visuals::visualIds)
    {
        auto *parameter = p.params.getParameter(id);
        check(p.value(id) == parameter->convertFrom0to1(parameter->getDefaultValue()),
              "visual defaults restored");
    }
    check(p.value("wDepth") == depth && p.source.load() == sample && p.value("uiCompact") == 1,
          "appearance actions preserve audio/sample/window preference");
    sheet->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
    editor->synchroniseUi();
    check(!sheet->isVisible() && macro->isEnabled(), "Escape restores instrument interaction");
    check(button("customize")->hasKeyboardFocus(true), "focus returns to opener");
    static_cast<juce::Component *>(button("customize"))
        ->keyPressed(juce::KeyPress(juce::KeyPress::returnKey));
    pumpUi();
    check(sheet->isVisible(), "keyboard Return opens customizer");
    clickUi(*editor, "visualTab1");
    auto *tab = button("visualTab1");
    tab->grabKeyboardFocus();
    sheet->keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
    check(button("visualTab2")->hasKeyboardFocus(true), "arrow keys switch tabs and focus");
    sheet->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
    clickUi(*editor, "compactMode");
    editor->synchroniseUi();
    checkUiLayout(*editor);
    auto mainBefore = macro->createComponentSnapshot(macro->getLocalBounds());
    p.set("knobDesign", 2);
    editor->synchroniseUi();
    check(imageDiffers(mainBefore, macro->createComponentSnapshot(macro->getLocalBounds())),
          "style changes actual instrument slider geometry");
    editor->setSize(980, 700);
    editor->synchroniseUi();
    checkUiLayout(*editor);
    editor->setSize(1180, 840);
    editor->synchroniseUi();
    p.language = 1;
    editor->synchroniseUi();
    clickUi(*editor, "customize");
    clickUi(*editor, "visualTab2");
    editor->synchroniseUi();
    checkUiLayout(*editor);
    saveUi(*editor, "rastro-final-english.png");
    sheet->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
    p.language = 0;
    editor->synchroniseUi();
    saveUi(*editor, "rastro-final-hidpi.png", 2);
    editor->setSize(1680, 1100);
    editor->synchroniseUi();
    checkUiLayout(*editor);
    editor->removeFromDesktop();
    std::cout << "PASS UI: compact/standard hit testing, focus/keyboard, undo/revert, X/Y/springs, "
                 "contrast, 2x render, no covered animation/layout\n";
}
void noteOffProcessorTests()
{
    for (bool screenKeyboard : {false, true})
        for (int mode = 0; mode < 5; ++mode)
        {
            RastroProcessor p;
            p.prepareToPlay(48000, 512);
            auto sample = makeAudio(true, false);
            check(p.loadData(sample.getData(), sample.getSize(), "noteoff.wav"), "note-off sample");
            p.setPlaybackMode(mode);
            p.equalSlices(2);
            p.set("drywet", 0);
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            if (screenKeyboard)
                p.keyboard.noteOn(1, 60, 1);
            else
                midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.f), 0);
            p.processBlock(buffer, midi);
            midi.clear();
            if (screenKeyboard)
                p.keyboard.noteOff(1, 60, 0);
            else
                midi.addEvent(juce::MidiMessage::noteOff(1, 60), 127);
            p.processBlock(buffer, midi);
            midi.clear();
            for (int i = 0; i < 5; ++i)
                p.processBlock(buffer, midi);
            check(mode == 0 ? buffer.getMagnitude(0, 512) > .001f
                            : buffer.getMagnitude(0, 512) == 0,
                  "MIDI and screen keyboard: only One Shot survives early note-off");
        }
}
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    struct Clock : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setBpm(93.);
            return p;
        }
    } clock;
    noteOffProcessorTests();
    characterStateTests();
    sampleEditingTests();
    recordingImportTests();
    hardwareAndPitchTests();
    {
        RastroProcessor p;
        p.prepareToPlay(48000, 512);
        auto bytes = makeAudio(true, false);
        check(p.loadData(bytes.getData(), bytes.getSize(), "queue.wav"),
              "keyboard overflow fixture");
        p.set("loop", 1);
        p.set("drywet", 0);
        for (int i = 0; i < 300; ++i)
        {
            p.keyboard.noteOn(1, 60, .8f);
            p.keyboard.noteOff(1, 60, 0.f);
        }
        for (int i = 0; i < 120; ++i)
            process(p, 2);
        check(p.engine.meter < 1e-5, "keyboard queue overflow cannot leave a stuck note");
    }
    // Host block sizes and sample-rate changes exercise the complete callback, not just frame().
    for (double rate : {44100., 48000., 96000.})
        for (int size : {1, 17, 64, 511, 1024, 4096})
        {
            RastroProcessor p;
            p.prepareToPlay(rate, size);
            auto bytes = makeAudio(true, false);
            check(p.loadData(bytes.getData(), bytes.getSize(), "blocks.wav"), "block fixture");
            juce::AudioBuffer<float> buffer(1, size);
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, .7f), size / 2);
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), size - 1);
            inAudio = true;
            p.processBlock(buffer, midi);
            inAudio = false;
            for (int i = 0; i < size; ++i)
                check(std::isfinite(buffer.getSample(0, i)) &&
                          std::abs(buffer.getSample(0, i)) <= 1,
                      "mono variable block bounded");
        }
    for (bool mono : {false, true})
        for (bool aiff : {false, true})
        {
            RastroProcessor p;
            p.prepareToPlay(48000, 512);
            p.setPlayHead(&clock);
            auto data = makeAudio(mono, aiff);
            check(p.loadData(data.getData(), data.getSize(), aiff ? "test.aiff" : "test.wav"),
                  "import");
            p.set("drywet", 0);
            p.set("loop", 1);
            check(process(p, mono ? 1 : 2, 127) > 0, "MIDI onset");
            check(p.hostBpm == 93.f, "host tempo");
            process(p, mono ? 1 : 2, -1, 25);
            for (int j = 0; j < 100; ++j)
                process(p, mono ? 1 : 2);
            p.language = 1;
            p.moveModule(0, 2);
            p.set("cPresent", 0);
            p.set("gMix", .78f);
            p.cc[0] = 74;
            juce::MemoryBlock state;
            p.getStateInformation(state);
            RastroProcessor restored;
            restored.prepareToPlay(48000, 512);
            restored.setStateInformation(state.getData(), (int)state.getSize());
            check(restored.source.load() != nullptr, "sample restored");
            check(restored.source.load()->size() == 48000, "sample frames restored");
            check(restored.language == 1 && restored.cc[0] == 74, "language and CC restored");
            check(restored.value("cPresent") == 0 &&
                      std::abs(restored.value("gMix") - .78f) < .001f,
                  "rack parameters restored");
            check(restored.settings().order == p.settings().order, "rack order restored");
            check(process(restored, 2, 0) > 0, "restored playback");
            auto before = restored.source.load();
            restored.mutate();
            check(before == restored.source.load() && restored.value("level") == .7f,
                  "mutation preserves source and output");
            char invalid[]{1, 2, 3};
            check(!restored.loadData(invalid, 3, "bad"), "reject invalid");
            check(before == restored.source.load(), "failed load preserves source");
            // MIDI learn / value is retained without an open editor and serialized.
            restored.learn = 1;
            juce::AudioBuffer<float> b(2, 512);
            juce::MidiBuffer m;
            m.addEvent(juce::MidiMessage::controllerEvent(1, 21, 90), 0);
            restored.processBlock(b, m);
            restored.applyCC();
            check(restored.cc[1] == 21 && std::abs(restored.value("trail") - 90 / 127.f) < .001f,
                  "CC learn");
            if (!mono && !aiff)
            {
                restored.resetEffects();
                restored.set("cPresent", 1);
                restored.language = 0;
                std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
                auto render = [&](const char *filename, int width, int height)
                {
                    editor->setSize(width, height);
                    dynamic_cast<RastroEditor *>(editor.get())->synchroniseUi();
                    auto image = editor->createComponentSnapshot(editor->getLocalBounds());
                    auto out = juce::File::getCurrentWorkingDirectory()
                                   .getChildFile(filename)
                                   .createOutputStream();
                    check(out != nullptr, "snapshot output");
                    out->setPosition(0);
                    out->truncate();
                    juce::PNGImageFormat png;
                    check(png.writeImageToStream(image, *out), "editor render");
                };
                render("editor-preview.png", 1000, 700);
                render("editor-preview-pt.png", 1100, 920);
                for (auto *child : editor->getChildren())
                    if (auto *combo = dynamic_cast<juce::ComboBox *>(child);
                        combo && combo->getComponentID() == "language")
                        combo->setSelectedId(2, juce::sendNotificationSync);
                check(restored.language == 1, "language selector");
                render("editor-preview-en.png", 1100, 920);
                for (int variant = 0; variant < 4; ++variant)
                {
                    restored.set("visualTheme", (float)variant);
                    restored.set("visualPalette", (float)variant);
                    for (auto id : {"controlStyle", "avatarBody", "avatarEyes", "avatarAccessory"})
                        restored.set(id, (float)(variant % 3));
                    render(("editor-visual-" + std::to_string(variant) + ".png").c_str(), 1100,
                           920);
                }
                editor->setSize(1600, 1200);
                editor->createComponentSnapshot(editor->getLocalBounds());
            }
        }
    visualStageTests();
    std::cout << "PASS: legacy migration, six-module state/order, XY mouse/keyboard/CC, exact "
                 "undo/session history, reactive/frozen avatar; WAV/AIFF mono/stereo, MIDI "
                 "offsets, release, state/sample/rack/language/CC "
                 "roundtrip, mutation bounds, invalid input, editor renders at min/max sizes\n";
}
