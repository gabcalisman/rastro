#include "Editor.h"
namespace
{
const juce::Colour ink(0xff261e19), cream(0xfff2e3c5), orange(0xffdf682f);
const juce::Colour accents[]{juce::Colour(0xffe69b62), juce::Colour(0xffbec863),
                             juce::Colour(0xff68b8ab)};
const char *names[]{"GHOST", "CRUMBLE", "BLOOM"};
const char *onIds[]{"gOn", "cOn", "bOn"};
const char *presentIds[]{"gPresent", "cPresent", "bPresent"};
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
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, r - 3, r - 3, 0, a, a + (b - a) * v, true);
    g.setColour(orange);
    g.strokePath(arc, juce::PathStrokeType(3));
    float angle = a + (b - a) * v;
    g.setColour(cream);
    g.drawLine(c.x + std::sin(angle) * r * .5f, c.y - std::cos(angle) * r * .5f,
               c.x + std::sin(angle) * r * .8f, c.y - std::cos(angle) * r * .8f, 3);
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
    bypass.onClick = [this] { ui.p.set(onIds[module], 1 - ui.p.value(onIds[module])); };
    const char *ids[3][4] = {{"gTime", "gFeedback", "gTone", "gMix"},
                             {"cRate", "cVariation", "cMix", nullptr},
                             {"bSize", "bTone", "bDecay", "bMix"}};
    for (auto id : ids[m])
        if (id)
        {
            knobs.push_back(std::make_unique<Knob>(ui.p, id));
            addAndMakeVisible(*knobs.back());
        }
}
void RastroEditor::Card::paint(juce::Graphics &g)
{
    bool present = ui.p.value(presentIds[module]) > .5f;
    g.setColour(present ? accents[module] : cream.darker(.25f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 12);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(27, juce::Font::bold)));
    g.drawText(names[module], 19, 12, getWidth() - 120, 35, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    if (module == 0)
    {
        for (int i = 0; i < 3; ++i)
            g.drawEllipse(18.f + i * 4, 52.f, 10.f, 10.f, 1.f);
    }
    else if (module == 1)
    {
        for (int i = 0; i < 3; ++i)
            g.fillRect(18 + i * 6, 52 + (i % 2) * 3, 4, 7);
    }
    else
    {
        for (int i = 0; i < 3; ++i)
            g.drawEllipse(18.f + i * 3, 51.f + i * 3, 16.f - i * 6, 16.f - i * 6, 1.f);
    }
    g.drawText(ui.tr(module == 0   ? "ghost"
                     : module == 1 ? "crumble"
                                   : "bloom"),
               42, 47, getWidth() - 52, 22, juce::Justification::left);
    g.drawLine(18, 77, (float)getWidth() - 18, 77, 1);
    if (!present)
        g.drawFittedText(ui.tr("removed"), getLocalBounds().reduced(15).withTrimmedTop(85),
                         juce::Justification::centred, 2);
}
void RastroEditor::Card::resized()
{
    remove.setBounds(getWidth() - 34, 12, 24, 25);
    bypass.setBounds(getWidth() - 114, 12, 75, 25);
    int w = (getWidth() - 22) / 2;
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i]->setBounds(10 + (int(i) % 2) * w, 84 + (int(i) / 2) * 91, w, 89);
}
void RastroEditor::Card::mouseDrag(const juce::MouseEvent &e)
{
    if (e.getMouseDownY() < 77)
    {
        ui.dragged = module;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
}
void RastroEditor::Card::mouseUp(const juce::MouseEvent &e)
{
    if (ui.dragged < 0)
        return;
    auto point = ui.getLocalPoint(this, e.getPosition());
    auto order = ui.p.settings().order;
    for (int to = 0; to < 3; ++to)
        if (ui.cards[order[to]]->getBounds().contains(point))
            for (int from = 0; from < 3; ++from)
                if (order[from] == module)
                    ui.p.moveModule(from, to);
    ui.dragged = -1;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    ui.resized();
}
RastroEditor::RastroEditor(RastroProcessor &processor)
    : AudioProcessorEditor(processor), p(processor),
      keyboard(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&style);
    for (auto *b : {&load, &mutate, &reset, &add, &about})
        addAndMakeVisible(b);
    addAndMakeVisible(language);
    language.addItem(text::get("pt", 0), 1);
    language.addItem(text::get("en", 0), 2);
    language.onChange = [this]
    {
        p.language = language.getSelectedId() - 1;
        localize();
    };
    addAndMakeVisible(loop);
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
    for (int i = 0; i < 3; ++i)
    {
        cards[i] = std::make_unique<Card>(*this, i);
        addAndMakeVisible(*cards[i]);
        addAndMakeVisible(learns[i]);
        learns[i].onClick = [this, i] { p.learn = p.learn == i ? -1 : i; };
    }
    addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(36, 96);
    keyboard.setKeyWidth(20);
    keyboard.setLowestVisibleKey(48);
    load.onClick = [this]
    {
        chooser =
            std::make_unique<juce::FileChooser>(tr("choose"), juce::File{}, "*.wav;*.aiff;*.aif");
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<RastroEditor>(this)](const juce::FileChooser &fc)
            {
                if (safe && fc.getResult().existsAsFile() && !safe->p.loadFile(fc.getResult()))
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                           "RASTRO", safe->tr("error"));
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
        for (int i = 0; i < 3; ++i)
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
    setSize(1100, 920);
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
    about.setButtonText(tr("about"));
    add.setButtonText(tr("add"));
    loop.setButtonText(tr("loop"));
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
        c->bypass.setTooltip(tr("bypassHelp"));
        for (auto &k : c->knobs)
        {
            k->label.setText(tr(k->id), juce::dontSendNotification);
            k->slider.setTooltip(tr("help"));
        }
    }
    mutate.setTooltip(tr("mutateHelp"));
    repaint();
}
void RastroEditor::timerCallback()
{
    p.applyCC();
    if (lastLanguage != p.language)
        localize();
    glow = std::max(glow * .85f, p.engine.meter.load());
    if (p.midiFlash > 0)
        --p.midiFlash;
    for (int i = 0; i < 3; ++i)
    {
        bool present = p.value(presentIds[i]) > .5f;
        for (auto &k : cards[i]->knobs)
            k->setVisible(present);
        cards[i]->bypass.setVisible(present);
        cards[i]->remove.setVisible(present);
        cards[i]->bypass.setButtonText(tr(p.value(onIds[i]) > .5f ? "on" : "off"));
        learns[i].setButtonText(
            p.learn == i ? tr("learning")
                         : tr("learn") + (p.cc[i] >= 0 ? " " + juce::String(p.cc[i]) : ""));
    }
    resized();
    repaint();
}
void RastroEditor::resized()
{
    int w = getWidth();
    language.setBounds(w - 190, 22, 165, 28);
    about.setBounds(w - 280, 22, 75, 28);
    load.setBounds(w - 205, 108, 175, 31);
    loop.setBounds(w - 195, 146, 150, 27);
    wave = {28, 145, w - 250, 85};
    int kw = (w - 56) / 6;
    for (int i = 0; i < 6; ++i)
        sampleKnobs[i]->setBounds(28 + i * kw, 237, kw - 8, 70);
    rack = {28, 350, w - 56, 275};
    auto order = p.settings().order;
    int cw = (w - 76) / 3;
    for (int i = 0; i < 3; ++i)
        cards[order[i]]->setBounds(28 + i * (cw + 10), 350, cw, 275);
    add.setBounds(w - 155, 316, 125, 27);
    performance = {28, 672, w - 56, 145};
    int mw = (w - 270) / 4;
    for (int i = 0; i < 4; ++i)
    {
        macros[i]->setBounds(28 + i * mw, 674, mw - 12, 110);
        if (i < 3)
            learns[i].setBounds(36 + i * mw, 790, mw - 28, 24);
    }
    mutate.setBounds(w - 225, 697, 195, 52);
    reset.setBounds(w - 225, 764, 195, 30);
    keyboard.setBounds(28, 830, w - 56, std::max(24, getHeight() - 850));
}
void RastroEditor::paint(juce::Graphics &g)
{
    g.fillAll(ink);
    g.setColour(orange);
    g.setFont(juce::Font(juce::FontOptions(52, juce::Font::bold | juce::Font::italic)));
    g.drawText("RASTRO", 27, 12, 330, 65, juce::Justification::left);
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(14)));
    g.drawText("by Gab Calisman", 340, 43, 200, 24, juce::Justification::left);
    g.setColour(cream);
    g.fillRoundedRectangle(18, 94, (float)getWidth() - 36, 215, 12);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText(tr("sample"), 30, 105, 450, 25, juce::Justification::left);
    g.drawText(p.sampleName(), 30, 126, getWidth() - 260, 20, juce::Justification::left);
    g.setColour(ink);
    g.fillRoundedRectangle(wave.toFloat(), 6);
    auto *s = p.source.load();
    if (s)
    {
        auto r = wave.reduced(8);
        int n = s->size();
        float start = p.value("start"), end = std::max(start, p.value("end"));
        g.setColour(orange.withAlpha(.22f));
        g.fillRect((float)r.getX() + start * r.getWidth(), (float)r.getY(),
                   (end - start) * r.getWidth(), (float)r.getHeight());
        g.setColour(cream);
        for (int x = 0; x < r.getWidth(); ++x)
        {
            int a = (int)((int64_t)x * n / r.getWidth()),
                b = std::max(a + 1, (int)((int64_t)(x + 1) * n / r.getWidth()));
            float peak = 0;
            for (int i = a; i < b; i += std::max(1, (b - a) / 32))
                peak = std::max(peak, std::abs(s->data[0][i]));
            g.drawVerticalLine(r.getX() + x, r.getCentreY() - peak * r.getHeight() * .45f,
                               r.getCentreY() + peak * r.getHeight() * .45f);
        }
        g.setColour(orange);
        for (float f : {start, end})
            g.drawVerticalLine(r.getX() + int(f * r.getWidth()), (float)r.getY(),
                               (float)r.getBottom());
        if (glow > .001f)
        {
            g.setColour(accents[1]);
            g.drawVerticalLine(r.getX() + int(p.engine.position * r.getWidth()), (float)r.getY(),
                               (float)r.getBottom());
        }
    }
    else
    {
        g.setColour(cream);
        g.setFont(juce::Font(juce::FontOptions(14)));
        g.drawFittedText(tr("empty"), wave.reduced(12), juce::Justification::centred, 2);
    }
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText(tr("rack"), 30, 316, getWidth() - 200, 28, juce::Justification::left);
    g.setColour(orange.withAlpha(.35f + std::min(.65f, glow)));
    g.fillRect(32, 640, getWidth() - 64, 3);
    g.setColour(cream);
    g.drawText(tr("perform"), 30, 646, 400, 24, juce::Justification::left);
    g.setColour(cream);
    g.fillRoundedRectangle(performance.toFloat(), 12);
    g.setColour(p.midiFlash > 0 ? accents[1] : cream.darker(.6f));
    g.fillEllipse((float)getWidth() - 310, 72, 8, 8);
    g.setColour(cream);
    g.drawText("MIDI", getWidth() - 295, 64, 55, 24, juce::Justification::left);
    float bpm = p.hostBpm;
    g.drawText((bpm > 0 ? tr("host") + " " + juce::String(bpm, 1)
                        : tr("tempo") + " " + juce::String(p.value("tempo"), 0)) +
                   " BPM",
               getWidth() - 230, 64, 200, 24, juce::Justification::right);
}
