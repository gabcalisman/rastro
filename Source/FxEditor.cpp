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
juce::Colour moduleHue(int index)
{
    const juce::uint32 colours[]{0xffb7a6ff, 0xffff8f9e, 0xffffbb7c, 0xff78c9ff, 0xffc9a7ff,
                                 0xff8fdbce, 0xffff8a70, 0xff8fb5ff, 0xff84d4ff, 0xffd9a6ff,
                                 0xff83e6e0, 0xffffa34d, 0xffff7d8a, 0xff9be2b0, 0xffffd56e};
    return juce::Colour(colours[juce::jlimit(0, count - 1, index)]);
}
juce::String moduleDescriptor(int index)
{
    const char *names[]{"ECHO SPIRIT", "BROKEN REPEATS", "DIFFUSE SPACE", "TAPE MOTION", "MOVING TEXTURE",
                        "DIGITAL DAMAGE", "VOWEL RESONATOR", "MODULATED FLANGE", "COLOURED ECHO",
                        "THREE-VOICE CHORUS", "PITCH CRYSTAL", "SATURATION", "COMPRESSION",
                        "RESONANT FILTER", "STEREO MOTION"};
    return names[juce::jlimit(0, count - 1, index)];
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
                          float end, juce::Slider &slider) override
    {
        float radius = std::min(w, h) * .34f;
        juce::Point<float> centre(x + w * .5f, y + h * .5f);
        auto bounds = juce::Rectangle<float>(radius * 2, radius * 2).withCentre(centre);
        auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (accent.isTransparent())
            accent = hue(0);
        g.setColour(ink().withAlpha(.55f));
        g.fillEllipse(bounds.translated(2, 4));
        g.setColour(juce::Colour(0xff282c49));
        g.fillEllipse(bounds);
        juce::Path track, progress;
        track.addCentredArc(centre.x, centre.y, radius + 4, radius + 4, 0, start, end, true);
        progress.addCentredArc(centre.x, centre.y, radius + 4, radius + 4, 0, start,
                               start + pos * (end - start), true);
        g.setColour(juce::Colour(0xff181b32));
        g.strokePath(track, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour(accent);
        g.strokePath(progress, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        float angle = start + pos * (end - start);
        auto tip = centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * radius * .76f;
        g.setColour(accent.brighter(.1f));
        g.drawLine(centre.x, centre.y, tip.x, tip.y, 3);
        g.setColour(juce::Colour(0xfffff9ec));
        g.fillEllipse(centre.x - 3, centre.y - 3, 6, 6);
    }
};
struct RastroFxEditor::Knob : juce::Component
{
    juce::Slider slider;
    juce::String title;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    Knob(RastroFxProcessor &p, const juce::String &id, const juce::String &name, bool rotary = true,
         juce::Colour accent = juce::Colour(0xffffd67c))
        : title(name), tint(accent)
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
        slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, accent.withAlpha(.24f));
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
        g.setColour(tint);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawFittedText(title.toUpperCase(), {0, 1, getWidth(), 23}, juce::Justification::centred, 1);
    }
    void resized() override { slider.setBounds(getLocalBounds().withTrimmedTop(26)); }
    juce::Colour tint;
};
struct RastroFxEditor::Segmented : juce::Component
{
    juce::String id, title;
    juce::StringArray options;
    juce::Colour tint;
    std::array<juce::TextButton, 5> buttons;
    int optionCount;
    RastroFxProcessor &processor;
    Segmented(RastroFxProcessor &p, juce::String parameter, juce::String caption,
              juce::StringArray choices, juce::Colour colour)
        : id(std::move(parameter)), title(std::move(caption)), options(std::move(choices)), tint(colour),
          optionCount(options.size()), processor(p)
    {
        setComponentID(id);
        setName(title);
        for (int i = 0; i < optionCount; ++i)
        {
            buttons[size_t(i)].setButtonText(options[i]);
            buttons[size_t(i)].setComponentID(id + "Option" + juce::String(i));
            buttons[size_t(i)].setTooltip(title + " / " + options[i]);
            buttons[size_t(i)].onClick = [this, i]
            {
                auto *parameter = processor.params.getParameter(id);
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost(optionCount > 1 ? float(i) / float(optionCount - 1) : 0.f);
                parameter->endChangeGesture();
                refresh();
            };
            addAndMakeVisible(buttons[size_t(i)]);
        }
        refresh();
    }
    void paint(juce::Graphics &g) override
    {
        g.setColour(tint);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawFittedText(title.toUpperCase(), {0, 1, getWidth(), 22}, juce::Justification::centred, 1);
    }
    void resized() override
    {
        auto area = getLocalBounds().withTrimmedTop(27).reduced(2, 5);
        int gap = 3, width = (area.getWidth() - gap * (optionCount - 1)) / optionCount;
        for (int i = 0; i < optionCount; ++i)
            buttons[size_t(i)].setBounds(area.getX() + i * (width + gap), area.getY(), width, area.getHeight());
    }
    void refresh()
    {
        auto *parameter = processor.params.getParameter(id);
        int selected = juce::jlimit(
            0, optionCount - 1,
            int(std::round(parameter->getValue() * float(optionCount - 1))));
        for (int i = 0; i < optionCount; ++i)
        {
            bool active = i == selected;
            buttons[size_t(i)].setColour(juce::TextButton::buttonColourId,
                                         active ? tint : juce::Colour(0xffeee8dc));
            buttons[size_t(i)].setColour(juce::TextButton::textColourOffId,
                                         active ? juce::Colour(0xff20243d) : juce::Colour(0xff5c6075));
        }
    }
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
    juce::TextButton remove{"REMOVE"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::unique_ptr<Pattern> pattern;
    int dragStart = 0;
    Row(RastroFxProcessor &processor, int index) : p(processor), id(index)
    {
        setComponentID("fxRow" + juce::String(id));
        setName(modules[id].name);
        addAndMakeVisible(enabled);
        addAndMakeVisible(up);
        addAndMakeVisible(down);
        addAndMakeVisible(remove);
        enabled.setComponentID(RastroFxProcessor::moduleId(id, "On"));
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            p.params, RastroFxProcessor::moduleId(id, "On"), enabled);
        up.onClick = [this] { p.moveModule(id, -1); };
        down.onClick = [this] { p.moveModule(id, 1); };
        remove.setComponentID("fxRemove" + juce::String(id));
        remove.setTooltip("Remove this module from the active rack");
        remove.onClick = [this] { p.setValue(RastroFxProcessor::moduleId(id, "InRack"), 0); };
        for (int k = 0; k < modules[id].controls; ++k)
        {
            auto parameter = RastroFxProcessor::moduleId(id, "P" + juce::String(k));
            std::unique_ptr<juce::Component> control;
            if (id == 6 && k == 0)
                control = std::make_unique<Segmented>(p, parameter, "VOWEL",
                                                      juce::StringArray{"A", "E", "I", "O", "U"},
                                                      moduleHue(id));
            else if (id == 11 && k == 3)
                control = std::make_unique<Segmented>(p, parameter, "SATURATION",
                                                      juce::StringArray{"DRIVE", "TUBE", "TAPE"},
                                                      moduleHue(id));
            else if (id == 13 && k == 2)
                control = std::make_unique<Segmented>(p, parameter, "FILTER",
                                                      juce::StringArray{"LP", "BP", "HP"},
                                                      moduleHue(id));
            else
                control = std::make_unique<Knob>(p, parameter, modules[id].knob[k].name, true,
                                                  moduleHue(id));
            addAndMakeVisible(*control);
            controls.push_back(std::move(control));
        }
        auto mix = std::make_unique<Knob>(p, RastroFxProcessor::moduleId(id, "Mix"), "Mix", true,
                                          moduleHue(id));
        addAndMakeVisible(*mix);
        controls.push_back(std::move(mix));
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
        auto tint = moduleHue(id);
        g.setColour(juce::Colour(0xff20243d));
        g.fillRoundedRectangle(box, 14);
        g.setColour(tint.withAlpha(.14f));
        g.fillRoundedRectangle(box.withWidth(8), 4);
        g.setColour(tint.withAlpha(.65f));
        g.drawRoundedRectangle(box, 14, 1.3f);
        g.setColour(tint);
        g.setFont(juce::FontOptions(18, juce::Font::bold));
        g.drawText(modules[id].name, 18, 6, 132, 25, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xffc9c9d8));
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(moduleDescriptor(id), 19, 31, 160, 16, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff969bb3));
        g.setFont(juce::FontOptions(10));
        g.drawFittedText(modules[id].description, {184, 7, std::max(40, getWidth() - 390), 30},
                         juce::Justification::centredLeft, 1);
        // Each effect gets its own little sound-world rather than a shared generic pedal face.
        auto art = juce::Rectangle<float>(getWidth() - 102.f, 39.f, 74.f, 45.f);
        g.setColour(tint.withAlpha(.11f));
        g.fillRoundedRectangle(art, 11.f);
        g.setColour(tint.withAlpha(.9f));
        const float cx = art.getCentreX(), cy = art.getCentreY();
        switch (id)
        {
        case 0: // echo apparition
            for (int i = 0; i < 4; ++i)
                g.drawEllipse(cx - 6.f - i * 8.f, cy - 5.f - i * 3.f, 12.f + i * 16.f,
                              10.f + i * 6.f, 1.4f);
            break;
        case 1: // fractured repeats
            for (int i = 0; i < 5; ++i)
                g.fillRect(cx - 28.f + i * 13.f, cy + float(i % 2) * -10.f, 8.f, 9.f + float(i % 3) * 5.f);
            break;
        case 2: // bloom petals
            for (int i = 0; i < 6; ++i)
            {
                g.saveState();
                g.addTransform(juce::AffineTransform::rotation(float(i) * juce::MathConstants<float>::twoPi / 6, cx, cy));
                g.drawEllipse(cx - 4.f, cy - 18.f, 8.f, 18.f, 1.5f);
                g.restoreState();
            }
            break;
        case 3: // tape wobble
        case 8: // echo wave
            {
                juce::Path wave;
                for (int i = 0; i < 30; ++i)
                {
                    float x = art.getX() + 5.f + i * (art.getWidth() - 10.f) / 29.f;
                    float y = cy + std::sin(float(i) * (id == 3 ? .6f : .3f)) * (id == 3 ? 11.f : 7.f);
                    if (i == 0) wave.startNewSubPath(x, y); else wave.lineTo(x, y);
                }
                g.strokePath(wave, juce::PathStrokeType(2.2f));
            }
            break;
        case 4: // dust grains
            for (int i = 0; i < 18; ++i)
            {
                float x = art.getX() + 6.f + float((i * 17) % 62), y = art.getY() + 5.f + float((i * 13) % 35);
                g.fillEllipse(x, y, 2.f + float(i % 3), 2.f + float(i % 3));
            }
            break;
        case 5: // pixels
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 3; ++j)
                    if ((i + j) % 2 == 0) g.fillRect(cx - 23.f + i * 12.f, cy - 13.f + j * 12.f, 8.f, 8.f);
            break;
        case 6: // vowel mouth
            g.drawEllipse(cx - 18.f, cy - 14.f, 36.f, 28.f, 2.2f);
            g.setColour(juce::Colour(0xffff91a1));
            g.fillEllipse(cx - 10.f, cy - 5.f, 20.f, 12.f);
            for (int i = 0; i < 5; ++i) g.fillEllipse(cx - 26.f + i * 13.f, cy + 18.f, 4.f, 4.f);
            break;
        case 7: // orbiting flanger
            g.drawEllipse(cx - 17.f, cy - 14.f, 34.f, 28.f, 1.8f);
            g.saveState();
            g.addTransform(juce::AffineTransform::rotation(.65f, cx, cy));
            g.drawEllipse(cx - 26.f, cy - 9.f, 52.f, 18.f, 1.3f);
            g.restoreState();
            g.fillEllipse(cx + 18.f, cy - 6.f, 8.f, 8.f);
            break;
        case 9: // choir voices
            for (int i = 0; i < 3; ++i)
            {
                float x = cx - 19.f + i * 19.f;
                g.drawEllipse(x - 4.f, cy - 13.f - (i == 1 ? 4.f : 0.f), 8.f, 8.f, 1.4f);
                g.drawLine(x, cy - 3.f, x + (i - 1) * 4.f, cy + 15.f, 2.f);
            }
            break;
        case 10: // prism
            for (int i = 0; i < 4; ++i)
            {
                juce::Path tri;
                tri.startNewSubPath(cx - 25.f + i * 15.f, cy + 13.f);
                tri.lineTo(cx - 18.f + i * 15.f, cy - 15.f + float(i % 2) * 8.f);
                tri.lineTo(cx - 11.f + i * 15.f, cy + 13.f);
                tri.closeSubPath();
                g.strokePath(tri, juce::PathStrokeType(1.5f));
            }
            break;
        case 11: // ember
            {
                juce::Path flame;
                flame.startNewSubPath(cx, cy + 16.f); flame.lineTo(cx - 16.f, cy + 8.f);
                flame.lineTo(cx - 8.f, cy - 1.f); flame.lineTo(cx - 10.f, cy - 12.f);
                flame.lineTo(cx, cy - 5.f); flame.lineTo(cx + 4.f, cy - 18.f);
                flame.lineTo(cx + 11.f, cy - 2.f); flame.lineTo(cx + 17.f, cy + 7.f);
                flame.closeSubPath(); g.fillPath(flame);
            }
            break;
        case 12: // squash envelope
            {
                juce::Path env; env.startNewSubPath(art.getX() + 5.f, cy + 12.f);
                env.cubicTo(cx - 13.f, cy - 30.f, cx + 5.f, cy - 11.f, art.getRight() - 5.f, cy + 2.f);
                g.strokePath(env, juce::PathStrokeType(2.2f));
                g.drawLine(art.getX() + 4.f, cy + 7.f, art.getRight() - 4.f, cy + 7.f, 1.f);
            }
            break;
        case 13: // filter curve
            {
                juce::Path curve; curve.startNewSubPath(art.getX() + 4.f, cy + 13.f);
                curve.cubicTo(cx - 5.f, cy + 11.f, cx - 4.f, cy - 18.f, art.getRight() - 4.f, cy - 16.f);
                g.strokePath(curve, juce::PathStrokeType(2.2f));
                g.drawLine(cx, art.getY() + 4.f, cx, art.getBottom() - 4.f, 1.f);
            }
            break;
        case 14: // panner path
            for (int i = 1; i < 8; ++i)
                g.drawLine(art.getX() + 5.f + (i - 1) * (art.getWidth() - 10.f) / 7.f,
                           art.getY() + 7.f + (1.f - p.value("fxPan" + juce::String(i - 1))) * (art.getHeight() - 14.f),
                           art.getX() + 5.f + i * (art.getWidth() - 10.f) / 7.f,
                           art.getY() + 7.f + (1.f - p.value("fxPan" + juce::String(i))) * (art.getHeight() - 14.f), 1.6f);
            for (int i = 0; i < 8; ++i)
                g.fillEllipse(art.getX() + 3.f + i * (art.getWidth() - 7.f) / 7.f - 2.f,
                              art.getY() + 5.f + (1.f - p.value("fxPan" + juce::String(i))) * (art.getHeight() - 14.f) - 2.f, 4.f, 4.f);
            break;
        }
    }
    void resized() override
    {
        enabled.setBounds(getWidth() - 236, 9, 62, 27);
        remove.setBounds(getWidth() - 168, 10, 66, 25);
        up.setBounds(getWidth() - 94, 10, 36, 25);
        down.setBounds(getWidth() - 52, 10, 43, 25);
        int width = (getWidth() - 24) / int(controls.size());
        for (size_t i = 0; i < controls.size(); ++i)
            controls[i]->setBounds(12 + int(i) * width, 46, width, 110);
        if (pattern)
            pattern->setBounds(14, 164, getWidth() - 28, 102);
    }
    void refresh()
    {
        for (auto &control : controls)
            if (auto *segmented = dynamic_cast<Segmented *>(control.get()))
                segmented->refresh();
        repaint();
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
    moduleView.setComponentID("fxModuleShelf");
    addAndMakeVisible(rackView);
    rackView.setViewedComponent(&rackBody, false);
    rackView.setScrollBarsShown(true, false);
    addAndMakeVisible(moduleView);
    moduleView.setViewedComponent(&moduleBody, false);
    moduleView.setScrollBarsShown(false, false);
    modulation = std::make_unique<ModPanel>(p);
    addAndMakeVisible(modView);
    modView.setViewedComponent(modulation.get(), false);
    modView.setScrollBarsShown(true, false);
    for (int i = 0; i < count; ++i)
    {
        rows[i] = std::make_unique<Row>(p, i);
        rackBody.addAndMakeVisible(*rows[i]);
        moduleButtons[size_t(i)].setComponentID("fxModule" + juce::String(i));
        moduleButtons[size_t(i)].setTooltip(juce::String("Add ") + modules[i].name + " to the active rack");
        moduleButtons[size_t(i)].onClick = [this, i]
        {
            auto inserted = RastroFxProcessor::moduleId(i, "InRack");
            if (p.value(inserted) >= .5f)
            {
                p.setValue(inserted, 0);
            }
            else
            {
                p.setValue(inserted, 1);
                p.setValue(RastroFxProcessor::moduleId(i, "On"), 1);
            }
            refresh();
        };
        moduleBody.addAndMakeVisible(moduleButtons[size_t(i)]);
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
    g.fillAll(appearance == 1 ? juce::Colour(0xff111326)
              : appearance == 2
                  ? hue(p.appearance("accent")).interpolatedWith(juce::Colour(0xff242842), .72f)
                  : juce::Colour(0xff171a30));
    auto panel = juce::Colour(0xfff8f3e9);
    g.setColour(juce::Colour(0xff262b48));
    g.fillRoundedRectangle(10, 8, float(getWidth() - 20), 62, 18);
    g.setColour(hue(p.appearance("accent")).withAlpha(.14f));
    g.fillEllipse(float(getWidth() - 240), -65.f, 260.f, 190.f);
    g.setColour(hue(p.appearance("accent")));
    g.fillRoundedRectangle(10, 8, 7.f, 62, 4.f);
    if (logo.isValid())
        g.drawImageWithin(logo, 22, 17, 150, 42, juce::RectanglePlacement::centred);
    g.setColour(juce::Colour(0xfff9f5ec));
    g.setFont(juce::FontOptions(22, juce::Font::bold));
    g.drawText("FX / ORBITAL PLAYGROUND", juce::Rectangle<int>{186, 10, 390, 31},
               juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffbfc4d8));
    g.setFont(juce::FontOptions(11));
    g.drawText("15 sonic characters  /  shape your signal",
               juce::Rectangle<int>{187, 39, 380, 20}, juce::Justification::centredLeft);
    g.setColour(panel);
    g.fillRoundedRectangle(10, 78, float(getWidth() - 20), 90, 14);
    label(g, "MIX", {22, 80, 60, 18}, 12);
    g.setColour(juce::Colour(0xfff9f5ec));
    g.setFont(juce::FontOptions(13, juce::Font::bold));
    g.drawText("PRESETS", juce::Rectangle<int>{16, 174, 100, 20}, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xfff9f5ec));
    g.setFont(juce::FontOptions(16, juce::Font::bold));
    g.drawText("MODULE BANK  /  add a sound to the rack",
               juce::Rectangle<int>{18, 242, getWidth() - 400, 24},
               juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffbfc4d8));
    g.setFont(juce::FontOptions(14, juce::Font::bold));
    g.drawText("ACTIVE EFFECT CHAIN  /  drag cards to reorder",
               juce::Rectangle<int>{18, 340, getWidth() - 400, 24},
               juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff4b516d));
    g.drawLine(getWidth() - 365.f, 270, getWidth() - 365.f, float(getHeight() - 14), 1.2f);
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
    moduleView.setBounds(12, 270, rackWidth, 64);
    moduleBody.setSize(std::min(rackWidth - 16, 760), 62);
    int moduleGap = 5, moduleButtonWidth = (moduleBody.getWidth() - 4 * moduleGap) / 5;
    for (int i = 0; i < count; ++i)
    {
        int row = i / 5, column = i % 5;
        moduleButtons[size_t(i)].setBounds(column * (moduleButtonWidth + moduleGap), row * 20,
                                           moduleButtonWidth, 18);
    }
    rackView.setBounds(12, 368, rackWidth, getHeight() - 382);
    modView.setBounds(getWidth() - 354, 242, 342, getHeight() - 256);
    rackBody.setSize(rackView.getWidth() - 16, count * 180);
    int y = 0;
    auto sorted = p.order();
    for (int id : sorted)
    {
        bool inserted = p.value(RastroFxProcessor::moduleId(id, "InRack")) >= .5f;
        rows[size_t(id)]->setVisible(inserted);
        if (!inserted)
            continue;
        rows[id]->setBounds(0, y, rackBody.getWidth(), rows[id]->rowHeight());
        y += rows[id]->rowHeight() + 10;
    }
    rackBody.setSize(rackView.getWidth() - 16, std::max(y, 1));
    modulation->setSize(modView.getWidth() - 16, 1180);
}
void RastroFxEditor::refresh()
{
    if (shownOrder != p.order())
    {
        shownOrder = p.order();
        resized();
    }
    bool insertChanged = false;
    for (int i = 0; i < count; ++i)
    {
        bool inserted = p.value(RastroFxProcessor::moduleId(i, "InRack")) >= .5f;
        insertChanged |= shownInserted[size_t(i)] != inserted;
        shownInserted[size_t(i)] = inserted;
        moduleButtons[size_t(i)].setButtonText(juce::String(inserted ? "✓ " : "+ ") + modules[i].name);
        moduleButtons[size_t(i)].setColour(juce::TextButton::buttonColourId,
                                           inserted ? moduleHue(i) : juce::Colour(0xff292d49));
        moduleButtons[size_t(i)].setColour(juce::TextButton::textColourOffId,
                                           inserted ? juce::Colour(0xff20243d) : juce::Colour(0xfff5f1e8));
    }
    if (insertChanged)
        resized();
    if (!presetName.hasKeyboardFocus(true))
        presetName.setText(p.presetName(), false);
    compare.setButtonText(p.compareSlot() == 0 ? "A > B" : "B > A");
    copy.setButtonText(p.compareSlot() == 0 ? "Copy to B" : "Copy to A");
    status.setColour(juce::Label::textColourId, juce::Colour(0xffc8cce0));
    factory.setSelectedId(
        p.presetName() == p.getProgramName(p.getCurrentProgram()) ? p.getCurrentProgram() + 1 : 0,
        juce::dontSendNotification);
    accent.setSelectedId(p.appearance("accent") + 1, juce::dontSendNotification);
    skin.setSelectedId(juce::jlimit(0, 2, p.appearance("skin")) + 1, juce::dontSendNotification);
    meters->repaint();
    modulation->repaint();
    for (auto &row : rows)
        row->refresh();
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
           getLocalBounds().contains(moduleView.getBounds()) &&
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
