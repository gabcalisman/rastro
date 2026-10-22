#pragma once
#include "CaptureBridge.h"
#include "CaptureLink.h"
#include "Engine.h"
#include "PatchSearch.h"
#include "RecordingImport.h"
#include <juce_audio_utils/juce_audio_utils.h>
class RastroProcessor : public juce::AudioProcessor, private juce::MidiKeyboardStateListener
{
  public:
    explicit RastroProcessor(bool directCaptureForTests = false);
    ~RastroProcessor() override
    {
        patchSearch.stop();
        shuttingDown = true;
        capture.stop();
        capture.finishStop();
        loader.removeAllJobs(true, -1);
        keyboard.removeListener(this);
    }
    void applyCC();
    rastro::PatchSearch patchSearch;
    rastro::PatchValues patchValues() const;
    void applyPatch(const rastro::PatchValues &);
    void patchFileAsync(juce::File, bool save, std::function<void(bool)>);
    void setPlaybackMode(int);
    void equalSlices(int);
    std::atomic<int> previewRequest{0};
    const juce::String getName() const override { return "RASTRO"; }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    bool canAddBus(bool input) const override
    {
        return input && directInput && getBusCount(true) == 0;
    }
    bool canApplyBusCountChange(bool input, bool adding, BusProperties &properties) override
    {
        if (!input || !adding || !directInput)
            return false;
        properties.busName = "Capture";
        properties.defaultLayout = juce::AudioChannelSet::stereo();
        properties.isActivatedByDefault = true;
        return true;
    }
    rastro::CaptureRecorder capture;
    std::function<void()> standaloneAudioSettings;
    std::function<bool()> standaloneInputAvailable;
    void beginCapture(std::function<void(bool)> complete);
    void pollCaptureLinks(
        std::function<void(std::vector<juce::File>, std::vector<rastro::CaptureLink>)>);
    void remoteCapture(juce::String id, bool start, std::function<void(bool)>);
    void listCaptures(std::function<void(std::vector<juce::File>)> complete);
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor *createEditor() override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::AudioProcessorValueTreeState params;
    juce::MidiKeyboardState keyboard;
    rastro::Engine engine;
    std::atomic<const rastro::Sample *> source{nullptr};
    std::atomic<int> language{0}, learn{-1};
    std::array<std::atomic<int>, 5> cc{{-1, -1, -1, -1, -1}};
    std::atomic<int> midiFlash{0};
    std::atomic<float> hostBpm{0}, hostBeat{-1};
    std::atomic<bool> hostPlaying{false};
    bool loadFile(const juce::File &);
    rastro::ImportError importFile(const juce::File &);
    rastro::ImportError importData(const void *, size_t, juce::String);
    void importFileAsync(juce::File, std::function<void(rastro::ImportError)>);
    std::atomic<rastro::ImportError> importError{rastro::ImportError::none};
    juce::String importMessage(rastro::ImportError) const;
    void loadFileAsync(juce::File, std::function<void(bool)>);
    bool loadData(const void *, size_t, juce::String);
    juce::String sampleName();
    rastro::Settings settings() const;
    float value(const char *id) const { return params.getRawParameterValue(id)->load(); }
    void set(const juce::String &, float);
    void mutate();
    void resetEffects();
    void undoMutation();
    bool canUndoMutation() const { return mutationUndoAvailable.load(); }
    void moveModule(int, int);

  private:
    bool restoreState(const void *, int);
    std::atomic<bool> bridgePollPending{false};
    bool directInput = false;
    std::atomic<bool> shuttingDown{false};
    float monitorGain = 0;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    juce::AudioFormatManager formats;
    juce::ThreadPool loader{1};
    struct KeyEvent
    {
        int note;
        float velocity;
        bool on;
    };
    std::array<KeyEvent, 256> keyEvents{};
    std::atomic<unsigned> keyRead{0}, keyWrite{0};
    std::atomic<bool> overflow{false};
    void handleNoteOn(juce::MidiKeyboardState *, int, int n, float v) override
    {
        enqueue(n, v, true);
    }
    void handleNoteOff(juce::MidiKeyboardState *, int, int n, float) override
    {
        enqueue(n, 0, false);
    }
    void enqueue(int n, float v, bool on)
    {
        auto w = keyWrite.load();
        if (w - keyRead.load(std::memory_order_acquire) >= 256)
        {
            overflow = true;
            return;
        }
        keyEvents[w % 256] = {n, v, on};
        keyWrite.store(w + 1, std::memory_order_release);
    }
    std::array<std::atomic<float>, 5> ccPending{{-1, -1, -1, -1, -1}};
    std::array<std::array<float, 4>, rastro::moduleCount> mutationBefore{};
    std::array<bool, rastro::moduleCount> mutationAffected{};
    std::atomic<bool> mutationUndoAvailable{false};
    bool wasHostPlaying = false;
    const rastro::Sample *previousSource = nullptr;
    juce::CriticalSection stateLock;
    std::vector<std::unique_ptr<rastro::Sample>> owned;
    size_t ownedBytes = 0;
    juce::MemoryBlock sampleBytes;
    juce::String loadedName;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RastroProcessor)
};
