#include "FxProcessor.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

static void check(bool ok, const char *message)
{
    if (!ok)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
static void set(RastroFxProcessor &p, const char *id, float value)
{
    auto *parameter = p.params.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
static void configure(RastroFxProcessor &p, int channels)
{
    auto layout = p.getBusesLayout();
    layout.inputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
    layout.outputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
    check(p.setBusesLayout(layout), "matching mono/stereo layout accepted");
}
template <typename Sample> static void transparency()
{
    for (double rate : {44100., 48000., 96000.})
        for (int channels : {1, 2})
        {
            RastroFxProcessor p;
            configure(p, channels);
            p.prepareToPlay(rate, 1024);
            check(p.getLatencySamples() == 0 && p.getTailLengthSeconds() >= 0,
                  "zero latency and declared finite tail");
            for (int size : {0, 1, 17, 511, 1024, 4096})
            {
                juce::AudioBuffer<Sample> b(channels, size), original(channels, size);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < size; ++i)
                    {
                        const Sample values[]{Sample(0),
                                              Sample(-0.),
                                              Sample(1.75),
                                              Sample(-2.5),
                                              std::numeric_limits<Sample>::denorm_min(),
                                              Sample(std::sin(i * .317 + c) * 3.)};
                        b.setSample(c, i, values[i % 6]);
                    }
                original.makeCopyOf(b);
                juce::MidiBuffer midi;
                p.processBlock(b, midi);
                for (int c = 0; c < channels; ++c)
                    check(size == 0 || std::memcmp(b.getReadPointer(c), original.getReadPointer(c),
                                                   size * sizeof(Sample)) == 0,
                          "unity is bit-exact, including headroom, silence and subnormals");
            }
        }
}
static void gainAndBypass()
{
    for (bool hostBypass : {false, true})
        for (int channels : {1, 2})
        {
            RastroFxProcessor p;
            configure(p, channels);
            p.prepareToPlay(48000, 64);
            juce::MidiBuffer midi;
            auto render = [&](int size, bool bypassed)
            {
                juce::AudioBuffer<float> b(channels, size);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < size; ++i)
                        b.setSample(c, i, c == 0 ? .25f : -.75f);
                if (bypassed)
                    p.processBlockBypassed(b, midi);
                else
                    p.processBlock(b, midi);
                if (channels == 2)
                    for (int i = 0; i < size; ++i)
                        check(std::abs(b.getSample(1, i) + 3 * b.getSample(0, i)) < 1e-6,
                              "stereo gain linked without crosstalk");
                return b;
            };
            set(p, "fxInputGain", 6);
            float previous = .25f;
            for (int i = 0; i < 1000; ++i)
            {
                float value = render(1, false).getSample(0, 0);
                check(value >= previous && value - previous < .001f, "input automation smooth");
                previous = value;
            }
            check(std::abs(previous - .25 * std::pow(10., 6. / 20.)) < 1e-6, "input gain dB");
            set(p, "fxOutputGain", -12);
            render(1100, false);
            const float active = render(1, false).getSample(0, 0);
            check(std::abs(active - .25 * std::pow(10., -6. / 20.)) < 1e-6, "both gains applied");
            if (!hostBypass)
                set(p, "fxBypass", 1);
            previous = active;
            for (int i = 0; i < 500; ++i)
            {
                float value = render(1, hostBypass).getSample(0, 0);
                check(value >= previous && value - previous < .001f, "bypass smooth toward dry");
                previous = value;
            }
            check(previous == .25f, "settled bypass exactly dry, ignoring gains");
            set(p, "fxInputGain", 24);
            set(p, "fxOutputGain", 24);
            check(render(1500, hostBypass).getSample(0, 1499) == .25f,
                  "bypass automation stays dry");
            set(p, "fxInputGain", 6);
            set(p, "fxOutputGain", -12);
            render(1100, hostBypass);
            set(p, "fxBypass", 0);
            previous = .25f;
            for (int i = 0; i < 500; ++i)
            {
                float value = render(1, false).getSample(0, 0);
                check(value <= previous && previous - value < .001f, "reenable smooth");
                previous = value;
            }
            check(std::abs(previous - active) < 1e-6, "reenable reaches gain");
            // Reverse a transition before it completes.
            set(p, "fxBypass", 1);
            auto first = render(100, false);
            set(p, "fxBypass", 0);
            auto second = render(1, false);
            check(std::abs(first.getSample(0, 99) - second.getSample(0, 0)) < .001f,
                  "rapid bypass reversal has no jump");
        }
}
static void blockPartitioning()
{
    RastroFxProcessor whole, split;
    whole.prepareToPlay(48000, 2048);
    split.prepareToPlay(48000, 17);
    for (auto *p : {&whole, &split})
    {
        set(*p, "fxInputGain", -18);
        set(*p, "fxOutputGain", 12);
        set(*p, "fxBypass", 1);
    }
    juce::AudioBuffer<double> expected(2, 2048);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 2048; ++i)
            expected.setSample(c, i, std::sin(i * .17 + c));
    juce::AudioBuffer<double> actual;
    actual.makeCopyOf(expected);
    juce::MidiBuffer midi;
    whole.processBlock(expected, midi);
    for (int offset = 0; offset < 2048;)
    {
        int size = std::min(17, 2048 - offset);
        double *channels[]{actual.getWritePointer(0, offset), actual.getWritePointer(1, offset)};
        juce::AudioBuffer<double> block(channels, 2, size);
        split.processBlock(block, midi);
        offset += size;
    }
    for (int c = 0; c < 2; ++c)
        check(std::memcmp(expected.getReadPointer(c), actual.getReadPointer(c),
                          2048 * sizeof(double)) == 0,
              "ramps independent of block partitioning");
    set(whole, "fxBypass", 0);
    set(whole, "fxInputGain", 24);
    set(whole, "fxOutputGain", 24);
    whole.reset();
    juce::AudioBuffer<double> loud(2, 1);
    loud.setSample(0, 0, 1.);
    loud.setSample(1, 0, 0.);
    whole.processBlock(loud, midi);
    check(std::abs(loud.getSample(0, 0) - std::pow(10., 48. / 20.)) < 1e-10 &&
              loud.getSample(1, 0) == 0.,
          "double gain keeps headroom and does not leak channels");
}
static void stateAndLayouts()
{
    RastroFxProcessor p;
    check(!p.acceptsMidi() && !p.producesMidi() && !p.isMidiEffect(), "audio-only insert");
    check(p.getName() == "RASTRO FX" && p.getParameters().size() > 90,
          "independent product/parameters");
    check(p.getBypassParameter() == p.params.getParameter("fxBypass"), "host bypass parameter");
    auto b = p.getBusesLayout();
    b.inputBuses.set(0, juce::AudioChannelSet::mono());
    check(!p.isBusesLayoutSupported(b), "reject mismatched mono/stereo");
    b.inputBuses.set(0, juce::AudioChannelSet::discreteChannels(6));
    b.outputBuses.set(0, juce::AudioChannelSet::discreteChannels(6));
    check(!p.isBusesLayoutSupported(b), "reject unsupported surround");
    b.inputBuses.set(0, juce::AudioChannelSet::disabled());
    check(!p.isBusesLayoutSupported(b), "reject absent input");
    set(p, "fxInputGain", 3);
    set(p, "fxOutputGain", -9);
    set(p, "fxBypass", 1);
    juce::MemoryBlock data;
    p.getStateInformation(data);
    RastroFxProcessor restored;
    restored.setStateInformation(data.getData(), int(data.getSize()));
    for (auto id : {"fxInputGain", "fxOutputGain", "fxBypass"})
        check(std::abs(restored.params.getRawParameterValue(id)->load() -
                       p.params.getRawParameterValue(id)->load()) < 1e-5,
              "state roundtrip");
    juce::MemoryBlock foreign;
    juce::AudioProcessor::copyXmlToBinary(juce::XmlElement("RASTRO_CAPTURE"), foreign);
    restored.setStateInformation(foreign.getData(), int(foreign.getSize()));
    restored.setStateInformation("bad", 3);
    check(restored.params.getRawParameterValue("fxInputGain")->load() == 3,
          "foreign/invalid state ignored");
    restored.prepareToPlay(96000, 17);
    juce::AudioBuffer<float> audio(2, 17);
    audio.clear();
    audio.setSample(1, 0, 2.f);
    juce::MidiBuffer midi;
    restored.processBlock(audio, midi);
    check(audio.getSample(1, 0) == 2.f && audio.getSample(0, 0) == 0,
          "restored bypass immediately dry");
}
#include "FxRackIntegration.h"
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    fullFxTests();
    transparency<float>();
    transparency<double>();
    gainAndBypass();
    blockPartitioning();
    stateAndLayouts();
    std::cout << "PASS FX: float/double bit-exact unity; mono/stereo; rates/block sizes; gains; "
                 "parameter/host bypass ramps and reversal; state isolation; zero latency\n";
}
