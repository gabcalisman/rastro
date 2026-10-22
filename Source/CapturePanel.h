#pragma once
#include "CaptureBridge.h"
#include "Strings.h"
#include "Visuals.h"
class CapturePanel : public juce::Component
{
    rastro::CaptureRecorder &rec;
    bool direct;
    juce::TextButton close, settings, record, stop, preview, refresh, receive;
    juce::ToggleButton monitor;
    juce::Slider gain;
    juce::ComboBox route, takes;
    juce::Label status;
    std::vector<juce::File> files;
    visuals::Palette pal;
    int language = -1, previousState = -1;
    float lastPeak = -1;

  public:
    explicit CapturePanel(rastro::CaptureRecorder &, bool directInput);
    std::function<void()> onClose, onSettings, onRecord, onPreview, onRefresh;
    std::function<void(juce::File)> onReceive;
    void hostInsertMode()
    {
        monitor.setVisible(false);
        settings.setVisible(false);
        close.setVisible(false);
        refresh.setVisible(false);
        takes.setVisible(false);
        receive.setVisible(false);
        preview.setVisible(false);
    }
    void setFiles(std::vector<juce::File>);
    void message(juce::String text) { status.setText(text, juce::dontSendNotification); }
    void update(const visuals::Palette &, int lang, bool inputAvailable);
    void resized() override;
    void paint(juce::Graphics &) override;
    bool keyPressed(const juce::KeyPress &) override;
};
