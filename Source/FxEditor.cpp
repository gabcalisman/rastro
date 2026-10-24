#include "FxEditor.h"
#include "RastroFxAssets.h"
using namespace rastro::fx;
namespace
{
juce::Colour ink() { return juce::Colour(0xff24243e); }
juce::Colour hue(int index)
{
    const juce::uint32 colours[]{0xff91e0bd, 0xffff9ca3, 0xffbcafff, 0xffffd67c, 0xff91cfff};
    return juce::Colour(colours[juce::jlimit(0, 4, index)]);
}
void label(juce::Graphics &g, const juce::String &text, juce::Rectangle<int> area, float size = 14,
           juce::Justification align = juce::Justification::centredLeft)
{
    g.setColour(ink());
    g.setFont(juce::FontOptions(size, juce::Font::bold));
    g.drawFittedText(text, area, align, 2, .65f);
}
} // namespace
struct RastroFxEditor::Style : juce::LookAndFeel_V4
{
    Style()
    {
        setColour(juce::Slider::textBoxTextColourId, ink());
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xfffffbef));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xfffffbef));
        setColour(juce::ComboBox::textColourId, ink());
        setColour(juce::ComboBox::outlineColourId, ink());
        setColour(juce::ComboBox::arrowColourId, ink());
        setColour(juce::ToggleButton::tickColourId, ink());
        setColour(juce::ToggleButton::tickDisabledColourId, ink());
        setColour(juce::TextButton::buttonColourId, hue(0));
        setColour(juce::TextButton::textColourOffId, ink());
        setColour(juce::ToggleButton::textColourId, ink());
    }
    void drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float pos, float start,
                          float end, juce::Slider &) override
    {
        float radius = std::min(w, h) * .36f;
        juce::Point<float> centre(x + w * .5f, y + h * .5f);
        auto bounds = juce::Rectangle<float>(radius * 2, radius * 2).withCentre(centre);
        g.setColour(ink());
        g.fillEllipse(bounds.translated(2, 3));
        g.setColour(juce::Colour(0xffffd67c));
        g.fillEllipse(bounds);
        g.setColour(ink());
        g.drawEllipse(bounds, 2.3f);
        float angle = start + pos * (end - start);
        auto tip = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * radius * .76f;
        g.drawLine(centre.x, centre.y, tip.x, tip.y, 3);
    }
};
struct RastroFxEditor::Knob : juce::Component
{
    juce::Slider slider;
    juce::String title;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    Knob(RastroFxProcessor &p, const juce::String &id, const juce::String &name, bool rotary = true)
        : title(name)
    {
        setComponentID(id + "Control");
        slider.setComponentID(id);
        slider.setName(name);
        slider.setTooltip(name);
        slider.setSliderStyle(rotary ? juce::Slider::RotaryHorizontalVerticalDrag
                                     : juce::Slider::LinearHorizontal);
        slider.setColour(juce::Slider::textBoxTextColourId, ink());
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xfffffbef));
        slider.setColour(juce::Slider::textBoxOutlineColourId, ink().withAlpha(.2f));
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
        addAndMakeVisible(slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.params, id, slider);
        auto *parameter = p.params.getParameter(id);
        slider.textFromValueFunction = [parameter](double v)
        {
            return parameter->getText(parameter->convertTo0to1(float(v)), 32) + " " +
                   parameter->getLabel();
        };
        slider.valueFromTextFunction = [parameter](const juce::String &text)
        { return parameter->convertFrom0to1(parameter->getValueForText(text)); };
        slider.updateText();
    }
    void paint(juce::Graphics &g) override
    {
        label(g, title, {0, 0, getWidth(), 28}, 12, juce::Justification::centred);
    }
    void resized() override { slider.setBounds(getLocalBounds().withTrimmedTop(26)); }
};
struct RastroFxEditor::Pattern : juce::Component
{
    RastroFxProcessor &p;
    int dragged = -1;
    ~Pattern() override
    {
        if (dragged >= 0)
            p.params.getParameter("fxPan" + juce::String(dragged))->endChangeGesture();
    }
    explicit Pattern(RastroFxProcessor &processor) : p(processor)
    {
        setComponentID("fxPattern");
        setName("SWING editable eight-point pattern");
    }
    juce::Point<float> point(int i) const
    {
        return {14.f + (getWidth() - 28.f) * i / 7,
                24.f + (getHeight() - 36.f) * (1 - p.value("fxPan" + juce::String(i)))};
    }
    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colour(0xfff5efe2));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);
        label(g, "SWING PATTERN / drag the eight points", {10, 0, getWidth() - 20, 22}, 12);
        g.setColour(ink());
        for (int i = 0; i < 8; ++i)
        {
            auto pt = point(i);
            if (i)
                g.drawLine({point(i - 1), pt}, 2);
            g.setColour(hue(2));
            g.fillEllipse(pt.x - 5, pt.y - 5, 10, 10);
            g.setColour(ink());
            g.drawEllipse(pt.x - 5, pt.y - 5, 10, 10, 1.5f);
        }
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        dragged = juce::jlimit(0, 7, int(std::round((e.x - 14.f) / (getWidth() - 28) * 7)));
        p.params.getParameter("fxPan" + juce::String(dragged))->beginChangeGesture();
        mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (dragged < 0)
            return;
        auto *v = p.params.getParameter("fxPan" + juce::String(dragged));
        v->setValueNotifyingHost(juce::jlimit(0.f, 1.f, 1 - (e.y - 24.f) / (getHeight() - 36)));
        repaint();
    }
    void mouseUp(const juce::MouseEvent &) override
    {
        if (dragged >= 0)
            p.params.getParameter("fxPan" + juce::String(dragged))->endChangeGesture();
        dragged = -1;
    }
};
struct RastroFxEditor::Row : juce::Component
{
    RastroFxProcessor &p;
    int id;
    juce::ToggleButton enabled{"ON"};
    juce::TextButton up{"UP"}, down{"DOWN"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::unique_ptr<Pattern> pattern;
    int dragStart = 0;
    Row(RastroFxProcessor &processor, int index) : p(processor), id(index)
    {
        setComponentID("fxRow" + juce::String(id));
        setName(modules[id].name);
        addAndMakeVisible(enabled);
        addAndMakeVisible(up);
        addAndMakeVisible(down);
        enabled.setComponentID(RastroFxProcessor::moduleId(id, "On"));
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            p.params, RastroFxProcessor::moduleId(id, "On"), enabled);
        up.onClick = [this] { p.moveModule(id, -1); };
        down.onClick = [this] { p.moveModule(id, 1); };
        for (int k = 0; k < modules[id].controls; ++k)
        {
            auto control =
                std::make_unique<Knob>(p, RastroFxProcessor::moduleId(id, "P" + juce::String(k)),
                                       modules[id].knob[k].name);
            addAndMakeVisible(*control);
            knobs.push_back(std::move(control));
        }
        auto mix = std::make_unique<Knob>(p, RastroFxProcessor::moduleId(id, "Mix"), "Mix");
        addAndMakeVisible(*mix);
        knobs.push_back(std::move(mix));
        if (id == 14)
        {
            pattern = std::make_unique<Pattern>(p);
            addAndMakeVisible(*pattern);
        }
    }
    int rowHeight() const { return pattern ? 280 : 168; }
    void paint(juce::Graphics &g) override
    {
        auto box = getLocalBounds().reduced(3).toFloat();
        g.setColour(ink());
        g.fillRoundedRectangle(box.translated(2, 3), 14);
        g.setColour(
            hue((id + p.appearance("accent")) % 5).interpolatedWith(juce::Colours::white, .58f));
        g.fillRoundedRectangle(box, 14);
        g.setColour(ink());
        g.drawRoundedRectangle(box, 14, 2);
        label(g, modules[id].name, {16, 7, 125, 25}, 20);
        label(g, modules[id].description, {144, 8, std::max(35, getWidth() - 338), 25}, 11);
        // Original FX mascot: a small orbiting pedal face, separate from the PR #5 artwork.
        g.setColour(ink());
        g.fillEllipse(17, 37, 4, 6);
        g.fillEllipse(28, 37, 4, 6);
        g.drawLine(19, 48, 30, 48, 2);
    }
    void resized() override
    {
        enabled.setBounds(getWidth() - 177, 9, 62, 27);
        up.setBounds(getWidth() - 113, 10, 48, 25);
        down.setBounds(getWidth() - 61, 10, 52, 25);
        int width = (getWidth() - 24) / int(knobs.size());
        for (size_t i = 0; i < knobs.size(); ++i)
            knobs[i]->setBounds(12 + int(i) * width, 40, width, 118);
        if (pattern)
            pattern->setBounds(14, 164, getWidth() - 28, 102);
    }
    void mouseDown(const juce::MouseEvent &e) override { dragStart = e.y; }
    void mouseUp(const juce::MouseEvent &e) override
    {
        if (dragStart < 37 && std::abs(e.y - dragStart) > 50)
            p.moveModule(id, (e.y - dragStart) / rowHeight());
    }
};
struct RastroFxEditor::Pad : juce::Component
{
    RastroFxProcessor &p;
    bool dragging = false;
    ~Pad() override
    {
        if (dragging)
            for (auto id : {"fxX", "fxY"})
                p.params.getParameter(id)->endChangeGesture();
    }
    explicit Pad(RastroFxProcessor &processor) : p(processor)
    {
        setComponentID("fxPad");
        setName("Assignable XY pad");
    }
    void paint(juce::Graphics &g) override
    {
        auto area = getLocalBounds().reduced(8).toFloat();
        g.setColour(ink());
        g.fillRoundedRectangle(area, 18);
        g.setColour(hue(p.appearance("accent")));
        g.drawRoundedRectangle(area.reduced(2), 18, 3);
        g.setColour(juce::Colours::white.withAlpha(.25f));
        g.drawLine(area.getCentreX(), area.getY(), area.getCentreX(), area.getBottom());
        g.drawLine(area.getX(), area.getCentreY(), area.getRight(), area.getCentreY());
        float x = area.getX() + p.value("fxX") * area.getWidth(),
              y = area.getBottom() - p.value("fxY") * area.getHeight();
        g.setColour(hue(p.appearance("accent")));
        g.fillEllipse(x - 10, y - 10, 20, 20);
        g.setColour(ink());
        g.fillEllipse(x - 4, y - 3, 2, 3);
        g.fillEllipse(x + 3, y - 3, 2, 3);
        g.drawLine(x - 3, y + 4, x + 4, y + 4, 1.5f);
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if (!dragging)
            for (auto id : {"fxX", "fxY"})
                p.params.getParameter(id)->beginChangeGesture();
        dragging = true;
        mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        auto area = getLocalBounds().reduced(8);
        p.params.getParameter("fxX")->setValueNotifyingHost(
            juce::jlimit(0.f, 1.f, (e.x - area.getX()) / float(area.getWidth())));
        p.params.getParameter("fxY")->setValueNotifyingHost(
            juce::jlimit(0.f, 1.f, 1 - (e.y - area.getY()) / float(area.getHeight())));
        repaint();
    }
    void mouseUp(const juce::MouseEvent &) override
    {
        if (dragging)
            for (auto id : {"fxX", "fxY"})
                p.params.getParameter(id)->endChangeGesture();
        dragging = false;
    }
    void mouseDoubleClick(const juce::MouseEvent &) override
    {
        for (auto id : {"fxX", "fxY"})
            if (dragging)
                p.params.getParameter(id)->setValueNotifyingHost(.5f);
            else
                p.setValue(id, .5f);
        repaint();
    }
};
struct RastroFxEditor::ModPanel : juce::Component
{
    RastroFxProcessor &p;
    Pad pad;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::array<juce::ComboBox, 5> combos;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 5>
        attachments;
    std::array<juce::String, 5> comboNames{{"X destination", "Y destination", "LFO destination",
                                            "LFO waveform", "Envelope destination"}};
    explicit ModPanel(RastroFxProcessor &processor) : p(processor), pad(p)
    {
        addAndMakeVisible(pad);
        const char *ids[]{"fxXTarget", "fxYTarget", "fxLfoTarget", "fxLfoShape", "fxEnvTarget"};
        for (int i = 0; i < 5; ++i)
        {
            auto *parameter =
                dynamic_cast<juce::AudioParameterChoice *>(p.params.getParameter(ids[i]));
            combos[i].addItemList(parameter->choices, 1);
            combos[i].setName(comboNames[i]);
            combos[i].setComponentID(ids[i]);
            addAndMakeVisible(combos[i]);
            attachments[i] =
                std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    p.params, ids[i], combos[i]);
        }
        const char *controlIds[]{"fxXDepth",    "fxYDepth",     "fxLfoRate",        "fxLfoDepth",
                                 "fxEnvAttack", "fxEnvRelease", "fxEnvSensitivity", "fxEnvDepth"};
        const char *names[]{"X amount",    "Y amount",     "LFO rate / Hz",    "LFO amount",
                            "Attack / ms", "Release / ms", "Sensitivity / dB", "Env amount"};
        for (int i = 0; i < 8; ++i)
        {
            auto knob = std::make_unique<Knob>(p, controlIds[i], names[i]);
            addAndMakeVisible(*knob);
            knobs.push_back(std::move(knob));
        }
    }
    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colour(0xfffffbef));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 16);
        label(g, "MOD / touch an orbit", {14, 5, getWidth() - 28, 30}, 19);
        label(g, "LFO / motion generator", {14, 450, getWidth() - 28, 30}, 17);
        label(g, "ENVELOPE / follow the input", {14, 745, getWidth() - 28, 30}, 17);
        for (int i = 0; i < 5; ++i)
            label(g, comboNames[i], {16, comboY(i) - 20, getWidth() - 32, 18}, 11);
        auto drawSignal = [&](float v, int y, juce::Colour colour)
        {
            g.setColour(ink());
            g.fillRoundedRectangle(16.f, float(y), getWidth() - 32.f, 10, 5);
            g.setColour(colour);
            g.fillRoundedRectangle(16.f, float(y), (getWidth() - 32.f) * juce::jlimit(0.f, 1.f, v),
                                   10, 5);
        };
        drawSignal((p.lfoMeter.load() + 1) * .5f, 710, hue(2));
        drawSignal(p.envelopeMeter.load(), 1075, hue(1));
        label(g,
              "Amounts are bipolar. None disables a route.\nX/Y add around centre; LFO is bipolar; "
              "envelope is unipolar.",
              {16, 1100, getWidth() - 32, 58}, 11);
    }
    int comboY(int i) const
    {
        const int y[]{245, 295, 505, 560, 800};
        return y[i];
    }
    void resized() override
    {
        pad.setBounds(14, 40, getWidth() - 28, 174);
        for (int i = 0; i < 5; ++i)
            combos[i].setBounds(16, comboY(i), getWidth() - 32, 28);
        int w = (getWidth() - 32) / 2;
        const int ys[]{330, 330, 600, 600, 845, 845, 960, 960};
        for (int i = 0; i < 8; ++i)
            knobs[i]->setBounds(16 + (i % 2) * w, ys[i], w, 103);
    }
};
struct RastroFxEditor::Meter : juce::Component
{
    RastroFxProcessor &p;
    explicit Meter(RastroFxProcessor &processor) : p(processor) {}
    void paint(juce::Graphics &g) override
    {
        int w = getWidth() / 2;
        for (int c = 0; c < 2; ++c)
        {
            float peak = c ? p.outputPeak.load() : p.inputPeak.load();
            float db = juce::Decibels::gainToDecibels(std::max(peak, 1e-6f), -60.f);
            label(g, c ? "OUT" : "IN", {c * w, 0, w, 18}, 11, juce::Justification::centred);
            auto rect = juce::Rectangle<float>(float(c * w + 7), 22, float(w - 14), 25);
            g.setColour(ink());
            g.fillRoundedRectangle(rect, 4);
            g.setColour(peak > 1 ? hue(1) : hue(0));
            g.fillRoundedRectangle(
                rect.withWidth(rect.getWidth() * juce::jlimit(0.f, 1.f, (db + 60) / 60)), 4);
            label(g, juce::String(db, 1) + " dBFS", {c * w, 51, w, 18}, 10,
                  juce::Justification::centred);
        }
    }
};
RastroFxEditor::RastroFxEditor(RastroFxProcessor &processor)
    : AudioProcessorEditor(processor), p(processor), style(std::make_unique<Style>())
{
    setLookAndFeel(style.get());
    logo = juce::ImageCache::getFromMemory(RastroFxAssets::logo_png, RastroFxAssets::logo_pngSize);
    // Use only the family wordmark; the PLAY suffix belongs to the instrument.
    if (logo.isValid())
        logo = logo.getClippedImage({0, 0, int(logo.getWidth() * .7), logo.getHeight()});
    rackView.setComponentID("fxRackViewport");
    modView.setComponentID("fxModViewport");
    addAndMakeVisible(rackView);
    rackView.setViewedComponent(&rackBody, false);
    rackView.setScrollBarsShown(true, false);
    modulation = std::make_unique<ModPanel>(p);
    addAndMakeVisible(modView);
    modView.setViewedComponent(modulation.get(), false);
    modView.setScrollBarsShown(true, false);
    for (int i = 0; i < count; ++i)
    {
        rows[i] = std::make_unique<Row>(p, i);
        rackBody.addAndMakeVisible(*rows[i]);
    }
    input = std::make_unique<Knob>(p, "fxInputGain", "INPUT / dB", false);
    output = std::make_unique<Knob>(p, "fxOutputGain", "OUTPUT / dB", false);
    dryWet = std::make_unique<Knob>(p, "fxDryWet", "DRY / WET", false);
    meters = std::make_unique<Meter>(p);
    for (auto *c : std::array<juce::Component *, 15>{
             {input.get(), output.get(), dryWet.get(), meters.get(), &bypass, &route, &factory,
              &accent, &skin, &presetName, &compare, &copy, &open, &save, &status}})
        addAndMakeVisible(c);
    route.addItemList({"SERIES", "PARALLEL"}, 1);
    route.setComponentID("fxRouting");
    routeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.params, "fxRouting", route);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.params, "fxBypass", bypass);
    for (int i = 0; i < p.getNumPrograms(); ++i)
        factory.addItem(p.getProgramName(i), i + 1);
    factory.setTextWhenNothingSelected("FACTORY PRESETS");
    factory.onChange = [this]
    {
        p.loadFactory(factory.getSelectedId() - 1);
        refresh();
    };
    presetName.setComponentID("fxPresetName");
    presetName.setText(p.presetName());
    presetName.setTooltip("Preset name (saved with the sound and appearance)");
    presetName.onTextChange = [this] { p.setPresetName(presetName.getText()); };
    compare.setComponentID("fxCompare");
    copy.setComponentID("fxCopyAB");
    compare.onClick = [this]
    {
        p.compare();
        refresh();
    };
    copy.onClick = [this]
    {
        p.copyCompare();
        refresh();
    };
    open.onClick = [this] { choosePreset(false); };
    save.onClick = [this] { choosePreset(true); };
    accent.addItemList({"Mint", "Coral", "Lilac", "Honey", "Sky"}, 1);
    skin.addItemList({"Paper", "Night", "Bubble"}, 1);
    accent.setTooltip("Preset accent colour");
    skin.setTooltip("Preset surface style");
    accent.onChange = [this]
    {
        p.setAppearance("accent", accent.getSelectedId() - 1);
        repaint();
    };
    skin.onChange = [this]
    {
        p.setAppearance("skin", skin.getSelectedId() - 1);
        repaint();
    };
    status.setText("PRESETS / name + appearance travel with the sound", juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, ink());
    status.setFont(juce::FontOptions(11));
    setResizable(true, true);
    setResizeLimits(980, 720, 1800, 1200);
    setSize(1180, 840);
    shownOrder.fill(-1);
    refresh();
    startTimerHz(30);
}
RastroFxEditor::~RastroFxEditor()
{
    stopTimer();
    rackView.setViewedComponent(nullptr, false);
    modView.setViewedComponent(nullptr, false);
    setLookAndFeel(nullptr);
}
void RastroFxEditor::paint(juce::Graphics &g)
{
    int appearance = p.appearance("skin");
    g.fillAll(appearance == 1 ? juce::Colour(0xff3c405c)
              : appearance == 2
                  ? hue(p.appearance("accent")).interpolatedWith(juce::Colours::white, .65f)
                  : juce::Colour(0xffefe5d2));
    g.setColour(hue(p.appearance("accent")));
    g.fillRoundedRectangle(10, 8, float(getWidth() - 20), 62, 18);
    if (logo.isValid())
        g.drawImageWithin(logo, 22, 17, 150, 42, juce::RectanglePlacement::centred);
    label(g, "FX / ORBITAL PLAYGROUND", {186, 10, 390, 31}, 22);
    label(g, "15 real modules / external audio / zero latency", {187, 39, 380, 20}, 12);
    g.setColour(juce::Colour(0xfffffbef));
    g.fillRoundedRectangle(10, 78, float(getWidth() - 20), 90, 14);
    label(g, "MIX", {22, 80, 60, 18}, 12);
    g.setColour(appearance == 1 ? juce::Colours::white : ink());
    g.setFont(juce::FontOptions(13, juce::Font::bold));
    g.drawText("PRESETS", juce::Rectangle<int>{16, 174, 100, 20}, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(16, juce::Font::bold));
    g.drawText("RACK / drag a header or use UP + DOWN",
               juce::Rectangle<int>{18, 242, getWidth() - 400, 26},
               juce::Justification::centredLeft);
    g.setColour(ink());
    g.drawLine(getWidth() - 365.f, 270, getWidth() - 365.f, float(getHeight() - 14), 2);
}
void RastroFxEditor::resized()
{
    compare.setBounds(getWidth() - 194, 23, 82, 31);
    copy.setBounds(getWidth() - 104, 23, 88, 31);
    int content = getWidth() - 40;
    int meterWidth = 164, routeWidth = 132, bypassWidth = 90;
    int knobWidth = (content - meterWidth - routeWidth - bypassWidth - 24) / 3;
    input->setBounds(24, 95, knobWidth, 67);
    output->setBounds(24 + knobWidth, 95, knobWidth, 67);
    dryWet->setBounds(24 + 2 * knobWidth, 95, knobWidth, 67);
    int x = 24 + 3 * knobWidth;
    route.setBounds(x, 110, routeWidth, 30);
    bypass.setBounds(x + routeWidth + 4, 110, bypassWidth, 30);
    meters->setBounds(getWidth() - meterWidth - 20, 90, meterWidth, 72);
    factory.setBounds(16, 198, 190, 30);
    presetName.setBounds(214, 198, getWidth() - 660, 30);
    open.setBounds(getWidth() - 438, 198, 65, 30);
    save.setBounds(getWidth() - 365, 198, 65, 30);
    accent.setBounds(getWidth() - 290, 198, 124, 30);
    skin.setBounds(getWidth() - 156, 198, 140, 30);
    status.setBounds(118, 171, getWidth() - 140, 24);
    int rackWidth = getWidth() - 390;
    rackView.setBounds(12, 274, rackWidth, getHeight() - 288);
    modView.setBounds(getWidth() - 354, 242, 342, getHeight() - 256);
    rackBody.setSize(rackView.getWidth() - 16, count * 180);
    int y = 0;
    auto sorted = p.order();
    for (int id : sorted)
    {
        rows[id]->setBounds(0, y, rackBody.getWidth(), rows[id]->rowHeight());
        y += rows[id]->rowHeight() + 10;
    }
    rackBody.setSize(rackView.getWidth() - 16, y);
    modulation->setSize(modView.getWidth() - 16, 1180);
}
void RastroFxEditor::refresh()
{
    if (shownOrder != p.order())
    {
        shownOrder = p.order();
        resized();
    }
    if (!presetName.hasKeyboardFocus(true))
        presetName.setText(p.presetName(), false);
    compare.setButtonText(p.compareSlot() == 0 ? "A > B" : "B > A");
    copy.setButtonText(p.compareSlot() == 0 ? "Copy to B" : "Copy to A");
    status.setColour(juce::Label::textColourId,
                     p.appearance("skin") == 1 ? juce::Colours::white : ink());
    factory.setSelectedId(
        p.presetName() == p.getProgramName(p.getCurrentProgram()) ? p.getCurrentProgram() + 1 : 0,
        juce::dontSendNotification);
    accent.setSelectedId(p.appearance("accent") + 1, juce::dontSendNotification);
    skin.setSelectedId(juce::jlimit(0, 2, p.appearance("skin")) + 1, juce::dontSendNotification);
    meters->repaint();
    modulation->repaint();
    rackBody.repaint();
    repaint();
}
bool RastroFxEditor::controlsFit() const
{
    for (const auto &row : rows)
        for (auto *child : row->getChildren())
            if (!row->getLocalBounds().contains(child->getBounds()))
                return false;
    return getLocalBounds().contains(rackView.getBounds()) &&
           getLocalBounds().contains(modView.getBounds()) &&
           getLocalBounds().contains(skin.getBounds());
}
void RastroFxEditor::choosePreset(bool write)
{
    auto directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("RASTRO FX/Presets");
    directory.createDirectory();
    chooser = std::make_unique<juce::FileChooser>(
        write ? "Save RASTRO FX preset" : "Open RASTRO FX preset", directory, "*.rastrofx");
    juce::Component::SafePointer<RastroFxEditor> safe(this);
    chooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles |
            (write ? juce::FileBrowserComponent::saveMode |
                         juce::FileBrowserComponent::warnAboutOverwriting
                   : juce::FileBrowserComponent::openMode),
        [safe, write](const juce::FileChooser &result)
        {
            if (!safe || result.getResult() == juce::File{})
                return;
            bool ok = write ? safe->p.savePreset(result.getResult().withFileExtension("rastrofx"))
                            : safe->p.loadPreset(result.getResult());
            safe->status.setText(ok ? (write ? "Preset saved" : "Preset loaded")
                                    : "Preset failed: check the selected file or folder",
                                 juce::dontSendNotification);
            safe->refresh();
        });
}
