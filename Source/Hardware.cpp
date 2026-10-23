#include "Hardware.h"
#include "Visuals.h"
bool SampleDeck::update(int animationMode)
{
    float oldAngle = angle, oldLevel = level;
    bool oldPlaying = playing;
    int oldMode = mode;
    mode = animationMode;
    float dt = mode == 0 ? 1.f / 6 : mode == 1 ? 1.f / 12 : 1.f / 24;
    auto block = p.engine.blockSerial.load(std::memory_order_relaxed);
    idleTime = block == lastBlock ? std::min(1.f, idleTime + dt) : 0;
    lastBlock = block;
    if (seenStop != p.stopSerial.load())
    {
        seenStop = p.stopSerial.load();
        angle = level = 0;
        playing = false;
        repaint();
        return true;
    }
    if (p.playbackTransport == 1)
    {
        playing = false;
        if (oldPlaying || mode != oldMode)
            repaint();
        return oldPlaying || mode != oldMode;
    }
    bool fresh = idleTime < .3f && p.playbackTransport == 0;
    playing = fresh && p.engine.playbackActive.load(std::memory_order_relaxed);
    float peak = fresh ? p.engine.meter.load(std::memory_order_relaxed) : 0;
    if (!std::isfinite(peak))
        peak = 0;
    peak = juce::jlimit(0.f, 1.f, peak);
    level += (peak - level) * (1 - std::exp(-dt / (peak > level ? .06f : .22f)));
    if (level < .0001f)
        level = 0;
    if (playing && mode > 0)
        angle = std::fmod(angle + dt * juce::MathConstants<float>::twoPi * .17f *
                                      (mode == 1 ? .45f : 1.f),
                          juce::MathConstants<float>::twoPi);
    auto displayDb = [](float v)
    { return v <= 0 ? -1000 : juce::roundToInt(juce::Decibels::gainToDecibels(v, -60.f)); };
    bool changed = displayDb(level) != displayDb(oldLevel) || angle != oldAngle ||
                   playing != oldPlaying || mode != oldMode ||
                   juce::roundToInt(level * 500) != juce::roundToInt(oldLevel * 500);
    if (changed)
        repaint();
    return changed;
}
void SampleDeck::paint(juce::Graphics &g)
{
    auto pal = visuals::palette(p);
    bool cosmic = p.value("cosmicArcade") > .5f;
    auto box = getLocalBounds().toFloat().reduced(1);
    auto glass = juce::Colour(cosmic ? 0xff1c1034 : 0xff171a24);
    auto silver = juce::Colour(0xffe5ddca), steel = juce::Colour(0xff7c8195);
    g.setColour(pal.outline);
    g.fillRoundedRectangle(box, 8);
    g.setColour(pal.bg);
    g.fillRoundedRectangle(box.reduced(1), 7);
    auto window = box.reduced(5).withHeight(box.getHeight() * .62f);
    g.setGradientFill(juce::ColourGradient(glass.brighter(.15f), 0, window.getY(), glass, 0,
                                           window.getBottom(), false));
    g.fillRoundedRectangle(window, 6);
    if (cosmic)
    {
        visuals::cosmicTexture(g, window, pal.highlight);
        g.setColour(pal.highlight.withAlpha(.15f));
        g.drawRoundedRectangle(window.reduced(2), 5, 5);
        g.setColour(pal.highlight);
        g.drawRoundedRectangle(window.reduced(2), 5, 1);
    }
    float radius = std::min(getWidth() * .18f, window.getHeight() * .37f);
    float cy = window.getY() + window.getHeight() * .56f;
    float position = juce::jlimit(0.f, 1.f, p.engine.position.load(std::memory_order_relaxed));
    for (int reel = 0; reel < 2; ++reel)
    {
        float cx = getWidth() * (reel ? .73f : .27f);
        auto disc = juce::Rectangle<float>(cx - radius, cy - radius, radius * 2, radius * 2);
        g.setColour(juce::Colours::black.withAlpha(.6f));
        g.fillEllipse(disc.translated(0, 2));
        g.setGradientFill(juce::ColourGradient(silver, cx - radius, cy - radius, steel, cx + radius,
                                               cy + radius, false));
        g.fillEllipse(disc);
        g.setColour(juce::Colour(0xff4e3540));
        float pack = radius * (.42f + .12f * (reel ? position : 1 - position));
        g.fillEllipse(cx - pack, cy - pack, pack * 2, pack * 2);
        for (int spoke = 0; spoke < 3; ++spoke)
        {
            float t = angle + reel * .4f + spoke * juce::MathConstants<float>::twoPi / 3;
            float x = cx + std::cos(t) * radius * .65f, y = cy + std::sin(t) * radius * .65f;
            g.setColour(glass);
            g.fillEllipse(x - radius * .21f, y - radius * .21f, radius * .42f, radius * .42f);
        }
        g.setColour(silver);
        g.fillEllipse(cx - 6, cy - 6, 12, 12);
        g.setColour(cosmic ? pal.control : glass);
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5, 5);
        g.setColour(cosmic ? pal.highlight : silver.withAlpha(.7f));
        g.drawEllipse(disc.reduced(1), 1);
    }
    float tapeY = window.getBottom() - 6;
    g.setColour(cosmic ? pal.control : steel);
    g.drawLine(getWidth() * .29f, cy + radius * .86f, getWidth() * .4f, tapeY, 1.5f);
    g.drawLine(getWidth() * .4f, tapeY, getWidth() * .6f, tapeY, 1.5f);
    g.drawLine(getWidth() * .6f, tapeY, getWidth() * .71f, cy + radius * .86f, 1.5f);
    g.setColour(silver);
    g.fillRoundedRectangle(getWidth() * .44f, tapeY - 3, getWidth() * .12f, 6, 2);
    auto meter = box.withTrimmedTop(window.getBottom() + 5).reduced(6, 0).withTrimmedBottom(5);
    g.setColour(cosmic ? glass : juce::Colour(0xffeee2c6));
    g.fillRoundedRectangle(meter, 4);
    auto ink = cosmic ? pal.text : juce::Colour(0xff252432);
    auto needle = cosmic ? pal.highlight : juce::Colour(0xffa04428);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(10, juce::Font::bold)));
    g.drawText(p.playbackTransport == 1 ? "PAUSE / OUT"
               : playing                ? "PLAY / OUT"
                                        : "STOP / OUT",
               meter.getX() + 5, meter.getY() + 1, 65, 13, juce::Justification::left);
    float db = juce::Decibels::gainToDecibels(level, -60.f);
    g.setFont(juce::Font(juce::FontOptions(11)));
    g.drawText((level > 0 ? juce::String(db, 0) : "--") + " dBFS", meter.getX() + 5,
               meter.getY() + 15, 65, 16, juce::Justification::left);
    auto gauge = meter.withTrimmedLeft(72).reduced(2);
    auto pivot = juce::Point<float>(gauge.getCentreX(), gauge.getBottom() + 4);
    float r = std::min(gauge.getWidth() * .5f, gauge.getHeight() + 2);
    for (int i = 0; i <= 6; ++i)
    {
        float t = -1.1f + i * 2.2f / 6;
        g.setColour(ink.withAlpha(.65f));
        g.drawLine(pivot.x + std::sin(t) * (r - 3), pivot.y - std::cos(t) * (r - 3),
                   pivot.x + std::sin(t) * r, pivot.y - std::cos(t) * r, 1);
    }
    float t = -1.1f + juce::jlimit(0.f, 1.f, (db + 48) / 48) * 2.2f;
    g.setColour(needle);
    g.drawLine(pivot.x, pivot.y, pivot.x + std::sin(t) * (r - 4), pivot.y - std::cos(t) * (r - 4),
               1.8f);
    g.setColour(ink);
    g.fillEllipse(pivot.x - 3, pivot.y - 3, 6, 6);
    for (float x : {8.f, getWidth() - 10.f})
    {
        g.setColour(steel);
        g.fillEllipse(x, 3, 3, 3);
        g.drawLine(x, 4.5f, x + 3, 4.5f, .6f);
    }
}
