#pragma once
#include "Processor.h"
#include "Visuals.h"
// Original vector wardrobe, fitted in the shared creature rig's local coordinates.
namespace wardrobe
{
inline const juce::Colour ink{0xff141320}, cream{0xffffedcc}, pink{0xffee39bf}, cyan{0xff3de1e8},
    purple{0xff8c36e2}, gold{0xfff5bd42}, orange{0xffff8b35};
inline void outline(juce::Graphics &g, const juce::Path &path, juce::Colour fill, float width = 2)
{
    auto bounds = path.getBounds();
    g.setGradientFill(juce::ColourGradient(fill.brighter(.18f), bounds.getX(), bounds.getY(),
                                           fill.darker(.18f), bounds.getRight(), bounds.getBottom(),
                                           false));
    g.fillPath(path);
    g.setColour(ink);
    g.strokePath(path, juce::PathStrokeType(width, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
}
inline void star(juce::Graphics &g, float x, float y, float radius, juce::Colour colour)
{
    juce::Path p;
    p.addStar({x, y}, 5, radius * .43f, radius);
    g.setColour(colour);
    g.fillPath(p);
}
inline void badge(juce::Graphics &g, int kind, float x, float y, float r = 7)
{
    if (!kind)
        return;
    g.setColour(ink);
    g.fillEllipse(x - r - 1, y - r - 1, 2 * r + 2, 2 * r + 2);
    auto c = kind == 2 ? gold : kind == 3 ? pink : kind == 4 ? cyan : purple;
    g.setColour(c);
    g.fillEllipse(x - r, y - r, 2 * r, 2 * r);
    g.setColour(cream);
    if (kind == 1)
    {
        g.fillEllipse(x - r * .55f, y - r * .55f, r * 1.1f, r * 1.1f);
        g.setColour(ink);
        g.fillEllipse(x - r * .25f, y - r * .4f, r * .5f, r * .8f);
    }
    else if (kind == 5)
        star(g, x, y, r * .85f, gold);
    else if (kind == 3)
    {
        juce::Path heart;
        heart.startNewSubPath(x, y + r * .6f);
        heart.cubicTo(x - r * 1.3f, y - r * .2f, x - r * .3f, y - r * 1.1f, x, y - r * .3f);
        heart.cubicTo(x + r * .3f, y - r * 1.1f, x + r * 1.3f, y - r * .2f, x, y + r * .6f);
        g.fillPath(heart);
    }
    else if (kind == 4)
    {
        for (float side : {-1.f, 1.f})
        {
            float ex = x + side * r * .4f;
            g.setColour(ink);
            g.drawLine(ex - r * .2f, y - r * .3f, ex + r * .2f, y + r * .3f, 1.5f);
            g.drawLine(ex - r * .2f, y + r * .3f, ex + r * .2f, y - r * .3f, 1.5f);
        }
    }
    else
    {
        g.setColour(ink);
        g.fillEllipse(x - r * .4f, y - r * .3f, 2, 2);
        g.fillEllipse(x + r * .2f, y - r * .3f, 2, 2);
        juce::Path smile;
        smile.startNewSubPath(x - r * .5f, y + r * .1f);
        smile.quadraticTo(x, y + r * .8f, x + r * .5f, y + r * .1f);
        g.strokePath(smile, juce::PathStrokeType(1));
    }
}
inline void hat(juce::Graphics &g, RastroProcessor &p, float width = 52)
{
    int kind = int(p.value("stageHeadwear"));
    if (!kind)
        return;
    if (kind == 2)
    {
        g.setColour(juce::Colour(0xff541b3a));
        g.fillEllipse(-width - 13, -87, 2 * width + 26, 19);
        juce::Path crown;
        crown.startNewSubPath(-width * .67f, -81);
        crown.lineTo(-width * .57f, -108);
        crown.quadraticTo(-10, -117, 3, -106);
        crown.quadraticTo(width * .4f, -116, width * .6f, -107);
        crown.lineTo(width * .7f, -81);
        crown.closeSubPath();
        outline(g, crown, juce::Colour(0xff6a2142));
        g.setColour(gold);
        g.fillRoundedRectangle(-width * .62f, -92, width * 1.25f, 9, 3);
        juce::Path feather;
        feather.startNewSubPath(27, -91);
        feather.cubicTo(57, -112, 39, -133, 21, -107);
        feather.closeSubPath();
        outline(g, feather, cream, 1);
        g.setColour(orange);
        g.drawLine(27, -91, 37, -117, 1);
        badge(g, int(p.value("stagePatch")), 0, -88, 6);
    }
    else
    {
        auto colour = kind == 3 ? orange : purple;
        juce::Path cap;
        cap.startNewSubPath(-width, -72);
        cap.quadraticTo(-width - 3, -112, -12, -116);
        cap.quadraticTo(22, -127, width - 2, -94);
        cap.lineTo(width, -72);
        cap.closeSubPath();
        outline(g, cap, colour);
        g.setColour(colour.brighter(.23f));
        g.fillRoundedRectangle(-width, -86, 2 * width, 18, 5);
        g.setColour(ink.withAlpha(.5f));
        for (int i = 0; i < 15; ++i)
        {
            float x = -width + 4 + i * (2 * width - 8) / 14;
            g.drawLine(x, -83, x, -70, 1);
            juce::Path rib;
            rib.startNewSubPath(x, -87);
            rib.quadraticTo(x * .7f, -108, -8 + (x / width) * 14, -115);
            g.strokePath(rib, juce::PathStrokeType(.7f));
        }
        badge(g, int(p.value("stagePatch")), 0, -77, 7);
    }
}
inline void phones(juce::Graphics &g, float width = 55)
{
    juce::Path arch;
    arch.startNewSubPath(-width, -36);
    arch.cubicTo(-width - 7, -114, width + 7, -114, width, -36);
    g.setColour(ink);
    g.strokePath(arch, juce::PathStrokeType(9));
    g.setColour(purple);
    g.strokePath(arch, juce::PathStrokeType(3));
    for (float side : {-1.f, 1.f})
    {
        float x = side * width;
        g.setColour(ink);
        g.fillRoundedRectangle(x - 12, -60, 24, 35, 9);
        g.setColour(side < 0 ? purple : orange);
        g.fillRoundedRectangle(x - 10, -58, 20, 31, 7);
        g.setColour(ink);
        g.fillEllipse(x - 8, -54, 16, 22);
        g.setColour(side < 0 ? cyan : gold);
        g.drawEllipse(x - 8, -54, 16, 22, 2);
        if (side > 0)
            star(g, x, -43, 6, gold);
        g.setColour(cream);
        g.fillRoundedRectangle(x - 5, -67, 10, 7, 2);
    }
}
inline void glasses(juce::Graphics &g, int kind, bool cyclops = false)
{
    if (!kind)
        return;
    g.setColour(gold);
    if (cyclops)
    {
        auto r = juce::Rectangle<float>(-25, -69, 50, 41);
        g.setColour(orange.withAlpha(.35f));
        g.fillEllipse(r);
        g.setColour(gold);
        g.drawEllipse(r, 2);
        g.drawLine(-25, -51, -49, -53, 2);
        g.drawLine(25, -51, 49, -53, 2);
    }
    else
        for (float side : {-1.f, 1.f})
        {
            auto r = juce::Rectangle<float>(side * 20 - 16, -57, 32, 28);
            g.setColour((kind == 3 ? orange : purple).withAlpha(.6f));
            g.fillEllipse(r);
            g.setColour(kind == 3 ? gold : cream);
            g.drawEllipse(r, 2);
            g.drawLine(side * 34, -45, side * 49, -49, 2);
        }
    if (!cyclops)
    {
        g.setColour(gold);
        g.drawLine(-4, -44, 4, -44, 2);
    }
}
inline void flower(juce::Graphics &g, float x, float y, float size)
{
    g.setColour(orange);
    for (int petal = 0; petal < 5; ++petal)
    {
        float a = petal * juce::MathConstants<float>::twoPi / 5;
        g.fillEllipse(x + std::sin(a) * size * .4f - size * .3f,
                      y + std::cos(a) * size * .4f - size * .3f, size * .6f, size * .6f);
    }
    g.setColour(cream);
    g.fillEllipse(x - size * .17f, y - size * .17f, size * .34f, size * .34f);
}
inline void top(juce::Graphics &g, RastroProcessor &p, const juce::Path &shape)
{
    int shirt = int(p.value("stageShirt")), jacket = int(p.value("stageJacket"));
    if (!shirt && !jacket)
        return;
    auto fill = shirt == 1 ? juce::Colour(0xff59212c) : cream;
    if (!shirt)
        fill = juce::Colour(0xff252336);
    outline(g, shape, fill);
    if (shirt == 1)
    {
        juce::Graphics::ScopedSaveState clip(g);
        g.reduceClipRegion(shape);
        for (int row = 0; row < 6; ++row)
            for (int col = 0; col < 6; ++col)
                flower(g, -40 + col * 16 + (row % 2) * 5, 8 + row * 11, 9);
    }
    if (shirt == 3)
    {
        juce::Graphics::ScopedSaveState clipped(g);
        g.reduceClipRegion(shape);
        g.setColour(cyan);
        for (int i = 0; i < 6; ++i)
            g.fillRect(-40 + i * 14, 4, 6, 61);
    }
    if (shirt == 2)
    {
        g.setColour(ink);
        g.fillRect(-15, 16, 30, 20);
        g.setColour(orange);
        for (int row = 0; row < 6; ++row)
            g.drawLine(-13, 18 + row * 3, 13, 20 + row * 3, 1);
    }
    if (jacket)
    {
        auto cloth = jacket == 2 ? juce::Colour(0xff8b411f) : ink;
        for (float side : {-1.f, 1.f})
        {
            juce::Path panel;
            panel.startNewSubPath(side * 3, 1);
            panel.lineTo(side * 43, 7);
            panel.lineTo(side * 39, 66);
            panel.lineTo(side * 10, 66);
            panel.lineTo(side * 8, 25);
            panel.closeSubPath();
            outline(g, panel, cloth);
            g.setColour(jacket == 1 ? cyan : orange);
            g.drawLine(side * 8, 4, side * 13, 54, 2);
            g.setColour(jacket == 1 ? pink : gold);
            g.fillRoundedRectangle(side * 26 - 8, 37, 16, 17, 3);
            g.setColour(ink);
            g.drawLine(side * 20, 40, side * 31, 40, 1);
            if (jacket == 2)
            {
                g.setColour(gold);
                for (int fringe = 0; fringe < 8; ++fringe)
                    g.drawLine(side * (13 + fringe * 3), 18, side * (13 + fringe * 3),
                               34 + (fringe % 3) * 2, 1);
            }
        }
        g.setColour(jacket == 1 ? purple : orange);
        g.drawLine(-37, 8, -18, -6, 5);
        g.drawLine(37, 8, 18, -6, 5);
    }
    if (p.value("stageHeadwear") < .5f)
        badge(g, int(p.value("stagePatch")), 25, 20, 6);
}
inline void legDetails(juce::Graphics &g, int kind, float x, float y, float side)
{
    if (!kind)
        return;
    if (kind == 4)
    {
        g.setColour(orange);
        g.drawLine(x + side * 12, 58, x + side * 17, y + 4, 5);
        return;
    }
    if (kind == 2)
    {
        g.setColour(orange);
        for (int i = 0; i < 4; ++i)
            g.drawLine(x - 12 + i * 7, 58, x - 14 + i * 8, y + 4, 1.2f);
        return;
    }
    g.setColour(kind == 1 ? pink : orange);
    g.fillRoundedRectangle(x - 12, y - 21, 24, 15, 3);
    g.setColour(ink);
    g.fillRoundedRectangle(x - 9, y - 18, 18, 9, 2);
    if (kind == 1)
    {
        g.setColour(cyan);
        g.drawEllipse(x - 7, y - 16, 5, 5, 1);
        g.drawEllipse(x + 2, y - 16, 5, 5, 1);
        g.drawLine(x - 4, y - 11, x + 5, y - 11, 1);
        g.setColour(side < 0 ? cyan : pink);
        g.drawLine(x + side * 13, 60, x + side * 14, y - 6, 2);
    }
    else
    {
        g.setColour(gold);
        for (int i = 0; i < 5; ++i)
            g.drawLine(x - 8 + i * 4, y - 18, x - 6 + i * 4, y - 8, 1);
    }
}
inline void shoe(juce::Graphics &g, int kind, float x, float y, float side)
{
    if (!kind)
        return;
    auto colour = kind == 1   ? purple
                  : kind == 2 ? juce::Colour(0xff934321)
                  : kind == 4 ? orange
                              : ink;
    juce::Graphics::ScopedSaveState mirror(g);
    if (side < 0)
        g.addTransform(juce::AffineTransform(-1, 0, 2 * x, 0, 1, 0));
    juce::Path shape;
    shape.startNewSubPath(x - 16, y - 18);
    shape.lineTo(x + 8, y - 19);
    shape.lineTo(x + 13, y - 6);
    shape.quadraticTo(x + 30, y - 5, x + 26, y + 9);
    shape.lineTo(x - 21, y + 9);
    shape.closeSubPath();
    outline(g, shape, colour);
    g.setColour(cream);
    g.fillRoundedRectangle(x - 21, y + 3, 47, 9, 3);
    g.setColour(kind == 1 ? cyan : orange);
    g.fillRoundedRectangle(x - 15, y - 15, 24, 7, 3);
    for (int i = 0; i < 4; ++i)
    {
        g.setColour(i % 2 ? cream : (kind == 1 ? pink : orange));
        g.drawLine(x - 9, y - 11 + i * 3, x + 5, y - 10 + i * 3, 1.7f);
    }
    g.setColour(ink);
    g.drawLine(x - 18, y + 8, x + 24, y + 8, 1);
    if (kind == 1)
        star(g, x + 11, y - 7, 5, gold);
    else
    {
        g.setColour(orange);
        g.drawLine(x - 16, y + 2, x - 7, y - 1, 3);
        g.drawLine(x - 7, y - 1, x + 2, y + 3, 3);
    }
}
inline void instrument(juce::Graphics &g, int edition, float energy)
{
    auto colour = edition == 1   ? pink
                  : edition == 2 ? juce::Colour(0xffda382a)
                  : edition == 4 ? orange
                                 : ink;
    juce::Path casing;
    casing.startNewSubPath(-53, 33);
    casing.lineTo(44, 31);
    if (edition != 3)
    {
        casing.lineTo(71, 23);
        casing.lineTo(74, 34);
        casing.lineTo(48, 44);
    }
    casing.lineTo(51, 66);
    casing.lineTo(-52, 68);
    casing.lineTo(-58, 56);
    casing.closeSubPath();
    outline(g, casing, colour);
    g.setColour(edition == 1 ? ink : colour.darker(.4f));
    g.fillRoundedRectangle(-48, 35, 92, 28, 3);
    g.setColour(cream);
    g.fillRect(-42, 37, 71, 17);
    for (int i = 0; i < 12; ++i)
    {
        g.setColour(ink);
        g.drawLine(-42 + i * 6, 37, -42 + i * 6, 54, .6f);
        if (i % 7 != 2 && i % 7 != 6)
            g.fillRect(-39 + i * 6, 37, 3, 10);
    }
    if (edition != 3)
        for (int i = 0; i < 5; ++i)
        {
            g.setColour((i % 2 ? gold : cyan).withAlpha(.4f + .6f * energy));
            g.fillRect(49 + i * 4, 28 - i, 3, 5);
        }
    g.setColour(cyan.withAlpha(.5f + .5f * energy));
    for (int i = 0; i < 3; ++i)
        g.drawEllipse(-8 + i * 10, 58, 5, 5, 1);
    if (edition == 2)
    {
        g.setColour(gold);
        g.drawLine(-40, 59, -15, 59, 2);
        g.fillEllipse(33, 52, 9, 9);
    }
    else
    {
        g.setColour(purple);
        g.fillEllipse(-37, 56, 13, 8);
        g.setColour(cream);
        g.drawEllipse(-40, 58, 19, 4, 1);
        star(g, 33, 60, 5, gold);
    }
}
} // namespace wardrobe
