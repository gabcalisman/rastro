#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace visuals
{
// Original vectors informed by the supplied material studies. No reference pixels/assets.
inline bool materialKnob(juce::Graphics &g, juce::Rectangle<float> box, float value, float start,
                         float end, int design, juce::Colour accent)
{
    if (design != 0 && design != 1 && design != 4 && design != 5 && (design < 6 || design > 9))
        return false;
    juce::Graphics::ScopedSaveState state(g);
    float r = std::max(3.f, std::min(box.getWidth(), box.getHeight()) * .43f);
    auto c = box.getCentre();
    auto disc = juce::Rectangle<float>(c.x - r, c.y - r, 2 * r, 2 * r);
    auto black = juce::Colour(0xff11151b), gold = juce::Colour(0xffc39a44);
    float angle = start + (end - start) * value;
    auto point = [&](float radius)
    { return juce::Point<float>(c.x + std::sin(angle) * radius, c.y - std::cos(angle) * radius); };
    g.setColour(juce::Colours::black.withAlpha(.55f));
    g.fillEllipse(disc.expanded(2).translated(0, 3));
    if (design >= 6)
    {
        bool amber = design == 7, onyx = design == 8, rgb = design == 9;
        auto rim = amber || onyx ? gold : juce::Colour(0xff8995a4);
        g.setGradientFill(juce::ColourGradient(rim.brighter(.65f), c.x - r, c.y - r,
                                               rim.darker(.75f), c.x + r, c.y + r, false));
        g.fillEllipse(disc);
        g.setColour(black);
        g.fillEllipse(disc.reduced(r * .09f));
        if (rgb)
        {
            for (int i = 0; i < 24; ++i)
            {
                float a0 = i * juce::MathConstants<float>::twoPi / 24;
                juce::Path segment;
                segment.addCentredArc(c.x, c.y, r * .81f, r * .81f, 0, a0, a0 + .19f, true);
                auto led = juce::Colour::fromHSV(.5f + i / 24.f * .4f, .9f, 1.f, 1.f);
                g.setColour(led.withAlpha(i / 24.f <= value ? 1.f : .28f));
                g.strokePath(segment, juce::PathStrokeType(r * .14f));
            }
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff535a6b), c.x - r, c.y - r,
                                                   black, c.x + r, c.y + r, false));
            g.fillEllipse(disc.reduced(r * .3f));
        }
        else if (amber || onyx)
        {
            juce::Path shape;
            int points = amber ? 28 : 96;
            for (int i = 0; i <= points; ++i)
            {
                float t = i * juce::MathConstants<float>::twoPi / points;
                float rr = r * (amber ? (i % 2 ? .75f : .91f) : (.83f + .07f * std::cos(t * 10)));
                auto p = juce::Point<float>(c.x + std::sin(t) * rr, c.y - std::cos(t) * rr);
                if (i == 0)
                    shape.startNewSubPath(p);
                else
                    shape.lineTo(p);
            }
            shape.closeSubPath();
            auto top = amber ? juce::Colour(0xffffd36e) : juce::Colour(0xff53545c);
            auto bottom = amber ? juce::Colour(0xffa54704) : juce::Colour(0xff0e0b11);
            g.setGradientFill(
                juce::ColourGradient(top, c.x - r, c.y - r, bottom, c.x + r, c.y + r, false));
            g.fillPath(shape);
            g.setColour(rim.brighter(.5f));
            g.strokePath(shape, juce::PathStrokeType(1));
            if (amber)
            {
                for (int i = 0; i < 14; ++i)
                {
                    float t = i * juce::MathConstants<float>::twoPi / 14;
                    g.setColour(i % 2 ? juce::Colour(0xffffd169).withAlpha(.8f)
                                      : juce::Colour(0xff743607).withAlpha(.5f));
                    g.drawLine(c.x + std::sin(t) * r * .68f, c.y - std::cos(t) * r * .68f,
                               c.x + std::sin(t) * r * .89f, c.y - std::cos(t) * r * .89f, 2);
                }
                g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffbf3e), c.x - r, c.y - r,
                                                       juce::Colour(0xffb75e06), c.x + r, c.y + r,
                                                       false));
                g.fillEllipse(disc.reduced(r * .32f));
            }
        }
        else // Clear pink resin dome with restrained material highlights.
        {
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xffff65bc), c.x - r * .5f, c.y - r,
                                                   juce::Colour(0xff990647), c.x + r * .7f, c.y + r,
                                                   false));
            g.fillEllipse(disc.reduced(r * .14f));
            g.setColour(juce::Colour(0xffffc5e5).withAlpha(.8f));
            g.fillEllipse(c.x - r * .5f, c.y - r * .65f, r * .24f, r * .35f);
            g.setColour(juce::Colours::white.withAlpha(.9f));
            g.fillEllipse(c.x - r * .27f, c.y - r * .55f, r * .11f, r * .12f);
            g.setColour(juce::Colour(0xffffb3df).withAlpha(.5f));
            g.drawEllipse(disc.reduced(r * .2f), 1);
        }
        auto from = point(r * .14f), to = point(r * (rgb ? .62f : .7f));
        g.setColour(black);
        g.drawLine(from.x, from.y, to.x, to.y, std::max(3.f, r * .14f));
        g.setColour(rgb     ? juce::Colour(0xff23edff)
                    : amber ? juce::Colour(0xffffcb54)
                            : juce::Colour(0xfffff4df));
        g.drawLine(from.x, from.y, to.x, to.y, std::max(1.5f, r * .075f));
    }
    else if (design == 0) // Rounded ivory flutes, raised cap, brass fastener, amber inlay.
    {
        juce::Path flutes;
        constexpr int n = 120;
        for (int i = 0; i <= n; ++i)
        {
            float t = i * juce::MathConstants<float>::twoPi / n;
            float rr = r * (.96f + .04f * std::cos(t * 20));
            auto p = juce::Point<float>(c.x + std::sin(t) * rr, c.y - std::cos(t) * rr);
            if (i == 0)
                flutes.startNewSubPath(p);
            else
                flutes.lineTo(p);
        }
        flutes.closeSubPath();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffff6dc), c.x - r, c.y - r,
                                               juce::Colour(0xffb09a6d), c.x + r, c.y + r, false));
        g.fillPath(flutes);
        g.setColour(juce::Colour(0xffe5d2a8));
        g.strokePath(flutes, juce::PathStrokeType(1));
        for (int i = 0; i < 20; ++i)
        {
            float t = i * juce::MathConstants<float>::twoPi / 20;
            g.setColour(juce::Colour(0xff80673f).withAlpha(.35f));
            g.drawLine(c.x + std::sin(t) * r * .81f, c.y - std::cos(t) * r * .81f,
                       c.x + std::sin(t) * r * .95f, c.y - std::cos(t) * r * .95f,
                       std::max(.7f, r * .025f));
        }
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffffed), c.x - r * .6f, c.y - r,
                                               juce::Colour(0xffdbc79d), c.x + r * .7f, c.y + r,
                                               false));
        g.fillEllipse(disc.reduced(r * .15f));
        g.setColour(juce::Colour(0xff8b6c3b));
        g.drawEllipse(disc.reduced(r * .15f), 1);
        g.setColour(juce::Colours::white.withAlpha(.65f));
        g.drawEllipse(disc.reduced(r * .2f), 1);
        g.setColour(juce::Colour(0xff54350f));
        g.drawLine(point(r * .23f).x, point(r * .23f).y, point(r * .77f).x, point(r * .77f).y,
                   std::max(3.f, r * .13f));
        g.setColour(juce::Colour(0xffffa21d));
        g.drawLine(point(r * .23f).x, point(r * .23f).y, point(r * .77f).x, point(r * .77f).y,
                   std::max(1.5f, r * .075f));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffeab1), c.x - r * .2f,
                                               c.y - r * .2f, gold.darker(.4f), c.x + r * .2f,
                                               c.y + r * .2f, false));
        g.fillEllipse(disc.withSizeKeepingCentre(r * .4f, r * .4f));
        g.setColour(juce::Colour(0xff574018));
        g.drawEllipse(disc.withSizeKeepingCentre(r * .4f, r * .4f), 1);
        g.drawLine(c.x - r * .12f, c.y + r * .09f, c.x + r * .12f, c.y - r * .09f,
                   std::max(1.f, r * .045f));
    }
    else if (design == 1) // Machined aluminium: dark knurling and a brushed silver top.
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffb8bdc5), c.x - r, c.y - r, black,
                                               c.x + r, c.y + r, false));
        g.fillEllipse(disc);
        for (int i = 0; i < 40; ++i)
        {
            float t = i * juce::MathConstants<float>::twoPi / 40;
            g.setColour(i % 2 ? juce::Colour(0xffe5e7ea) : juce::Colour(0xff444951));
            g.drawLine(c.x + std::sin(t) * r * .77f, c.y - std::cos(t) * r * .77f,
                       c.x + std::sin(t) * r * .96f, c.y - std::cos(t) * r * .96f,
                       std::max(.7f, r * .035f));
        }
        auto cap = disc.reduced(r * .18f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffafcff), cap.getX(), cap.getY(),
                                               juce::Colour(0xff717984), cap.getRight(),
                                               cap.getBottom(), false));
        g.fillEllipse(cap);
        g.setColour(juce::Colours::white.withAlpha(.6f));
        g.drawEllipse(cap, 1);
        for (int i = 0; i < 12; ++i)
        {
            g.setColour(juce::Colours::black.withAlpha(.045f));
            g.drawEllipse(cap.reduced(i * r * .055f), .6f);
        }
        auto from = point(r * .09f), to = point(r * .71f);
        g.setColour(black);
        g.drawLine(from.x, from.y, to.x, to.y, std::max(3.f, r * .14f));
        g.setColour(juce::Colour(0xfffffdf1));
        g.drawLine(from.x, from.y, to.x, to.y, std::max(1.5f, r * .075f));
    }
    else if (design == 4) // Bakelite selector with brass bezel; the entire grip rotates with value.
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffe4a1), c.x - r, c.y - r,
                                               gold.darker(.6f), c.x + r, c.y + r, false));
        g.fillEllipse(disc);
        g.setColour(juce::Colour(0xfff6d992));
        g.drawEllipse(disc.reduced(1), 1);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff44474a), c.x - r, c.y - r, black,
                                               c.x + r, c.y + r, false));
        g.fillEllipse(disc.reduced(r * .13f));
        juce::Graphics::ScopedSaveState gripState(g);
        g.addTransform(juce::AffineTransform::rotation(angle, c.x, c.y));
        juce::Path grip;
        grip.startNewSubPath(c.x - r * .12f, c.y - r * .91f);
        grip.quadraticTo(c.x - r * .34f, c.y - r * .3f, c.x - r * .31f, c.y + r * .17f);
        grip.quadraticTo(c.x - r * .13f, c.y + r * .82f, c.x, c.y + r * .88f);
        grip.quadraticTo(c.x + r * .19f, c.y + r * .7f, c.x + r * .31f, c.y + r * .17f);
        grip.quadraticTo(c.x + r * .34f, c.y - r * .3f, c.x + r * .12f, c.y - r * .91f);
        grip.closeSubPath();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff55585a), c.x - r * .3f, c.y, black,
                                               c.x + r * .3f, c.y, false));
        g.fillPath(grip);
        g.setColour(juce::Colour(0xff999b9a));
        g.strokePath(grip, juce::PathStrokeType(.8f));
        juce::Path pointer;
        pointer.addTriangle(c.x, c.y - r * .83f, c.x - r * .09f, c.y - r * .22f, c.x + r * .09f,
                            c.y - r * .22f);
        g.setColour(juce::Colour(0xffffd275));
        g.fillPath(pointer);
    }
    else // Smoked fluted glass, chrome rims and cyan light pipe.
    {
        auto light = accent.interpolatedWith(juce::Colour(0xff20edff), .6f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff7d8b98), c.x - r, c.y - r,
                                               juce::Colour(0xff101c28), c.x + r, c.y + r, false));
        g.fillEllipse(disc);
        g.setColour(light.withAlpha(.28f));
        g.drawEllipse(disc.reduced(r * .11f), r * .13f);
        g.setColour(light);
        g.drawEllipse(disc.reduced(r * .13f), std::max(1.f, r * .045f));
        for (int i = 0; i < 16; ++i)
        {
            float t = i * juce::MathConstants<float>::twoPi / 16;
            g.setColour(i % 2 ? juce::Colour(0xff9eb1bd) : juce::Colour(0xff0a111b));
            g.drawLine(c.x + std::sin(t) * r * .69f, c.y - std::cos(t) * r * .69f,
                       c.x + std::sin(t) * r * .97f, c.y - std::cos(t) * r * .97f,
                       std::max(1.f, r * .045f));
        }
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff647583), c.x - r, c.y - r,
                                               juce::Colour(0xff14212b), c.x + r, c.y + r, false));
        g.fillEllipse(disc.reduced(r * .23f));
        g.setColour(juce::Colour(0xffb0d2e3));
        g.drawEllipse(disc.reduced(r * .24f), .8f);
        auto from = point(r * .16f), to = point(r * .67f);
        g.setColour(black);
        g.drawLine(from.x, from.y, to.x, to.y, std::max(4.f, r * .16f));
        g.setColour(light);
        g.drawLine(from.x, from.y, to.x, to.y, std::max(2.f, r * .08f));
    }
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, r + 3, r + 3, 0, start, start + (end - start) * value, true);
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(1.5f));
    return true;
}
} // namespace visuals
