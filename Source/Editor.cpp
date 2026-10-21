#include "Editor.h"
#include "Visuals.h"
namespace
{
const juce::Colour ink(0xff261e19), cream(0xfff2e3c5), orange(0xffdf682f);
const juce::Colour accents[]{juce::Colour(0xffe69b62), juce::Colour(0xffbec863),
                             juce::Colour(0xff68b8ab), juce::Colour(0xffe8ba63),
                             juce::Colour(0xffd4aaac), juce::Colour(0xffb7a1d8)};
using namespace rastro;
const auto &names = moduleNames;
} // namespace
RastroEditor::Style::Style()
{
    setColour(juce::TextButton::buttonColourId, orange);
    setColour(juce::TextButton::textColourOffId, ink);
    setColour(juce::Slider::textBoxTextColourId, ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, cream);
    setColour(juce::ComboBox::textColourId, ink);
    setColour(juce::ToggleButton::textColourId, ink);
    setColour(juce::ToggleButton::tickColourId, orange);
}
void RastroEditor::Style::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float v,
                                           float a, float b, juce::Slider &)
{
    auto box = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(3);
    float r = std::min(box.getWidth(), box.getHeight()) * .5f;
    auto c = box.getCentre();
    g.setColour(ink.withAlpha(.2f));
    g.fillEllipse(c.x - r + 2, c.y - r + 4, 2 * r, 2 * r);
    g.setColour(ink);
    g.fillEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
    if (controlStyle == 1)
    {
        g.setColour(accent.withAlpha(.4f));
        g.drawRoundedRectangle(box, 3, 2);
    }
    else if (controlStyle == 2)
    {
        g.setColour(cream.withAlpha(.7f));
        for (int i = 0; i <= 10; ++i)
        {
            float t = a + (b - a) * i / 10.f;
            g.drawLine(c.x + std::sin(t) * r * .82f, c.y - std::cos(t) * r * .82f,
                       c.x + std::sin(t) * r, c.y - std::cos(t) * r, 1);
        }
    }
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, r - 3, r - 3, 0, a, a + (b - a) * v, true);
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(3));
    float angle = a + (b - a) * v;
    g.setColour(cream);
    g.drawLine(c.x + std::sin(angle) * r * .5f, c.y - std::cos(angle) * r * .5f,
               c.x + std::sin(angle) * r * .8f, c.y - std::cos(angle) * r * .8f, 3);
}
void RastroEditor::Style::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                               const juce::Colour &colour, bool over, bool down)
{
    g.setColour(colour.withMultipliedBrightness(down ? .8f : over ? 1.1f : 1.f));
    auto box = b.getLocalBounds().toFloat().reduced(1);
    g.fillRoundedRectangle(box, controlStyle == 1 ? 2.f : controlStyle == 2 ? 12.f : 6.f);
    g.setColour(juce::Colour(0xff261e19));
    g.drawRoundedRectangle(box, controlStyle == 1 ? 2.f : controlStyle == 2 ? 12.f : 6.f, 1);
}
RastroEditor::Knob::Knob(RastroProcessor &p, juce::String key, bool rotary) : id(key)
{
    slider.setColour(juce::Slider::textBoxTextColourId, ink);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::thumbColourId, orange);
    slider.setColour(juce::Slider::trackColourId, orange);
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
void RastroEditor::Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(19));
    slider.setBounds(r);
}
RastroEditor::Card::Card(RastroEditor &e, int m) : ui(e), module(m)
{
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
    g.setColour(present ? accents[module] : cream.darker(.25f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);
    if (ui.selected == module)
    {
        g.setColour(cream);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 9, 2);
    }
    g.setColour(ink.withAlpha(active ? 1.f : .55f));
    g.setFont(juce::Font(juce::FontOptions(22, juce::Font::bold)));
    g.drawText(names[module], 12, 3, getWidth() - 116, 30, juce::Justification::left);
    if (!present)
    {
        g.setFont(juce::Font(juce::FontOptions(11)));
        g.drawFittedText(ui.tr("removed"), 12, 36, getWidth() - 24, 30, juce::Justification::left,
                         2);
    }
    else
    {
        g.setFont(juce::Font(juce::FontOptions(11)));
        g.drawText(ui.tr("mix"), 12, 39, 48, 22, juce::Justification::left);
    }
}
void RastroEditor::Card::resized()
{
    remove.setBounds(getWidth() - 29, 5, 22, 23);
    bypass.setBounds(getWidth() - 105, 5, 71, 23);
    mix->setBounds(58, 27, getWidth() - 66, std::max(43, getHeight() - 29));
    mix->slider.setTextBoxStyle(juce::Slider::TextBoxRight, true, 43, 18);
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
    ui.resized();
}
void RastroEditor::selectModule(int m)
{
    selected = m;
    resized();
    repaint();
}
RastroEditor::RastroEditor(RastroProcessor &processor)
    : AudioProcessorEditor(processor), p(processor), play(p), avatar(p), sampleView(p),
      keyboard(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&style);
    for (auto *b : {&load, &mutate, &undo, &reset, &add, &about, &center, &preview, &customize})
        addAndMakeVisible(b);
    customize.setComponentID("customize");
    customize.setButtonText("VISUAL");
    customize.onClick = [this] { showCustomizer(); };
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
                                                      "*.wav;*.aiff;*.aif;*.mp3");
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
    setResizeLimits(900, 860, 1600, 1200);
    setSize(1180, 960);
    localize();
    timerCallback();
    startTimerHz(24);
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
    load.setButtonText(tr("load"));
    mutate.setButtonText(tr("mutate"));
    reset.setButtonText(tr("reset"));
    undo.setButtonText(tr("undoMutation"));
    center.setButtonText(tr("center"));
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
void RastroEditor::timerCallback()
{
    p.applyCC();
    style.controlStyle = (int)p.value("controlStyle");
    style.accent = visuals::accent((int)p.value("visualPalette"));
    style.setColour(juce::TextButton::buttonColourId, style.accent);
    if (lastLanguage != p.language)
        localize();
    int animationMode = (int)p.value("animation");
    int hz = animationMode == 0 ? 6 : animationMode == 1 ? 12 : 24;
    if (getTimerInterval() != 1000 / hz)
        startTimerHz(hz);
    glow = std::max(glow * .8f, p.engine.meter.load());
    avatar.update(animationMode);
    if (p.midiFlash > 0)
        --p.midiFlash;
    for (int i = 0; i < moduleCount; ++i)
    {
        bool present = p.value(presentIds[i]) > .5f;
        cards[i]->mix->setVisible(present);
        cards[i]->bypass.setVisible(present);
        cards[i]->remove.setVisible(present);
        cards[i]->bypass.setButtonText(tr(p.value(onIds[i]) > .5f ? "on" : "off"));
    }
    for (int i = 0; i < 5; ++i)
    {
        auto label = i < 3 ? tr("learn") : (i == 3 ? "X / " : "Y / ") + tr("learn");
        learns[i].setButtonText(p.learn == i
                                    ? tr("learning")
                                    : label + (p.cc[i] >= 0 ? " " + juce::String(p.cc[i]) : ""));
    }
    int mode = (int)p.value("playMode");
    if (mode == 0 && p.value("loop") > .5f)
        mode = 1;
    playbackMode.setSelectedId(mode + 1, juce::dontSendNotification);
    sliceCount.setSelectedId((int)p.value("sliceCount"), juce::dontSendNotification);
    sliceCount.setVisible(mode == 4);
    preview.setButtonText(tr(p.engine.previewActive ? "stopPreview" : "preview"));
    undo.setEnabled(p.canUndoMutation());
    resized();
    repaint();
}
void RastroEditor::resized()
{
    int w = getWidth();
    float scale = getHeight() / 960.f;
    auto y = [scale](int n) { return juce::roundToInt(n * scale); };
    customize.setBounds(w - 510, 53, 90, 26);
    language.setBounds(w - 190, 18, 165, 28);
    about.setBounds(w - 276, 18, 75, 28);
    animations.setBounds(w - 410, 53, 385, 26);
    load.setBounds(w - 205, y(104), 175, 30);
    playbackMode.setBounds(w - 205, y(140), 175, y(26));
    preview.setBounds(w - 205, y(173), 105, y(26));
    sliceCount.setBounds(w - 94, y(173), 64, y(26));
    wave = {28, y(138), w - 250, y(64)};
    sampleView.setBounds(wave);
    int kw = (w - 56) / 6;
    for (int i = 0; i < 6; ++i)
        sampleKnobs[i]->setBounds(28 + i * kw, y(207), kw - 8, y(65));
    int padWidth = juce::roundToInt((w - 56) * .41f),
        avatarWidth = juce::roundToInt((w - 56) * .25f);
    play.setBounds(28, y(312), padWidth, y(224));
    avatar.setBounds(40 + padWidth, y(312), avatarWidth, y(224));
    center.setBounds(28, y(543), 90, y(26));
    int learnW = (padWidth - 106) / 2;
    learns[3].setBounds(128, y(543), learnW, y(26));
    learns[4].setBounds(134 + learnW, y(543), learnW, y(26));
    int macroX = 52 + padWidth + avatarWidth, macroW = (w - 28 - macroX) / 2;
    for (int i = 0; i < 4; ++i)
    {
        macros[i]->setBounds(macroX + (i % 2) * macroW, y(310 + (i / 2) * 110), macroW - 4, y(84));
        if (i < 3)
            learns[i].setBounds(macroX + (i % 2) * macroW, y(395 + (i / 2) * 110), macroW - 5,
                                y(22));
    }
    mutate.setBounds(macroX, y(543), macroW, y(28));
    undo.setBounds(macroX + macroW + 4, y(543), macroW - 4, y(28));
    rack = {28, y(609), w - 56, y(157)};
    auto order = p.settings().order;
    int cw = (w - 76) / 3;
    for (int i = 0; i < moduleCount; ++i)
        cards[order[i]]->setBounds(28 + (i % 3) * (cw + 10), y(609 + (i / 3) * 82), cw, y(75));
    add.setBounds(w - 152, y(578), 122, y(25));
    reset.setBounds(w - 340, y(578), 180, y(25));
    detail = {28, y(778), w - 56, y(103)};
    for (int m = 0; m < moduleCount; ++m)
        for (auto &k : cards[m]->knobs)
            k->setVisible(m == selected && p.value(presentIds[m]) > .5f);
    auto &controls = cards[selected]->knobs;
    int dk = (w - 270) / std::max(1, (int)controls.size());
    for (size_t i = 0; i < controls.size(); ++i)
        controls[i]->setBounds(240 + (int)i * dk, y(782), dk - 8, y(94));
    keyboard.setBounds(28, y(895), w - 56, std::max(24, getHeight() - y(915)));
}
void RastroEditor::paint(juce::Graphics &g)
{
    style.controlStyle = (int)p.value("controlStyle");
    style.accent = visuals::accent((int)p.value("visualPalette"));
    style.setColour(juce::TextButton::buttonColourId, style.accent);
    g.fillAll(visuals::background((int)p.value("visualTheme")));
    g.setColour(orange);
    g.setFont(juce::Font(juce::FontOptions(52, juce::Font::bold | juce::Font::italic)));
    g.drawText("RASTRO", 27, 12, 330, 65, juce::Justification::left);
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(14)));
    g.drawText("by Gab Calisman", 340, 43, 200, 24, juce::Justification::left);
    float scale = getHeight() / 960.f;
    auto y = [scale](int n) { return juce::roundToInt(n * scale); };
    g.setColour(cream);
    g.fillRoundedRectangle(18.f, (float)y(94), (float)getWidth() - 36, (float)y(183), 12);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText(tr("sample"), 30, y(101), getWidth() - 260, y(22), juce::Justification::left);
    g.drawText(p.sampleName(), 30, y(119), getWidth() - 260, y(18), juce::Justification::left);
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText(tr("perform"), 30, y(284), 450, y(25), juce::Justification::left);
    int mx = 52 + juce::roundToInt((getWidth() - 56) * .41f) +
             juce::roundToInt((getWidth() - 56) * .25f);
    g.setColour(cream);
    g.fillRoundedRectangle((float)mx - 4, (float)y(310), (float)(getWidth() - 24 - mx),
                           (float)y(222), 10);
    g.setColour(cream);
    g.drawText(tr("rack"), 30, y(578), getWidth() - 390, y(25), juce::Justification::left);
    g.setColour(cream);
    g.fillRoundedRectangle(detail.toFloat(), 10);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(21, juce::Font::bold)));
    g.drawText(names[selected], 40, y(789), 185, y(28), juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawFittedText(tr(moduleDescriptions[selected]), 40, y(821), 185, y(43),
                     juce::Justification::left, 2);
    if (p.value(presentIds[selected]) < .5f)
        g.drawText(tr("removed"), 240, y(813), getWidth() - 290, y(40),
                   juce::Justification::centred);
    g.setColour(p.midiFlash > 0 ? accents[1] : cream.darker(.6f));
    g.fillEllipse(250.f, 67, 8, 8);
    g.setColour(cream);
    g.drawText("MIDI", 265, 60, 55, 24, juce::Justification::left);
    float bpm = p.hostBpm;
    g.drawText((bpm > 0 ? tr("host") + " " + juce::String(bpm, 1)
                        : tr("tempo") + " " + juce::String(juce::roundToInt(p.value("tempo")))) +
                   " BPM",
               28, 69, 450, 20, juce::Justification::left);
}

void RastroEditor::import(juce::File file)
{
    p.loadFileAsync(file,
                    [safe = juce::Component::SafePointer<RastroEditor>(this)](bool ok)
                    {
                        if (safe && !ok)
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::MessageBoxIconType::WarningIcon, "RASTRO", safe->tr("error"));
                    });
}
bool RastroEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    return files.size() == 1 && juce::File(files[0]).hasFileExtension("wav;aiff;aif;mp3");
}
void RastroEditor::filesDropped(const juce::StringArray &files, int, int)
{
    if (isInterestedInFileDrag(files))
        import(juce::File(files[0]));
}

void RastroEditor::showCustomizer()
{
    const bool pt = p.language == 0;
    juce::PopupMenu menu;
    const char *ids[]{"visualTheme", "visualPalette", "controlStyle",
                      "avatarBody",  "avatarEyes",    "avatarAccessory"};
    const juce::String titles[]{pt ? "Tema" : "Theme",         pt ? "Paleta" : "Palette",
                                pt ? "Controles" : "Controls", pt ? "Corpo" : "Body",
                                pt ? "Olhos" : "Eyes",         pt ? "Acessório" : "Accessory"};
    const juce::StringArray choices[]{
        {"Retro", "Futuro", "Arcade", pt ? "Orgânico" : "Organic"},
        {pt ? "Laranja" : "Orange", pt ? "Turquesa" : "Teal", "Rosa", "Lima"},
        {"Classic", "Studio", "Groove"},
        {pt ? "Engrenagem" : "Gear", pt ? "Cápsula" : "Capsule", pt ? "Quadrado" : "Square"},
        {pt ? "Redondos" : "Round", pt ? "Visor" : "Visor", "Pixel"},
        {pt ? "Antena" : "Antenna", pt ? "Fones" : "Headphones", pt ? "Lenço" : "Scarf"}};
    for (int group = 0; group < 6; ++group)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < choices[group].size(); ++i)
            sub.addItem(group * 10 + i + 1, choices[group][i], true, (int)p.value(ids[group]) == i);
        menu.addSubMenu(titles[group], sub);
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&customize),
                       [safe = juce::Component::SafePointer<RastroEditor>(this)](int result)
                       {
                           if (!safe || result == 0)
                               return;
                           const char *keys[]{"visualTheme", "visualPalette", "controlStyle",
                                              "avatarBody",  "avatarEyes",    "avatarAccessory"};
                           int group = (result - 1) / 10, choice = (result - 1) % 10;
                           if (group < 6)
                               safe->p.set(keys[group], (float)choice);
                           safe->repaint();
                       });
}
