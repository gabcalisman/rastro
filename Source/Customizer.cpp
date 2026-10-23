#include "Customizer.h"
namespace
{
juce::String utf(const char *s) { return juce::String::fromUTF8(s); }
} // namespace
VisualCustomizer::Snapshot VisualCustomizer::capture() const
{
    Snapshot out{};
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = p.value(visuals::visualIds[i]);
    return out;
}
void VisualCustomizer::restore(const Snapshot &s)
{
    restoring = true;
    for (size_t i = 0; i < s.size(); ++i)
        p.set(visuals::visualIds[i], s[i]);
    restoring = false;
    observed = capture();
    syncFields();
    repaint();
}
void VisualCustomizer::beforeEdit()
{
    if (restoring)
        return;
    if (undoStack.size() == 32)
        undoStack.erase(undoStack.begin());
    undoStack.push_back(capture());
    redoStack.clear();
    undo.setEnabled(true);
    redo.setEnabled(false);
}
void VisualCustomizer::undoAppearance()
{
    if (undoStack.empty())
        return;
    redoStack.push_back(capture());
    auto s = undoStack.back();
    undoStack.pop_back();
    restore(s);
}
void VisualCustomizer::redoAppearance()
{
    if (redoStack.empty())
        return;
    undoStack.push_back(capture());
    auto s = redoStack.back();
    redoStack.pop_back();
    restore(s);
}
void VisualCustomizer::chooseLook(bool saving)
{
    chooser = std::make_unique<juce::FileChooser>(saving ? "Salvar visual / Save look"
                                                         : "Abrir visual / Load look",
                                                  juce::File{}, "*.rastro-look");
    chooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles |
            (saving ? juce::FileBrowserComponent::saveMode |
                          juce::FileBrowserComponent::warnAboutOverwriting
                    : juce::FileBrowserComponent::openMode),
        [safe = juce::Component::SafePointer<VisualCustomizer>(this),
         saving](const juce::FileChooser &fc)
        {
            if (!safe || fc.getResult() == juce::File{})
                return;
            auto file = saving ? fc.getResult().withFileExtension("rastro-look") : fc.getResult();
            if (!saving)
                safe->beforeEdit();
            safe->fileBusy = true;
            safe->p.appearanceFileAsync(
                file, saving, safe->lookScope.getSelectedId() - 1,
                [safe, saving](bool ok)
                {
                    if (!safe)
                        return;
                    safe->fileBusy = false;
                    safe->fileStatus.setText(
                        ok ? (saving ? "Visual salvo / Look saved"
                                     : "Visual aplicado / Look applied")
                           : juce::String::fromUTF8("Arquivo visual inválido / Look file failed"),
                        juce::dontSendNotification);
                    safe->observed = safe->capture();
                    safe->syncFields();
                    safe->repaint();
                });
        });
}
void VisualCustomizer::open()
{
    entry = observed = capture();
    undoStack.clear();
    redoStack.clear();
    syncFields();
    setVisible(true);
    toFront(true);
    grabKeyboardFocus();
}
void VisualCustomizer::dismiss()
{
    setVisible(false);
    if (onClose)
        onClose();
}
void VisualCustomizer::visibilityChanged()
{
    // No timer/animation owns a lifetime here; the editor steps only the visible page.
}
VisualCustomizer::Choice::Choice(VisualCustomizer &owner, int i, int type)
    : ui(owner), index(i), kind(type)
{
    setComponentID((kind == 0   ? "paletteChoice"
                    : kind == 1 ? "knobChoice"
                                : "bodyChoice") +
                   juce::String(i));
    setButtonText(kind == 0   ? juce::String(visuals::paletteNames[i])
                  : kind == 1 ? juce::StringArray{"Vintage", "Amplifier", "Synth", "Console"}[i]
                              : juce::StringArray{"Orbit", "Soul", "Metro"}[i]);
    setWantsKeyboardFocus(true);
    onClick = [this]
    {
        ui.beforeEdit();
        if (kind == 0)
        {
            ui.p.set("cosmicArcade", index == 8 ? 1.f : 0.f);
            if (index < 8)
                ui.p.set("colourSet", (float)index + 1);
            for (auto id : {"visualBackground", "visualAccent", "visualControls"})
                ui.p.set(id, 0);
        }
        else
            ui.p.set(kind == 1 ? "knobDesign" : "avatarBody", (float)index + (kind == 1 ? 1 : 0));
        ui.observed = ui.capture();
        ui.syncFields();
        ui.repaint();
    };
}
void VisualCustomizer::Choice::paintButton(juce::Graphics &g, bool over, bool)
{
    auto pal = visuals::palette(ui.p);
    bool selected = kind == 0   ? (index == 8 ? ui.p.value("cosmicArcade") > .5f
                                              : ui.p.value("cosmicArcade") < .5f &&
                                                  (int)ui.p.value("colourSet") == index + 1)
                    : kind == 1 ? visuals::knobStyle(ui.p) == index
                                : (int)ui.p.value("avatarBody") == index;
    auto box = getLocalBounds().toFloat().reduced(3);
    g.setColour(pal.button.brighter(over ? .1f : 0));
    g.fillRoundedRectangle(box, 12);
    g.setColour(selected ? pal.highlight : pal.outline);
    g.drawRoundedRectangle(box, 12, selected ? 2.f : 1.f);
    if (kind == 0)
    {
        auto sample = visuals::colourSet(index);
        auto tile = box.reduced(9, 7).withHeight(32);
        g.setColour(sample.bg);
        g.fillRoundedRectangle(tile, 6);
        g.setColour(sample.surface);
        g.fillRoundedRectangle(tile.reduced(5), 4);
        g.setColour(sample.text);
        g.setFont(juce::Font(juce::FontOptions(11, juce::Font::bold)));
        g.drawText("RASTRO", tile.reduced(7, 3).withHeight(11), juce::Justification::left);
        for (int i = 0; i < 3; ++i)
        {
            g.setColour(i == 0 ? sample.highlight : i == 1 ? sample.control : sample.secondary);
            g.fillRoundedRectangle(tile.getX() + 8 + i * (tile.getWidth() - 16) / 3,
                                   tile.getY() + 19, (tile.getWidth() - 16) / 3 - 4, 7, 2);
        }
    }
    else if (kind == 1)
        visuals::knob(g, {box.getCentreX() - 38, box.getY() + 9, 76, 76}, .64f, 3.8f, 8.75f, index,
                      pal.control);
    else
    {
        auto b = juce::Rectangle<float>(box.getCentreX() - 21, box.getY() + 15, 42, 45);
        g.setColour(pal.highlight);
        if (index == 0)
            g.fillRoundedRectangle(b, 18);
        else if (index == 1)
            g.fillEllipse(b);
        else
            g.fillRoundedRectangle(b, 5);
        g.setColour(juce::Colour(0xff171626));
        g.fillRoundedRectangle(b.reduced(5).withHeight(12).translated(0, 9), 4);
        g.setColour(pal.secondary);
        g.drawLine(b.getX() + 9, b.getY() + 14, b.getRight() - 9, b.getY() + 14, 2);
        g.setColour(pal.text);
        g.fillRoundedRectangle(b.getX() - 2, b.getBottom() + 2, 18, 7, 2);
        g.fillRoundedRectangle(b.getRight() - 16, b.getBottom() + 2, 21, 7, 2);
    }
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(14, juce::Font::bold)));
    auto caption = box.withTrimmedTop(kind == 0 ? 43 : kind == 1 ? 91 : 70);
    if (kind == 1)
        caption.setHeight(19);
    g.drawText((selected ? utf("• ") : "") + getButtonText(), caption,
               juce::Justification::centred);
    if (kind == 1)
    {
        const juce::StringArray material =
            ui.p.language == 0 ? juce::StringArray{utf("Marfim canelado"), utf("Metal escovado"),
                                                   utf("Borracha"), utf("Plástico de console")}
                               : juce::StringArray{"Fluted ivory", "Brushed metal", "Soft rubber",
                                                   "Console plastic"};
        g.setFont(juce::Font(juce::FontOptions(11)));
        g.setColour(pal.muted);
        g.drawText(material[index], box.withTrimmedTop(110), juce::Justification::centred);
    }
    if (hasKeyboardFocus(true))
    {
        g.setColour(pal.text);
        g.drawRoundedRectangle(box.reduced(4), 9, 2);
    }
}
VisualCustomizer::VisualCustomizer(RastroProcessor &processor) : p(processor), avatar(p)
{
    setComponentID("visualCustomizer");
    setWantsKeyboardFocus(true);
    setFocusContainerType(FocusContainerType::focusContainer);
    for (auto *b : {&close, &reset, &undo, &redo, &revert, &saveLook, &loadLook})
    {
        addAndMakeVisible(b);
        b->setWantsKeyboardFocus(true);
    }
    close.setComponentID("closeCustomizer");
    reset.setComponentID("resetVisuals");
    undo.setComponentID("undoVisuals");
    redo.setComponentID("redoVisuals");
    saveLook.setComponentID("saveLook");
    loadLook.setComponentID("loadLook");
    lookScope.setComponentID("lookScope");
    addAndMakeVisible(lookScope);
    addAndMakeVisible(fileStatus);
    lookScope.addItemList({"Visual completo / Full look", "Personagem / Character",
                           utf("Cenário / Room"), utf("Dança / Dance"), "Rack"},
                          1);
    lookScope.setSelectedId(1);
    lookScope.setTooltip(
        "Salvar: escolha os grupos incluidos. Abrir: aplica apenas os grupos contidos no arquivo, "
        "sem mudar o som. / Save scope; loading applies only file groups, never sound.");
    saveLook.onClick = [this] { chooseLook(true); };
    loadLook.onClick = [this] { chooseLook(false); };
    redo.onClick = [this] { redoAppearance(); };
    revert.setComponentID("revertVisuals");
    close.onClick = [this] { dismiss(); };
    undo.onClick = [this] { undoAppearance(); };
    revert.onClick = [this]
    {
        beforeEdit();
        restore(entry);
    };
    reset.onClick = [this]
    {
        beforeEdit();
        Snapshot s{};
        for (size_t i = 0; i < s.size(); ++i)
        {
            auto *param = p.params.getParameter(visuals::visualIds[i]);
            s[i] = param->convertFrom0to1(param->getDefaultValue());
        }
        restore(s);
    };
    for (int i = 0; i < 6; ++i)
    {
        addAndMakeVisible(tabs[i]);
        tabs[i].setComponentID("visualTab" + juce::String(i));
        tabs[i].setWantsKeyboardFocus(true);
        tabs[i].onClick = [this, i] { chooseTab(i); };
        if (i < 3)
        {
            bodies[i] = std::make_unique<Choice>(*this, i, 2);
            addChildComponent(*bodies[i]);
        }
    }
    for (int i = 0; i < 9; ++i)
    {
        palettes[i] = std::make_unique<Choice>(*this, i, 0);
        addChildComponent(*palettes[i]);
        if (i < 4)
        {
            knobs[i] = std::make_unique<Choice>(*this, i, 1);
            addChildComponent(*knobs[i]);
        }
    }
    addChildComponent(avatar);
    addChildComponent(previewKnob);
    addChildComponent(previewLabel);
    previewKnob.setComponentID("liveKnobPreview");
    previewKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    previewKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
    previewKnob.setRange(0, 100, 1);
    previewKnob.setValue(64);
    previewKnob.setTextValueSuffix("%");
    previewKnob.setWantsKeyboardFocus(true);
    previewLabel.setJustificationType(juce::Justification::centred);
    const char *ids[]{"visualTheme",  "visualBackground", "visualAccent",     "visualControls",
                      "avatarColour", "avatarBody",       "avatarEyes",       "avatarAccessory",
                      "avatarHair",   "avatarHorns",      "avatarClothes",    "avatarGlasses",
                      "avatarTail",   "avatarWings",      "avatarInstrument", "avatarPresentation",
                      "avatarHalo",   "avatarSpecies",    "avatarMaterial",   "sceneKind",
                      "sceneColour",  "sceneLight",       "sceneDecor",       "sceneReaction",
                      "animation",    "danceStyle",       "danceAmount",      "gColour",
                      "cColour",      "bColour",          "wColour",          "dColour",
                      "kColour",      "gJamLock",         "cJamLock",         "bJamLock",
                      "wJamLock",     "dJamLock",         "kJamLock"};
    for (int i = 0; i < int(std::size(ids)); ++i)
    {
        auto f = std::make_unique<Field>();
        f->id = ids[i];
        f->area = i < 4 ? 0 : i < 19 ? 2 : i < 24 ? 3 : i < 27 ? 4 : 5;
        f->combo.setComponentID(f->id);
        f->label.setComponentID(f->id + "Label");
        f->combo.setWantsKeyboardFocus(true);
        f->label.setFont(juce::Font(juce::FontOptions(13)));
        addChildComponent(f->combo);
        addChildComponent(f->label);
        auto *field = f.get();
        f->combo.onChange = [this, field]
        {
            if (restoring)
                return;
            beforeEdit();
            p.set(field->id, (float)(field->combo.getSelectedId() - 1));
            observed = capture();
            previewDirty = true;
            repaint();
        };
        fields.push_back(std::move(f));
    }
    localize();
    observed = capture();
    chooseTab(0);
}
void VisualCustomizer::syncFields()
{
    previewDirty = true;
    for (auto &f : fields)
        f->combo.setSelectedId((int)p.value(f->id.toRawUTF8()) + 1, juce::dontSendNotification);
    undo.setEnabled(!undoStack.empty());
    redo.setEnabled(!redoStack.empty());
}
void VisualCustomizer::localize()
{
    lastLanguage = p.language;
    bool pt = lastLanguage == 0;
    close.setButtonText(pt ? "VOLTAR" : "BACK");
    reset.setButtonText(pt ? utf("Padrão") : "Defaults");
    undo.setButtonText(pt ? "Desfazer" : "Undo");
    redo.setButtonText(pt ? "Refazer" : "Redo");
    saveLook.setButtonText(pt ? "Salvar visual" : "Save look");
    loadLook.setButtonText(pt ? "Abrir visual" : "Load look");
    revert.setButtonText(pt ? "Voltar ao abrir" : "Revert to opening");
    const char *namesPt[]{"CORES", "KNOBS", "PERSONAGEM", "CENÁRIO", "DANÇA", "RACK"},
        *namesEn[]{"COLOURS", "KNOBS", "CHARACTER", "ROOM", "DANCE", "RACK"};
    for (int i = 0; i < 6; ++i)
        tabs[i].setButtonText(utf(pt ? namesPt[i] : namesEn[i]));
    const juce::StringArray bodyNames = pt ? juce::StringArray{"Orbit", "Soul", "Metro"}
                                           : juce::StringArray{"Orbit", "Soul", "Metro"};
    for (int i = 0; i < 3; ++i)
        bodies[i]->setButtonText(bodyNames[i]);
    previewLabel.setText(pt ? utf("Prévia interativa\nNão altera o áudio")
                            : "Interactive preview\nDoes not change audio",
                         juce::dontSendNotification);
    const juce::String labelsPt[]{"Chassi",
                                  "Fundo",
                                  "Destaque",
                                  "Controles",
                                  "Cor do personagem",
                                  "Corpo",
                                  "Olhos",
                                  juce::String::fromUTF8("Acessório"),
                                  "Cabelo",
                                  "Metal",
                                  "Roupa",
                                  juce::String::fromUTF8("Óculos"),
                                  "Cauda",
                                  "Ombreiras",
                                  "Instrumento",
                                  utf("Apresentação"),
                                  utf("Auréola"),
                                  "Criatura",
                                  "Material",
                                  "Ambiente",
                                  "Cor do quarto",
                                  "Luz",
                                  utf("Decoração"),
                                  utf("Reação musical"),
                                  utf("Animação"),
                                  "Estilo",
                                  "Intensidade",
                                  "GHOST / cor",
                                  "CRUMBLE / cor",
                                  "BLOOM / cor",
                                  "WOBBLE / cor",
                                  "DUST / cor",
                                  "CRUSH / cor",
                                  "GHOST / JAM",
                                  "CRUMBLE / JAM",
                                  "BLOOM / JAM",
                                  "WOBBLE / JAM",
                                  "DUST / JAM",
                                  "CRUSH / JAM"};
    const juce::String labelsEn[]{"Chassis",
                                  "Background",
                                  "Accent",
                                  "Controls",
                                  "Character colour",
                                  "Body",
                                  "Eyes",
                                  "Accessory",
                                  "Hair",
                                  "Metal",
                                  "Clothes",
                                  "Glasses",
                                  "Tail",
                                  "Shoulders",
                                  "Instrument",
                                  "Presentation",
                                  "Halo",
                                  "Creature",
                                  "Material",
                                  "Room",
                                  "Room colour",
                                  "Lighting",
                                  "Decoration",
                                  "Musical reaction",
                                  "Animation",
                                  "Style",
                                  "Intensity",
                                  "GHOST / colour",
                                  "CRUMBLE / colour",
                                  "BLOOM / colour",
                                  "WOBBLE / colour",
                                  "DUST / colour",
                                  "CRUSH / colour",
                                  "GHOST / JAM",
                                  "CRUMBLE / JAM",
                                  "BLOOM / JAM",
                                  "WOBBLE / JAM",
                                  "DUST / JAM",
                                  "CRUSH / JAM"};
    const juce::StringArray choices[]{
        {"Retro", "Futuro", "Arcade", pt ? juce::String::fromUTF8("Orgânico") : "Organic"},
        {pt ? "Da paleta" : "Follow palette", "Midnight", "Lagoon", "Afterparty", "Ember"},
        {pt ? "Da paleta" : "Follow palette", pt ? "Lavanda" : "Lavender", pt ? "Ciano" : "Cyan",
         pt ? "Magenta" : "Magenta", pt ? juce::String::fromUTF8("Âmbar") : "Amber"},
        {pt ? "Da paleta" : "Follow palette", pt ? "Lavanda" : "Lavender", pt ? "Ciano" : "Cyan",
         "Magenta", pt ? juce::String::fromUTF8("Âmbar") : "Amber"},
        {pt ? "Da paleta" : "Follow palette", pt ? "Lavanda" : "Lavender", pt ? "Ciano" : "Cyan",
         "Magenta", pt ? juce::String::fromUTF8("Âmbar") : "Amber"},
        {"Orbit", "Soul", "Metro"},
        {"Soul", "Mono", "Digital"},
        {pt ? "Nenhum" : "None", pt ? juce::String::fromUTF8("Fones de estúdio") : "Studio phones",
         pt ? juce::String::fromUTF8("Lenço de seda") : "Silk scarf"},
        {pt ? "Nenhum" : "None", "Pompadour", "Sleek"},
        {pt ? "Nenhum" : "None", "Chrome fins", "Orbit fins"},
        {"Flight suit", "Soul jacket", "Studio vest"},
        {pt ? "Nenhum" : "None", "Aviator", pt ? "Visor" : "Visor"},
        {pt ? "Nenhuma" : "None", "Audio cable", "Ribbon"},
        {pt ? "Nenhuma" : "None", "Pleated cape", "Chrome plates"},
        {pt ? "Nenhum" : "None", pt ? "Keytar" : "Keytar", "Beat pads"},
        {pt ? utf("Andrógina") : "Androgynous", pt ? "Masculina" : "Masculine",
         pt ? "Feminina" : "Feminine"},
        {pt ? "Desligada" : "Off", pt ? "Ligada" : "On"},
        {"Pilot", "Mooncat", "Nova"},
        {"Satin", "Chrome", "Enamel"},
        {"Stage", "Soul Bedroom", "Orbital Studio", "Sunset Loft"},
        {pt ? "Da paleta" : "Follow palette", "Violet", "Coral", "Lagoon", "Ivory"},
        {"Soft", "Neon", "Golden hour"},
        {"Minimal", "Plants", "Vinyl", "Synth wall"},
        {"Off", "Soft", "Bold"},
        {"Off", "Reduced", "Full"},
        {"Soul step", "Disco", "Half-time"},
        {"Still", "Groove", "Expressive"},
        {"Auto", "Violet", "Cyan", "Coral", "Gold"},
        {"Auto", "Violet", "Cyan", "Coral", "Gold"},
        {"Auto", "Violet", "Cyan", "Coral", "Gold"},
        {"Auto", "Violet", "Cyan", "Coral", "Gold"},
        {"Auto", "Violet", "Cyan", "Coral", "Gold"},
        {"Auto", "Violet", "Cyan", "Coral", "Gold"},
        {"JAM: free", "JAM: locked"},
        {"JAM: free", "JAM: locked"},
        {"JAM: free", "JAM: locked"},
        {"JAM: free", "JAM: locked"},
        {"JAM: free", "JAM: locked"},
        {"JAM: free", "JAM: locked"}};
    for (size_t i = 0; i < fields.size(); ++i)
    {
        auto &f = *fields[i];
        f.label.setText(pt ? labelsPt[i] : labelsEn[i], juce::dontSendNotification);
        f.combo.clear(juce::dontSendNotification);
        f.combo.addItemList(choices[i], 1);
    }
    syncFields();
}
void VisualCustomizer::chooseTab(int next)
{
    tab = juce::jlimit(0, 5, next);
    for (int i = 0; i < 6; ++i)
        tabs[i].setToggleState(tab == i, juce::dontSendNotification);
    for (auto &b : palettes)
        b->setVisible(tab == 0);
    for (auto &b : knobs)
        b->setVisible(tab == 1);
    for (auto &b : bodies)
        b->setVisible(tab == 2);
    for (auto &f : fields)
    {
        bool visible = f->area == tab && f->id != "avatarBody";
        f->label.setVisible(visible);
        f->combo.setVisible(visible);
    }
    if (tab >= 2 && tab <= 4)
        avatar.resume();
    avatar.setVisible(tab >= 2 && tab <= 4);
    previewKnob.setVisible(tab == 1);
    previewLabel.setVisible(tab == 1);
    resized();
    repaint();
}
bool VisualCustomizer::update(int mode)
{
    if (lastLanguage != p.language)
        localize();
    auto now = capture();
    if (now != observed)
    {
        observed = now;
        syncFields();
        repaint();
    }
    if ((tab == 0 || tab == 5) && previewDirty && renderInstrumentPreview)
    {
        instrumentPreview = renderInstrumentPreview();
        previewDirty = false;
        repaint();
    }
    undo.setEnabled(!undoStack.empty());
    redo.setEnabled(!redoStack.empty());
    saveLook.setEnabled(!fileBusy);
    loadLook.setEnabled(!fileBusy);
    lookScope.setEnabled(!fileBusy);
    for (auto &f : fields)
        f->combo.setEnabled(!fileBusy);
    for (auto &b : palettes)
        b->setEnabled(!fileBusy);
    for (auto &b : knobs)
        b->setEnabled(!fileBusy);
    for (auto &b : bodies)
        b->setEnabled(!fileBusy);
    undo.setEnabled(!fileBusy && !undoStack.empty());
    redo.setEnabled(!fileBusy && !redoStack.empty());
    reset.setEnabled(!fileBusy);
    revert.setEnabled(!fileBusy);
    if (tab >= 2 && tab <= 4)
    {
        avatar.update(mode);
        return true;
    }
    return false;
}
bool VisualCustomizer::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Z')
    {
        if (key.getModifiers().isShiftDown())
            redoAppearance();
        else
            undoAppearance();
        return true;
    }
    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
        for (int i = 0; i < 6; ++i)
            if (tabs[i].hasKeyboardFocus(true))
            {
                chooseTab((tab + (key == juce::KeyPress::rightKey ? 1 : 5)) % 6);
                tabs[tab].grabKeyboardFocus();
                return true;
            }
    return false;
}
void VisualCustomizer::resized()
{
    int w = getWidth(), h = getHeight(), right = w * 62 / 100, left = 24, width = right - left - 20;
    close.setBounds(w - 114, 15, 90, 30);
    for (int i = 0; i < 6; ++i)
        tabs[i].setBounds(24 + i * (w - 48) / 6, 62, (w - 48) / 6 - 6, 34);
    for (int i = 0; i < 9; ++i)
        palettes[i]->setBounds(left + (i % 3) * width / 3, 115 + (i / 3) * 74, width / 3 - 5, 70);
    for (int i = 0; i < 4; ++i)
        knobs[i]->setBounds(left + (i % 2) * width / 2, 120 + (i / 2) * 143, width / 2 - 8, 130);
    for (int i = 0; i < 3; ++i)
        bodies[i]->setBounds(left + i * width / 3, 118, width / 3 - 8, 102);
    std::array<int, 6> indices{};
    for (auto &f : fields)
    {
        int n = indices[f->area]++;
        if (f->id == "avatarBody")
        {
            --indices[f->area];
            continue;
        }
        int columns = f->area == 0 ? 2 : 3;
        int x = left + (n % columns) * width / columns, y = (f->area == 0   ? 353
                                                             : f->area == 2 ? 230
                                                                            : 120) +
                                                            (n / columns) * 54;
        f->label.setBounds(x, y, width / columns - 10, 20);
        f->combo.setBounds(x, y + 22, width / columns - 10, 30);
    }
    int previewWidth = w - right - 32;
    avatar.setBounds(right + 8, 116, previewWidth, std::min(h - 240, 410));
    previewKnob.setBounds(right + 30, 160, previewWidth - 44, std::min(245, h - 275));
    previewLabel.setBounds(right + 8, 120, previewWidth, 40);
    undo.setBounds(24, h - 90, 100, 32);
    redo.setBounds(132, h - 90, 100, 32);
    revert.setBounds(240, h - 90, 174, 32);
    reset.setBounds(422, h - 90, 100, 32);
    lookScope.setBounds(24, h - 48, 210, 32);
    saveLook.setBounds(242, h - 48, 120, 32);
    loadLook.setBounds(370, h - 48, 120, 32);
    fileStatus.setBounds(504, h - 48, w - 528, 32);
}
void VisualCustomizer::paint(juce::Graphics &g)
{
    auto pal = visuals::palette(p);
    bool pt = p.language == 0;
    g.setColour(pal.bg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 14);
    g.setColour(pal.outline);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 14, 1.5f);
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(23, juce::Font::bold)));
    g.drawText(pt ? "SEU RASTRO, SEU JEITO" : "YOUR RASTRO, YOUR WAY", 24, 14, getWidth() - 160, 32,
               juce::Justification::left);
    g.setColour(pal.muted);
    g.setFont(juce::Font(juce::FontOptions(13)));
    if ((tab == 0 || tab == 5) && instrumentPreview.isValid())
    {
        int right = getWidth() * 62 / 100;
        g.setColour(pal.text);
        g.drawText(pt ? "INTERFACE APLICADA" : "APPLIED INTERFACE", right + 16, 121,
                   getWidth() - right - 38, 22, juce::Justification::left);
        g.drawImageWithin(instrumentPreview, right + 12, 153, getWidth() - right - 30, 242,
                          juce::RectanglePlacement::centred);
    }
    if (tab < 2)
    {
        int right = getWidth() * 62 / 100;
        g.drawFittedText(
            tab == 0 ? (pt ? utf("Captura da interface real com suas escolhas.\n\nVOLTAR fecha o "
                                 "painel para tocar. As cores não alteram o áudio.")
                           : "The actual interface with your choices.\n\nBACK closes this panel to "
                             "play. Colours do not change the audio.")
            : pt
                ? utf("O estilo escolhido aparece nos knobs reais do instrumento.\n\nArraste a "
                      "prévia para experimentar o gesto sem mudar o som.")
                : "Your choice styles the instrument's real knobs.\n\nDrag this preview to try the "
                  "gesture without changing sound.",
            right + 20, 425, getWidth() - right - 46, std::max(30, getHeight() - 540),
            juce::Justification::topLeft, 7);
    }
}
