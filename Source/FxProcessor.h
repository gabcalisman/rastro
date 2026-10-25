#pragma once
#include "FxDsp.h"
#include <juce_audio_utils/juce_audio_utils.h>

class RastroFxProcessor : public juce::AudioProcessor
{
  public:
    static constexpr int maxTargets = 96;
    struct Target
    {
        juce::String id, label;
        std::atomic<float> *raw = nullptr;
        float low = 0, high = 1;
    };
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
    double getTailLengthSeconds() const override { return 300; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int i) override { loadFactory(i); }
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String &name) override { setPresetName(name); }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor *createEditor() override;
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;

    juce::AudioProcessorValueTreeState params;
    std::vector<Target> targets; // built once; callback only reads cached pointers/numeric ranges
    std::atomic<float> inputPeak{0}, outputPeak{0}, lfoMeter{0}, envelopeMeter{0};
    std::array<std::atomic<float>, rastro::fx::count> activity{};
    // Output scope for the interface visualiser: every 4th output frame (mono) in a ring buffer.
    // Telemetry only; it never feeds back into the audio.
    static constexpr int scopeSize = 512;
    std::array<std::atomic<float>, scopeSize> scope{};
    std::atomic<int> scopeWrite{0};
    static juce::String moduleId(int module, const juce::String &suffix);
    void setValue(const juce::String &, float);
    float value(const juce::String &) const;
    void moveModule(int module, int delta);
    void compare();
    void copyCompare();
    int compareSlot() const { return currentBank; }
    void loadFactory(int);
    bool savePreset(const juce::File &);
    bool loadPreset(const juce::File &);
    juce::String presetName() const;
    void setPresetName(const juce::String &);
    int appearance(const char *key, int fallback = 0) const;
    void setAppearance(const char *key, int value);
    std::array<int, rastro::fx::count> order() const;

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    template <typename Sample> void process(juce::AudioBuffer<Sample> &, bool hostBypass);
    void registerTargets();
    juce::ValueTree snapshot();
    std::array<juce::ValueTree, 2> banks;
    std::atomic<int> currentBank{0}, currentProgram{0};
    mutable juce::CriticalSection stateLock;
    std::array<juce::SmoothedValue<double>, maxTargets> smooth;
    std::array<double, maxTargets> modulationOffset{};
    std::array<juce::SmoothedValue<float>, rastro::fx::count> enables;
    std::array<std::atomic<float> *, rastro::fx::count> on{}, inRack{}, position{};
    std::array<std::array<int, 4>, rastro::fx::count> knobIndex{};
    std::array<int, rastro::fx::count> mixIndex{};
    std::array<int, 8> panIndex{};
    std::array<std::atomic<float> *, 16> mod{};
    std::atomic<float> *bypass = nullptr, *routing = nullptr;
    juce::SmoothedValue<double> activeMix;
    std::array<juce::SmoothedValue<float>, 2> xy;
    rastro::fx::Rack rack;
    rastro::fx::Frame frame;
    // Constant plugin latency (EMBER/SQUASH oversampling). The dry path is delayed to match:
    // raw host samples in the host's own type (copied, never converted, so subnormals survive the
    // denormal flush), and the gain-scaled rack input as float.
    static constexpr int latency = rastro::fx::Rack::latency();
    std::array<std::array<float, latency + 1>, 2> rawHistoryFloat{};
    std::array<std::array<double, latency + 1>, 2> rawHistoryDouble{};
    template <typename Sample> std::array<std::array<Sample, latency + 1>, 2> &rawHistory()
    {
        if constexpr (std::is_same_v<Sample, float>)
            return rawHistoryFloat;
        else
            return rawHistoryDouble;
    }
    std::array<std::array<float, latency + 1>, 2> inputHistory{};
    std::array<double, latency + 1> inputGainHistory{};
    int historyPosition = 0, activeHold = 0, scopeDecimation = 0;
    double rate = 48000, lfoPhase = 0;
    float envelope = 0, lfoSmooth = 0, topology = 1, inputHeld = 0, outputHeld = 0;
    bool parallel = false;
    std::array<int, rastro::fx::count> activeOrder{};
};
