#pragma once
#include "Performance.h"
#include "Processor.h"
#include "SampleView.h"
#include "Strings.h"
class RastroEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     public juce::FileDragAndDropTarget
{
    struct Style : juce::LookAndFeel_V4
    {
        Style();
        int controlStyle = 0;
        juce::Colour accent{0xffdf682f};
        void drawButtonBackground(juce::Graphics &, juce::Button &, const juce::Colour &, bool,
                                  bool) override;
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
    struct Card : juce::Component, public juce::SettableTooltipClient
    {
        RastroEditor &ui;
        int module;
        juce::TextButton bypass, remove;
        std::unique_ptr<Knob> mix;
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
    juce::TextButton load, mutate, undo, reset, add, about, center, preview, customize;
    juce::ComboBox language, animations, playbackMode, sliceCount;
    SampleView sampleView;
    void import(juce::File);
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> animationAttach;
    PlayPad play;
    RastroAvatar avatar;
    juce::ToggleButton loop;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopAttach;
    std::vector<std::unique_ptr<Knob>> sampleKnobs, macros;
    std::array<std::unique_ptr<Card>, rastro::moduleCount> cards;
    std::array<juce::TextButton, 5> learns;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> wave, rack, performance;
    int lastLanguage = -1;
    float glow = 0;
    int dragged = -1, selected = 3;
    void selectModule(int);
    juce::Rectangle<int> detail;
    void timerCallback() override;
    void localize();
    void showCustomizer();
    juce::String tr(const juce::String &k) const { return text::get(k, p.language); }

  public:
    explicit RastroEditor(RastroProcessor &);
    ~RastroEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray &) override;
    void filesDropped(const juce::StringArray &, int, int) override;
};
