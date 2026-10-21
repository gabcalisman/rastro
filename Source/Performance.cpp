#include "Performance.h"
#include "Visuals.h"
namespace
{
const juce::Colour ink(0xff261e19), cream(0xfff2e3c5), orange(0xffdf682f), lime(0xffced968),
    aqua(0xff68b8ab);
}
PlayPad::PlayPad(RastroProcessor &processor) : p(processor)
{
    setWantsKeyboardFocus(true);
    setTitle("PLAY");
}
PlayPad::~PlayPad() { finishGesture(); }
void PlayPad::finishGesture()
{
    if (dragging)
    {
        for (auto id : {"playX", "playY"})
            p.params.getParameter(id)->endChangeGesture();
        dragging = false;
    }
}
void PlayPad::position(juce::Point<float> point)
{
    auto r = getLocalBounds().toFloat().reduced(26, 30);
    float x = juce::jlimit(0.f, 1.f, (point.x - r.getX()) / std::max(1.f, r.getWidth()));
    float y = juce::jlimit(0.f, 1.f, 1 - (point.y - r.getY()) / std::max(1.f, r.getHeight()));
    p.params.getParameter("playX")->setValueNotifyingHost(x);
    p.params.getParameter("playY")->setValueNotifyingHost(y);
    repaint();
}
void PlayPad::mouseDown(const juce::MouseEvent &e)
{
    grabKeyboardFocus();
    if (e.getNumberOfClicks() > 1)
        return;
    dragging = true;
    for (auto id : {"playX", "playY"})
        p.params.getParameter(id)->beginChangeGesture();
    position(e.position);
}
void PlayPad::mouseDrag(const juce::MouseEvent &e)
{
    if (dragging)
        position(e.position);
}
void PlayPad::mouseUp(const juce::MouseEvent &) { finishGesture(); }
void PlayPad::mouseDoubleClick(const juce::MouseEvent &)
{
    finishGesture();
    center();
}
void PlayPad::center()
{
    p.set("playX", .5f);
    p.set("playY", .5f);
    repaint();
}
bool PlayPad::keyPressed(const juce::KeyPress &k)
{
    if (k == juce::KeyPress::homeKey)
    {
        center();
        return true;
    }
    float dx = 0, dy = 0;
    if (k == juce::KeyPress::leftKey)
        dx = -.02f;
    else if (k == juce::KeyPress::rightKey)
        dx = .02f;
    else if (k == juce::KeyPress::upKey)
        dy = .02f;
    else if (k == juce::KeyPress::downKey)
        dy = -.02f;
    else
        return false;
    p.set("playX", juce::jlimit(0.f, 1.f, p.value("playX") + dx));
    p.set("playY", juce::jlimit(0.f, 1.f, p.value("playY") + dy));
    repaint();
    return true;
}
void PlayPad::paint(juce::Graphics &g)
{
    auto box = getLocalBounds().toFloat();
    g.setColour(ink);
    g.fillRoundedRectangle(box, 12);
    g.setColour(cream.withAlpha(.5f));
    g.drawRoundedRectangle(box.reduced(1), 12, 1);
    auto r = box.reduced(26, 30);
    g.setColour(cream.withAlpha(.16f));
    for (int i = 0; i <= 4; ++i)
    {
        float t = i / 4.f;
        g.drawVerticalLine(juce::roundToInt(r.getX() + r.getWidth() * t), r.getY(), r.getBottom());
        g.drawHorizontalLine(juce::roundToInt(r.getY() + r.getHeight() * t), r.getX(),
                             r.getRight());
    }
    float x = p.value("playX"), y = p.value("playY");
    auto c = juce::Point<float>(r.getX() + x * r.getWidth(), r.getBottom() - y * r.getHeight());
    g.setColour(aqua.withAlpha(.22f));
    g.fillRect(juce::Rectangle<float>(r.getX(), c.y, c.x - r.getX(), r.getBottom() - c.y));
    g.setColour(orange);
    g.drawLine(c.x, r.getY(), c.x, r.getBottom(), 1.5f);
    g.drawLine(r.getX(), c.y, r.getRight(), c.y, 1.5f);
    g.setColour(lime);
    g.fillEllipse(c.x - 12, c.y - 12, 24, 24);
    g.setColour(ink);
    g.drawEllipse(c.x - 6, c.y - 6, 12, 12, 2);
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(18, juce::Font::bold)));
    g.drawText("PLAY", 14, 5, 90, 24, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText("X " + juce::String(juce::roundToInt(x * 100)) + "%   Y " +
                   juce::String(juce::roundToInt(y * 100)) + "%",
               100, 6, getWidth() - 116, 22, juce::Justification::right);
    g.drawText(text::get("playAxes", p.language), 12, getHeight() - 25, getWidth() - 24, 21,
               juce::Justification::centred);
    if (hasKeyboardFocus(true))
    {
        g.setColour(lime);
        g.drawRoundedRectangle(box.reduced(2), 12, 2);
    }
}
void RastroAvatar::update(int animationMode)
{
    mode = animationMode;
    const unsigned serial = p.engine.noteSerial.load(std::memory_order_relaxed);
    if (serial != lastNote)
        note = p.engine.noteVelocity.load(std::memory_order_relaxed);
    lastNote = serial;
    if (mode == 0)
    {
        level = wobble = dust = crush = tail = note = 0;
        x = y = .5f;
        repaint();
        return;
    }
    float blend = mode == 1 ? .2f : .4f;
    auto follow = [blend](float &a, float target) { a += (target - a) * blend; };
    follow(level, p.engine.meter.load());
    follow(wobble, p.engine.visualWobble.load());
    follow(dust, std::min(1.f, p.engine.visualDust.load() * 100));
    follow(crush, std::min(1.f, p.engine.visualCrush.load() * 12));
    follow(tail, std::min(1.f, p.engine.visualBloom.load() * 8));
    follow(x, p.engine.visualX.load());
    follow(y, p.engine.visualY.load());
    note *= .85f;
    repaint();
}
void RastroAvatar::paint(juce::Graphics &g)
{
    auto box = getLocalBounds().toFloat();
    g.setColour(aqua);
    g.fillRoundedRectangle(box, 12);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
    g.drawText("RASTRO / " + text::get("alive", p.language), 12, 7, getWidth() - 24, 23,
               juce::Justification::left);
    const float strength = mode == 1 ? .25f : 1.f;
    const float breath = std::min(1.f, level * 4), impact = note * strength;
    float cx = getWidth() * .5f + (x - .5f) * 22 * strength,
          cy = getHeight() * .53f - (y - .5f) * 12 * strength - impact * 12;
    float radius = std::min(getWidth() * .29f, getHeight() * .29f) *
                   (1 + impact * .08f + breath * .08f * strength);
    auto transform = juce::AffineTransform::rotation(wobble * .22f * strength, cx, cy);
    juce::Graphics::ScopedSaveState state(g);
    g.addTransform(transform);
    // The tail length is measured from BLOOM, not generated by an unrelated clock.
    for (int i = 3; i > 0; --i)
    {
        g.setColour(cream.withAlpha(tail * .16f * strength));
        g.fillRoundedRectangle(cx - radius + i * 10, cy - radius * .85f + i * 4, radius * 2,
                               radius * 1.7f, 18);
    }
    // Limbs follow the measured note impulse and envelope; no autonomous dance clock.
    g.setColour(ink);
    for (float side : {-1.f, 1.f})
    {
        g.drawLine(cx + side * radius * .7f, cy + radius * .6f,
                   cx + side * radius * (.85f + impact * .15f), cy + radius * 1.12f, 7);
        g.drawLine(cx + side * radius * .86f, cy, cx + side * radius * 1.25f,
                   cy - impact * radius * .55f + radius * .15f, 7);
    }
    juce::Path body;
    float angular = crush * strength;
    const int points = 12;
    for (int i = 0; i < points; ++i)
    {
        float a = float(i) * 6.2831853f / points;
        float rr = radius * (1 + angular * ((i % 2) ? -.13f : .13f));
        auto v = juce::Point<float>(cx + std::cos(a) * rr, cy + std::sin(a) * rr * .83f);
        if (i == 0)
            body.startNewSubPath(v);
        else
            body.lineTo(v);
    }
    body.closeSubPath();
    g.setColour(ink);
    int bodyStyle = (int)p.value("avatarBody");
    if (bodyStyle == 0)
        g.fillPath(body);
    else
        g.fillRoundedRectangle(cx - radius, cy - radius * .83f, radius * 2, radius * 1.66f,
                               bodyStyle == 1 ? radius : 5.f);
    g.setColour(visuals::accent((int)p.value("visualPalette")));
    g.fillRoundedRectangle(cx - radius * .74f, cy - radius * .5f, radius * 1.48f, radius * .91f,
                           12 - angular * 8);
    float eye = radius * .17f;
    g.setColour(cream);
    int eyes = (int)p.value("avatarEyes");
    if (eyes == 1)
        g.fillRoundedRectangle(cx - radius * .55f, cy - radius * .32f, radius * 1.1f,
                               eye * (1 + impact), 3);
    for (float side : {-1.f, 1.f})
    {
        float ex = cx + side * radius * .32f + (x - .5f) * 4;
        if (eyes != 1)
            g.fillRoundedRectangle(ex - eye * .5f, cy - radius * .2f - eye * .5f, eye,
                                   eye * (1 + impact * .6f),
                                   eyes == 2 || angular > .3f ? 1.f : eye * .5f);
    }
    g.setColour(ink);
    g.drawLine(cx - radius * .2f, cy + radius * .18f, cx + radius * .2f, cy + radius * .18f,
               2 + crush * 3);
    int accessory = (int)p.value("avatarAccessory");
    if (accessory == 0)
    {
        g.setColour(lime);
        g.fillEllipse(cx - radius * .13f, cy - radius * 1.15f, radius * .26f, radius * .26f);
        g.setColour(ink);
        g.drawLine(cx, cy - radius * .8f, cx, cy - radius * .99f, 3);
    }
    else if (accessory == 1)
    {
        g.setColour(lime);
        g.drawRoundedRectangle(cx - radius * .94f, cy - radius * .76f, radius * 1.88f,
                               radius * 1.1f, radius * .5f, 7);
        for (float side : {-1.f, 1.f})
            g.fillRoundedRectangle(cx + side * radius * .95f - 7, cy - radius * .2f, 14,
                                   radius * .65f, 5);
    }
    else
    {
        g.setColour(lime);
        g.fillRoundedRectangle(cx - radius * .68f, cy + radius * .46f, radius * 1.36f, radius * .2f,
                               4);
        g.fillRect(cx + radius * .35f, cy + radius * .6f, radius * .22f, radius * .4f);
    }
    g.setColour(cream.withAlpha(std::min(.9f, dust * strength)));
    for (int i = 0; i < 18; ++i)
    {
        float a = i * 2.39996f + wobble * .5f;
        float rr = radius * (1.18f + (i % 3) * .16f);
        g.fillRect(cx + std::cos(a) * rr, cy + std::sin(a) * rr * .8f, 2.f + (i % 2), 2.f);
    }
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(11)));
    g.drawText(text::get(mode == 0   ? "still"
                         : mode == 1 ? "reduced"
                                     : "reactive",
                         p.language),
               10, getHeight() - 27, getWidth() - 20, 20, juce::Justification::centred);
}
