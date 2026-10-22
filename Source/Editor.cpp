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
                    .withAlpha(over ? 1.f : .65f));
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
        label.setBounds(r.removeFromTop(18));
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
    auto colour =
        visuals::readable(accents[module].interpolatedWith(pal.highlight, .18f), pal.surface);
    auto box = getLocalBounds().toFloat().reduced(1);
    g.setColour(pal.surface.interpolatedWith(colour, active ? .06f : .01f));
    g.fillRoundedRectangle(box, 6);
    g.setColour(ui.selected == module ? colour : pal.outline);
    g.drawRoundedRectangle(box, 6, ui.selected == module ? 2.f : 1.f);
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
    g.drawText(ui.tr(shortKeys[module]), 10, 25, getWidth() - 18, 16, juce::Justification::left);
    g.drawText(!present ? ui.tr("removed") : ui.tr("mix"), 10, 96, 27, 24,
               juce::Justification::left);
    g.setColour(pal.muted);
    g.drawText(ui.tr(primary->id), getWidth() / 2, 43, getWidth() / 2 - 9, 24,
               juce::Justification::right);
}
void RastroEditor::Card::resized()
{
    remove.setBounds(getWidth() - 29, 3, 26, 25);
    bypass.setBounds(9, 43, getWidth() / 2 - 13, 24);
    primary->setBounds(8, 68, getWidth() - 16, 26);
    primary->slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 43, 22);
    mix->setBounds(36, 95, getWidth() - 44, 26);
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
      keyboard(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&style);
    for (auto *b :
         {&load, &mutate, &undo, &reset, &add, &about, &center, &preview, &customize, &compact})
        addAndMakeVisible(b);
    customize.setComponentID("customize");
    customize.setButtonText("CORES / KNOBS / GROOVEIRO");
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
    setSize(p.value("uiCompact") > .5f ? 1000 : 1180, p.value("uiCompact") > .5f ? 700 : 840);
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
    customize.setButtonText(p.language == 0 ? "CORES / KNOBS / GROOVEIRO"
                                            : "COLOURS / KNOBS / GROOVEIRO");
    load.setButtonText(tr("load"));
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
    play.setTooltip(tr("playHelp"));
    about.setButtonText(tr("about"));
    add.setButtonText(tr("add"));
    loop.setButtonText(tr("loop"));
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
        c->setTooltip(tr(moduleDescriptions[c->module]));
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
    mutate.setTooltip(tr("mutateHelp"));
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
void RastroEditor::timerCallback() { synchroniseUi(); }
void RastroEditor::synchroniseUi()
{
    auto start = juce::Time::getMillisecondCounterHiRes();
    ++measurements.ticks;
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
            setSize(requested ? 1000 : 1180, requested ? 700 : 840);
        compact.setToggleState(requested != 0, juce::dontSendNotification);
    }
    int animationMode = (int)p.value("animation"), hz = animationMode == 0   ? 6
                                                        : animationMode == 1 ? 12
                                                                             : 24;
    if (getTimerInterval() != 1000 / hz)
        startTimerHz(hz);
    bool editing = customizer->isVisible();
    if (editing)
    {
        if (customizer->update(animationMode))
            ++measurements.previewAvatar;
    }
    else
    {
        if (deck.isShowing())
        {
            deck.update(animationMode);
            ++measurements.deckUpdates;
        }
        avatar.update(animationMode);
        ++measurements.mainAvatar;
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
        bool present = p.value(presentIds[i]) > .5f;
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
        auto text = p.learn == i ? tr("learning")
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
    wasCustomizing = editing;
    measurements.updateMs += juce::Time::getMillisecondCounterHiRes() - start;
}
void RastroEditor::resized()
{
    ++measurements.layouts;
    int w = getWidth(), h = getHeight();
    compactLayout = h < 780;
    int margin = 20, content = w - margin * 2;
    customize.setBounds(w - 655, 12, 270, 30);
    compact.setBounds(w - 375, 12, 100, 30);
    about.setBounds(w - 265, 12, 70, 30);
    language.setBounds(w - 185, 12, 165, 30);
    animations.setBounds(w - 350, 48, 330, 28);
    customizer->setBounds(12, 80, w - 24, h - 92);
    samplePanel = {20, 84, content, compactLayout ? 146 : 162};
    load.setBounds(w - 190, 96, 170, 30);
    playbackMode.setBounds(w - 390, 96, 190, 30);
    preview.setBounds(w - 510, 96, 110, 30);
    sliceCount.setBounds(w - 576, 96, 56, 30);
    int sampleX = compactLayout ? 204 : 220;
    deck.setBounds(32, 94, sampleX - 44, samplePanel.getBottom() - 106);
    wave = {sampleX, 128, w - sampleX - 32, compactLayout ? 43 : 53};
    sampleView.setBounds(wave);
    int sampleY = compactLayout ? 176 : 190;
    int column = (w - 180 - sampleX - 10) / 6;
    for (int i = 0; i < 6; ++i)
        sampleKnobs[i]->setBounds(sampleX + i * column, sampleY, column - 8, 50);
    pitchLabel.setBounds(w - 180, sampleY, 148, 18);
    pitchMode.setBounds(w - 180, sampleY + 20, 148, 28);
    int performanceY = samplePanel.getBottom() + 12;
    int performanceHeight = compactLayout ? 144 : std::max(174, h - 640);
    int playW = content * 31 / 100, avatarW = content * 20 / 100, macroX = 44 + playW + avatarW,
        macroW = (w - 20 - macroX) / 4;
    play.setBounds(20, performanceY, playW, performanceHeight);
    avatar.setBounds(30 + playW, performanceY, avatarW, performanceHeight);
    macroPanel = {macroX - 4, performanceY, w - 16 - macroX, performanceHeight};
    for (int i = 0; i < 4; ++i)
    {
        macros[i]->setBounds(macroX + i * macroW, performanceY + 5, macroW - 6,
                             performanceHeight - 37);
        if (i < 3)
            learns[i].setBounds(macroX + i * macroW + 3, performanceY + performanceHeight - 30,
                                macroW - 12, 26);
    }
    int barY = performanceY + performanceHeight + 8;
    center.setBounds(20, barY, 92, 28);
    learns[3].setBounds(120, barY, (playW - 100) / 2, 28);
    learns[4].setBounds(126 + (playW - 100) / 2, barY, (playW - 100) / 2, 28);
    mutate.setBounds(macroX, barY, macroW * 2 - 8, 28);
    undo.setBounds(macroX + macroW * 2, barY, macroW * 2 - 8, 28);
    rackTitle = {20, barY + 36, content, 24};
    reset.setBounds(w - 328, rackTitle.getY(), 186, 28);
    add.setBounds(w - 134, rackTitle.getY(), 114, 28);
    rack = {20, rackTitle.getBottom() + 8, content, 124};
    auto order = p.settings().order;
    int cw = (content - 40) / 6;
    for (int i = 0; i < moduleCount; ++i)
        cards[order[i]]->setBounds(20 + i * (cw + 8), rack.getY(), cw, 124);
    int keyboardHeight = compactLayout ? 34 : 44;
    keyboard.setBounds(20, h - keyboardHeight - 8, content, keyboardHeight);
    detail = {20, rack.getBottom() + 10, content, keyboard.getY() - rack.getBottom() - 18};
    for (int m = 0; m < moduleCount; ++m)
        for (auto &k : cards[m]->knobs)
            k->setVisible(m == selected && p.value(presentIds[m]) > .5f);
    auto &controls = cards[selected]->knobs;
    int dk = (content - 210) / std::max(1, (int)controls.size());
    for (size_t i = 0; i < controls.size(); ++i)
    {
        auto &slider = controls[i]->slider;
        slider.setSliderStyle(compactLayout ? juce::Slider::LinearHorizontal
                                            : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(
            compactLayout ? juce::Slider::TextBoxRight : juce::Slider::TextBoxBelow, false, 65, 19);
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
    g.setFont(juce::Font(juce::FontOptions(36, juce::Font::bold | juce::Font::italic)));
    g.drawText("RASTRO", 20, 7, 220, 40, juce::Justification::left);
    g.setColour(pal.muted);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText("PLAY / by Gab Calisman", 20, 49, 220, 18, juce::Justification::left);
    g.setColour(pal.surface);
    g.fillRoundedRectangle(samplePanel.toFloat(),
                           visuals::panelRadius((int)p.value("visualTheme")));
    g.fillRoundedRectangle(macroPanel.toFloat(), visuals::panelRadius((int)p.value("visualTheme")));
    g.fillRoundedRectangle(detail.toFloat(), visuals::panelRadius((int)p.value("visualTheme")));
    g.setColour(pal.text.withAlpha(.025f));
    for (int y = macroPanel.getY() + 4; y < macroPanel.getBottom() - 3; y += 7)
        g.drawHorizontalLine(y, (float)macroPanel.getX() + 5, (float)macroPanel.getRight() - 5);
    g.setColour(pal.outline);
    g.drawRoundedRectangle(samplePanel.toFloat(), visuals::panelRadius((int)p.value("visualTheme")),
                           1);
    g.drawRoundedRectangle(detail.toFloat(), visuals::panelRadius((int)p.value("visualTheme")), 1);
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(13, juce::Font::bold)));
    int titleWidth = std::max(120, sliceCount.getX() - wave.getX() - 10);
    g.drawText(tr("sample"), wave.getX(), 92, titleWidth, 18, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.setColour(pal.muted);
    g.drawText(p.sampleName(), wave.getX(), 110, titleWidth, 17, juce::Justification::left);
    g.setColour(pal.text);
    g.drawText(tr("rack"), rackTitle.withWidth(440), juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(compactLayout ? 15 : 20, juce::Font::bold)));
    g.drawText(names[selected], 32, detail.getY() + (compactLayout ? 2 : 12), 175, 24,
               juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.setColour(pal.muted);
    g.drawFittedText(tr(moduleDescriptions[selected]), 32,
                     detail.getY() + (compactLayout ? 27 : 44), 170,
                     std::max(20, detail.getHeight() - (compactLayout ? 29 : 50)),
                     juce::Justification::topLeft, 3);
    if (p.value(presentIds[selected]) < .5f)
        g.drawText(tr("removed"), 224, detail.getY() + 24, getWidth() - 260, 40,
                   juce::Justification::centred);
    g.setColour(p.midiFlash > 0 ? pal.secondary : pal.outline);
    g.fillEllipse(230, 54, 7, 7);
    g.setColour(pal.muted);
    g.drawText("MIDI", 242, 48, 40, 20, juce::Justification::left);
    float bpm = p.hostBpm;
    g.drawText(juce::String(bpm > 0 ? bpm : p.value("tempo"), bpm > 0 ? 1 : 0) + " BPM", 290, 48,
               120, 20, juce::Justification::left);
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
