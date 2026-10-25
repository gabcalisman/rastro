#pragma once
#include "FxProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

#if RASTRO_FX_WEB_UI
// RASTRO FX web interface host (macOS). The audio engine is untouched: this editor reads atomic
// telemetry, reads/writes host parameters on the message thread and forwards preset actions.
// The native RastroFxEditor remains the fallback on other platforms and in the test suite.
class RastroFxWebEditor : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit RastroFxWebEditor(RastroFxProcessor &);
    // False when the system web engine is missing (e.g. no WebView2 runtime on Windows); the
    // processor then opens the native editor instead of a blank window.
    static bool available();
    ~RastroFxWebEditor() override;
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
    RastroFxProcessor &p;
    std::unique_ptr<Browser> web;
    std::vector<juce::RangedAudioParameter *> parameters;
    std::vector<float> sentValues;
    std::unique_ptr<juce::FileChooser> chooser;
    bool pageReady = false;
    std::optional<juce::WebBrowserComponent::Resource> resource(const juce::String &url);
    juce::var initialState();
    void setParameter(const Args &);
    void choosePreset(bool save, Completion done);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RastroFxWebEditor)
};
#endif
