#pragma once
#include "Processor.h"
#include <juce_gui_extra/juce_gui_extra.h>

#if RASTRO_WEB_UI
// Web interface host. The audio engine is untouched: this editor only reads atomic telemetry,
// reads/writes parameters on the message thread and forwards notes through the keyboard queue.
class RastroWebEditor : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit RastroWebEditor(RastroProcessor &);
    ~RastroWebEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    struct Browser : juce::WebBrowserComponent
    {
        using juce::WebBrowserComponent::WebBrowserComponent;
        bool pageAboutToLoad(const juce::String &url) override
        {
            return url.startsWith(getResourceProviderRoot());
        }
    };
    using Completion = juce::WebBrowserComponent::NativeFunctionCompletion;
    using Args = juce::Array<juce::var>;
    RastroProcessor &p;
    std::unique_ptr<Browser> web;
    std::vector<juce::RangedAudioParameter *> parameters;
    std::vector<float> sentValues;
    const rastro::Sample *lastSample = nullptr;
    std::array<int, rastro::moduleCount> lastOrder{{-1, -1, -1, -1, -1, -1}};
    std::unique_ptr<juce::FileChooser> chooser;
    bool pageReady = false;
    std::optional<juce::WebBrowserComponent::Resource> resource(const juce::String &url);
    juce::var initialState();
    juce::var parameterInfo(const juce::RangedAudioParameter &) const;
    juce::var sampleInfo() const;
    juce::var waveform(int columns) const;
    juce::var importResult(rastro::ImportError) const;
    void setParameter(const Args &);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RastroWebEditor)
};
#endif
