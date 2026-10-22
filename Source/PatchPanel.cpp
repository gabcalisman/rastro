#include "PatchPanel.h"
PatchPanel::PatchPanel(RastroProcessor &processor) : p(processor)
{
    setComponentID("patchPanel");
    for (auto *button : std::array<juce::TextButton *, 8>{&generate, &pause, &cancel, &compare,
                                                          &variation, &undo, &save, &load})
        addAndMakeVisible(button);
    generate.setComponentID("generatePatches");
    pause.setComponentID("pausePatches");
    cancel.setComponentID("cancelPatches");
    compare.setComponentID("comparePatch");
    variation.setComponentID("varyPatch");
    undo.setComponentID("undoPatch");
    save.setComponentID("savePatch");
    load.setComponentID("loadPatch");
    addAndMakeVisible(direction);
    direction.setComponentID("patchDirection");
    direction.addItem("Mais fiel / Faithful", 1);
    direction.addItem("Transformado / Transformed", 2);
    direction.setSelectedId(1);
    status.setComponentID("patchStatus");
    addAndMakeVisible(status);
    addAndMakeVisible(help);
    help.setMinimumHorizontalScale(1);
    help.setJustificationType(juce::Justification::topLeft);
    for (int i = 0; i < 6; ++i)
    {
        addAndMakeVisible(candidates[i]);
        candidates[i].setComponentID("patchCandidate" + juce::String(i));
        candidates[i].onClick = [this, i]
        {
            if (i >= int(results.size()) || p.patchSearch.sourceUsed() != p.source.load())
                return;
            if (!hasSelection || selectedSource != p.source.load())
                before = p.patchValues();
            selected = results[i].values;
            selectedSource = p.source.load();
            hasSelection = true;
            hasUndo = false;
            showingBefore = false;
            p.applyPatch(selected);
            audition();
            update();
        };
    }
    generate.onClick = [this]
    {
        fileStatus = {};
        p.patchSearch.start(p.source.load(), p.settings(), direction.getSelectedId() == 2,
                            juce::Random::getSystemRandom().nextInt());
        update();
    };
    pause.onClick = [this]
    {
        p.patchSearch.paused = !p.patchSearch.paused.load();
        update();
    };
    cancel.onClick = [this] { p.patchSearch.cancel(); };
    compare.onClick = [this]
    {
        if (!hasSelection)
            return;
        if (!showingBefore)
            selected = p.patchValues();
        showingBefore = !showingBefore;
        p.applyPatch(showingBefore ? before : selected);
        audition();
        update();
    };
    variation.onClick = [this]
    {
        if (!hasSelection)
            return;
        variationBefore = p.patchValues();
        auto values = variationBefore;
        auto &random = juce::Random::getSystemRandom();
        for (int i : {1, 2, 3, 4, 5, 6, 7, 9, 10})
        {
            auto *parameter = p.params.getParameter(rastro::patchIds[i]);
            auto normalized = parameter->convertTo0to1(values[i]);
            values[i] = parameter->convertFrom0to1(
                juce::jlimit(0.f, 1.f, normalized + (random.nextFloat() - .5f) * .22f));
        }
        p.applyPatch(values);
        selected = values;
        showingBefore = false;
        hasUndo = true;
        audition();
        update();
    };
    undo.onClick = [this]
    {
        if (hasUndo)
        {
            p.applyPatch(variationBefore);
            selected = variationBefore;
            hasUndo = false;
            showingBefore = false;
            audition();
            update();
        }
    };
    save.onClick = [this] { chooseFile(true); };
    load.onClick = [this] { chooseFile(false); };
    update();
}
PatchPanel::~PatchPanel()
{
    if (auditionNote >= 0)
        p.keyboard.noteOff(1, auditionNote, 0);
}
void PatchPanel::audition()
{
    if (auditionNote >= 0)
        p.keyboard.noteOff(1, auditionNote, 0);
    auditionNote = int(p.value("root"));
    p.keyboard.noteOn(1, auditionNote, .8f);
    auditionUntil = juce::Time::getMillisecondCounterHiRes() + 650;
    auto generation = ++auditionGeneration;
    juce::Timer::callAfterDelay(650,
                                [safe = juce::Component::SafePointer<PatchPanel>(this), generation]
                                {
                                    if (safe && safe->auditionGeneration == generation &&
                                        safe->auditionNote >= 0)
                                    {
                                        safe->p.keyboard.noteOff(1, safe->auditionNote, 0);
                                        safe->auditionNote = -1;
                                    }
                                });
}
void PatchPanel::chooseFile(bool saving)
{
    chooser = std::make_unique<juce::FileChooser>(saving ? "Salvar patch / Save patch"
                                                         : "Abrir patch / Load patch",
                                                  juce::File{}, "*.rastro");
    chooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles |
            (saving ? juce::FileBrowserComponent::saveMode |
                          juce::FileBrowserComponent::warnAboutOverwriting
                    : juce::FileBrowserComponent::openMode),
        [safe = juce::Component::SafePointer<PatchPanel>(this), saving](const juce::FileChooser &fc)
        {
            if (!safe)
                return;
            auto file = fc.getResult();
            if (file == juce::File{})
                return;
            if (saving)
                file = file.withFileExtension("rastro");
            safe->fileBusy = true;
            safe->update();
            safe->p.patchFileAsync(file, saving,
                                   [safe, saving](bool ok)
                                   {
                                       if (!safe)
                                           return;
                                       safe->fileBusy = false;
                                       safe->fileStatus =
                                           ok ? (saving ? "Patch salvo / Patch saved"
                                                        : "Patch carregado / Patch loaded")
                                              : "Falha no arquivo / File failed (max 64 MB)";
                                       if (ok && !saving)
                                       {
                                           safe->hasSelection = true;
                                           safe->selectedSource = safe->p.source.load();
                                           safe->selected = safe->p.patchValues();
                                           safe->before = safe->selected;
                                           safe->showingBefore = false;
                                           safe->hasUndo = false;
                                       }
                                       safe->update();
                                   });
        });
}
void PatchPanel::update()
{
    if (auditionNote >= 0 && juce::Time::getMillisecondCounterHiRes() >= auditionUntil)
    {
        p.keyboard.noteOff(1, auditionNote, 0);
        auditionNote = -1;
    }
    bool pt = p.language == 0, busy = p.patchSearch.busy;
    auto pal = visuals::palette(p);
    if (hasSelection && selectedSource != p.source.load())
    {
        hasSelection = false;
        hasUndo = false;
    }
    if (seenRevision != p.patchSearch.revision)
    {
        seenRevision = p.patchSearch.revision;
        results = p.patchSearch.results();
    }
    generate.setButtonText(pt ? "GERAR PATCHES" : "GENERATE PATCHES");
    pause.setButtonText(p.patchSearch.paused ? (pt ? "RETOMAR" : "RESUME")
                                             : (pt ? "PAUSAR" : "PAUSE"));
    cancel.setButtonText(pt ? "CANCELAR" : "CANCEL");
    compare.setButtonText(showingBefore ? "A: ANTES / BEFORE" : "B: PATCH");
    variation.setButtonText(pt ? "VARIAR PATCH" : "VARY PATCH");
    undo.setButtonText(pt ? juce::String::fromUTF8("DESFAZER VARIAÇÃO") : "UNDO VARIATION");
    save.setButtonText(pt ? "SALVAR PATCH" : "SAVE PATCH");
    load.setButtonText(pt ? "ABRIR PATCH" : "LOAD PATCH");
    generate.setEnabled(!busy && !fileBusy && p.source.load());
    direction.setEnabled(!busy);
    pause.setEnabled(busy);
    cancel.setEnabled(busy);
    compare.setEnabled(hasSelection && !fileBusy);
    variation.setEnabled(hasSelection && !fileBusy && !showingBefore);
    undo.setEnabled(hasUndo && !fileBusy);
    save.setEnabled(p.source.load() && !fileBusy);
    load.setEnabled(!busy && !fileBusy);
    juce::String message =
        pt ? juce::String::fromUTF8("Selecione a região na waveform e gere opções.")
           : "Select the waveform region and generate options.";
    if (busy)
        message = (p.patchSearch.paused ? (pt ? juce::String::fromUTF8("Pausado · ")
                                              : juce::String::fromUTF8("Paused · "))
                                        : (pt ? juce::String::fromUTF8("Renderizando · ")
                                              : juce::String::fromUTF8("Rendering · "))) +
                  juce::String(int(p.patchSearch.progress * 100)) +
                  juce::String::fromUTF8("% · CPU ~50% de um núcleo / one core");
    else if (p.patchSearch.status == rastro::PatchSearch::complete)
        message = pt ? juce::String::fromUTF8(
                           "Clique numa opção para aplicar e ouvir; depois edite no SYNTH.")
                     : "Click an option to apply and audition; then edit it in SYNTH.";
    else if (p.patchSearch.status == rastro::PatchSearch::silent)
        message = pt ? juce::String::fromUTF8("Fonte silenciosa: escolha um trecho com áudio.")
                     : "Silent source: select a region with audio.";
    else if (p.patchSearch.status == rastro::PatchSearch::canceled)
        message = pt ? "Busca cancelada; o patch atual foi preservado."
                     : "Search canceled; current patch preserved.";
    else if (p.patchSearch.status == rastro::PatchSearch::failed)
        message =
            pt ? "A busca falhou. Tente um trecho menor." : "Search failed. Try a smaller region.";
    if (!fileStatus.isEmpty())
        message = fileStatus;
    status.setText(message, juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, pal.text);
    help.setText(
        pt ? juce::String::fromUTF8(
                 "Busca aproximada: 48 candidatos reais, trecho de até 0,4 s a 16 kHz. "
                 "Compara envelope e bandas espectrais; não é IA nem transcrição. Usa o "
                 "sample original e o trim, sem inferir a raiz. O rack atual continua na "
                 "audição. A/B compara parâmetros; salvar inclui o áudio e a sessão inteira.")
           : "Approximate search: 48 real candidates, up to 0.4 s at 16 kHz. Compares "
             "envelope and spectral bands; no AI or transcription. Uses the source sample "
             "and trim; does not infer root. Audition retains the current rack. A/B "
             "compares settings; save embeds audio and the whole session.",
        juce::dontSendNotification);
    help.setColour(juce::Label::textColourId, pal.muted);
    for (int i = 0; i < 6; ++i)
    {
        bool valid = i < int(results.size()) && p.patchSearch.sourceUsed() == p.source.load();
        candidates[i].setEnabled(valid && !busy && !fileBusy);
        candidates[i].setButtonText(valid ? ("PATCH " + juce::String(i + 1) +
                                             juce::String::fromUTF8("   ·   Δ ") +
                                             juce::String(results[i].distance, 3))
                                          : (pt ? "Aguardando busca" : "Awaiting search"));
    }
    generate.setTooltip(pt ? juce::String::fromUTF8(
                                 "Renderiza candidatos no worker; não muda a fonte, raiz ou trim.")
                           : "Renders candidates on a worker; preserves source, root and trim.");
    variation.setTooltip(
        pt ? juce::String::fromUTF8(
                 "Variação limitada do synth, separada do ALEATÓRIO do rack. Pode ser desfeita.")
           : "Bounded synth variation, separate from rack RANDOM. Undo available.");
    compare.setTooltip(
        pt ? juce::String::fromUTF8(
                 "Alterna A (antes da escolha) e B (patch escolhido), com audição MIDI.")
           : "Switches A (before choosing) and B (selected patch), with MIDI audition.");
}
void PatchPanel::resized()
{
    int w = getWidth();
    direction.setBounds(8, 8, 220, 32);
    generate.setBounds(240, 8, 180, 32);
    pause.setBounds(432, 8, 104, 32);
    cancel.setBounds(548, 8, 104, 32);
    status.setBounds(8, 48, w - 16, 38);
    int cw = (w - 36) / 3;
    for (int i = 0; i < 6; ++i)
        candidates[i].setBounds(8 + i % 3 * (cw + 10), 98 + i / 3 * 66, cw, 52);
    int bw = (w - 56) / 5;
    std::array<juce::TextButton *, 5> tools{&compare, &variation, &undo, &save, &load};
    for (int i = 0; i < 5; ++i)
        tools[i]->setBounds(8 + i * (bw + 10), 240, bw, 38);
    help.setBounds(8, 298, w - 16, 100);
}
void PatchPanel::paint(juce::Graphics &g)
{
    auto pal = visuals::palette(p);
    g.fillAll(pal.bg);
    g.setColour(pal.control);
    g.fillRoundedRectangle(8, 89, float(getWidth() - 16), 2, 1);
}
