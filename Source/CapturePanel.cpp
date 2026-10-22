#include "CapturePanel.h"
CapturePanel::CapturePanel(rastro::CaptureRecorder &capture, bool directInput)
    : rec(capture), direct(directInput)
{
    setComponentID("capturePanel");
    setWantsKeyboardFocus(true);
    setFocusContainerType(FocusContainerType::focusContainer);
    for (auto *b : {&close, &settings, &record, &stop, &preview, &refresh, &receive})
        addAndMakeVisible(b);
    close.setComponentID("closeCapture");
    record.setComponentID("recordCapture");
    stop.setComponentID("stopCapture");
    receive.setComponentID("receiveCapture");
    addAndMakeVisible(monitor);
    addAndMakeVisible(gain);
    addAndMakeVisible(route);
    addAndMakeVisible(takes);
    addAndMakeVisible(status);
    close.onClick = [this]
    {
        rec.monitor = false;
        monitor.setToggleState(false, juce::dontSendNotification);
        setVisible(false);
        if (onClose)
            onClose();
    };
    settings.onClick = [this]
    {
        if (onSettings)
            onSettings();
    };
    record.onClick = [this]
    {
        if (onRecord)
            onRecord();
    };
    stop.onClick = [this]
    {
        rec.stop();
        rec.finishStop();
    };
    preview.onClick = [this]
    {
        if (onPreview)
            onPreview();
    };
    refresh.onClick = [this]
    {
        if (onRefresh)
            onRefresh();
    };
    receive.onClick = [this]
    {
        int i = takes.getSelectedId() - 1;
        if (i >= 0 && i < int(files.size()) && onReceive)
            onReceive(files[i]);
    };
    monitor.onClick = [this] { rec.monitor = monitor.getToggleState(); };
    gain.setRange(0, 2, .01);
    gain.setValue(1);
    gain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
    gain.setTextValueSuffix(" x");
    gain.onValueChange = [this] { rec.gain = float(gain.getValue()); };
    route.addItem("Stereo", 1);
    route.addItem("L / mono", 2);
    route.addItem("R / mono", 3);
    route.setSelectedId(1);
    route.onChange = [this] { rec.route = route.getSelectedId() - 1; };
    for (juce::Component *c : std::array<juce::Component *, 3>{&monitor, &gain, &route})
        c->setVisible(direct);
    record.setVisible(direct);
    stop.setVisible(direct);
    settings.setVisible(direct);
}
void CapturePanel::setFiles(std::vector<juce::File> next)
{
    files = std::move(next);
    takes.clear(juce::dontSendNotification);
    for (size_t i = 0; i < files.size(); ++i)
        takes.addItem(files[i].getFileNameWithoutExtension(), int(i) + 1);
    if (!files.empty())
        takes.setSelectedId(1);
    receive.setEnabled(!files.empty());
}
void CapturePanel::update(const visuals::Palette &palette, int lang, bool available)
{
    pal = palette;
    if (lang != language)
    {
        language = lang;
        bool pt = lang == 0;
        close.setButtonText(pt ? "VOLTAR" : "BACK");
        settings.setButtonText(pt ? "Dispositivo / entrada" : "Device / input");
        record.setButtonText(pt ? "GRAVAR" : "RECORD");
        stop.setButtonText(pt ? "PARAR" : "STOP");
        preview.setButtonText(pt ? "OUVIR SAMPLE" : "PREVIEW SAMPLE");
        refresh.setButtonText(pt ? "Atualizar capturas" : "Refresh takes");
        receive.setButtonText(pt ? juce::String::fromUTF8("USAR GRAVAÇÃO") : "USE RECORDING");
        monitor.setButtonText(pt ? juce::String::fromUTF8("Monitor (-20 dB) · use fones")
                                 : juce::String::fromUTF8("Monitor (-20 dB) · use headphones"));
        monitor.setTooltip(
            pt ? juce::String::fromUTF8(
                     "Desligado por padrão. Evite microfone próximo às caixas. Fechar o "
                     "painel desliga o monitor.")
               : "Off by default. Keep microphones away from speakers. Closing this "
                 "panel turns monitoring off.");
        gain.setTooltip(
            pt ? juce::String::fromUTF8(
                     "Ganho de gravação. Vermelho indica clipping; reduza aqui ou na interface de "
                     "áudio.")
               : "Recording gain. Red indicates clipping; reduce here or on the audio interface.");
        repaint();
    }
    int state = rec.state.load();
    bool idle = state == rastro::CaptureRecorder::idle;
    record.setEnabled(direct && available && idle);
    stop.setEnabled(state == rastro::CaptureRecorder::recording ||
                    state == rastro::CaptureRecorder::writing);
    settings.setEnabled(idle && bool(onSettings));
    receive.setEnabled(!files.empty() && idle);
    status.setColour(juce::Label::textColourId, pal.text);
    gain.setColour(juce::Slider::textBoxTextColourId, pal.text);
    takes.setColour(juce::ComboBox::textColourId, pal.text);
    route.setColour(juce::ComboBox::textColourId, pal.text);
    if (state != previousState)
    {
        previousState = state;
        repaint();
    }
    float peak = rec.peak.load();
    if (std::abs(peak - lastPeak) > .002f || state == rastro::CaptureRecorder::recording)
    {
        lastPeak = peak;
        repaint(24, 122, getWidth() - 48, 72);
    }
    if (direct && !available)
        message(language == 0
                    ? "Nenhuma entrada ativa. Abra Dispositivo / entrada e selecione a interface."
                    : "No active input. Open Device / input and select your interface.");
}
void CapturePanel::resized()
{
    int w = getWidth();
    close.setBounds(w - 118, 14, 94, 30);
    settings.setBounds(24, 74, 210, 30);
    route.setBounds(250, 74, 170, 30);
    record.setBounds(24, 210, 150, 36);
    stop.setBounds(190, 210, 150, 36);
    gain.setBounds(24, 264, w - 48, 32);
    monitor.setBounds(24, 306, w - 48, 30);
    refresh.setBounds(24, 372, 210, 32);
    takes.setBounds(250, 372, w - 274, 32);
    receive.setBounds(24, 420, 240, 36);
    preview.setBounds(280, 420, 210, 36);
    status.setBounds(24, 478, w - 48, 52);
}
void CapturePanel::paint(juce::Graphics &g)
{
    g.setColour(pal.bg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 14);
    g.setColour(pal.outline);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 14, 1.5f);
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(23, juce::Font::bold)));
    g.drawText(language == 0 ? juce::String::fromUTF8("CAPTURAR ÁUDIO") : "CAPTURE AUDIO", 24, 14,
               getWidth() - 170, 32, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(14)));
    if (direct)
    {
        g.setColour(pal.surface);
        g.fillRoundedRectangle(24, 126, getWidth() - 48.f, 22, 5);
        g.setColour(rec.peak >= 1 ? juce::Colours::orangered : pal.control);
        g.fillRoundedRectangle(24, 126, (getWidth() - 48.f) * std::min(1.f, rec.peak.load()), 22,
                               5);
        g.setColour(pal.text);
        g.drawText(juce::String(rec.seconds(), 1) + " / " + juce::String(rec.maxSeconds(), 1) +
                       juce::String::fromUTF8(" s  ·  ") +
                       juce::String(20 * std::log10(std::max(.00001f, rec.peak.load())), 1) +
                       " dBFS",
                   24, 160, getWidth() - 48, 25, juce::Justification::left);
    }
    else
        g.drawFittedText(
            language == 0
                ? juce::String::fromUTF8(
                      "Este instrumento preserva seus barramentos MIDI.\nInsira RASTRO Capture na "
                      "faixa/aux de áudio, grave e pare.\nAtualize a lista abaixo e use a gravação "
                      "no "
                      "PLAY — sem exportar para outro aplicativo.\nFluxo AU ainda precisa ser "
                      "validado "
                      "no Logic.")
                : juce::String::fromUTF8("This instrument preserves its MIDI bus layout.\nInsert "
                                         "RASTRO Capture on your "
                                         "audio track/aux, record and stop.\nRefresh below and use "
                                         "the take in PLAY — no "
                                         "external application export.\nThe AU workflow still "
                                         "needs validation in Logic."),
            24, 80, getWidth() - 48, 230, juce::Justification::topLeft, 7);
    g.setColour(pal.muted);
    g.drawText(language == 0
                   ? juce::String::fromUTF8(
                         "Capturas locais compartilhadas · arquivos originais ficam preservados")
                   : juce::String::fromUTF8("Shared local takes · original files are retained"),
               24, 342, getWidth() - 48, 24, juce::Justification::left);
}
bool CapturePanel::keyPressed(const juce::KeyPress &key)
{
    if (key != juce::KeyPress::escapeKey)
        return false;
    close.triggerClick();
    return true;
}
