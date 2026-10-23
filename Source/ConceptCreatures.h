#pragma once
#include "DanceMotion.h"
#include "StageWardrobe.h"
// Four non-human silhouettes derived from the attached turnaround/performance studies.
// All paths/joints are drawn on the message thread. Sound parameters are never written here.
inline void drawConceptCreature(juce::Graphics &g, RastroProcessor &p,
                                const rastro::DanceMotion &dance, float strength, float impact,
                                float energy, float gaze)
{
    using namespace wardrobe;
    int model = int(p.value("creatureModel")), pants = int(p.value("stagePants")),
        shoes = int(p.value("stageShoes"));
    bool fuzz = model == 1, soul = model == 2, curls = model == 3, fin = model == 4;
    auto skin = fuzz    ? juce::Colour(0xff32dbe8)
                : soul  ? juce::Colour(0xffa22a62)
                : curls ? juce::Colour(0xff9861ed)
                        : juce::Colour(0xff35cfdf);
    if (p.value("avatarColour") > .5f)
        skin = visuals::accent(int(p.value("avatarColour")) - 1);
    if (p.value("avatarMaterial") > .5f && p.value("avatarMaterial") < 1.5f)
        skin = skin.interpolatedWith(juce::Colour(0xffbdc9d2), .5f);
    float form = p.value("avatarBody") + p.value("avatarPresentation") * .25f;
    auto fur = [&](juce::Rectangle<float> box, int spikes = 52)
    {
        juce::Path path;
        for (int i = 0; i <= spikes * 2; ++i)
        {
            float a = i * juce::MathConstants<float>::twoPi / (spikes * 2),
                  t = (i % 2 ? 1.f : .95f);
            auto point =
                juce::Point<float>(box.getCentreX() + std::sin(a) * box.getWidth() * .5f * t,
                                   box.getCentreY() - std::cos(a) * box.getHeight() * .5f * t);
            if (i == 0)
                path.startNewSubPath(point);
            else
                path.lineTo(point);
        }
        path.closeSubPath();
        return path;
    };
    if (p.value("avatarTail") > .5f)
    {
        juce::Path tail;
        tail.startNewSubPath(27, 55);
        tail.cubicTo(95, 98, 88, 32, 70, 38 + dance.hip * 6 * strength);
        g.setColour(ink);
        g.strokePath(tail, juce::PathStrokeType(9));
        g.setColour(skin);
        g.strokePath(tail, juce::PathStrokeType(5));
    }
    if (p.value("avatarWings") > .5f)
        for (float side : {-1.f, 1.f})
        {
            juce::Path cape;
            cape.startNewSubPath(side * 30, 6);
            cape.quadraticTo(side * 84, 9, side * 78, 65);
            cape.lineTo(side * 27, 51);
            cape.closeSubPath();
            outline(g, cape, p.value("avatarWings") > 1.5f ? cream : purple);
        }
    // Legs stay rooted under the torso while weight, knees and feet move independently.
    for (float side : {-1.f, 1.f})
    {
        float step = (side < 0 ? dance.footLeft : dance.footRight) * strength;
        float x = side * (fuzz ? 27.f : 25.f) + step * (soul ? 21.f : 15.f),
              y = 113 - std::max(0.f, step) * (fuzz ? 17 : 13);
        float knee = side * 23 + step * 9, flare = pants == 2 ? 22 : 16;
        juce::Path leg;
        leg.startNewSubPath(side * 5, 52);
        leg.lineTo(side * 36, 52);
        leg.quadraticTo(knee + side * 12, 80, x + side * flare, y);
        leg.quadraticTo(x, y + 5, x - side * flare, y);
        leg.quadraticTo(knee - side * 12, 82, side * 5, 52);
        leg.closeSubPath();
        outline(g, leg, pants == 2 ? juce::Colour(0xff5b1c39) : pants ? ink : skin.darker(.12f));
        if (pants == 4)
        {
            g.setColour(orange);
            g.drawLine(side * 34, 56, x + side * 15, y - 3, 5);
        }
        else
        {
            juce::Graphics::ScopedSaveState clipped(g);
            g.reduceClipRegion(leg);
            legDetails(g, pants, knee, y, side);
        }
        if (shoes)
            shoe(g, shoes, x, y, side);
        else
        {
            g.setColour(skin);
            g.fillRoundedRectangle(x - 18, y - 5, 40, 15, 7);
            g.setColour(ink);
            g.drawRoundedRectangle(x - 18, y - 5, 40, 15, 7, 1.5f);
            for (int toe = 0; toe < 3; ++toe)
                g.drawLine(x - 8 + toe * 9, y + 3, x - 8 + toe * 9, y + 8, 1);
        }
    }
    juce::Graphics::ScopedSaveState upper(g);
    g.addTransform(juce::AffineTransform::rotation(dance.torso * .15f * strength, 0, 57)
                       .translated(dance.hip * 6 * strength, dance.bounce * 9 * strength));
    int edition = int(p.value("instrumentEdition"));
    int instrument = edition ? 1 : int(p.value("avatarInstrument"));
    float tilt = (edition == 3 ? .03f : -.15f) + dance.shoulder * .08f * strength;
    auto instrumentPose = juce::AffineTransform::rotation(tilt, 0, 46);
    auto hand = [&](int which)
    {
        float arm = (which == 0 ? dance.armLeft : dance.armRight) * strength;
        if (!instrument)
            return juce::Point<float>((which == 0 ? -66.f : 66.f) + arm * 10, 32 - arm * 24);
        if (instrument == 2 || edition == 3)
            return juce::Point<float>(which == 0 ? -22.f : 22.f,
                                      45 + which * 9 - std::max(0.f, arm) * 3)
                .transformedBy(instrumentPose);
        return juce::Point<float>(which == 0 ? -17.f + arm * 7 : 54.f,
                                  which == 0 ? 45.f - std::max(0.f, arm) * 2 : 33.f)
            .transformedBy(instrumentPose);
    };
    auto torso = fur({-45 - form * 2, -4, 90 + form * 4, 83}, fuzz ? 30 : 16);
    outline(g, torso, skin.darker(.08f));
    for (int which = 0; which < 2; ++which)
    {
        float side = which == 0 ? -1.f : 1.f;
        auto wrist = hand(which);
        float shoulderY = 10 + side * dance.shoulder * 5 * strength;
        auto elbow = juce::Point<float>(
            side * 58, 33 + (which == 0 ? dance.armLeft : dance.armRight) * strength * 6);
        juce::Path arm;
        arm.startNewSubPath(side * 34, shoulderY);
        arm.quadraticTo(elbow.x, elbow.y, wrist.x, wrist.y);
        g.setColour(ink);
        g.strokePath(arm, juce::PathStrokeType(fuzz ? 24 : 21, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        auto sleeve = p.value("stageJacket") > .5f   ? ink
                      : p.value("stageShirt") > 1.5f ? cream
                      : p.value("stageShirt") > .5f  ? juce::Colour(0xff752538)
                                                     : skin;
        g.setGradientFill(juce::ColourGradient(sleeve.brighter(.4f), side * 34, shoulderY,
                                               sleeve.darker(.1f), wrist.x, wrist.y, false));
        g.strokePath(arm, juce::PathStrokeType(fuzz ? 20 : 17, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        if (p.value("stageJacket") > .5f)
        {
            g.setColour(p.value("stageJacket") == 4 ? orange : cyan);
            g.drawLine(side * 36, shoulderY - 4, elbow.x, elbow.y - 3, 2);
        }
        g.setColour(skin);
        g.fillEllipse(wrist.x - 9, wrist.y - 8, 18, 17);
    }
    top(g, p, torso);
    if (p.value("stageHeadwear") < .5f && p.value("stageShirt") < .5f &&
        p.value("stageJacket") < .5f)
        badge(g, int(p.value("stagePatch")), 24, 20, 6);
    if (p.value("stageLanyard") > .5f)
    {
        juce::Path loop;
        loop.startNewSubPath(34, 57);
        loop.cubicTo(84, 78, 69, 109, 42, 81);
        g.setColour(orange);
        g.strokePath(loop, juce::PathStrokeType(4));
        g.setColour(gold);
        g.drawRect(32, 52, 8, 10, 1);
    }
    if (p.value("stageJewelry") > .5f)
    {
        juce::Path chain;
        chain.startNewSubPath(-18, 1);
        chain.quadraticTo(0, 33, 18, 1);
        g.setColour(gold);
        g.strokePath(chain, juce::PathStrokeType(2));
        g.fillEllipse(-5, 17, 10, 10);
    }
    // Species-specific head anatomy and natural hair are part of the base creature, never glasses.
    {
        juce::Graphics::ScopedSaveState headPose(g);
        g.addTransform(
            juce::AffineTransform::rotation(dance.head * .15f * strength, 0, -10)
                .translated(dance.shoulder * 2 * strength, -std::abs(dance.head) * 2 * strength));
        if (soul)
        {
            g.setColour(ink);
            g.fillEllipse(-69, -123, 138, 108);
            for (int row = 0; row < 6; ++row)
                for (int col = 0; col < 10; ++col)
                {
                    float x = -55 + col * 12 + (row % 2) * 4, y = -114 + row * 12;
                    if (x * x / 3600 + (y + 74) * (y + 74) / 2200 > 1)
                        continue;
                    g.setColour(juce::Colour(0xff3d2946));
                    g.drawEllipse(x - 4, y - 4, 8, 8, 1);
                }
            for (int i = 0; i < 39; ++i)
            {
                float a = i * juce::MathConstants<float>::twoPi / 39;
                float x = std::sin(a) * 49, y = -64 + std::cos(a) * 42;
                g.setColour(orange.darker(.15f));
                g.fillEllipse(x - 12, y - 13, 24, 25);
                g.setColour(ink);
                g.fillEllipse(x - 11, y - 12, 22, 24);
                g.setColour(purple.withAlpha(.16f));
                g.drawEllipse(x - 8, y - 8, 12, 12, 1);
            }
        }
        if (curls)
        {
            for (int i = 0; i < 22; ++i)
            {
                float a = i * juce::MathConstants<float>::twoPi / 22;
                float x = std::sin(a) * 52, y = -47 + std::cos(a) * 45;
                juce::Path curl;
                curl.startNewSubPath(x * .8f, -45 + (y + 45) * .7f);
                curl.cubicTo(x * 1.5f, y - 25, x * 1.45f, y + 20, x, y + 10);
                g.setColour(ink);
                g.strokePath(curl, juce::PathStrokeType(13));
                g.setColour(i % 3 == 0 ? orange : purple);
                g.strokePath(curl, juce::PathStrokeType(7));
                g.setColour(ink);
                g.fillEllipse(x - 4, y + 3, 8, 8);
            }
        }
        juce::Path head;
        if (fuzz)
            head = fur({-57, -93, 114, 92}, 45);
        else
        {
            head.startNewSubPath(-43, -67);
            head.quadraticTo(-53, -88, -8, -90);
            head.quadraticTo(44, -90, 47, -54);
            head.quadraticTo(47, -15, 9, -6);
            head.quadraticTo(-35, -3, -43, -67);
            head.closeSubPath();
        }
        outline(g, head, skin);
        if (soul || fin)
        {
            for (float side : {-1.f, 1.f})
            {
                juce::Path ear;
                ear.startNewSubPath(side * 35, -56);
                ear.lineTo(side * 66, -70);
                ear.lineTo(side * 47, -34);
                ear.closeSubPath();
                outline(g, ear, skin.darker(.2f));
                g.setColour(gold);
                g.drawLine(side * 42, -51, side * 60, -64, 1);
            }
        }
        if (fin)
        {
            for (int i = 0; i < 7; ++i)
            {
                float x = -39 + i * 12;
                juce::Path crest;
                crest.startNewSubPath(x, -67);
                crest.cubicTo(x - 18, -100, x + 25, -119 - i % 2 * 5, x + 10, -90);
                crest.quadraticTo(x + 15, -75, x, -67);
                outline(g, crest, ink);
                g.setColour(purple.withAlpha(.4f));
                g.drawLine(x, -77, x + 12, -95, 1);
            }
            g.setColour(skin);
            g.fillRoundedRectangle(-12, -106, 16, 44, 7);
            g.setColour(orange);
            g.fillEllipse(-9, -98, 7, 10);
        }
        if (fuzz)
        {
            g.setColour(cream);
            g.fillEllipse(-27, -78, 54, 57);
            g.setColour(ink);
            g.drawEllipse(-27, -78, 54, 57, 2);
            g.fillEllipse(-12 + (gaze - .5f) * 9, -69, 29, 40);
            g.setColour(juce::Colour(0xff2775aa));
            g.fillEllipse(-6 + (gaze - .5f) * 9, -66, 18, 32);
            g.setColour(ink);
            g.fillEllipse(-2 + (gaze - .5f) * 9, -64, 12, 27);
            g.setColour(cream);
            g.fillEllipse(0, -65, 8, 10);
        }
        else
            for (float side : {-1.f, 1.f})
            {
                float x = side * 20, y = -52;
                g.setColour(soul ? gold : cream);
                g.fillEllipse(x - 15, y - 13, 30, 27);
                g.setColour(ink);
                g.fillEllipse(x - 4 + (gaze - .5f) * 5, y - 11, 9, 23);
                g.setColour(cream);
                g.fillEllipse(x + 1, y - 9, 4, 5);
                if (curls || fin)
                {
                    g.setColour(skin.darker(.12f));
                    juce::Path lid;
                    lid.addTriangle(x - 17, y - 14, x + 17, y - 14, x + (side < 0 ? -8 : 8), y + 1);
                    g.fillPath(lid);
                }
            }
        {
            juce::Graphics::ScopedSaveState mouthPose(g);
            if (!fuzz)
                g.addTransform(juce::AffineTransform::scale(.8f, .7f, 0, -17));
            juce::Path mouth;
            mouth.startNewSubPath(-24, -25);
            mouth.quadraticTo(2, -4, 28, -28);
            mouth.quadraticTo(11, 4, -17, -8);
            mouth.closeSubPath();
            outline(g, mouth, ink, 1);
            g.setColour(cream);
            for (int tooth = 0; tooth < 4; ++tooth)
            {
                float x = -18 + tooth * 11;
                juce::Path t;
                t.addTriangle(x, -20, x + 7, -18, x + 4, -12 + impact * 2);
                g.fillPath(t);
            }
            g.setColour(pink);
            g.fillEllipse(-2, -9, 15, 5);
        }
        if (p.value("stageEarring") > .5f)
        {
            g.setColour(gold);
            g.drawEllipse(47, -34, 9, 13, 1.5f);
            star(g, 52, -19, 6, gold);
        }
        if (p.value("avatarHalo") > .5f)
        {
            g.setColour(gold);
            g.drawEllipse(-34, -131, 68, 9, 1.5f);
        }
        hat(g, p, fuzz ? 53 : 48);
        if (p.value("stageHeadphones") > .5f || int(p.value("avatarAccessory")) == 1)
            phones(g, fuzz ? 57 : 53);
        int eyewear = p.value("glassesEdition") > .5f ? 3 : int(p.value("avatarGlasses"));
        glasses(g, eyewear, fuzz);
    }
    if (int(p.value("avatarAccessory")) == 2)
    {
        g.setColour(purple);
        g.fillRoundedRectangle(-23, -2, 46, 9, 3);
        g.fillRoundedRectangle(11, 0, 12, 33, 3);
    }
    if (instrument)
    {
        juce::Graphics::ScopedSaveState instrumentState(g);
        g.addTransform(instrumentPose);
        wardrobe::instrument(g, edition ? edition : instrument == 2 ? 3 : 2, energy);
    }
    // Wrists are reused exactly after painting the instrument; no disconnected floating hands.
    for (int which = 0; which < 2; ++which)
    {
        auto wrist = hand(which);
        g.setColour(ink);
        g.fillRoundedRectangle(wrist.x - 10, wrist.y - 7, 21, 15, 6);
        g.setColour(skin);
        g.fillRoundedRectangle(wrist.x - 9, wrist.y - 6, 19, 13, 5);
        float tap = std::max(0.f, (which == 0 ? dance.armLeft : dance.armRight) * strength);
        for (int finger = 0; finger < 3; ++finger)
        {
            g.setColour(skin.darker(.35f));
            g.drawLine(wrist.x - 5 + finger * 5, wrist.y, wrist.x - 4 + finger * 5,
                       wrist.y + 5 + tap * 2, 1);
        }
    }
}
