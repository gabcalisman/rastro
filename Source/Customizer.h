#pragma once
#include "Performance.h"
#include "Visuals.h"
class VisualCustomizer : public juce::Component
{
    RastroProcessor &p;
    using Snapshot = std::array<float, std::size(visuals::visualIds)>;
    Snapshot entry{}, observed{};
    std::vector<Snapshot> undoStack, redoStack;
    int tab = 0, lastLanguage = -1;
    bool restoring = false, previewDirty = true;
    juce::Image instrumentPreview;
    struct Choice : juce::TextButton
    {
        VisualCustomizer &ui;
        int index, kind; // palette / knob / body thumbnail
        Choice(VisualCustomizer &, int, int);
        void paintButton(juce::Graphics &, bool, bool) override;
    };
    std::array<std::unique_ptr<Choice>, 9> palettes;
    std::array<std::unique_ptr<Choice>, 4> knobs;
    std::array<std::unique_ptr<Choice>, 3> bodies;
    std::array<juce::TextButton, 6> tabs;
    juce::TextButton close, reset, undo, redo, revert, saveLook, loadLook;
    juce::ComboBox lookScope;
    juce::Label fileStatus;
    std::unique_ptr<juce::FileChooser> chooser;
    bool fileBusy = false;
    void chooseLook(bool);
    juce::Slider previewKnob;
    juce::Label previewLabel;
    RastroAvatar avatar;
    struct Field
    {
        juce::String id;
        juce::Label label;
        juce::ComboBox combo;
        int area;
    };
    std::vector<std::unique_ptr<Field>> fields;
    Snapshot capture() const;
    void restore(const Snapshot &);
    void beforeEdit();
    void syncFields();
    void localize();
    void chooseTab(int);
    void dismiss();

  public:
    explicit VisualCustomizer(RastroProcessor &);
    std::function<void()> onClose;
    std::function<juce::Image()> renderInstrumentPreview;
    juce::Image appliedPreview() const { return instrumentPreview; }
    void open();
    void invalidatePreview() { previewDirty = true; }
    bool update(int); // true only when the visible avatar was stepped
    void undoAppearance();
    void redoAppearance();
    void paint(juce::Graphics &) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress &) override;
    void visibilityChanged() override;
};
