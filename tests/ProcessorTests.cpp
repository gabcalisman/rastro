#include "Processor.h"
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
                    if (auto *combo = dynamic_cast<juce::ComboBox *>(child))
                        combo->setSelectedId(2, juce::sendNotificationSync);
                check(restored.language == 1, "language selector");
                render("editor-preview-en.png", 1100, 920);
                editor->setSize(1600, 1200);
                editor->createComponentSnapshot(editor->getLocalBounds());
            }
        }
    std::cout << "PASS: WAV/AIFF mono/stereo, MIDI offsets, release, state/sample/rack/language/CC "
                 "roundtrip, mutation bounds, invalid input, editor renders at min/max sizes\n";
}
