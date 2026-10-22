#pragma once
#include "CaptureLink.h"
#include "RastroStrings.h"
#include "Visuals.h"
class CapturePanel : public juce::Component
{
    rastro::CaptureRecorder &rec;
    bool direct, insert = false;
    juce::TextButton close, settings, record, stop, preview, refresh, receive;
    juce::ToggleButton monitor;
    juce::Slider gain;
    juce::ComboBox route, takes, sources;
    juce::Label status, context;
    std::vector<juce::File> files;
    juce::File pendingSelection;
    std::vector<rastro::CaptureLink> links;
    visuals::Palette pal;
    juce::String insertId, lastRemote;
    int language = -1, previousState = -1, previousOutcome = -1;
    float lastPeak = -1;
    unsigned lastBlock = 0;
    double lastAudio = 0;
    rastro::CaptureLink selectedLink() const;

  public:
    explicit CapturePanel(rastro::CaptureRecorder &, bool directInput);
    std::function<void()> onClose, onSettings, onRecord, onPreview, onRefresh;
    std::function<void(juce::File)> onReceive;
    std::function<void(juce::String, bool)> onRemoteCommand;
    void hostInsertMode();
    void closeAfterLoad() { close.onClick(); }
    void setInsertIdentifier(juce::String id) { insertId = id; }
    void setFiles(std::vector<juce::File>);
    void setLinks(std::vector<rastro::CaptureLink>);
    void message(juce::String text) { status.setText(text, juce::dontSendNotification); }
    static juce::String outcomeText(int, bool pt);
    void update(const visuals::Palette &, int lang, bool inputAvailable);
    void resized() override;
    void paint(juce::Graphics &) override;
    bool keyPressed(const juce::KeyPress &) override;
};
