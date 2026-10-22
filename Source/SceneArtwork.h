#pragma once
#include "Processor.h"
#include "Visuals.h"
// Original, deterministic vector room. Called only by the message-thread avatar painter.
inline void drawRastroRoom(juce::Graphics &g, RastroProcessor &p, juce::Rectangle<float> bounds,
                           float energy, float pulse)
{
    int kind = int(p.value("sceneKind"));
    if (kind == 0)
        return;
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(bounds.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(bounds.getWidth() / 400, bounds.getHeight() / 360)
                       .translated(bounds.getX(), bounds.getY()));
    auto pal = visuals::palette(p);
    juce::Colour wall = pal.bg, floor = pal.surface, glow = pal.highlight;
    const juce::Colour colours[]{pal.bg, juce::Colour(0xff292044), juce::Colour(0xff754a47),
                                 juce::Colour(0xff143e46), juce::Colour(0xffe5d3ac)};
    int tint = int(p.value("sceneColour"));
    wall = colours[juce::jlimit(0, 4, tint)];
    if (kind == 3)
        wall = wall.interpolatedWith(juce::Colour(0xffc97974), .38f);
    floor = wall.interpolatedWith(pal.surface, .5f);
    int light = int(p.value("sceneLight"));
    glow = light == 2 ? juce::Colour(0xffffbe70) : light == 1 ? pal.secondary : pal.highlight;
    float response = std::clamp(p.value("sceneReaction") * .5f, 0.f, 1.f);
    float lit = std::clamp((energy * .7f + pulse * .3f) * response, 0.f, 1.f);
    auto ink = wall.contrasting(.85f).withAlpha(.32f);
    g.setGradientFill(
        juce::ColourGradient(wall.brighter(.2f), 0, 0, wall.darker(.15f), 400, 300, false));
    g.fillRect(0.f, 0.f, 400.f, 360.f);
    // A round-cornered observatory window and an original abstract skyline.
    auto window = juce::Rectangle<float>(105, 24, 190, 176);
    g.setColour(wall.darker(.7f));
    g.fillRoundedRectangle(window.expanded(6), 35);
    g.setGradientFill(juce::ColourGradient(glow.withMultipliedBrightness(.75f), 200, 25,
                                           wall.darker(.1f), 200, 200, false));
    g.fillRoundedRectangle(window, 31);
    g.setColour(glow.brighter(.4f).withAlpha(.65f));
    g.fillEllipse(210, 44, 58, 58);
    for (int i = 0; i < 8; ++i)
    {
        float x = 112 + i * 22.f, h = 25 + (i * 29 % 58);
        g.setColour(wall.darker(.4f));
        g.fillRoundedRectangle(x, 195 - h, 17, h, 3);
        g.setColour(glow.withAlpha(.4f + .25f * lit));
        for (int j = 0; j < 3; ++j)
            g.fillRect(x + 5, 202 - h + j * 12, 3.f, 3.f);
    }
    g.setColour(ink);
    g.drawLine(105, 115, 295, 115, 3);
    g.drawLine(199, 24, 199, 200, 3);
    // Perspective floor and concentric listening rug.
    juce::Path ground;
    ground.startNewSubPath(0, 269);
    ground.lineTo(400, 269);
    ground.lineTo(400, 360);
    ground.lineTo(0, 360);
    ground.closeSubPath();
    g.setColour(floor);
    g.fillPath(ground);
    g.setColour(ink.withAlpha(.16f));
    for (int i = 0; i < 8; ++i)
        g.drawLine(180, 269, i * 65.f - 40, 360, 1);
    for (int i = 0; i < 3; ++i)
        g.drawHorizontalLine(285 + i * 27, 0, 400);
    for (int i = 0; i < 4; ++i)
    {
        g.setColour((i % 2 ? glow : wall).withAlpha(.23f));
        g.fillEllipse(80.f + i * 13, 294.f + i * 4, 242.f - i * 26, 57.f - i * 8);
    }
    auto speaker = [&](float x)
    {
        g.setColour(wall.darker(.5f));
        g.fillRoundedRectangle(x, 190, 45, 99, 7);
        g.setColour(ink);
        g.drawRoundedRectangle(x + 2, 192, 41, 95, 6, 1);
        for (int i = 0; i < 2; ++i)
        {
            float r = i ? 15.f : 9.f;
            g.setColour(glow.withAlpha(.2f + .3f * lit));
            g.fillEllipse(x + 22 - r, 211.f + i * 44 - r, r * 2, r * 2);
            g.setColour(wall.darker(.8f));
            g.fillEllipse(x + 17, 206.f + i * 44, 10, 10);
        }
    };
    speaker(18);
    speaker(337);
    if (kind == 1) // Low bed/divan, deliberately kept behind the actor.
    {
        g.setColour(wall.darker(.4f));
        g.fillRoundedRectangle(28, 222, 111, 58, 9);
        g.setColour(glow.withAlpha(.35f));
        g.fillRoundedRectangle(30, 220, 107, 24, 7);
        g.setColour(pal.secondary.withAlpha(.3f));
        g.fillRoundedRectangle(37, 216, 28, 18, 5);
        g.setColour(ink);
        for (int i = 0; i < 5; ++i)
            g.drawVerticalLine(75 + i * 10, 230, 274);
    }
    if (kind == 2)
    {
        g.setColour(wall.darker(.5f));
        g.fillRoundedRectangle(12, 116, 77, 60, 6);
        for (int i = 0; i < 4; ++i)
        {
            g.setColour(glow.withAlpha(.22f + .42f * lit));
            g.fillRoundedRectangle(21, 123.f + i * 12, 59, 7, 2);
        }
    }
    // Hanging lamp: brightness follows measured audio/control energy, never an idle oscillator.
    g.setColour(ink);
    g.drawLine(319, 0, 319, 59, 2);
    g.setColour(glow.withAlpha(.07f + .15f * lit));
    g.fillEllipse(280, 45, 78, 155);
    juce::Path lamp;
    lamp.startNewSubPath(300, 48);
    lamp.quadraticTo(319, 23, 338, 48);
    lamp.lineTo(342, 62);
    lamp.lineTo(296, 62);
    lamp.closeSubPath();
    g.setColour(wall.darker(.3f));
    g.fillPath(lamp);
    g.setColour(glow);
    g.fillRoundedRectangle(299, 59, 40, 4, 2);
    int decor = int(p.value("sceneDecor"));
    g.setColour(ink);
    g.drawLine(12, 105, 82, 105, 4);
    if (decor == 1)
    {
        g.setColour(glow.darker(.25f));
        g.fillRoundedRectangle(31, 84, 29, 20, 4);
        for (int i = 0; i < 5; ++i)
        {
            juce::Path leaf;
            float x = 20 + i * 10.f;
            leaf.startNewSubPath(45, 89);
            leaf.quadraticTo(x - 15, 47 + i * 4.f, x, 46 + i * 3.f);
            leaf.quadraticTo(x + 10, 74, 45, 89);
            g.setColour(pal.secondary.darker(.2f));
            g.fillPath(leaf);
        }
    }
    if (decor == 2)
        for (int i = 0; i < 3; ++i)
        {
            g.setColour(glow.withAlpha(.6f));
            g.fillRoundedRectangle(15.f + i * 22, 65, 20, 38, 2);
            g.setColour(wall.darker(.6f));
            g.fillEllipse(17.f + i * 22, 74, 16, 16);
        }
    if (decor == 3)
    {
        g.setColour(wall.darker(.5f));
        g.fillRoundedRectangle(12, 56, 70, 47, 4);
        for (int i = 0; i < 8; ++i)
        {
            g.setColour(i % 3 ? pal.secondary : glow);
            g.fillRect(18.f + i * 7, 80.f, 5.f, 17.f);
        }
    }
}
