#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

// Independent insert processor: no sampler, capture link or PLAY state.
class RastroFxProcessor : public juce::AudioProcessor
{
  public:
    RastroFxProcessor();
    const juce::String getName() const override { return "RASTRO FX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool supportsDoublePrecisionProcessing() const override { return true; }
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlock(juce::AudioBuffer<double> &, juce::MidiBuffer &) override;
    void processBlockBypassed(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlockBypassed(juce::AudioBuffer<double> &, juce::MidiBuffer &) override;
    juce::AudioProcessorParameter *getBypassParameter() const override;
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor *createEditor() override;
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;

    juce::AudioProcessorValueTreeState params;

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    template <typename Sample> void process(juce::AudioBuffer<Sample> &, bool hostBypass);
    std::atomic<float> *inputDb, *outputDb, *bypass;
    juce::SmoothedValue<double> inputGain, outputGain, activeMix;
};
