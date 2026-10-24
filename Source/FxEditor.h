#pragma once
#include "FxProcessor.h"
class RastroFxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit RastroFxEditor(RastroFxProcessor &);
    ~RastroFxEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void refresh();
    // Exposed for integration layout checks, not part of the audio callback.
    bool controlsFit() const;

  private:
    struct Style;
    struct Knob;
    struct Segmented;
    struct Row;
    struct ModPanel;
    struct Pad;
    struct Pattern;
    struct Meter;
    void timerCallback() override { refresh(); }
    void choosePreset(bool save);
    RastroFxProcessor &p;
    std::unique_ptr<Style> style;
    juce::Viewport rackView, modView;
    juce::Viewport moduleView;
    juce::Component rackBody, moduleBody;
    std::unique_ptr<ModPanel> modulation;
    std::array<std::unique_ptr<Row>, rastro::fx::count> rows;
    std::array<juce::TextButton, rastro::fx::count> moduleButtons;
    std::unique_ptr<Knob> input, output, dryWet;
    std::unique_ptr<Meter> meters;
    juce::ToggleButton bypass{"BYPASS"};
    juce::ComboBox route, factory, accent, skin;
    juce::TextEditor presetName;
    juce::TextButton compare{"A / B"}, copy{"Copy to B"}, open{"OPEN"}, save{"SAVE"};
    juce::Label status;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> routeAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image logo;
    std::array<int, rastro::fx::count> shownOrder{};
    std::array<bool, rastro::fx::count> shownInserted{};
};
