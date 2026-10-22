#include "CapturePanel.h"
#include <juce_audio_utils/juce_audio_utils.h>
class CaptureProcessor : public juce::AudioProcessor
{
    juce::ThreadPool worker{1};
    std::atomic<bool> closing{false};

  public:
    rastro::CaptureRecorder capture;
    std::atomic<int> language{0};
    CaptureProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("Input", juce::AudioChannelSet::stereo(), true)
                             .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }
    ~CaptureProcessor() override
    {
        closing = true;
        capture.stop();
        capture.finishStop();
        worker.removeAllJobs(true, -1);
    }
    const juce::String getName() const override { return "RASTRO Capture"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    void prepareToPlay(double sr, int) override { capture.prepare(sr); }
    void releaseResources() override
    {
        capture.stop();
        capture.finishStop();
    }
    bool isBusesLayoutSupported(const BusesLayout &b) const override
    {
        return b.getMainInputChannelSet() == b.getMainOutputChannelSet() &&
               (b.getMainInputChannelSet() == juce::AudioChannelSet::stereo() ||
                b.getMainInputChannelSet() == juce::AudioChannelSet::mono());
    }
    void processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &) override
    {
        juce::ScopedNoDenormals no;
        capture.process(b.getArrayOfReadPointers(), getTotalNumInputChannels(), b.getNumSamples());
    }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &out) override
    {
        juce::XmlElement xml("RASTRO_CAPTURE");
        xml.setAttribute("language", language.load());
        xml.setAttribute("gain", capture.gain.load());
        xml.setAttribute("route", capture.route.load());
        copyXmlToBinary(xml, out);
    }
    void setStateInformation(const void *d, int n) override
    {
        if (auto xml = getXmlFromBinary(d, n); xml && xml->hasTagName("RASTRO_CAPTURE"))
        {
            language = juce::jlimit(0, 1, xml->getIntAttribute("language"));
            capture.gain = juce::jlimit(0.f, 2.f, float(xml->getDoubleAttribute("gain", 1)));
            capture.route = juce::jlimit(0, 2, xml->getIntAttribute("route"));
        }
    }
    void record(std::function<void(juce::File)> complete)
    {
        if (!capture.start())
            return;
        worker.addJob(
            [this, complete]
            {
                while (!closing && capture.state != rastro::CaptureRecorder::ready)
                {
                    capture.finishStop();
                    juce::Thread::sleep(5);
                }
                juce::File file;
                if (!closing && capture.claim())
                {
                    juce::MemoryBlock bytes;
                    if (rastro::encodeCapture(capture, bytes))
                        file = rastro::publishCapture(bytes);
                    capture.release();
                }
                juce::MessageManager::callAsync(
                    [complete, file]
                    {
                        if (complete)
                            complete(file);
                    });
            });
    }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor *createEditor() override;
};
class CaptureEditor : public juce::AudioProcessorEditor, private juce::Timer
{
    CaptureProcessor &p;
    CapturePanel panel;

  public:
    explicit CaptureEditor(CaptureProcessor &processor)
        : AudioProcessorEditor(processor), p(processor), panel(p.capture, true)
    {
        addAndMakeVisible(panel);
        panel.hostInsertMode();
        panel.onRecord = [this]
        {
            p.record(
                [safe = juce::Component::SafePointer<CapturePanel>(&panel)](juce::File file)
                {
                    if (safe)
                        safe->message(
                            file.existsAsFile()
                                ? juce::String::fromUTF8("PLAY: CAPTURAR → Atualizar capturas / "
                                                         "Refresh takes → USAR / USE")
                                : "Erro ao salvar captura / Capture save failed");
                });
        };
        panel.onClose = [this]
        {
            if (auto *window = findParentComponentOfClass<juce::DocumentWindow>())
                window->setVisible(false);
        };
        setSize(650, 570);
        startTimerHz(12);
        timerCallback();
    }
    ~CaptureEditor() override { stopTimer(); }
    void resized() override { panel.setBounds(getLocalBounds()); }
    void timerCallback() override
    {
        panel.update(visuals::colourSet(0), p.language, p.getTotalNumInputChannels() > 0);
    }
};
juce::AudioProcessorEditor *CaptureProcessor::createEditor() { return new CaptureEditor(*this); }
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new CaptureProcessor(); }
