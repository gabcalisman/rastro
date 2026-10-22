#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace visuals
{
inline const juce::Colour text{0xfff2efff}, muted{0xffb5aecb}, panel{0xff201e32}, edge{0xff49425f};
inline constexpr const char *visualIds[]{
    "visualTheme",        "visualPalette", "controlStyle",     "avatarBody",    "avatarEyes",
    "avatarAccessory",    "knobDesign",    "visualBackground", "visualAccent",  "visualControls",
    "avatarColour",       "avatarHair",    "avatarHorns",      "avatarClothes", "avatarGlasses",
    "avatarTail",         "avatarWings",   "avatarInstrument", "colourSet",     "cosmicArcade",
    "avatarPresentation", "avatarHalo"};
inline juce::Colour accent(int palette)
{
    constexpr juce::uint32 colours[]{0xffa794ff, 0xff64e4d2, 0xffff8cc7, 0xffffc179};
    return juce::Colour(colours[juce::jlimit(0, 3, palette)]);
}
inline juce::Colour background(int theme)
{
    constexpr juce::uint32 colours[]{0xff100e1d, 0xff0c1c26, 0xff231026, 0xff1e1714};
    return juce::Colour(colours[juce::jlimit(0, 3, theme)]);
}
struct Palette
{
    juce::Colour bg, surface, highlight, control, secondary, button, outline, text, muted;
};
inline const char *paletteNames[]{"After Hours",   "Solar Studio", "Mint Condition",
                                  "Sunrise",       "Sunset Drive", "Poolside",
                                  "Electric Soul", "Burnt Velvet", "Cosmic Arcade"};
inline float panelRadius(int theme)
{
    constexpr float values[]{7.f, 3.f, 1.f, 13.f};
    return values[juce::jlimit(0, 3, theme)];
}
inline float luminance(juce::Colour c)
{
    auto linear = [](float v)
    { return v <= .04045f ? v / 12.92f : std::pow((v + .055f) / 1.055f, 2.4f); };
    return .2126f * linear(c.getFloatRed()) + .7152f * linear(c.getFloatGreen()) +
           .0722f * linear(c.getFloatBlue());
}
inline float contrast(juce::Colour a, juce::Colour b)
{
    float x = luminance(a), y = luminance(b);
    return (std::max(x, y) + .05f) / (std::min(x, y) + .05f);
}
inline juce::Colour readable(juce::Colour colour, juce::Colour bg, float minimum = 4.5f)
{
    auto ink = luminance(bg) > .25f ? juce::Colour(0xff171626) : juce::Colour(0xfffaf4e9);
    for (int i = 0; i < 12 && contrast(colour, bg) < minimum; ++i)
        colour = colour.interpolatedWith(ink, .22f);
    return colour;
}
inline Palette finishPalette(juce::Colour base, juce::Colour highlight, juce::Colour control,
                             juce::Colour secondary)
{
    auto surface = base.interpolatedWith(highlight, .07f);
    auto button = base.interpolatedWith(control, .10f);
    auto ink = luminance(base) > .25f ? juce::Colour(0xff171626) : juce::Colour(0xfffaf4e9);
    auto soft = ink.interpolatedWith(base, .20f);
    auto edge = readable(base.interpolatedWith(control, .55f), surface, 3.f);
    return {base,
            surface,
            readable(highlight, surface),
            readable(control, button),
            readable(secondary, surface),
            button,
            edge,
            ink,
            soft};
}
inline Palette colourSet(int index)
{
    // Distinct studio materials, including truly light backgrounds and dark ink.
    constexpr juce::uint32 sets[9][4]{{0xff141829, 0xffe9b967, 0xff8ce1d4, 0xffe7a6bd},
                                      {0xfff2e7ce, 0xff9b391d, 0xff23665c, 0xff614274},
                                      {0xffdce9dd, 0xff164e48, 0xff324cab, 0xff9c362c},
                                      {0xfff6c9a7, 0xff923b3c, 0xff733f7e, 0xff874113},
                                      {0xff381d41, 0xffffb47c, 0xfff6a4c2, 0xfff8d487},
                                      {0xff083e44, 0xffe7ee8f, 0xff8de6d6, 0xffffb591},
                                      {0xff201749, 0xffedb3ff, 0xff70e5e6, 0xffffba7a},
                                      {0xff2a1e1b, 0xfff4ba75, 0xffd7d79d, 0xffeba999},
                                      {0xff250b46, 0xffff87e7, 0xff75edff, 0xffffc875}};
    auto &c = sets[juce::jlimit(0, 8, index)];
    return finishPalette(juce::Colour(c[0]), juce::Colour(c[1]), juce::Colour(c[2]),
                         juce::Colour(c[3]));
}
template <typename Processor> Palette palette(const Processor &p)
{
    int colour = (int)p.value("visualPalette"), set = (int)p.value("colourSet");
    int bg = (int)p.value("visualBackground"), ac = (int)p.value("visualAccent"),
        ct = (int)p.value("visualControls");
    bool cosmic = p.value("cosmicArcade") > .5f;
    auto chosen = colourSet(cosmic ? 8 : std::max(0, set - 1));
    if (cosmic)
        set = 9;
    auto base =
        set ? chosen.bg
            : background(colour).interpolatedWith(background((int)p.value("visualTheme")), .2f);
    if (bg)
        base = background(bg - 1);
    return finishPalette(base,
                         ac    ? accent(ac - 1)
                         : set ? chosen.highlight
                               : accent(colour),
                         ct    ? accent(ct - 1)
                         : set ? chosen.control
                               : accent(colour),
                         set ? chosen.secondary : accent((colour + 1) % 4));
}
inline void cosmicTexture(juce::Graphics &g, juce::Rectangle<float> box, juce::Colour ink)
{
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(box.toNearestInt());
    for (int i = 0; i < 46; ++i)
    {
        float x = box.getX() + std::fmod(i * 137.13f + 29, box.getWidth());
        float y = box.getY() + std::fmod(i * 71.37f + 19, box.getHeight());
        g.setColour(ink.withAlpha(i % 5 == 0 ? .13f : .055f));
        g.fillEllipse(x, y, i % 5 == 0 ? 2.f : 1.f, i % 5 == 0 ? 2.f : 1.f);
    }
}
template <typename Processor> int knobStyle(const Processor &p)
{
    const int design = (int)p.value("knobDesign");
    // Keep old controlStyle's range/normalization unchanged for saved automation.
    return design ? design - 1 : (int)p.value("controlStyle");
}
inline void knob(juce::Graphics &g, juce::Rectangle<float> box, float v, float a, float b,
                 int design, juce::Colour colour)
{
    float r = std::max(3.f, std::min(box.getWidth(), box.getHeight()) * .46f);
    auto c = box.getCentre();
    auto disc = juce::Rectangle<float>(c.x - r, c.y - r, r * 2, r * 2);
    g.setColour(juce::Colours::black.withAlpha(.35f));
    g.fillEllipse(disc.translated(0, 3));
    // Four different physical constructions, shared by thumbnails and actual controls.
    if (design == 0) // vintage scalloped bakelite
    {
        juce::Path body;
        for (int i = 0; i < 64; ++i)
        {
            float angle = i * juce::MathConstants<float>::twoPi / 64;
            float radius = r * (i % 4 == 0 ? .91f : 1.f);
            float x = c.x + std::cos(angle) * radius, y = c.y + std::sin(angle) * radius;
            if (i == 0)
                body.startNewSubPath(x, y);
            else
                body.lineTo(x, y);
        }
        body.closeSubPath();
        g.setColour(juce::Colour(0xffb4a282));
        g.fillPath(body);
        g.setColour(juce::Colour(0xfff3e7cb));
        g.strokePath(body, juce::PathStrokeType(1));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffff3d6), c.x - r, c.y - r,
                                               juce::Colour(0xffc5b28d), c.x + r, c.y + r, false));
        g.fillEllipse(disc.reduced(r * .16f));
        g.setColour(juce::Colour(0xff9c8a6a));
        g.drawEllipse(disc.reduced(r * .25f), 1);
    }
    else if (design == 1) // amplifier: brushed silver cap, numbered bezel
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe6dfce), c.x - r, c.y - r,
                                               juce::Colour(0xff716a7c), c.x + r, c.y + r, false));
        g.fillEllipse(disc.reduced(r * .13f));
        g.setColour(juce::Colour(0xff272135));
        g.drawEllipse(disc.reduced(r * .26f), 1);
        juce::Graphics::ScopedSaveState metal(g);
        juce::Path clip;
        clip.addEllipse(disc.reduced(r * .15f));
        g.reduceClipRegion(clip);
        for (int i = -6; i <= 6; ++i)
        {
            g.setColour((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha(.055f));
            g.drawHorizontalLine(juce::roundToInt(c.y + i * r * .12f), c.x - r, c.x + r);
        }
        g.setColour(juce::Colours::white.withAlpha(.42f));
        g.drawEllipse(disc.reduced(r * .17f), 1);
    }
    else if (design == 2) // synth: dark cap and luminous ring
    {
        g.setColour(juce::Colour(0xff131522));
        g.fillEllipse(disc);
        g.setColour(colour.withAlpha(.18f));
        g.drawEllipse(disc.reduced(2), 4);
        g.setColour(juce::Colour(0xff45445d));
        g.drawEllipse(disc.reduced(r * .24f), 1.5f);
        for (int i = 0; i < 18; ++i)
        {
            float t = i * juce::MathConstants<float>::twoPi / 18;
            g.setColour(juce::Colour(0xff50535c));
            g.drawLine(c.x + std::sin(t) * r * .74f, c.y - std::cos(t) * r * .74f,
                       c.x + std::sin(t) * r * .87f, c.y - std::cos(t) * r * .87f, 1.4f);
        }
    }
    else // studio console: square bezel, knurled cap, centre badge
    {
        g.setColour(juce::Colour(0xff38374c));
        g.fillRoundedRectangle(disc.expanded(1), 4);
        g.setColour(juce::Colour(0xff11141e));
        g.fillEllipse(disc.reduced(3));
        for (int i = 0; i < 24; ++i)
        {
            float t = i * juce::MathConstants<float>::twoPi / 24;
            g.setColour(juce::Colour(0xff65617b));
            g.drawLine(c.x + std::sin(t) * r * .77f, c.y - std::cos(t) * r * .77f,
                       c.x + std::sin(t) * r * .89f, c.y - std::cos(t) * r * .89f, 1);
        }
        g.setColour(colour);
        g.fillEllipse(disc.reduced(r * .55f));
    }
    if (design != 0)
        for (int i = 0; i <= 10; ++i)
        {
            float t = a + (b - a) * i / 10.f;
            g.setColour(colour.withAlpha(.7f));
            g.drawLine(c.x + std::sin(t) * r * 1.08f, c.y - std::cos(t) * r * 1.08f,
                       c.x + std::sin(t) * r * 1.17f, c.y - std::cos(t) * r * 1.17f, 1);
        }
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, r + 2, r + 2, 0, a, a + (b - a) * v, true);
    g.setColour(colour);
    g.strokePath(arc, juce::PathStrokeType(design == 2 ? 3.f : 1.5f));
    float angle = a + (b - a) * v;
    g.setColour(design <= 1 ? juce::Colour(0xff221b2c) : juce::Colour(0xfffff3d6));
    g.drawLine(c.x + std::sin(angle) * r * .3f, c.y - std::cos(angle) * r * .3f,
               c.x + std::sin(angle) * r * .77f, c.y - std::cos(angle) * r * .82f,
               std::max(2.5f, r * .06f));
    g.fillEllipse(c.x + std::sin(angle) * r * .78f - 2, c.y - std::cos(angle) * r * .78f - 2, 4, 4);
}
inline void glyph(juce::Graphics &g, int effect, juce::Rectangle<float> box, juce::Colour colour)
{
    juce::Graphics::ScopedSaveState state(g);
    g.addTransform(juce::AffineTransform::scale(box.getWidth() / 24, box.getHeight() / 24)
                       .translated(box.getX(), box.getY()));
    g.setColour(colour);
    if (effect == 0)
        for (int i = 0; i < 3; ++i)
        {
            g.setColour(colour.withAlpha(1.f - i * .22f));
            g.drawRoundedRectangle(2.f + i * 6, 3.f + i * 3, 8.f, 12.f - i * 2, 2, 1.5f);
        }
    else if (effect == 2)
        for (int i = 0; i < 3; ++i)
        {
            float radius = 4.f + i * 3;
            g.setColour(colour.withAlpha(1.f - i * .18f));
            g.drawEllipse(12 - radius, 12 - radius, radius * 2, radius * 2, 1.5f);
        }
    else if (effect == 1)
    {
        for (int i = 0; i < 3; ++i)
        {
            float x = 1.f + i * 8;
            g.drawRect(x, 4.f + i * 2, 6.f, 15.f - i * 3, 1.f);
            g.drawLine(x + 1, 12, x + 3, 8 + i * 2, 1.5f);
            g.drawLine(x + 3, 8 + i * 2, x + 5, 14, 1.5f);
        }
    }
    else if (effect == 4)
        for (int i = 0; i < 9; ++i)
            g.fillEllipse(3.f + (i * 7 % 19), 3.f + (i * 11 % 18), 2.f, 2.f);
    else
    {
        juce::Path line;
        for (int i = 0; i < 24; ++i)
        {
            float y = effect == 5 ? 18.f - ((i / 5) % 3) * 5
                                  : 12.f + std::sin(i * (effect == 3 ? .3f : 1.2f)) * 6;
            if (i == 0)
                line.startNewSubPath(0, y);
            else
                line.lineTo((float)i, y);
        }
        g.strokePath(line, juce::PathStrokeType(2));
    }
}
} // namespace visuals
