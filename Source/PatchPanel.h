#pragma once
#include "Processor.h"
#include "Visuals.h"
class PatchPanel : public juce::Component
{
    RastroProcessor &p;
    juce::TextButton generate, pause, cancel, compare, variation, undo, save, load;
    juce::ComboBox direction, loadScope;
    juce::Label status, help, scopeNotice;
    std::array<juce::TextButton, 6> candidates;
    std::unique_ptr<juce::FileChooser> chooser;
    rastro::PatchValues before{}, selected{}, variationBefore{};
    bool hasSelection = false, showingBefore = false, hasUndo = false, fileBusy = false;
    const rastro::Sample *selectedSource = nullptr;
    unsigned seenRevision = ~0u;
    std::vector<rastro::SamplePatch> results;
    unsigned auditionGeneration = 0;
    int auditionNote = -1;
    double auditionUntil = 0;
    juce::String fileStatus;
    void audition();
    void chooseFile(bool);

  public:
    explicit PatchPanel(RastroProcessor &);
    ~PatchPanel() override;
    void update();
    void resized() override;
    void paint(juce::Graphics &) override;
};
