#pragma once
#include "DanceMotion.h"
#include "Processor.h"
#include "Visuals.h"
// Original vector costume/creature artwork, drawn only on the message thread.
inline void drawRastroCharacter(juce::Graphics &g, RastroProcessor &p,
                                const rastro::DanceMotion &dance, float strength, float impact,
                                float energy, float posture, float gaze)
{
    auto pal = visuals::palette(p);
    int skin = int(p.value("avatarColour")), body = int(p.value("avatarBody")),
        presentation = int(p.value("avatarPresentation")), clothes = int(p.value("avatarClothes")),
        instrument = int(p.value("avatarInstrument"));
    auto enamel = skin ? visuals::accent(skin - 1) : pal.highlight;
    int species = int(p.value("avatarSpecies")), finish = int(p.value("avatarMaterial"));
    if (finish == 1)
        enamel = enamel.interpolatedWith(juce::Colour(0xffb8c7da), .7f);
    auto ink = juce::Colour(0xff171925), shadow = juce::Colour(0xff0d1020),
         silver = juce::Colour(0xffe8ddc7), trim = pal.secondary;
    auto fabric = ink.interpolatedWith(enamel, finish == 2 ? .55f : clothes == 1 ? .42f : .16f);
    auto stroke = [&](const juce::Path &path, juce::Colour c, float width)
    {
        g.setColour(c);
        g.strokePath(path, juce::PathStrokeType(width, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    };
    auto line = [&](float a, float b, float c, float d, float width, juce::Colour colour)
    {
        g.setColour(colour);
        g.drawLine(a, b, c, d, width);
    };
    auto material =
        [&](juce::Path path, juce::Colour a, juce::Colour b, float x1, float y1, float x2, float y2)
    {
        g.setGradientFill(juce::ColourGradient(a, x1, y1, b, x2, y2, false));
        g.fillPath(path);
        stroke(path, shadow, 2);
        stroke(path, silver.withAlpha(.46f), .8f);
    };
    // Costume pieces emerge from the shoulders/hips, rather than floating around the character.
    int tail = int(p.value("avatarTail"));
    if (tail)
    {
        juce::Path cable;
        cable.startNewSubPath(23, 57);
        cable.cubicTo(85, 100, 108, 43, 79, 18 + dance.hip * 7 * strength);
        stroke(cable, shadow, tail == 1 ? 6 : 11);
        stroke(cable, tail == 1 ? trim : enamel, tail == 1 ? 3 : 7);
        g.setColour(silver);
        g.fillRoundedRectangle(72, 13 + dance.hip * 7 * strength, 15, 8, 3);
        line(85, 17 + dance.hip * 7 * strength, 91, 17 + dance.hip * 7 * strength, 2, silver);
    }
    int wings = int(p.value("avatarWings"));
    if (wings)
        for (float side : {-1.f, 1.f})
        {
            juce::Path cape;
            cape.startNewSubPath(side * 33, 13);
            cape.cubicTo(side * 59, 1, side * 77, 25, side * 83, 69);
            cape.quadraticTo(side * 55, 56, side * 27, 64);
            cape.closeSubPath();
            material(cape, wings == 1 ? fabric.brighter(.25f) : silver, shadow, -75, 5, 75, 73);
            for (int i = 0; i < 4; ++i)
                line(side * (38 + i * 7), 20, side * (35 + i * 13), 63 - i * 2, 1,
                     trim.withAlpha(.55f));
        }
    // Sculpted bell-bottom legs: continuous volume around the knee and independent weighted steps.
    for (float side : {-1.f, 1.f})
    {
        float step = (side < 0 ? dance.footLeft : dance.footRight) * strength;
        float foot = side * 30 + step * 14, y = 113 - std::max(0.f, step) * 16,
              knee = side * 26 + step * 7;
        juce::Path leg;
        leg.startNewSubPath(side * 7, 55);
        leg.quadraticTo(side * 23, 50, side * 39, 58);
        leg.quadraticTo(knee + side * 8, 78, foot + side * 19, y - 3);
        leg.quadraticTo(foot, y + 3, foot - side * 17, y - 2);
        leg.quadraticTo(knee - side * 13, 83, side * 7, 55);
        leg.closeSubPath();
        material(leg, fabric.brighter(.35f), shadow, side * 7, 60, side * 43, 114);
        juce::Path seam;
        seam.startNewSubPath(side * 23, 60);
        seam.quadraticTo(knee, 85, foot + side * 8, y - 7);
        stroke(seam, trim.withAlpha(.65f), 1.1f);
        line(knee - side * 5, 83, knee + side * 7, 87, 1, shadow.withAlpha(.6f));
        juce::Path shoe;
        shoe.startNewSubPath(foot - 17, y - 5);
        shoe.quadraticTo(foot + 8, y - 11, foot + 24, y - 1);
        shoe.quadraticTo(foot + 28, y + 3, foot + 23, y + 9);
        shoe.lineTo(foot - 20, y + 9);
        shoe.quadraticTo(foot - 23, y + 2, foot - 17, y - 5);
        shoe.closeSubPath();
        material(shoe, silver, juce::Colour(0xff777f94), foot - 10, y - 6, foot + 15, y + 9);
        line(foot - 18, y + 6, foot + 22, y + 6, 3, ink);
        for (int i = 0; i < 3; ++i)
            line(foot - 9 + i * 5, y - 2, foot - 6 + i * 5, y + 1, 1, ink);
    }
    // Padded sleeves and articulated forearms have volume and joined cuffs.
    for (float side : {-1.f, 1.f})
    {
        float arm = (side < 0 ? dance.armLeft : dance.armRight) * strength,
              shoulder = 14 + side * dance.shoulder * 5 * strength;
        float ex = side * 53 + arm * 7, ey = 39 - arm * 10,
              hx = instrument ? side * 45 : side * 69 + arm * 9,
              hy = instrument ? 45 + arm * 6 : 24 - arm * 29 - impact * 2;
        juce::Path sleeve;
        sleeve.startNewSubPath(side * 33, shoulder - 5);
        sleeve.quadraticTo(side * 48, shoulder - 9, ex + side * 8, ey - 5);
        sleeve.lineTo(ex - side * 7, ey + 10);
        sleeve.quadraticTo(side * 30, shoulder + 17, side * 33, shoulder - 5);
        sleeve.closeSubPath();
        material(sleeve, fabric.brighter(.35f), fabric.darker(.4f), side * 31, shoulder, ex,
                 ey + 8);
        juce::Path forearm;
        forearm.startNewSubPath(ex, ey);
        forearm.quadraticTo((ex + hx) / 2 - side * 6, (ey + hy) / 2 + 3, hx, hy);
        stroke(forearm, shadow, 19);
        stroke(forearm, enamel.darker(.3f), 16);
        stroke(forearm, silver.withAlpha(.35f), 2);
        line(hx - side * 5, hy + 6, hx + side * 5, hy + 6, 5, ink);
        g.setGradientFill(juce::ColourGradient(silver, hx - 6, hy - 5, enamel.darker(.4f), hx + 9,
                                               hy + 8, false));
        g.fillRoundedRectangle(hx - 8, hy - 7, 16, 15, 6);
        line(hx - 5, hy - 3, hx + 3, hy - 4, 1, shadow);
        g.setColour(silver);
        g.fillEllipse(hx - side * 9 - 3, hy - 1, 7, 8);
    }
    float shoulders = presentation == 1   ? 42.f
                      : presentation == 2 ? 35.f
                                          : 39.f,
          waist = presentation == 2 ? 25.f : 31.f;
    juce::Path coat;
    coat.startNewSubPath(-shoulders, 10);
    coat.cubicTo(-28, -1, 21, -2, shoulders, 10);
    coat.quadraticTo(shoulders + 3, 31, waist, 49);
    coat.lineTo(waist + 7, 65);
    coat.quadraticTo(0, 76, -waist - 7, 65);
    coat.lineTo(-waist, 49);
    coat.quadraticTo(-shoulders - 3, 31, -shoulders, 10);
    coat.closeSubPath();
    material(coat, fabric.brighter(.5f), shadow, -35, 3, 37, 72);
    juce::Path panel;
    panel.startNewSubPath(-27, 8);
    panel.quadraticTo(-12, 4, -3, 13);
    panel.lineTo(5, 50);
    panel.lineTo(-16, 58);
    panel.closeSubPath();
    g.setColour(enamel.withAlpha(clothes == 1 ? .7f : .2f));
    g.fillPath(panel);
    // Satin lapels, stitching, zipper, pleated sleeves, shaped hem and brushed buckle.
    for (float side : {-1.f, 1.f})
    {
        juce::Path lapel;
        lapel.startNewSubPath(side * 24, 7);
        lapel.lineTo(side * 8, 37);
        lapel.lineTo(side * 3, 18);
        lapel.lineTo(side * 11, 5);
        lapel.closeSubPath();
        material(lapel, clothes == 2 ? silver : trim, trim.darker(.5f), -20, 8, 20, 37);
        line(side * 32, 38, side * 23, 56, 1, silver.withAlpha(.5f));
        for (int i = 0; i < 3; ++i)
            line(side * (29 + i * 3), 14 + i * 2, side * (33 + i * 2), 24 + i * 2, 1, shadow);
    }
    line(0, 30, 0, 62, 1.5f, silver);
    for (int i = 0; i < 5; ++i)
        line(-2, 36 + i * 4, 2, 36 + i * 4, 1, shadow);
    line(-29, 61, 29, 61, 4, enamel.darker(.2f));
    g.setColour(silver);
    g.fillRoundedRectangle(-6, 58, 12, 7, 2);
    g.setColour(shadow);
    g.fillRoundedRectangle(-3, 60, 6, 3, 1);
    g.setColour(trim);
    g.fillRoundedRectangle(18, 40, 9, 3, 1);
    line(-26, 46, -16, 44, 2, silver);
    if (clothes == 2)
        for (float side : {-1.f, 1.f})
            for (int i = 0; i < 3; ++i)
            {
                g.setColour(silver.withAlpha(.09f));
                g.fillRoundedRectangle(side * 19 - 6, 35 + i * 7, 12, 4, 2);
            }
    g.setGradientFill(juce::ColourGradient(silver, -8, -7, enamel.darker(.5f), 10, 13, false));
    g.fillRoundedRectangle(-9, -9, 18, 26, 6);
    line(-8, 5, 8, 5, 2, shadow);
    // Head: asymmetric organic helmet, recessed face, sculpted brow and recognisable cheek fins.
    {
        juce::Graphics::ScopedSaveState headPose(g);
        g.addTransform(
            juce::AffineTransform::rotation(dance.head * .13f * strength, 0, -12)
                .translated(dance.shoulder * 2 * strength, -std::abs(dance.head) * 2 * strength));
        juce::Path head;
        head.startNewSubPath(-30, -42);
        head.cubicTo(-37, -71, -16, -91, 9, -85);
        head.cubicTo(34, -84, 43, -61, 31, -40);
        head.cubicTo(40, -10, 14, 4, -4, -2);
        head.cubicTo(-28, -3, -38, -20, -30, -42);
        head.closeSubPath();
        if (species)
        {
            head.clear();
            head.startNewSubPath(-33, -43);
            if (species == 1)
            {
                head.cubicTo(-60, -53, -68, -104, -29, -79);
                head.cubicTo(-8, -91, 9, -91, 29, -78);
                head.cubicTo(68, -103, 60, -51, 34, -42);
            }
            else
            {
                head.cubicTo(-81, -60, -48, -105, -13, -86);
                head.cubicTo(13, -110, 67, -83, 47, -54);
                head.quadraticTo(63, -26, 34, -32);
            }
            head.cubicTo(42, -13, 16, 8, -6, -3);
            head.cubicTo(-32, -2, -49, -17, -33, -43);
            head.closeSubPath();
        }
        if (body == 1)
            head.applyTransform(juce::AffineTransform::scale(1.1f, .94f));
        if (body == 2)
            head.applyTransform(juce::AffineTransform::shear(.10f, 0));
        material(head, enamel.brighter(.38f), enamel.darker(.48f), -29, -75, 34, -5);
        for (float side : {-1.f, 1.f})
        {
            juce::Path fin;
            fin.startNewSubPath(side * 28, -36);
            fin.quadraticTo(side * 50, -53, side * 37, -14);
            fin.quadraticTo(side * 31, -19, side * 28, -36);
            fin.closeSubPath();
            material(fin, species ? enamel : silver, enamel.darker(.6f), side * 31, -42, side * 40,
                     -16);
            line(side * 33, -33, side * 36, -25, 1, shadow);
        }
        juce::Path face;
        face.startNewSubPath(-25, -49);
        face.cubicTo(-7, -65, 23, -58, 28, -42);
        face.quadraticTo(34, -17, 9, -10);
        face.quadraticTo(-23, -7, -26, -32);
        face.closeSubPath();
        material(face, enamel.interpolatedWith(silver, .7f), enamel.darker(.23f), -15, -51, 22, -9);
        juce::Path glint;
        glint.startNewSubPath(-23, -62);
        glint.cubicTo(-16, -78, 4, -79, 18, -68);
        stroke(glint, silver.withAlpha(.65f), 2);
        line(-24, -57, -19, -59, 1, silver);
        auto eye = [&](float cx, float tilt)
        {
            juce::Path socket;
            float opening = 4.5f + impact * 1.5f + std::max(0.f, posture) * 1.5f;
            if (int(p.value("avatarEyes")) == 2)
            {
                socket.addRoundedRectangle(cx - 8, -40, 16, opening + 4, 1);
                g.setColour(shadow);
                g.fillPath(socket);
                for (int i = 0; i < 3; ++i)
                {
                    g.setColour(trim);
                    g.fillRect(cx - 5 + i * 4, -37.f, 2.f, opening);
                }
                return;
            }
            socket.addEllipse(cx - 8, -40, 16, opening + 4);
            g.setColour(shadow);
            g.fillPath(socket);
            g.setColour(species ? pal.secondary.interpolatedWith(silver, .5f) : silver);
            g.fillEllipse(cx - 6, -38, 12, opening);
            g.setColour(ink);
            g.fillEllipse(cx - 1 + (gaze - .5f) * 4, -39, 5, opening + 2);
            line(cx - 9, -42, cx + 7, -43 + tilt, 2.5f, ink);
        };
        if (int(p.value("avatarEyes")) == 1)
        {
            g.setColour(shadow);
            g.fillRoundedRectangle(-23, -42, 46, 12, 6);
            line(-16, -38, 16, -38, 1.6f, trim);
        }
        else
        {
            eye(-12, -1);
            eye(12, 2);
        }
        juce::Path nose;
        nose.startNewSubPath(0, -34);
        nose.quadraticTo(-5, -23, 3, -23);
        stroke(nose, enamel.darker(.65f), 1.8f);
        line(-17, -20, -11, -18, 1.2f, silver.withAlpha(.7f));
        line(14, -22, 20, -25, 1.2f, silver.withAlpha(.7f));
        juce::Path mouth;
        mouth.startNewSubPath(-6, -13);
        mouth.quadraticTo(4, -11 + energy * strength, 13, -17 - posture);
        stroke(mouth, ink, 2);
        if (species)
        {
            juce::Path fang;
            fang.startNewSubPath(-3, -12);
            fang.lineTo(1, -5);
            fang.lineTo(4, -12);
            fang.closeSubPath();
            g.setColour(silver);
            g.fillPath(fang);
            for (int i = 0; i < 3; ++i)
            {
                g.setColour(enamel.darker(.5f));
                g.fillEllipse(-29.f + i * 5, -28.f + i % 2 * 3, 2.5f, 2.5f);
            }
        }
        line(11, -18, 15, -16, 1, ink);
        int horns = int(p.value("avatarHorns"));
        if (horns)
            for (float side : {-1.f, 1.f})
            {
                juce::Path fin;
                fin.startNewSubPath(side * 20, -73);
                fin.cubicTo(side * 24, -83, side * (horns == 1 ? 27 : 42), -95, side * 31, -59);
                fin.closeSubPath();
                material(fin, silver, shadow, side * 22, -88, side * 31, -59);
            }
        int hair = int(p.value("avatarHair"));
        if (hair)
        {
            juce::Path plume;
            plume.startNewSubPath(-29, -52);
            plume.cubicTo(-52, -91, -1, -109, 29, -83);
            plume.cubicTo(38, -74, 31, -60, 27, -59);
            if (hair == 1)
                plume.cubicTo(8, -82, -10, -62, -29, -52);
            else
            {
                plume.quadraticTo(43, -53, 35, -25);
                plume.lineTo(20, -65);
                plume.quadraticTo(-4, -58, -29, -52);
            }
            plume.closeSubPath();
            material(plume, ink.brighter(.4f), shadow, -20, -88, 30, -52);
            juce::Path ridge;
            ridge.startNewSubPath(-24, -73);
            ridge.quadraticTo(-3, -96, 23, -76);
            stroke(ridge, trim.withAlpha(.8f), 1.7f);
        }
        int glasses = int(p.value("avatarGlasses"));
        if (glasses == 1)
        {
            for (float side : {-1.f, 1.f})
            {
                juce::Path lens;
                lens.startNewSubPath(side * 3, -43);
                lens.quadraticTo(side * 26, -49, side * 27, -35);
                lens.quadraticTo(side * 19, -20, side * 5, -31);
                lens.closeSubPath();
                material(lens, ink.brighter(.35f), shadow, side * 4, -43, side * 27, -29);
                line(side * 8, -40, side * 20, -38, 1.5f, trim);
            }
            line(-4, -38, 4, -38, 2, silver);
        }
        else if (glasses == 2)
        {
            juce::Path visor;
            visor.startNewSubPath(-30, -44);
            visor.quadraticTo(0, -56, 31, -43);
            visor.lineTo(26, -29);
            visor.quadraticTo(0, -21, -28, -30);
            visor.closeSubPath();
            material(visor, ink.brighter(.4f), shadow, -20, -45, 27, -26);
            line(-22, -40, 21, -37, 2, trim);
        }
        if (p.value("avatarHalo") > .5f)
        {
            g.setColour(silver);
            g.drawEllipse(-32, -100, 64, 10, 1.5f);
            line(-23, -94, -25, -80, 1, silver);
            line(23, -94, 25, -80, 1, silver);
        }
        if (int(p.value("avatarAccessory")) == 1)
        {
            juce::Path band;
            band.startNewSubPath(-34, -33);
            band.cubicTo(-45, -101, 47, -101, 35, -32);
            stroke(band, shadow, 7);
            stroke(band, silver, 2);
            for (float side : {-1.f, 1.f})
            {
                g.setColour(ink);
                g.fillRoundedRectangle(side * 35 - 7, -49, 14, 25, 6);
                g.setGradientFill(juce::ColourGradient(silver, side * 35 - 5, -48, ink,
                                                       side * 35 + 6, -24, false));
                g.fillRoundedRectangle(side * 35 - 5, -46, 10, 20, 5);
                line(side * 35, -42, side * 35, -30, 2, trim);
            }
        }
    }
    if (int(p.value("avatarAccessory")) == 2)
    {
        juce::Path scarf;
        scarf.startNewSubPath(-12, 5);
        scarf.quadraticTo(0, 14, 13, 5);
        scarf.lineTo(28, 30 + dance.armRight * 5 * strength);
        scarf.lineTo(18, 35);
        scarf.lineTo(7, 14);
        scarf.lineTo(-12, 13);
        scarf.closeSubPath();
        material(scarf, trim, silver.darker(.4f), -10, 5, 27, 33);
    }
    if (instrument)
    {
        line(-27, 10, 30, 57, 4, shadow);
        line(-26, 10, 31, 57, 1.3f, silver);
        juce::Graphics::ScopedSaveState hardware(g);
        g.addTransform(juce::AffineTransform::rotation(instrument == 1 ? -.16f : .08f, 0, 45));
        juce::Path casing;
        casing.addRoundedRectangle(-52, 32, 104, 34, 7);
        if (instrument == 1)
            casing.addRoundedRectangle(41, 26, 27, 18, 5);
        material(casing, ink.brighter(.3f), shadow, -45, 30, 48, 66);
        line(-44, 35, 37, 35, 1, trim);
        if (instrument == 1)
        {
            g.setColour(silver);
            g.fillRoundedRectangle(-43, 40, 70, 20, 2);
            for (int i = 0; i < 10; ++i)
            {
                line(-43 + i * 7, 40, -43 + i * 7, 60, .8f, ink);
                if (i % 7 != 2 && i % 7 != 6)
                    line(-39 + i * 7, 40, -39 + i * 7, 51, 4, ink);
            }
            g.setColour(trim);
            g.fillRoundedRectangle(33, 43, 11, 6, 2);
            for (int i = 0; i < 2; ++i)
            {
                g.setColour(silver);
                g.fillEllipse(33 + i * 8, 54, 4, 4);
            }
        }
        else
        {
            for (int row = 0; row < 2; ++row)
                for (int col = 0; col < 4; ++col)
                {
                    auto colour = (row + col) % 2 ? trim : enamel;
                    g.setColour(colour.withAlpha(.4f + .5f * energy));
                    g.fillRoundedRectangle(-23 + col * 16, 39 + row * 12, 12, 9, 2);
                }
            g.setColour(silver);
            g.fillRoundedRectangle(-42, 41, 12, 7, 2);
            g.setColour(ink);
            g.fillRect(-39, 43, 6, 3);
        }
        // Hands rest on the instrument, with note/energy-driven finger accents. No idle playing.
        for (int hand = 0; hand < 2; ++hand)
        {
            float hx = hand == 0 ? -20.f : 35.f, hy = 45 - impact * (hand ? 2 : 4);
            g.setColour(ink);
            g.fillRoundedRectangle(hx - 9, hy - 5, 20, 14, 5);
            g.setColour(enamel);
            g.fillRoundedRectangle(hx - 8, hy - 5, 18, 11, 5);
            for (int finger = 0; finger < 3; ++finger)
            {
                g.setColour(enamel.darker(.45f));
                g.drawLine(hx - 4 + finger * 4, hy, hx - 4 + finger * 4,
                           hy + 5 + (finger == 1 ? impact * 3 : 0), 1);
            }
        }
    }
}
