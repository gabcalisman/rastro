#pragma once
#include "EnvelopeView.h"
#include "PatchPanel.h"
#include "Performance.h"
#include "Visuals.h"
class InstrumentPanel : public juce::Component
{
    RastroProcessor &p;
    juce::TextButton close, synthTab, patchTab;
    PatchPanel patches;
    EnvelopeView envelope;
    juce::Label amplitudeTitle, filterTitle;
    bool patchVisible = false;
    juce::Viewport viewport;
    juce::Component content;
    juce::ToggleButton enableLoop;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopAttach;
    juce::ToggleButton enable, loopRegion;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> regionAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabled;
    struct SliderField
    {
        juce::String id;
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct MenuField
    {
        juce::String id;
        juce::Label label;
        juce::ComboBox menu;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
    std::vector<std::unique_ptr<SliderField>> sliders;
    std::vector<std::unique_ptr<MenuField>> menus;
    int lastLanguage = -1;

  public:
    explicit InstrumentPanel(RastroProcessor &);
    std::function<void()> onClose;
    void showPage(bool patch)
    {
        if (patch)
            patchTab.onClick();
        else
            synthTab.onClick();
    }
    void update();
    bool keyPressed(const juce::KeyPress &) override;
    void resized() override;
    void paint(juce::Graphics &) override;
};
