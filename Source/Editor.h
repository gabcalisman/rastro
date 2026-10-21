#pragma once
#include "Processor.h"
#include "Strings.h"
class RastroEditor : public juce::AudioProcessorEditor, private juce::Timer
{
    struct Style : juce::LookAndFeel_V4
    {
        Style();
        void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float,
                              juce::Slider &) override;
    };
    struct Knob : juce::Component
    {
        juce::Slider slider;
        juce::Label label;
        juce::String id;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        Knob(RastroProcessor &, juce::String, bool rotary = true);
        void resized() override;
    };
    struct Card : juce::Component
    {
        RastroEditor &ui;
        int module;
        juce::TextButton bypass, remove;
        std::vector<std::unique_ptr<Knob>> knobs;
        Card(RastroEditor &, int);
        void paint(juce::Graphics &) override;
        void resized() override;
        void mouseDrag(const juce::MouseEvent &) override;
        void mouseUp(const juce::MouseEvent &) override;
    };
    RastroProcessor &p;
    Style style;
    juce::TooltipWindow tips{this, 700};
    juce::TextButton load, mutate, reset, add, about;
    juce::ComboBox language;
    juce::ToggleButton loop;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopAttach;
    std::vector<std::unique_ptr<Knob>> sampleKnobs, macros;
    std::array<std::unique_ptr<Card>, 3> cards;
    std::array<juce::TextButton, 3> learns;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> wave, rack, performance;
    int lastLanguage = -1;
    float glow = 0;
    int dragged = -1;
    void timerCallback() override;
    void localize();
    juce::String tr(const juce::String &k) const { return text::get(k, p.language); }

  public:
    explicit RastroEditor(RastroProcessor &);
    ~RastroEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
};
