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
    refresh.setComponentID("refreshCaptures");
    for (juce::Component *c : std::array<juce::Component *, 7>{&monitor, &gain, &route, &takes,
                                                               &sources, &status, &context})
        addAndMakeVisible(c);
    takes.setComponentID("captureTakes");
    sources.setComponentID("captureSources");
    status.setComponentID("captureStatus");
    context.setComponentID("captureContext");
    status.setJustificationType(juce::Justification::topLeft);
    context.setJustificationType(juce::Justification::topLeft);
    status.setMinimumHorizontalScale(1);
    context.setMinimumHorizontalScale(1);
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
        if (direct)
        {
            if (onRecord)
                onRecord();
        }
        else
        {
            auto link = selectedLink();
            if (onRemoteCommand && !link.id.isEmpty())
                onRemoteCommand(link.id, true);
        }
    };
    stop.onClick = [this]
    {
        if (direct)
        {
            rec.stop();
            rec.finishStop();
        }
        else
        {
            auto link = selectedLink();
            if (onRemoteCommand && !link.id.isEmpty())
                onRemoteCommand(link.id, false);
        }
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
    gain.setValue(rec.gain.load());
    gain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 24);
    gain.setTextValueSuffix(" x");
    gain.onValueChange = [this] { rec.gain = float(gain.getValue()); };
    route.addItem("Stereo", 1);
    route.addItem("L / mono", 2);
    route.addItem("R / mono", 3);
    route.setSelectedId(rec.route.load() + 1);
    route.onChange = [this] { rec.route = route.getSelectedId() - 1; };
    for (juce::Component *c : std::array<juce::Component *, 4>{&monitor, &gain, &route, &settings})
        c->setVisible(direct);
    sources.setVisible(!direct);
    sources.onChange = [this]
    {
        lastRemote.clear();
        repaint();
    };
    context.setText("", juce::dontSendNotification);
}
void CapturePanel::hostInsertMode()
{
    insert = true;
    for (juce::Component *c : std::array<juce::Component *, 8>{
             &monitor, &settings, &close, &refresh, &takes, &receive, &preview, &sources})
        c->setVisible(false);
}
rastro::CaptureLink CapturePanel::selectedLink() const
{
    int index = sources.getSelectedId() - 1;
    return index >= 0 && index < int(links.size()) ? links[index] : rastro::CaptureLink{};
}
void CapturePanel::setFiles(std::vector<juce::File> next)
{
    int selected = takes.getSelectedId() - 1;
    auto previous = selected >= 0 && selected < int(files.size()) ? files[selected] : juce::File{};
    bool changed = next != files;
    if (changed)
    {
        files = std::move(next);
        takes.clear(juce::dontSendNotification);
        for (size_t i = 0; i < files.size(); ++i)
            takes.addItem(files[i].getFileNameWithoutExtension(), int(i) + 1);
    }
    int choice = changed ? (files.empty() ? 0 : 1) : takes.getSelectedId();
    if (changed)
        for (size_t i = 0; i < files.size(); ++i)
            if (files[i] == previous)
                choice = int(i) + 1;
    if (pendingSelection != juce::File{})
        for (size_t i = 0; i < files.size(); ++i)
            if (files[i] == pendingSelection)
            {
                choice = int(i) + 1;
                pendingSelection = juce::File{};
                break;
            }
    takes.setSelectedId(choice, juce::dontSendNotification);
    receive.setEnabled(choice > 0);
}
void CapturePanel::setLinks(std::vector<rastro::CaptureLink> next)
{
    auto previous = selectedLink().id;
    juce::StringArray before, after;
    for (auto &l : links)
        before.add(l.id);
    for (auto &l : next)
        after.add(l.id);
    links = std::move(next);
    if (before != after)
    {
        sources.clear(juce::dontSendNotification);
        int selected = links.size() == 1 ? 1 : 0;
        for (size_t i = 0; i < links.size(); ++i)
        {
            sources.addItem(juce::String::fromUTF8("RASTRO Capture · ") +
                                links[i].id.substring(0, 6),
                            int(i) + 1);
            if (links[i].id == previous)
                selected = int(i) + 1;
        }
        sources.setSelectedId(selected, juce::dontSendNotification);
    }
    auto link = selectedLink();
    auto key = link.id + link.request + juce::String(link.outcome);
    if (key != lastRemote)
    {
        lastRemote = key;
        if (link.outcome != rastro::CaptureRecorder::none)
        {
            message(outcomeText(link.outcome, language == 0));
            if (link.outcome == rastro::CaptureRecorder::success)
            {
                pendingSelection = link.file;
                for (size_t i = 0; i < files.size(); ++i)
                    if (files[i] == link.file)
                    {
                        takes.setSelectedId(int(i) + 1, juce::dontSendNotification);
                        pendingSelection = juce::File{};
                    }
            }
        }
    }
}
juce::String CapturePanel::outcomeText(int result, bool pt)
{
    using R = rastro::CaptureRecorder;
    switch (result)
    {
    case R::success:
        return pt ? juce::String::fromUTF8(
                        "Gravação pronta. Use a tomada no PLAY e toque pelo MIDI.")
                  : "Recording ready. Use the take in PLAY and play it via MIDI.";
    case R::empty:
        return pt ? juce::String::fromUTF8("Gravação vazia: nenhum frame recebido. No Logic, ative "
                                           "monitoração de entrada/REC da faixa ou reproduza a "
                                           "fonte. O insert não pode forçar o host a enviar áudio.")
                  : "Empty recording: no frames received. In Logic, enable track input "
                    "monitoring/record-arm or play the source. The insert cannot force the "
                    "host to "
                    "render audio.";
    case R::noSignal:
        return pt ? "Sem sinal capturado: chegaram frames silenciosos. Confira entrada, canal, "
                    "ganho e o sinal ANTES do insert."
                  : "No signal captured: received silent frames. Check input, channel, gain "
                    "and "
                    "signal BEFORE the insert.";
    case R::encodeError:
        return pt ? "Falha ao codificar/carregar WAV. A tomada anterior foi preservada."
                  : "WAV encoding/loading failed. Previous take preserved.";
    case R::saveError:
        return pt ? juce::String::fromUTF8("Falha ao salvar WAV local. Confira espaço e permissões "
                                           "da pasta de capturas.")
                  : "Could not save local WAV. Check capture-folder permissions and free "
                    "space.";
    case R::notReady:
        return pt ? juce::String::fromUTF8("Áudio ainda não preparado pelo host. Ative a faixa ou "
                                           "inicie a reprodução.")
                  : "Audio has not been prepared by the host. Activate the track or start "
                    "playback.";
    default:
        return {};
    }
}
void CapturePanel::update(const visuals::Palette &palette, int lang, bool available)
{
    pal = palette;
    bool pt = lang == 0;
    if (lang != language)
    {
        language = lang;
        close.setButtonText(pt ? "VOLTAR" : "BACK");
        settings.setButtonText(pt ? "DISPOSITIVO / ENTRADA" : "DEVICE / INPUT");
        record.setButtonText(direct ? (pt ? "GRAVAR" : "RECORD")
                                    : (pt ? "GRAVAR NO INSERT" : "RECORD IN INSERT"));
        stop.setButtonText(pt ? "PARAR" : "STOP");
        preview.setButtonText(pt ? "OUVIR SAMPLE" : "PREVIEW SAMPLE");
        refresh.setButtonText(pt ? "ATUALIZAR" : "REFRESH");
        receive.setButtonText(pt ? juce::String::fromUTF8("USAR GRAVAÇÃO") : "USE RECORDING");
        monitor.setButtonText(pt ? juce::String::fromUTF8("Monitor −20 dB · use fones")
                                 : juce::String::fromUTF8("Monitor −20 dB · use headphones"));
        monitor.setTooltip(
            pt ? juce::String::fromUTF8("Desligado por padrão; fechar desliga o monitor.")
               : "Off by default; closing turns monitoring off.");
        sources.setTextWhenNothingSelected(pt ? "Selecione RASTRO Capture"
                                              : "Select RASTRO Capture");
        takes.setTextWhenNothingSelected(pt ? juce::String::fromUTF8("Nenhuma gravação salva")
                                            : "No saved recording");
        previousOutcome = -1;
        repaint();
    }
    int state = rec.state;
    auto link = selectedLink();
    if (!direct)
        state = link.state;
    bool idle = state == rastro::CaptureRecorder::idle;
    record.setEnabled(direct ? available && idle : !link.id.isEmpty() && idle);
    stop.setEnabled(state == rastro::CaptureRecorder::recording ||
                    state == rastro::CaptureRecorder::writing);
    sources.setEnabled(idle);
    receive.setEnabled(takes.getSelectedId() > 0 && idle);
    settings.setEnabled(idle && bool(onSettings));
    status.setColour(juce::Label::textColourId, pal.text);
    context.setColour(juce::Label::textColourId, pal.muted);
    gain.setColour(juce::Slider::textBoxTextColourId, pal.text);
    for (auto *combo : {&takes, &route, &sources})
        combo->setColour(juce::ComboBox::textColourId, pal.text);
    auto serial = rec.blockSerial.load();
    if (serial != lastBlock)
    {
        lastBlock = serial;
        lastAudio = rastro::captureNow();
    }
    bool flowing =
        direct ? rastro::captureNow() - lastAudio < 1500 && lastAudio > 0 : link.hasAudioFlow();
    juce::String hint;
    if (direct)
        hint =
            insert
                ? (pt ? juce::String::fromUTF8("Insert de áudio · selecione o mesmo ID no PLAY: ")
                      : juce::String::fromUTF8("Audio insert · select this ID in PLAY: ")) +
                      insertId.substring(0, 6)
                : (pt ? juce::String::fromUTF8("Captura direta da entrada do Standalone. "
                                               "Monitoramento desligado por padrão.")
                      : "Direct Standalone input capture. Monitoring off by default.");
    else if (links.empty())
        hint = pt ? juce::String::fromUTF8(
                        "Requer RASTRO Capture em Audio FX na faixa/aux de origem. A lista "
                        "detecta "
                        "o insert automaticamente; o PLAY não recebe áudio diretamente.")
                  : "Requires RASTRO Capture in Audio FX on the source track/aux. Inserts are "
                    "detected automatically; PLAY does not receive audio directly.";
    else
        hint = pt ? juce::String::fromUTF8("PLAY controla o insert selecionado. Após PARAR, a "
                                           "tomada aparece abaixo; USAR GRAVAÇÃO carrega o sample.")
                  : "PLAY controls the selected insert. After STOP the take appears below; USE "
                    "RECORDING loads the sample.";
    context.setText(hint, juce::dontSendNotification);
    int result = rec.outcome;
    if (direct && !available && result == rastro::CaptureRecorder::none && idle)
        message(pt ? (insert ? juce::String::fromUTF8("Sem entrada do host. Insira Capture como "
                                                      "Audio FX numa faixa/aux com áudio.")
                             : juce::String::fromUTF8(
                                   "Nenhuma entrada ativa. Abra DISPOSITIVO / ENTRADA e selecione "
                                   "o canal; confira a permissão de microfone."))
                   : (insert ? "No host input. Insert Capture in Audio FX on an audio track/aux."
                             : "No active input. Open DEVICE / INPUT, select channels and check "
                               "microphone permission."));
    if (direct && result != previousOutcome)
    {
        previousOutcome = result;
        if (result != rastro::CaptureRecorder::none)
            message(outcomeText(result, pt));
    }
    if (state != previousState)
    {
        previousState = state;
        repaint();
    }
    float peak = direct ? rec.peak.load() : link.peak;
    if (std::abs(peak - lastPeak) > .001f || state == rastro::CaptureRecorder::recording)
    {
        lastPeak = peak;
        repaint(24, 134, getWidth() - 48, 48);
    }
    if (state == rastro::CaptureRecorder::recording)
        message(flowing
                    ? (pt ? juce::String::fromUTF8(
                                "Gravando o áudio recebido. Clique PARAR para finalizar a tomada.")
                          : "Recording incoming audio. Click STOP to finalize the take.")
                    : (pt ? juce::String::fromUTF8(
                                "Aguardando blocos de áudio do host. Ative entrada/REC da faixa ou "
                                "reproduza a fonte; depois PARAR.")
                          : "Waiting for host audio blocks. Enable track input/record-arm or play "
                            "the source, then STOP."));
}
void CapturePanel::resized()
{
    int w = getWidth();
    close.setBounds(w - 118, 14, 94, 30);
    context.setBounds(24, 56, w - 48, 44);
    if (direct)
    {
        settings.setBounds(24, 104, 210, 30);
        route.setBounds(246, 104, 140, 30);
        gain.setBounds(444, 104, w - 468, 30);
    }
    else
    {
        sources.setBounds(24, 104, w - 352, 32);
        record.setBounds(w - 316, 104, 182, 32);
        stop.setBounds(w - 124, 104, 100, 32);
    }
    if (direct)
    {
        record.setBounds(24, 190, 160, 34);
        stop.setBounds(196, 190, 110, 34);
        monitor.setBounds(320, 190, w - 344, 34);
    }
    int listY = direct ? 258 : 230;
    takes.setBounds(24, listY, w - 190, 34);
    refresh.setBounds(w - 154, listY, 130, 34);
    receive.setBounds(24, listY + 48, 220, 36);
    preview.setBounds(256, listY + 48, 186, 36);
    status.setBounds(24, insert ? 246 : listY + 96, w - 48, 72);
}
void CapturePanel::paint(juce::Graphics &g)
{
    g.setColour(pal.bg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 12);
    g.setColour(pal.outline.withAlpha(.4f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 12, 1);
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(23, juce::Font::bold)));
    g.drawText(insert
                   ? "RASTRO Capture"
                   : (language == 0 ? juce::String::fromUTF8("CAPTURAR ÁUDIO") : "CAPTURE AUDIO"),
               24, 14, getWidth() - 160, 30, juce::Justification::left);
    auto link = selectedLink();
    float peak = direct ? rec.peak.load() : link.peak;
    int frames = direct ? rec.frames.load() : link.frames;
    bool flowing =
        direct ? lastAudio > 0 && rastro::captureNow() - lastAudio < 1500 : link.hasAudioFlow();
    g.setColour(pal.surface);
    g.fillRoundedRectangle(24, 146, getWidth() - 48.f, 10, 5);
    g.setColour(peak >= 1 ? juce::Colours::orangered : pal.control);
    g.fillRoundedRectangle(24, 146, (getWidth() - 48.f) * std::clamp(peak, 0.f, 1.f), 10, 5);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.setColour(pal.muted);
    auto level = peak > 1e-5f ? juce::String(juce::Decibels::gainToDecibels(peak), 1) + " dBFS"
                              : (language == 0 ? "sem sinal" : "no signal");
    auto flow =
        flowing
            ? (language == 0 ? juce::String::fromUTF8("áudio em fluxo") : "audio flowing")
            : (language == 0 ? juce::String::fromUTF8("sem blocos de áudio") : "no audio blocks");
    g.drawText(juce::String(direct ? rec.seconds() : frames / std::max(8000., link.rate), 2) +
                   juce::String::fromUTF8(" s  ·  ") + level + juce::String::fromUTF8("  ·  ") +
                   flow,
               24, 164, getWidth() - 48, 20, juce::Justification::left);
    if (!insert)
    {
        g.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
        g.setColour(pal.text);
        g.drawText(language == 0 ? juce::String::fromUTF8("GRAVAÇÕES LOCAIS") : "LOCAL RECORDINGS",
                   24, (direct ? 234 : 206), getWidth() - 48, 20, juce::Justification::left);
    }
}
bool CapturePanel::keyPressed(const juce::KeyPress &key)
{
    if (key != juce::KeyPress::escapeKey || insert)
        return false;
    close.triggerClick();
    return true;
}
