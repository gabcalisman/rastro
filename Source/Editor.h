#pragma once
#include "CapturePanel.h"
#include "Customizer.h"
#include "Hardware.h"
#include "InstrumentPanel.h"
#include "Performance.h"
#include "Processor.h"
#include "RastroStrings.h"
#include "SampleView.h"
class RastroEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     public juce::FileDragAndDropTarget
{
    struct Style : juce::LookAndFeel_V4
    {
        Style();
        int controlStyle = 0, chassis = 0;
        juce::Colour accent{0xffdf682f};
        juce::Colour surface{visuals::panel}, outline{visuals::edge};
        void apply(const visuals::Palette &, int, int);
        void drawButtonBackground(juce::Graphics &, juce::Button &, const juce::Colour &, bool,
                                  bool) override;
        void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float,
                              juce::Slider &) override;
    };
    struct Knob : juce::Component
    {
        RastroProcessor &p;
        void applyPalette();
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
        std::array<float, 7> displayState{};
        juce::TextButton bypass, remove, timing;
        bool updateTiming();
        std::unique_ptr<Knob> mix, primary;
        std::vector<std::unique_ptr<Knob>> knobs;
        Card(RastroEditor &, int);
        void paint(juce::Graphics &) override;
        void resized() override;
        void mouseDrag(const juce::MouseEvent &) override;
        void mouseUp(const juce::MouseEvent &) override;
    };
    RastroProcessor &p;
    Style style;
    std::unique_ptr<VisualCustomizer> customizer;
    std::unique_ptr<InstrumentPanel> instrument;
    juce::TextButton openInstrument, openPatches, openCapture;
    struct CaptureShade : juce::Component
    {
        void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black.withAlpha(.48f)); }
    } captureShade;
    std::unique_ptr<CapturePanel> capturePanel;
    juce::TooltipWindow tips{this, 700};
    juce::TextButton load, mutate, undo, reset, add, about, center, preview, customize, compact;
    juce::ComboBox language, animations, playbackMode, sliceCount, pitchMode;
    juce::Label pitchLabel;
    SampleDeck deck;
    SampleView sampleView;
    void import(juce::File);
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> animationAttach,
        pitchAttach;
    PlayPad play;
    RastroAvatar avatar;
    juce::ToggleButton loop, regionLoop;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> regionAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopAttach;
    std::vector<std::unique_ptr<Knob>> sampleKnobs, macros;
    std::array<std::unique_ptr<Card>, rastro::moduleCount> cards;
    std::array<juce::TextButton, 5> learns;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> wave, rack, performance;
    int lastLanguage = -1;
    float lastPlayX = -1, lastPlayY = -1;
    const rastro::Sample *lastSource = nullptr;
    bool compactLayout = false;
    int lastCompact = -1;
    std::array<float, std::size(visuals::visualIds)> lastVisual{};
    rastro::Order lastOrder{};
    int lastPresence = -1;
    bool wasCustomizing = false;
    juce::Rectangle<int> samplePanel, macroPanel, rackTitle;
    void applyAppearance();
    void customizerClosed();
    int dragged = -1, selected = 3;
    void selectModule(int);
    juce::Rectangle<int> detail;
    void timerCallback() override;
    void localize();
    void showCustomizer();
    juce::String tr(const juce::String &k) const { return text::get(k, p.language); }

  public:
    struct UiMetrics
    {
        uint64_t ticks = 0, layouts = 0, mainAvatar = 0, previewAvatar = 0, fullRepaints = 0,
                 sampleRefreshes = 0, deckUpdates = 0;
        double updateMs = 0;
    };
    UiMetrics uiMetrics() const { return measurements; }
    void synchroniseUi();

  private:
    UiMetrics measurements;
    bool windowWasShowing = false;
    double lastCapturePoll = 0;

  public:
    explicit RastroEditor(RastroProcessor &);
    ~RastroEditor() override;
    bool keyPressed(const juce::KeyPress &) override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void visibilityChanged() override;
    bool isInterestedInFileDrag(const juce::StringArray &) override;
    void filesDropped(const juce::StringArray &, int, int) override;
};
