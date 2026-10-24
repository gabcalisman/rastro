#include "FxProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout RastroFxProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    for (auto spec : {std::pair{"fxInputGain", "Input Gain"}, {"fxOutputGain", "Output Gain"}})
        result.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{spec.first, 1}, spec.second,
            juce::NormalisableRange<float>{-24.f, 24.f, .01f}, 0.f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
    result.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"fxBypass", 1}, "Bypass", false));
    return result;
}

RastroFxProcessor::RastroFxProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "RASTRO_FX_STATE", layout()),
      inputDb(params.getRawParameterValue("fxInputGain")),
      outputDb(params.getRawParameterValue("fxOutputGain")),
      bypass(params.getRawParameterValue("fxBypass"))
{
}

bool RastroFxProcessor::isBusesLayoutSupported(const BusesLayout &b) const
{
    return b.inputBuses.size() == 1 && b.outputBuses.size() == 1 &&
           b.getMainInputChannelSet() == b.getMainOutputChannelSet() &&
           (b.getMainInputChannelSet() == juce::AudioChannelSet::mono() ||
            b.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}

void RastroFxProcessor::prepareToPlay(double rate, int)
{
    inputGain.reset(rate, .02);
    outputGain.reset(rate, .02);
    activeMix.reset(rate, .01);
    reset();
}

void RastroFxProcessor::reset()
{
    inputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(double(inputDb->load())));
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(double(outputDb->load())));
    activeMix.setCurrentAndTargetValue(bypass->load() >= .5f ? 0. : 1.);
}

template <typename Sample>
void RastroFxProcessor::process(juce::AudioBuffer<Sample> &buffer, bool hostBypass)
{
    inputGain.setTargetValue(juce::Decibels::decibelsToGain(double(inputDb->load())));
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(double(outputDb->load())));
    activeMix.setTargetValue(hostBypass || bypass->load() >= .5f ? 0. : 1.);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const double in = inputGain.getNextValue(), out = outputGain.getNextValue();
        const double mix = activeMix.getNextValue();
        const double gain = 1. + mix * (in * out - 1.);
        // Do not touch unity samples: preserve signed zero, denormals and headroom.
        // No limiter, noise, dither, clipping, channel summing or extra latency.
        if (gain != 1.)
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.setSample(channel, sample,
                                 Sample(buffer.getSample(channel, sample) * gain));
    }
}

void RastroFxProcessor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &)
{
    process(b, false);
}
void RastroFxProcessor::processBlock(juce::AudioBuffer<double> &b, juce::MidiBuffer &)
{
    process(b, false);
}
void RastroFxProcessor::processBlockBypassed(juce::AudioBuffer<float> &b, juce::MidiBuffer &)
{
    process(b, true);
}
void RastroFxProcessor::processBlockBypassed(juce::AudioBuffer<double> &b, juce::MidiBuffer &)
{
    process(b, true);
}
juce::AudioProcessorParameter *RastroFxProcessor::getBypassParameter() const
{
    return params.getParameter("fxBypass");
}
juce::AudioProcessorEditor *RastroFxProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}
void RastroFxProcessor::getStateInformation(juce::MemoryBlock &out)
{
    if (auto xml = params.copyState().createXml())
        copyXmlToBinary(*xml, out);
}
void RastroFxProcessor::setStateInformation(const void *data, int size)
{
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName("RASTRO_FX_STATE"))
        params.replaceState(juce::ValueTree::fromXml(*xml));
}
