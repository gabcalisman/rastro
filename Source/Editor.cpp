#include "Editor.h"
#include "Visuals.h"
namespace
{
const auto ink = visuals::text, cream = visuals::panel, orange = visuals::accent(0);
const juce::Colour accents[]{juce::Colour(0xffe69b62), juce::Colour(0xffbec863),
                             juce::Colour(0xff68b8ab), juce::Colour(0xffe8ba63),
                             juce::Colour(0xffd4aaac), juce::Colour(0xffb7a1d8)};
using namespace rastro;
const auto &names = moduleNames;
} // namespace
RastroEditor::Style::Style() {}
void RastroEditor::Style::apply(const visuals::Palette &pal, int design, int theme)
{
    controlStyle = design;
    chassis = theme;
    accent = pal.control;
    surface = pal.button;
    outline = pal.outline;
    setColour(juce::TextButton::buttonColourId, pal.button);
    setColour(juce::TextButton::textColourOffId, pal.text);
    setColour(juce::TextButton::textColourOnId, pal.text);
    setColour(juce::Slider::textBoxTextColourId, pal.text);
    setColour(juce::Slider::textBoxBackgroundColourId, pal.bg);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::thumbColourId, pal.control);
    setColour(juce::Slider::trackColourId, pal.control);
    setColour(juce::Slider::backgroundColourId, pal.outline);
    setColour(juce::ComboBox::backgroundColourId, pal.button);
    setColour(juce::ComboBox::outlineColourId, pal.outline);
    setColour(juce::ComboBox::textColourId, pal.text);
    setColour(juce::ComboBox::arrowColourId, pal.control);
    setColour(juce::Label::textColourId, pal.text);
    setColour(juce::PopupMenu::backgroundColourId, pal.surface);
    setColour(juce::PopupMenu::textColourId, pal.text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, pal.button);
    setColour(juce::PopupMenu::highlightedTextColourId, pal.text);
    setColour(juce::ToggleButton::textColourId, pal.text);
    setColour(juce::ToggleButton::tickColourId, pal.control);
}
void RastroEditor::Style::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float v,
                                           float a, float b, juce::Slider &)
{
    visuals::knob(g, juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4), v,
                  a, b, controlStyle, accent);
}
void RastroEditor::Style::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                               const juce::Colour &colour, bool over, bool down)
{
    auto box = b.getLocalBounds().toFloat().reduced(1);
    g.setColour(surface.brighter(down   ? .15f
                                 : over ? .1f
                                        : 0)
                    .interpolatedWith(accent, b.getToggleState() ? .13f : 0));
    const float radius = visuals::panelRadius(chassis);
    g.fillRoundedRectangle(box, radius);
    g.setColour((b.isEnabled() ? (b.getToggleState() ? accent : outline) : outline.withAlpha(.5f))
                    .withAlpha(over || b.hasKeyboardFocus(true) ? 1.f : .25f));
    g.drawRoundedRectangle(box, radius, b.hasKeyboardFocus(true) ? 2.f : 1.f);
    juce::ignoreUnused(colour);
}
RastroEditor::Knob::Knob(RastroProcessor &p, juce::String key, bool rotary) : p(p), id(key)
{
    slider.setColour(juce::Slider::textBoxTextColourId, ink);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::thumbColourId, orange);
    slider.setColour(juce::Slider::trackColourId, orange);
    setComponentID("control_" + key);
    slider.setComponentID("parameter_" + key);
    slider.setWantsKeyboardFocus(true);
    slider.setMouseDragSensitivity(180);
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
    slider.setSliderStyle(rotary ? juce::Slider::RotaryHorizontalVerticalDrag
                                 : juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 19);
    slider.setNumDecimalPlacesToDisplay(key == "root" || key == "tempo" ? 0 : 2);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, ink);
    label.setFont(juce::Font(juce::FontOptions(12)));
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.params, key, slider);
    slider.setTextBoxIsEditable(false);
    slider.textFromValueFunction = [key](double v)
    {
        if (key == "gTime")
            return juce::String("1/") + juce::String(32 >> std::min(4, int(v * 4.99)));
        if (key == "cRate")
            return juce::String("1/") + juce::String(8 << std::min(4, int(v * 4.99)));
        if (key == "wRate")
            return juce::String(.15 + v * 2.85, 2) + " Hz";
        if (key == "root" || key == "tempo")
            return juce::String(juce::roundToInt(v));
        if (key.endsWith("FreeMs"))
            return juce::String(v, 0) + " ms";
        if (key.endsWith("FreeHz"))
            return juce::String(v, 2) + " Hz";
        if (key.endsWith("Division"))
            return juce::String(rastro::divisionNames[juce::jlimit(0, 3, int(v))]);
        return juce::String(juce::roundToInt(v * 100)) + "%";
    };
    slider.updateText();
}
void RastroEditor::Knob::applyPalette()
{
    auto pal = visuals::palette(p);
    label.setColour(juce::Label::textColourId, pal.text);
    slider.setColour(juce::Slider::textBoxTextColourId, pal.text);
    slider.setColour(juce::Slider::thumbColourId, pal.control);
    slider.setColour(juce::Slider::trackColourId, pal.control.withAlpha(.65f));
    slider.setColour(juce::Slider::backgroundColourId, pal.outline);
}
void RastroEditor::Knob::resized()
{
    auto r = getLocalBounds();
    if (label.isVisible())
    {
        bool macro = id == "presence" || id == "trail" || id == "mutation" || id == "drywet";
        label.setBounds(
            r.removeFromTop(macro ? 28 : 18).withTrimmedRight(macro && id != "drywet" ? 38 : 0));
    }
    slider.setBounds(r);
}
RastroEditor::Card::Card(RastroEditor &e, int m) : ui(e), module(m)
{
    setComponentID("rack_" + juce::String(m));
    addAndMakeVisible(bypass);
    addAndMakeVisible(remove);
    remove.setButtonText(juce::String::fromUTF8("×"));
    remove.onClick = [this] { ui.p.set(presentIds[module], 0); };
    bypass.onClick = [this]
    {
        ui.p.set(onIds[module], 1 - ui.p.value(onIds[module]));
        ui.selectModule(module);
    };
    if (int slot = rastro::timingSlot(module); slot >= 0)
    {
        addAndMakeVisible(timing);
        timing.setComponentID("effectTiming_" + juce::String(module));
        timing.onClick = [this, slot]
        {
            juce::PopupMenu menu;
            menu.addItem(1, juce::String::fromUTF8("FREE · ms / Hz"));
            for (int i = 0; i < 4; ++i)
                menu.addItem(2 + i, juce::String(juce::String::fromUTF8("SYNC · ")) +
                                        rastro::divisionNames[i]);
            menu.addSeparator();
            menu.addItem(9, ui.p.language == 0 ? juce::String::fromUTF8("Legado (preservar sessão)")
                                               : "Legacy (preserve session)");
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(timing),
                               [safe = juce::Component::SafePointer<Card>(this), slot](int result)
                               {
                                   if (!safe || result <= 0)
                                       return;
                                   if (result == 9)
                                       safe->ui.p.set(rastro::timingIds[slot], 0);
                                   else
                                   {
                                       safe->ui.p.set(rastro::timingIds[slot], result == 1 ? 1 : 2);
                                       if (result >= 2)
                                           safe->ui.p.set(rastro::divisionIds[slot], result - 2);
                                   }
                                   safe->ui.synchroniseUi();
                               });
        };
    }
    mix = std::make_unique<Knob>(ui.p, mixIds[m], false);
    addAndMakeVisible(*mix);
    mix->label.setVisible(false);
    primary = std::make_unique<Knob>(ui.p, controlIds[m][0], false);
    primary->setComponentID("rackPrimary_" + juce::String(m));
    primary->slider.setComponentID("rackPrimarySlider_" + juce::String(m));
    primary->label.setVisible(false);
    addAndMakeVisible(*primary);
    for (auto id : controlIds[m])
        if (id && juce::String(id) != mixIds[m])
        {
            knobs.push_back(std::make_unique<Knob>(ui.p, id));
            ui.addAndMakeVisible(*knobs.back());
        }
}
void RastroEditor::Card::paint(juce::Graphics &g)
{
    bool present = ui.p.value(presentIds[module]) > .5f,
         active = present && ui.p.value(onIds[module]) > .5f;
    auto pal = visuals::palette(ui.p);
    int custom = int(ui.p.value(rastro::rackColourIds[module]));
    const juce::Colour rackColours[]{juce::Colour(0xffae94ff), juce::Colour(0xff63ddd5),
                                     juce::Colour(0xffff8c76), juce::Colour(0xffe7c579)};
    auto colour = visuals::readable(custom ? rackColours[juce::jlimit(0, 3, custom - 1)]
                                           : accents[module].interpolatedWith(pal.highlight, .18f),
                                    pal.surface);
    auto box = getLocalBounds().toFloat().reduced(1);
    g.setColour(pal.surface.interpolatedWith(colour, active ? .06f : .01f));
    g.fillRoundedRectangle(box, 6);
    g.setColour(ui.selected == module ? colour.withAlpha(.8f) : pal.outline.withAlpha(.3f));
    g.drawRoundedRectangle(box, 6, ui.selected == module ? 1.5f : .7f);
    g.setColour(active ? colour : pal.muted);
    g.fillRoundedRectangle(3, 7, 3, getHeight() - 14.f, 1.5f);
    visuals::glyph(g, module, {10, 6, 19, 19}, active ? colour : pal.muted);
    g.setColour(active ? pal.text : pal.muted);
    g.setFont(juce::Font(juce::FontOptions(13, juce::Font::bold)));
    g.drawText(names[module], 33, 3, getWidth() - 62, 23, juce::Justification::left);
    const char *shortKeys[]{"ghostShort",  "crumbleShort", "bloomShort",
                            "wobbleShort", "dustShort",    "crushShort"};
    g.setFont(juce::Font(juce::FontOptions(11)));
    g.setColour(pal.muted);
    float amount = ui.p.value(controlIds[module][0]);
    bool xyZero = module == 3 ? ui.p.value("playX") < .001f
                              : (module == 4 || module == 5 ||
                                 (module == 1 && ui.p.value("xyCrumble") > .5f)) &&
                                    ui.p.value("playY") < .001f;
    bool zero = ui.p.value(mixIds[module]) < .001f || ui.p.value("drywet") < .001f || xyZero ||
                (module >= 3 && amount < .001f);
    auto status = !present  ? ui.tr("removed")
                  : !active ? "BYPASS"
                  : zero    ? (ui.p.language == 0 ? juce::String::fromUTF8("SEM FX · MIX/XY")
                                                  : juce::String::fromUTF8("DRY · MIX/XY"))
                            : ui.tr(shortKeys[module]);
    g.drawText(status, 10, 25, getWidth() - 18, 16, juce::Justification::left);
    // Miniature diagrams show the actual controls and measured wet module energy.
    // They are not a fabricated oscilloscope and have no independent animation clock.
    float activity =
        active ? std::min(1.f, ui.p.engine.rack.moduleActivity[module].load() * 12) : 0;
    auto graph = juce::Rectangle<float>(10, 43, getWidth() - 20.f, 14);
    float shape = ui.p.params.getParameter(primary->id)->getValue();
    g.setColour(colour.withAlpha(.2f + .65f * activity));
    if (module == 0)
        for (int i = 0; i < 6; ++i)
            g.fillRoundedRectangle(
                graph.getX() + i * graph.getWidth() / 6,
                graph.getCentreY() - 6 * std::pow(.25f + ui.p.value("gFeedback") * .7f, i),
                3 + shape * 5, 12 * std::pow(.25f + ui.p.value("gFeedback") * .7f, i), 1);
    else if (module == 2)
        for (int i = 0; i < 5; ++i)
            g.drawEllipse(graph.getCentreX() - 10 - i * (3 + shape * 6), graph.getCentreY() - 2 - i,
                          20 + i * (6 + shape * 12), 4 + i * 2.f, 1);
    else if (module == 4)
        for (int i = 0; i < 20; ++i)
            g.fillEllipse(graph.getX() + (i * 29 % int(graph.getWidth())),
                          graph.getY() + (i * 7 % 11), 1 + shape * 2, 1 + shape * 2);
    else
    {
        juce::Path trace;
        int points = module == 5 ? int(18 - shape * 12) : module == 1 ? 16 : 30;
        for (int i = 0; i <= points; ++i)
        {
            float t = float(i) / points;
            float value = std::sin(t * juce::MathConstants<float>::twoPi * (module == 1 ? 3 : 1));
            if (module == 3)
                value *= .2f + .8f * shape;
            if (module == 5)
                value = std::round(value * (8 - shape * 6)) / (8 - shape * 6);
            float x = graph.getX() + t * graph.getWidth(),
                  y = graph.getCentreY() - value * (3 + activity * 3);
            if (i == 0)
                trace.startNewSubPath(x, y);
            else if (module == 3)
                trace.lineTo(x, y);
            else
            {
                trace.lineTo(x, trace.getCurrentPosition().y);
                trace.lineTo(x, y);
            }
        }
        g.strokePath(trace, juce::PathStrokeType(1.3f));
    }
    g.setColour(pal.muted);
    g.drawText(!present ? ui.tr("removed") : ui.tr("mix"), 10, 114, 27, 24,
               juce::Justification::left);
    g.setColour(pal.muted);
    if (rastro::timingSlot(module) < 0)
        g.drawText(ui.tr(primary->id), getWidth() / 2, 61, getWidth() / 2 - 9, 24,
                   juce::Justification::right);
}
void RastroEditor::Card::resized()
{
    timing.setBounds(getWidth() / 2 + 3, 61, getWidth() / 2 - 12, 24);
    remove.setBounds(getWidth() - 29, 3, 26, 25);
    bypass.setBounds(9, 61, getWidth() / 2 - 13, 24);
    primary->setBounds(8, 86, getWidth() - 16, 26);
    primary->slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 43, 22);
    mix->setBounds(36, 113, getWidth() - 44, 26);
    mix->slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 43, 22);
}

void RastroEditor::Card::mouseDrag(const juce::MouseEvent &e)
{
    if (e.getMouseDownY() < 32 && e.getDistanceFromDragStart() > 5)
    {
        ui.dragged = module;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
}
void RastroEditor::Card::mouseUp(const juce::MouseEvent &e)
{
    if (ui.dragged < 0)
    {
        ui.selectModule(module);
        return;
    }
    auto point = ui.getLocalPoint(this, e.getPosition());
    auto order = ui.p.settings().order;
    for (int to = 0; to < moduleCount; ++to)
        if (ui.cards[order[to]]->getBounds().contains(point))
            for (int from = 0; from < moduleCount; ++from)
                if (order[from] == module)
                    ui.p.moveModule(from, to);
    ui.dragged = -1;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    ui.lastPresence = -1;
    ui.synchroniseUi();
}
void RastroEditor::selectModule(int m)
{
    selected = m;
    resized();
    repaint();
}
RastroEditor::RastroEditor(RastroProcessor &processor)
    : AudioProcessorEditor(processor), p(processor), deck(p), play(p), avatar(p), sampleView(p),
      quickEnvelope(p), keyboard(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&style);
    addAndMakeVisible(quickEnvelope);
    quickEnvelope.setComponentID("mainEnvelope");
    quickEnvelope.setWantsKeyboardFocus(true);
    quickEnvelope.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    quickEnvelope.onOpen = [this] { openInstrument.onClick(); };
    addChildComponent(captureShade);
    captureShade.setAlwaysOnTop(true);
    capturePanel = std::make_unique<CapturePanel>(p.capture, p.getTotalNumInputChannels() > 0);
    addChildComponent(*capturePanel);
    capturePanel->setAlwaysOnTop(true);
    addAndMakeVisible(openCapture);
    openCapture.setComponentID("openCapture");
    capturePanel->onClose = [this]
    {
        captureShade.setVisible(false);
        for (auto *c : getChildren())
            if (c != capturePanel.get())
                c->setEnabled(true);
        avatar.resume();
        deck.resume();
        openCapture.grabKeyboardFocus();
    };
    openCapture.onClick = [this]
    {
        captureShade.setVisible(true);
        captureShade.toFront(false);
        capturePanel->setVisible(true);
        capturePanel->toFront(true);
        for (auto *c : getChildren())
            if (c != capturePanel.get())
                c->setEnabled(false);
        capturePanel->grabKeyboardFocus();
        if (capturePanel->onRefresh)
            capturePanel->onRefresh();
    };
    capturePanel->onSettings = [this]
    {
        if (p.standaloneAudioSettings)
            p.standaloneAudioSettings();
    };
    capturePanel->onRecord = [this]
    {
        p.beginCapture(
            [safe = juce::Component::SafePointer<CapturePanel>(capturePanel.get())](bool ok)
            {
                if (safe)
                    if (ok)
                        safe->message("Sample carregado / Sample loaded");
            });
    };
    capturePanel->onPreview = [this] { p.previewRequest = p.engine.previewActive.load() ? -1 : 1; };
    capturePanel->onRefresh = [this]
    {
        p.pollCaptureLinks(
            [safe = juce::Component::SafePointer<CapturePanel>(capturePanel.get())](
                std::vector<juce::File> files, std::vector<rastro::CaptureLink> links)
            {
                if (safe)
                {
                    safe->setFiles(std::move(files));
                    safe->setLinks(std::move(links));
                }
            });
    };
    capturePanel->onRemoteCommand = [this](juce::String id, bool start)
    {
        p.remoteCapture(
            id, start,
            [safe = juce::Component::SafePointer<CapturePanel>(capturePanel.get())](bool ok)
            {
                if (safe)
                    safe->message(ok ? "Comando enviado ao insert / Command sent to insert"
                                     : "Falha ao contatar insert / Could not contact insert");
            });
    };
    capturePanel->onReceive = [this](juce::File file)
    {
        capturePanel->message("Carregando WAV / Loading WAV...");
        p.importFileAsync(
            file,
            [safe = juce::Component::SafePointer<RastroEditor>(this)](rastro::ImportError error)
            {
                if (!safe)
                    return;
                if (error == rastro::ImportError::none)
                {
                    safe->capturePanel->closeAfterLoad();
                    safe->synchroniseUi();
                }
                else
                    safe->capturePanel->message(safe->p.importMessage(error));
            });
    };

    addAndMakeVisible(openInstrument);
    openInstrument.setButtonText("SYNTH / ADSR");
    openInstrument.setComponentID("openInstrument");
    instrument = std::make_unique<InstrumentPanel>(p);
    addChildComponent(*instrument);
    instrument->setAlwaysOnTop(true);
    instrument->onClose = [this]
    {
        for (auto *child : getChildren())
            if (child != instrument.get())
                child->setEnabled(true);
        avatar.resume();
        deck.resume();
        openInstrument.grabKeyboardFocus();
    };
    openInstrument.onClick = [this]
    {
        instrument->showPage(false);
        instrument->setVisible(true);
        instrument->update();
        for (auto *child : getChildren())
            if (child != instrument.get())
                child->setEnabled(false);
        instrument->grabKeyboardFocus();
    };

    addAndMakeVisible(openPatches);
    openPatches.setButtonText("PATCH LAB");
    openPatches.setComponentID("openPatches");
    openPatches.onClick = [this]
    {
        openInstrument.onClick();
        instrument->showPage(true);
    };
    for (auto *b :
         {&load, &mutate, &undo, &reset, &add, &about, &center, &preview, &customize, &compact})
        addAndMakeVisible(b);
    customize.setComponentID("customize");
    customize.setButtonText("VISUAL / QUARTO / DANÇA");
    customize.onClick = [this] { showCustomizer(); };
    customizer = std::make_unique<VisualCustomizer>(p);
    addChildComponent(*customizer);
    customizer->setAlwaysOnTop(true);
    customizer->onClose = [this] { customizerClosed(); };
    customizer->renderInstrumentPreview = [this]
    {
        // Paint the existing components, excluding the sheet. No second editor/timer/DSP.
        constexpr float factor = .32f;
        juce::Image shot(juce::Image::ARGB, juce::roundToInt(getWidth() * factor),
                         juce::roundToInt(getHeight() * factor), true);
        juce::Graphics g(shot);
        g.addTransform(juce::AffineTransform::scale(factor));
        paint(g);
        for (auto *child : getChildren())
            if (child != customizer.get() && child->isVisible())
            {
                juce::Graphics::ScopedSaveState state(g);
                bool enabled = child->isEnabled();
                child->setEnabled(true);
                g.addTransform(
                    juce::AffineTransform::translation((float)child->getX(), (float)child->getY()));
                g.reduceClipRegion(child->getLocalBounds());
                child->paintEntireComponent(g, true);
                child->setEnabled(enabled);
            }
        return shot;
    };
    compact.setComponentID("compactMode");
    compact.onClick = [this]
    {
        p.set("uiCompact", p.value("uiCompact") > .5f ? 0 : 1);
        synchroniseUi();
    };
    play.setComponentID("playPad");
    avatar.setComponentID("mainAvatar");
    sampleView.setComponentID("sampleWaveform");
    lastVisual.fill(-1);
    addAndMakeVisible(deck);
    addAndMakeVisible(pitchMode);
    addAndMakeVisible(pitchLabel);
    pitchMode.setComponentID("pitchMode");
    pitchMode.addItem("REPITCH", 1);
    pitchMode.addItem("FIXED PITCH", 2);
    pitchLabel.setText("PITCH MODE", juce::dontSendNotification);
    pitchLabel.setJustificationType(juce::Justification::centred);
    pitchLabel.setFont(juce::Font(juce::FontOptions(12)));
    pitchAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.params, "pitchMode", pitchMode);
    addAndMakeVisible(sampleView);
    addAndMakeVisible(playbackMode);
    addAndMakeVisible(sliceCount);
    playbackMode.setComponentID("playbackMode");
    sliceCount.setComponentID("sliceCount");
    for (int i = 2; i <= 16; ++i)
        sliceCount.addItem(juce::String(i), i);
    playbackMode.onChange = [this] { p.setPlaybackMode(playbackMode.getSelectedId() - 1); };
    sliceCount.onChange = [this] { p.equalSlices(sliceCount.getSelectedId()); };
    preview.onClick = [this] { p.previewRequest = p.engine.previewActive.load() ? -1 : 1; };
    addAndMakeVisible(play);
    addAndMakeVisible(avatar);
    addAndMakeVisible(animations);
    animations.setComponentID("animationMode");
    language.setComponentID("language");
    for (int i = 0; i < 3; ++i)
        animations.addItem(text::get(i == 0   ? "animOff"
                                     : i == 1 ? "animReduced"
                                              : "animFull",
                                     p.language),
                           i + 1);
    animationAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.params, "animation", animations);
    center.onClick = [this] { play.center(); };
    undo.onClick = [this] { p.undoMutation(); };
    addAndMakeVisible(language);
    language.addItem(text::get("pt", 0), 1);
    language.addItem(text::get("en", 0), 2);
    language.onChange = [this]
    {
        p.language = language.getSelectedId() - 1;
        localize();
    };
    addAndMakeVisible(regionLoop);
    regionLoop.setButtonText("LOOP");
    regionLoop.setComponentID("regionLoop");
    regionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.params, "regionLoop", regionLoop);
    loop.setVisible(false);
    loopAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.params, "loop", loop);
    for (auto id : {"root", "start", "end", "velocity", "tempo", "level"})
    {
        sampleKnobs.push_back(std::make_unique<Knob>(p, id, false));
        addAndMakeVisible(*sampleKnobs.back());
    }
    for (auto id : {"presence", "trail", "mutation", "drywet"})
    {
        macros.push_back(std::make_unique<Knob>(p, id));
        addAndMakeVisible(*macros.back());
    }
    for (int i = 0; i < moduleCount; ++i)
    {
        cards[i] = std::make_unique<Card>(*this, i);
        addAndMakeVisible(*cards[i]);
    }
    for (int i = 0; i < 5; ++i)
    {
        addAndMakeVisible(learns[i]);
        learns[i].onClick = [this, i] { p.learn = p.learn == i ? -1 : i; };
    }
    addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(36, 96);
    keyboard.setKeyWidth(20);
    keyboard.setLowestVisibleKey(48);
    load.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>(tr("choose"), juce::File{},
                                                      "*.wav;*.aiff;*.aif;*.mp3;*.m4a;*.mov;*.qt");
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<RastroEditor>(this)](const juce::FileChooser &fc)
            {
                if (safe && fc.getResult().existsAsFile())
                    safe->import(fc.getResult());
            });
    };
    mutate.onClick = [this] { p.mutate(); };
    reset.onClick = [this] { p.resetEffects(); };
    about.onClick = [this]
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, tr("about"),
                                               tr("aboutBody"));
    };
    add.onClick = [this]
    {
        juce::PopupMenu menu;
        for (int i = 0; i < moduleCount; ++i)
            menu.addItem(i + 1, names[i], p.value(presentIds[i]) < .5f);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(add),
                           [safe = juce::Component::SafePointer<RastroEditor>(this)](int result)
                           {
                               if (safe && result > 0)
                                   safe->p.set(presentIds[result - 1], 1);
                           });
    };
    setResizable(true, true);
    setResizeLimits(980, 700, 1680, 1100);
    setSize(p.value("uiCompact") > .5f ? 1000 : 1100, p.value("uiCompact") > .5f ? 700 : 760);
    localize();
    timerCallback();
    startTimerHz((int)p.value("animation") == 0 ? 6 : (int)p.value("animation") == 1 ? 12 : 24);
}
RastroEditor::~RastroEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}
void RastroEditor::localize()
{
    lastLanguage = p.language;
    language.setSelectedId(lastLanguage + 1, juce::dontSendNotification);
    customize.setButtonText(p.language == 0 ? juce::String::fromUTF8("VISUAL / QUARTO / DANÇA")
                                            : "LOOK / ROOM / DANCE");
    load.setButtonText(p.language == 0 ? "IMPORTAR" : "IMPORT");
    openCapture.setButtonText(p.language == 0 ? "CAPTURAR" : "CAPTURE");
    mutate.setButtonText(tr("mutate"));
    reset.setButtonText(tr("reset"));
    undo.setButtonText(tr("undoMutation"));
    center.setButtonText(tr("center"));
    compact.setButtonText(p.language == 0 ? "Compacto" : "Compact");
    int animationSelection = animations.getSelectedId();
    animations.clear(juce::dontSendNotification);
    for (int i = 0; i < 3; ++i)
        animations.addItem(tr(i == 0 ? "animOff" : i == 1 ? "animReduced" : "animFull"), i + 1);
    animations.setSelectedId(animationSelection, juce::dontSendNotification);
    animations.setTooltip(tr("animationHelp"));
    play.setTooltip(tr("xyHelp"));
    about.setButtonText(tr("about"));
    add.setButtonText(tr("add"));
    loop.setButtonText(tr("loop"));
    regionLoop.setTooltip(tr("regionLoopHelp"));
    int mode = (int)p.value("playMode");
    if (mode == 0 && p.value("loop") > .5f)
        mode = 1;
    playbackMode.clear(juce::dontSendNotification);
    const char *modes[]{"oneShot", "loop", "reverse", "pingPong", "slice"};
    for (int i = 0; i < 5; ++i)
        playbackMode.addItem(tr(modes[i]), i + 1);
    playbackMode.setSelectedId(mode + 1, juce::dontSendNotification);
    playbackMode.setTooltip(tr("modeHelp"));
    sliceCount.setTooltip(tr("sliceHelp"));
    sampleView.setTooltip(tr("waveHelp"));
    pitchMode.setTooltip(tr("pitchHelp"));
    deck.setTooltip(tr("deckHelp"));
    for (auto &k : sampleKnobs)
    {
        k->label.setText(tr(k->id), juce::dontSendNotification);
        if (k->id == "tempo")
            k->label.setText("BPM", juce::dontSendNotification);
        if (k->id == "velocity")
            k->label.setText(p.language == 0 ? "Vel./filtro" : "Vel./filter",
                             juce::dontSendNotification);
        k->slider.setTooltip(tr("help"));
    }
    for (auto &k : macros)
    {
        k->label.setText(tr(k->id), juce::dontSendNotification);
        k->slider.setTooltip(tr("help"));
    }
    for (auto &c : cards)
    {
        c->remove.setTooltip(tr("remove"));
        c->mix->slider.setTooltip(tr("mix"));
        c->primary->slider.setTooltip(tr(c->primary->id));
        c->setTooltip(
            tr(moduleDescriptions[c->module]) +
            (p.language == 0
                 ? juce::String::fromUTF8("\nDiagrama dos controles; brilho = atividade medida no "
                                          "DSP. Não é osciloscópio.")
                 : "\nControl diagram; brightness = measured DSP activity. Not an oscilloscope."));
        c->bypass.setTooltip(tr("bypassHelp"));
        for (auto &k : c->knobs)
        {
            k->label.setText(tr(k->id), juce::dontSendNotification);
            k->slider.setTooltip(tr(k->id == "dCharacter" ? "dustHelp"
                                    : k->id == "wDepth"   ? "wobbleHelp"
                                    : k->id == "kAmount"  ? "crushHelp"
                                                          : "help"));
        }
    }
    mutate.setTooltip(tr("randomHelp"));
    undo.setTooltip(p.language == 0 ? juce::String::fromUTF8("Desfazer a última variação JAM")
                                    : "Undo the last JAM variation");
    repaint();
}
void RastroEditor::applyAppearance()
{
    style.apply(visuals::palette(p), visuals::knobStyle(p), (int)p.value("visualTheme"));
    // JUCE ComboBox caches its label's ink; changing only LookAndFeel leaves stale white text.
    auto pal = visuals::palette(p);
    std::function<void(juce::Component &)> refreshCombos = [&](juce::Component &c)
    {
        if (auto *combo = dynamic_cast<juce::ComboBox *>(&c))
            combo->setColour(juce::ComboBox::textColourId, pal.text);
        for (auto *child : c.getChildren())
            refreshCombos(*child);
    };
    refreshCombos(*this);
    for (auto &k : sampleKnobs)
        k->applyPalette();
    for (auto &k : macros)
        k->applyPalette();
    for (auto &c : cards)
    {
        c->mix->applyPalette();
        c->primary->applyPalette();
        for (auto &k : c->knobs)
            k->applyPalette();
    }
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                       pal.highlight.withAlpha(.6f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                       pal.control.withAlpha(.3f));
    ++measurements.fullRepaints;
    repaint();
}
void RastroEditor::timerCallback()
{
    // Parent windows can become visible without notifying the child editor. Keep only a cheap
    // visibility probe while hidden, so Standalone/DAW reopen always resumes the real UI.
    if (!isShowing())
    {
        windowWasShowing = false;
        if (getTimerInterval() != 166)
            startTimerHz(6);
        return;
    }
    if (!windowWasShowing)
    {
        avatar.resume();
        deck.resume();
        windowWasShowing = true;
    }
    synchroniseUi();
}
void RastroEditor::synchroniseUi()
{
    auto start = juce::Time::getMillisecondCounterHiRes();
    ++measurements.ticks;
    quickEnvelope.update();
    p.applyCC();
    if (auto *sample = p.source.load(std::memory_order_acquire); sample != lastSource)
    {
        lastSource = sample;
        sampleView.repaint();
        repaint(wave.getX(), 110, std::max(1, preview.getX() - wave.getX() - 10), 17);
        deck.repaint();
        customizer->invalidatePreview();
        ++measurements.sampleRefreshes;
    }
    bool appearance = false;
    for (size_t i = 0; i < lastVisual.size(); ++i)
    {
        auto value = p.value(visuals::visualIds[i]);
        if (value != lastVisual[i])
        {
            lastVisual[i] = value;
            appearance = true;
        }
    }
    if (appearance)
        applyAppearance();
    if (lastLanguage != p.language)
        localize();
    int requested = (int)p.value("uiCompact");
    if (lastCompact != requested)
    {
        bool first = lastCompact < 0;
        lastCompact = requested;
        if (!first)
            setSize(requested ? 1000 : 1100, requested ? 700 : 760);
        compact.setToggleState(requested != 0, juce::dontSendNotification);
    }
    int animationMode = (int)p.value("animation"), hz = animationMode == 0   ? 6
                                                        : animationMode == 1 ? 12
                                                                             : 24;
    if (getTimerInterval() != 1000 / hz)
        startTimerHz(hz);
    bool editing = customizer->isVisible() || instrument->isVisible() || capturePanel->isVisible();
    if (capturePanel->isVisible() &&
        juce::Time::getMillisecondCounterHiRes() - lastCapturePoll > 500)
    {
        lastCapturePoll = juce::Time::getMillisecondCounterHiRes();
        if (capturePanel->onRefresh)
            capturePanel->onRefresh();
    }
    if (capturePanel->isVisible())
        capturePanel->update(visuals::palette(p), p.language,
                             p.standaloneInputAvailable ? p.standaloneInputAvailable()
                                                        : p.capture.inputChannels.load() > 0);
    if (instrument->isVisible())
        instrument->update();
    if (editing)
    {
        if (customizer->isVisible() && customizer->update(animationMode))
            ++measurements.previewAvatar;
    }
    else
    {
        if (deck.isShowing())
        {
            deck.update(animationMode);
            ++measurements.deckUpdates;
        }
        if (avatar.isShowing())
        {
            avatar.update(animationMode);
            ++measurements.mainAvatar;
        }
        if (p.engine.meter > .0001f)
        {
            sampleView.repaint();
        }
        float x = p.value("playX"), y = p.value("playY");
        if (x != lastPlayX || y != lastPlayY)
        {
            lastPlayX = x;
            lastPlayY = y;
            play.repaint();
        }
    }
    if (p.midiFlash > 0)
    {
        --p.midiFlash;
        repaint(220, 47, 140, 26);
    }
    int presenceMask = 0;
    for (int i = 0; i < moduleCount; ++i)
    {
        std::array<float, 8> display{p.value(presentIds[i]),
                                     p.value(onIds[i]),
                                     p.value(mixIds[i]),
                                     p.value(controlIds[i][0]),
                                     p.value("playX"),
                                     p.value("playY"),
                                     p.value("drywet"),
                                     editing ? cards[i]->displayState[7]
                                             : p.engine.rack.moduleActivity[i].load()};
        if (cards[i]->displayState != display)
        {
            cards[i]->displayState = display;
            cards[i]->repaint();
            play.repaint();
        }
        bool present = p.value(presentIds[i]) > .5f;
        if (cards[i]->updateTiming())
        {
            resized();
            cards[i]->repaint();
        }
        cards[i]->timing.setVisible(present && rastro::timingSlot(i) >= 0);
        presenceMask |= (present ? 1 : 0) << i;
        cards[i]->mix->setVisible(present);
        cards[i]->primary->setVisible(present);
        cards[i]->bypass.setVisible(present);
        cards[i]->remove.setVisible(present);
        auto label = tr(p.value(onIds[i]) > .5f ? "on" : "off");
        if (cards[i]->bypass.getButtonText() != label)
        {
            cards[i]->bypass.setButtonText(label);
            cards[i]->repaint();
        }
    }
    auto order = p.settings().order;
    if (order != lastOrder || presenceMask != lastPresence)
    {
        lastOrder = order;
        lastPresence = presenceMask;
        resized();
        ++measurements.fullRepaints;
        repaint();
    }
    for (int i = 0; i < 5; ++i)
    {
        auto label = i < 3 ? tr("learn") : (i == 3 ? "X / " : "Y / ") + tr("learn");
        auto text = p.learn == i ? (i < 3 ? "..." : tr("learning"))
                    : i < 3      ? (p.cc[i] >= 0 ? juce::String(p.cc[i]) : "CC")
                                 : label + (p.cc[i] >= 0 ? " " + juce::String(p.cc[i]) : "");
        if (learns[i].getButtonText() != text)
            learns[i].setButtonText(text);
    }
    int mode = (int)p.value("playMode");
    if (mode == 0 && p.value("loop") > .5f)
        mode = 1;
    playbackMode.setSelectedId(mode + 1, juce::dontSendNotification);
    sliceCount.setSelectedId((int)p.value("sliceCount"), juce::dontSendNotification);
    sliceCount.setVisible(mode == 4);
    preview.setButtonText(tr(p.engine.previewActive ? "stopPreview" : "preview"));
    undo.setEnabled(!editing && p.canUndoMutation());
    bool jamAvailable = false;
    for (int m = 0; m < moduleCount; ++m)
        jamAvailable |= p.value(presentIds[m]) > .5f && p.value(onIds[m]) > .5f &&
                        p.value(rastro::jamLockIds[m]) < .5f;
    mutate.setEnabled(!editing && jamAvailable && p.value("mutation") > 0);
    wasCustomizing = editing;
    measurements.updateMs += juce::Time::getMillisecondCounterHiRes() - start;
}
void RastroEditor::resized()
{
    ++measurements.layouts;
    int w = getWidth(), h = getHeight();
    compactLayout = h < 740;
    int margin = 20, content = w - margin * 2;
    customize.setBounds(w - 588, 10, 230, 30);
    compact.setBounds(w - 348, 10, 88, 30);
    language.setBounds(w - 250, 10, 154, 30);
    about.setBounds(w - 86, 10, 66, 30);
    animations.setBounds(w - 270, 46, 250, 28);
    openInstrument.setBounds(w - 570, 46, 170, 28);
    openPatches.setBounds(w - 390, 46, 110, 28);
    regionLoop.setBounds(20, 52, 76, 26);
    instrument->setBounds(12, 80, w - 24, h - 92);
    customizer->setBounds(12, 80, w - 24, h - 92);
    captureShade.setBounds(getLocalBounds());
    capturePanel->setBounds((w - std::min(860, w - 48)) / 2, 92, std::min(860, w - 48),
                            std::min(450, h - 112));
    // Sampler/performance at left; a real room/character and envelope at right.
    int roomWidth = content * 30 / 100;
    int sampleRight = w - 32 - roomWidth;
    int leftWidth = sampleRight - 20;
    samplePanel = {20, 84, leftWidth, compactLayout ? 190 : 224};
    load.setBounds(sampleRight - 174, 96, 78, 30);
    openCapture.setBounds(sampleRight - 88, 96, 80, 30);
    playbackMode.setBounds(sampleRight - 322, 96, 140, 30);
    preview.setBounds(32, compactLayout ? 236 : 268, compactLayout ? 148 : 176, 28);
    sliceCount.setBounds(sampleRight - 384, 96, 56, 30);
    int sampleX = compactLayout ? 192 : 220;
    deck.setBounds(32, compactLayout ? 112 : 116, compactLayout ? 148 : 176,
                   compactLayout ? 120 : 140);
    wave = {sampleX, 130, sampleRight - sampleX - 12, compactLayout ? 96 : 128};
    sampleView.setBounds(wave);
    int sampleY = compactLayout ? 230 : 264;
    int column = (sampleRight - 126 - sampleX - 12) / 6;
    for (int i = 0; i < 6; ++i)
        sampleKnobs[i]->setBounds(sampleX + i * column, sampleY, column - 6, 42);
    pitchLabel.setBounds(sampleRight - 126, sampleY, 118, 18);
    pitchMode.setBounds(sampleRight - 126, sampleY + 20, 118, 28);
    int performanceY = samplePanel.getBottom() + 12;
    int performanceHeight = compactLayout ? 98 : 126 + std::max(0, h - 760) / 3;
    int playW = leftWidth * 36 / 100, macroX = 30 + playW, macroW = (sampleRight - macroX) / 4;
    play.setBounds(20, performanceY, playW, performanceHeight);
    avatar.setBounds(sampleRight + 12, 84, roomWidth, performanceY + performanceHeight - 84 - 76);
    quickEnvelope.setBounds(sampleRight + 12, avatar.getBottom() + 8, roomWidth, 68);
    macroPanel = {macroX - 4, performanceY, sampleRight - macroX + 4, performanceHeight};
    for (int i = 0; i < 4; ++i)
    {
        macros[i]->setBounds(macroX + i * macroW, performanceY + 5, macroW - 6,
                             performanceHeight - 8);
        if (i < 3)
            learns[i].setBounds(macroX + (i + 1) * macroW - 42, performanceY + 6, 34, 24);
    }
    int barY = performanceY + performanceHeight + 4;
    center.setBounds(20, barY, 92, 28);
    learns[3].setBounds(120, barY, (playW - 100) / 2, 28);
    learns[4].setBounds(126 + (playW - 100) / 2, barY, (playW - 100) / 2, 28);
    mutate.setBounds(macroX, barY, macroW * 2 - 8, 28);
    undo.setBounds(macroX + macroW * 2, barY, macroW * 2 - 8, 28);
    rackTitle = {20, barY + 32, content, 24};
    reset.setBounds(w - 328, rackTitle.getY(), 186, 28);
    add.setBounds(w - 134, rackTitle.getY(), 114, 28);
    rack = {20, rackTitle.getBottom() + 4, content, 142};
    auto order = p.settings().order;
    int cw = (content - 40) / 6;
    for (int i = 0; i < moduleCount; ++i)
        cards[order[i]]->setBounds(20 + i * (cw + 8), rack.getY(), cw, 142);
    int keyboardHeight = compactLayout ? 34 : 36;
    keyboard.setBounds(20, h - keyboardHeight - 8, content, keyboardHeight);
    detail = {20, rack.getBottom() + 6, content, keyboard.getY() - rack.getBottom() - 14};
    for (int m = 0; m < moduleCount; ++m)
        for (auto &k : cards[m]->knobs)
            k->setVisible(m == selected && p.value(presentIds[m]) > .5f);
    auto &controls = cards[selected]->knobs;
    int dk = (content - 210) / std::max(1, (int)controls.size());
    for (size_t i = 0; i < controls.size(); ++i)
    {
        auto &slider = controls[i]->slider;
        bool useLinear = compactLayout || detail.getHeight() < 100;
        slider.setSliderStyle(useLinear ? juce::Slider::LinearHorizontal
                                        : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(useLinear ? juce::Slider::TextBoxRight : juce::Slider::TextBoxBelow,
                               false, 65, 19);
        controls[i]->setBounds(224 + (int)i * dk, detail.getY() + 4, dk - 12,
                               detail.getHeight() - 8);
    }
}
void RastroEditor::paint(juce::Graphics &g)
{
    auto pal = visuals::palette(p);
    g.fillAll(pal.bg);
    if (p.value("cosmicArcade") > .5f)
        visuals::cosmicTexture(g, getLocalBounds().toFloat(), pal.highlight);
    g.setColour(pal.highlight);
    juce::Font logo(juce::FontOptions(31, juce::Font::bold));
    g.setFont(logo);
    g.drawSingleLineText("RASTRO", 20, 35);
    juce::GlyphArrangement word;
    word.addLineOfText(logo, "RASTRO", 20, 35);
    int playX = juce::roundToInt(word.getBoundingBox(0, -1, true).getRight()) + 6;
    g.setFont(juce::Font(juce::FontOptions(19)));
    g.drawSingleLineText("PLAY", playX, 35);
    g.setColour(pal.muted);
    g.setFont(juce::Font(juce::FontOptions(11)));
    g.drawText("by Gab Calisman", 21, 36, 190, 14, juce::Justification::left);
    g.setColour(pal.surface);
    g.fillRoundedRectangle(samplePanel.toFloat(),
                           visuals::panelRadius((int)p.value("visualTheme")));
    g.fillRoundedRectangle(macroPanel.toFloat(), visuals::panelRadius((int)p.value("visualTheme")));
    g.fillRoundedRectangle(detail.toFloat(), visuals::panelRadius((int)p.value("visualTheme")));
    g.setColour(pal.text.withAlpha(.025f));
    for (int y = macroPanel.getY() + 4; y < macroPanel.getBottom() - 3; y += 7)
        g.drawHorizontalLine(y, (float)macroPanel.getX() + 5, (float)macroPanel.getRight() - 5);
    g.setColour(pal.outline.withAlpha(.35f));
    g.drawRoundedRectangle(samplePanel.toFloat(), visuals::panelRadius((int)p.value("visualTheme")),
                           1);
    g.drawRoundedRectangle(detail.toFloat(), visuals::panelRadius((int)p.value("visualTheme")), 1);
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(13, juce::Font::bold)));
    int titleWidth = std::max(40, playbackMode.getX() - wave.getX() - 10);
    g.drawText(tr("sample"), wave.getX(), 92, titleWidth, 18, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.setColour(pal.muted);
    g.drawText(p.sampleName(), wave.getX(), 110, titleWidth, 17, juce::Justification::left);
    g.setColour(pal.text);
    g.drawText(tr("rack"), rackTitle.withWidth(440), juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(detail.getHeight() < 80 ? 15 : 20, juce::Font::bold)));
    g.drawText(names[selected], 32, detail.getY() + (detail.getHeight() < 80 ? 2 : 12), 175, 24,
               juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.setColour(pal.muted);
    g.drawFittedText(tr(moduleDescriptions[selected]), 32,
                     detail.getY() + (detail.getHeight() < 80 ? 27 : 44), 170,
                     std::max(20, detail.getHeight() - (detail.getHeight() < 80 ? 29 : 50)),
                     juce::Justification::topLeft, 3);
    if (p.value(presentIds[selected]) < .5f)
        g.drawText(tr("removed"), 224, detail.getY() + 24, getWidth() - 260, 40,
                   juce::Justification::centred);
    g.setColour(p.midiFlash > 0 ? pal.secondary : pal.outline);
    g.fillEllipse(125, 61, 6, 6);
    g.setColour(pal.muted);
    g.drawText("MIDI", 137, 54, 40, 20, juce::Justification::left);
    float bpm = p.hostBpm;
    g.drawText(juce::String(bpm > 0 ? bpm : p.value("tempo"), bpm > 0 ? 1 : 0) +
                   (bpm > 0 ? " BPM HOST" : " BPM MANUAL"),
               188, 54, 150, 20, juce::Justification::left);
}
bool RastroEditor::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::escapeKey && customizer->isVisible())
    {
        customizer->setVisible(false);
        customizerClosed();
        return true;
    }
    return false;
}
void RastroEditor::customizerClosed()
{
    avatar.resume();
    deck.resume();
    for (auto *child : getChildren())
        if (child != customizer.get())
            child->setEnabled(true);
    synchroniseUi();
    customize.grabKeyboardFocus();
    ++measurements.fullRepaints;
    repaint();
}
void RastroEditor::import(juce::File file)
{
    p.importFileAsync(
        file,
        [safe = juce::Component::SafePointer<RastroEditor>(this)](rastro::ImportError result)
        {
            if (safe && result != rastro::ImportError::none)
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                       "RASTRO", safe->p.importMessage(result));
        });
}
bool RastroEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    return files.size() == 1 && (rastro::offeredAudioFile(juce::File(files[0])) ||
                                 rastro::editableRecording(juce::File(files[0])));
}
void RastroEditor::filesDropped(const juce::StringArray &files, int, int)
{
    if (isInterestedInFileDrag(files))
        import(juce::File(files[0]));
}

void RastroEditor::showCustomizer()
{
    if (customizer->isVisible())
    {
        customizer->setVisible(false);
        customizerClosed();
        return;
    }
    for (auto *child : getChildren())
        if (child != customizer.get())
            child->setEnabled(false);
    customizer->open();
    synchroniseUi();
}

void RastroEditor::visibilityChanged()
{
    if (!isShowing())
    {
        windowWasShowing = false;
        startTimerHz(6);
        return;
    }
    windowWasShowing = true;
    avatar.resume();
    deck.resume();
    int mode = int(p.value("animation"));
    startTimerHz(mode == 0 ? 6 : mode == 1 ? 12 : 24);
}

bool RastroEditor::Card::updateTiming()
{
    int slot = rastro::timingSlot(module);
    if (slot < 0)
        return false;
    int mode = int(ui.p.value(rastro::timingIds[slot]));
    auto id = juce::String(rastro::timingParameter(slot, mode));
    auto label =
        mode == 1     ? "FREE"
        : mode == 2   ? juce::String("SYNC ") + rastro::divisionNames[juce::jlimit(
                                                  0, 3, int(ui.p.value(rastro::divisionIds[slot])))]
        : module == 3 ? "FREE*"
                      : "SYNC*";
    timing.setButtonText(label);
    timing.setTooltip(ui.tr("timingHelp"));
    bool changed = false;
    int index = module == 3 ? 1 : 0;
    if (knobs[index]->id != id)
    {
        knobs[index] = std::make_unique<Knob>(ui.p, id);
        ui.addChildComponent(*knobs[index]);
        knobs[index]->label.setText(ui.tr(id), juce::dontSendNotification);
        knobs[index]->slider.setTooltip(ui.tr("timingHelp"));
        knobs[index]->applyPalette();
        changed = true;
    }
    if (module != 3 && primary->id != id)
    {
        primary = std::make_unique<Knob>(ui.p, id, false);
        primary->setComponentID("rackPrimary_" + juce::String(module));
        primary->label.setVisible(false);
        primary->slider.setComponentID("rackPrimarySlider_" + juce::String(module));
        addAndMakeVisible(*primary);
        resized();
        changed = true;
    }
    return changed;
}
