#pragma once
#include "Processor.h"
#include "Visuals.h"
class EnvelopeView : public juce::Component, public juce::SettableTooltipClient
{
    RastroProcessor &p;
    std::array<float, 6> previous{};
    juce::uint32 lastColour = 0;

  public:
    explicit EnvelopeView(RastroProcessor &processor) : p(processor)
    {
        setComponentID("amplitudeEnvelope");
        setTooltip("Sustain follows held MIDI notes. Graph shows a fixed 400 ms sustain segment "
                   "for scale; Attack, Decay, level and Release use the real parameters.");
    }
    void update()
    {
        std::array<float, 6> next{p.value("ampAttack"),   p.value("ampDecay"),
                                  p.value("ampSustain"),  p.value("ampRelease"),
                                  p.value("synthEnable"), p.value("triggerMode")};
        auto colour = visuals::palette(p).control.getARGB();
        if (next != previous || colour != lastColour)
        {
            previous = next;
            lastColour = colour;
            repaint();
        }
    }
    std::array<juce::Point<float>, 5> points() const
    {
        auto area = getLocalBounds().toFloat().reduced(16, 26);
        float a = p.value("ampAttack"), d = p.value("ampDecay"), r = p.value("ampRelease"),
              hold = .4f, total = a + d + hold + r;
        float sustain = area.getBottom() - p.value("ampSustain") * area.getHeight();
        return {{{area.getX(), area.getBottom()},
                 {area.getX() + area.getWidth() * a / total, area.getY()},
                 {area.getX() + area.getWidth() * (a + d) / total, sustain},
                 {area.getX() + area.getWidth() * (a + d + hold) / total, sustain},
                 {area.getRight(), area.getBottom()}}};
    }
    void paint(juce::Graphics &g) override
    {
        auto pal = visuals::palette(p);
        auto area = getLocalBounds().toFloat();
        bool active = p.value("synthEnable") > .5f || p.value("triggerMode") == 2;
        g.setColour(pal.surface);
        g.fillRoundedRectangle(area, 9);
        auto pts = points();
        g.setColour(pal.outline.withAlpha(.35f));
        for (int i = 0; i < 4; ++i)
        {
            float y = pts[1].y + (pts[0].y - pts[1].y) * i / 3;
            g.drawHorizontalLine(int(y), pts[0].x, pts[4].x);
        }
        juce::Path path;
        path.startNewSubPath(pts[0]);
        for (size_t i = 1; i < pts.size(); ++i)
            path.lineTo(pts[i]);
        auto fill = path;
        fill.closeSubPath();
        auto ink = active ? pal.control : pal.muted;
        g.setGradientFill(juce::ColourGradient(ink.withAlpha(.25f), 0, pts[1].y,
                                               ink.withAlpha(.02f), 0, pts[0].y, false));
        g.fillPath(fill);
        g.setColour(ink);
        g.strokePath(path, juce::PathStrokeType(2.3f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
        for (int i = 1; i < 4; ++i)
            g.fillEllipse(pts[i].x - 3, pts[i].y - 3, 6, 6);
        g.setFont(juce::Font(juce::FontOptions(11)));
        g.setColour(pal.text);
        g.drawText(active ? juce::String::fromUTF8("ADSR · AMPLITUDE")
                          : juce::String::fromUTF8("ADSR · OFF"),
                   12, 5, getWidth() - 24, 18, juce::Justification::left);
        g.setColour(pal.muted);
        g.drawText("A     D       S                     R", 16, getHeight() - 22, getWidth() - 32,
                   18, juce::Justification::centred);
    }
};
