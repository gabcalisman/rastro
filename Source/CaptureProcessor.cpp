#include "CaptureLink.h"
#include "CapturePanel.h"
#include <juce_audio_utils/juce_audio_utils.h>
class CaptureProcessor : public juce::AudioProcessor
{

  public:
    rastro::CaptureRecorder capture;
    rastro::CaptureLinkServer link{capture};
    std::atomic<int> language{0};
    CaptureProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("Input", juce::AudioChannelSet::stereo(), true)
                             .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }
    ~CaptureProcessor() override
    {
        capture.stop();
        capture.finishStop();
        link.stop();
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
        panel.onRecord = [this] { p.capture.start(); };
        panel.setInsertIdentifier(p.link.identifier());
        panel.onClose = [this]
        {
            if (auto *window = findParentComponentOfClass<juce::DocumentWindow>())
                window->setVisible(false);
        };
        setSize(640, 390);
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
