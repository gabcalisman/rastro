#include "Performance.h"
#include "Processor.h"
#include "SampleView.h"
#include <cstdlib>
#include <iostream>
#include <new>
static bool inAudio = false;
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
    const char *visualIds[]{"visualTheme", "visualPalette", "controlStyle",
                            "avatarBody",  "avatarEyes",    "avatarAccessory"};
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
        check(restored.value(id) == 2, "visual customization restored independently");
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
                render("editor-preview.png", 900, 860);
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
    std::cout << "PASS: legacy migration, six-module state/order, XY mouse/keyboard/CC, exact "
                 "undo/session history, reactive/frozen avatar; WAV/AIFF mono/stereo, MIDI "
                 "offsets, release, state/sample/rack/language/CC "
                 "roundtrip, mutation bounds, invalid input, editor renders at min/max sizes\n";
}
