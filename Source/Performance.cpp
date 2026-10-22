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
    auto pal = visuals::palette(p);
    auto box = getLocalBounds().toFloat();
    g.setColour(pal.bg);
    g.fillRoundedRectangle(box, 12);
    g.setColour(hasKeyboardFocus(true) ? pal.secondary : pal.outline);
    g.drawRoundedRectangle(box.reduced(1), 12, hasKeyboardFocus(true) ? 2 : 1);
    auto r = box.reduced(26, 30);
    float x = p.value("playX"), y = p.value("playY");
    auto c = juce::Point<float>(r.getX() + x * r.getWidth(), r.getBottom() - y * r.getHeight());
    g.setColour(pal.text.withAlpha(.12f));
    for (int i = 0; i <= 4; ++i)
    {
        float t = i / 4.f;
        g.drawVerticalLine(juce::roundToInt(r.getX() + r.getWidth() * t), r.getY(), r.getBottom());
        g.drawHorizontalLine(juce::roundToInt(r.getY() + r.getHeight() * t), r.getX(),
                             r.getRight());
    }
    g.setColour(pal.highlight.withAlpha(.13f));
    g.fillRect(juce::Rectangle<float>(r.getX(), c.y, c.x - r.getX(), r.getBottom() - c.y));
    g.setColour(pal.highlight);
    g.drawLine(c.x, r.getY(), c.x, r.getBottom(), 1.5f);
    g.drawLine(r.getX(), c.y, r.getRight(), c.y, 1.5f);
    g.setColour(pal.secondary);
    g.fillEllipse(c.x - 9, c.y - 9, 18, 18);
    g.setColour(pal.bg);
    g.drawEllipse(c.x - 4, c.y - 4, 8, 8, 2);
    auto active = [&](int m)
    {
        return p.value(rastro::onIds[m]) > .5f && p.value(rastro::presentIds[m]) > .5f &&
               p.value(rastro::mixIds[m]) > .001f && p.value("drywet") > .001f;
    };
    bool xActive = active(3) && p.value("wDepth") > .001f;
    juce::String yNames;
    for (int m : {1, 4, 5})
        if (active(m) && (m != 1 || p.value("xyCrumble") > .5f))
        {
            if (yNames.isNotEmpty())
                yNames += " / ";
            yNames += rastro::moduleNames[m];
        }
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
    g.drawText("PLAY", 10, 3, 50, 22, juce::Justification::left);
    g.drawText("X " + juce::String(juce::roundToInt(x * 100)) + "%  Y " +
                   juce::String(juce::roundToInt(y * 100)) + "%",
               60, 3, getWidth() - 70, 22, juce::Justification::right);
    g.setFont(juce::Font(juce::FontOptions(10)));
    g.setColour(xActive ? pal.text : pal.muted);
    g.drawText(text::get("stable", p.language), int(r.getX()), int(r.getBottom()) - 16, 65, 15,
               juce::Justification::left);
    g.drawText(xActive ? "WOBBLE" : "X: " + text::get("xyOff", p.language), int(r.getRight()) - 130,
               int(r.getBottom()) - 16, 130, 15, juce::Justification::right);
    g.setColour(yNames.isNotEmpty() ? pal.text : pal.muted);
    g.drawText(text::get("degraded", p.language), int(r.getX()) + 4, int(r.getY()) + 1, 120, 14,
               juce::Justification::left);
    g.drawText("Y: " + (yNames.isEmpty() ? text::get("xyOff", p.language) : yNames), 8,
               getHeight() - 24, getWidth() - 16, 18, juce::Justification::centred);
    g.drawText(text::get("clean", p.language), int(r.getX()) + 4, int(r.getBottom()) - 31, 80, 14,
               juce::Justification::left);
}
bool RastroAvatar::update(int animationMode)
{
    bool modeChanged = mode != animationMode;
    mode = animationMode;
    unsigned serial = p.engine.noteSerial.load(std::memory_order_relaxed);
    unsigned events = serial - lastNote;
    lastNote = serial;
    float dt = mode == 1 ? 1.f / 12 : 1.f / 24;
    auto block = p.engine.blockSerial.load(std::memory_order_relaxed);
    staleTime = block == lastBlock ? staleTime + dt : 0;
    lastBlock = block;
    float change = 0;
    int ci = 0;
    auto observe = [&](const char *id)
    {
        float value = p.params.getParameter(id)->getValue();
        if (observedControls)
            change += std::abs(value - controlValues[ci]);
        controlValues[ci++] = value;
    };
    for (auto id : rastro::macroIds)
        observe(id);
    for (auto &module : rastro::controlIds)
        for (auto id : module)
            if (id)
                observe(id);
    for (auto id : {"gTiming",      "cTiming",         "wTiming",    "gDivision",  "cDivision",
                    "wDivision",    "gFreeMs",         "cFreeHz",    "wFreeHz",    "lfoTiming",
                    "lfoDivision",  "level",           "velocity",   "start",      "end",
                    "root",         "ampAttack",       "ampDecay",   "ampSustain", "ampRelease",
                    "filterCutoff", "filterResonance", "envAmount",  "lfoDepth",   "lfoRate",
                    "glide",        "pitchMode",       "triggerMode"})
        observe(id);
    observedControls = true;
    controlPulse = std::max(controlPulse, std::min(1.f, change * 2));
    controlPulse *= std::exp(-dt / .35f);
    if (controlPulse < .0001f)
        controlPulse = 0;
    if (events && mode > 0)
    {
        float velocity = p.engine.noteVelocity.load(std::memory_order_relaxed);
        if (note < .08f)
            gestureAge = 0;
        note = std::min(1.f, note + velocity * .7f);
        // Event-derived phase varies gestures even when several notes arrive per frame.
        gesturePhase = std::fmod(gesturePhase + std::min(events, 32u) * .381966f, 1.f);
        directionTarget = std::sin(gesturePhase * juce::MathConstants<float>::twoPi) * velocity;
    }
    auto follow = [dt](float &v, float target)
    {
        v += (target - v) * (1 - std::exp(-dt / .10f));
        if (std::abs(v - target) < .0001f)
            v = target;
    };
    if (mode == 0)
    {
        level = wobble = dust = crush = tail = note = swing = swingVelocity = lift = liftVelocity =
            0;
        x = y = .5f;
        dance.reset();
        effects = {};
        controlPulse = 0;
    }
    else
    {
        follow(level, staleTime < .3f ? p.engine.meter.load() : 0.f);
        for (int m = 0; m < 6; ++m)
            follow(effects[m], staleTime < .3f
                                   ? std::min(1.f, p.engine.rack.moduleActivity[m].load() * 12)
                                   : 0.f);
        float activity = std::clamp(level * 8 + note, 0.f, 1.f);
        follow(wobble, p.engine.visualWobble.load() * activity);
        follow(dust, staleTime < .3f ? std::min(1.f, p.engine.visualDust.load() * 100) : 0.f);
        follow(crush, staleTime < .3f ? std::min(1.f, p.engine.visualCrush.load() * 12) : 0.f);
        follow(tail, staleTime < .3f ? std::min(1.f, p.engine.visualBloom.load() * 8) : 0.f);
        follow(x, p.engine.visualX.load());
        follow(y, p.engine.visualY.load());
        gestureAge += dt;
        float desiredLift = gestureAge < .06f ? -.35f * note : note;
        // Stable substeps keep the anticipation/impact/recovery spring bounded at 12/24 Hz.
        for (int i = 0; i < 4; ++i)
        {
            float step = dt / 4;
            swingVelocity +=
                (directionTarget * note - swing) * 100 * step - swingVelocity * 16 * step;
            liftVelocity += (desiredLift - lift) * 140 * step - liftVelocity * 18 * step;
            swing = std::clamp(swing + swingVelocity * step, -1.f, 1.f);
            lift = std::clamp(lift + liftVelocity * step, -.5f, 1.f);
        }
        note *= std::exp(-dt / .20f);
        if (note < .0001f)
            note = 0;
        if (activity == 0 && std::abs(swing) < .0001f && std::abs(lift) < .0001f)
            swing = swingVelocity = lift = liftVelocity = 0;
    }
    if (mode > 0)
        dance.step(dt, std::min(1.f, level * 7 + note), note, x, y,
                   p.hostBpm > 0 ? p.hostBpm.load() : p.value("tempo"),
                   p.hostPlaying ? p.hostBeat.load() : -1.f, controlPulse);
    std::array<float, 32> frame{};
    std::array<float, 12> original{level, wobble, dust,  crush, tail,          note,
                                   x,     y,      swing, lift,  swingVelocity, liftVelocity};
    std::copy(original.begin(), original.end(), frame.begin());
    std::copy(effects.begin(), effects.end(), frame.begin() + 12);
    frame[18] = dance.phase;
    frame[19] = dance.footLeft;
    frame[20] = dance.footRight;
    frame[21] = dance.armLeft;
    frame[22] = dance.armRight;
    frame[23] = dance.hip;
    frame[24] = dance.shoulder;
    frame[25] = dance.head;
    frame[26] = dance.pulse;
    bool changed = modeChanged || frame != lastFrame;
    lastFrame = frame;
    if (changed)
        repaint();
    return changed;
}
void RastroAvatar::paint(juce::Graphics &g)
{
    auto pal = visuals::palette(p);
    const auto box = getLocalBounds().toFloat();
    g.setColour(pal.surface);
    g.fillRoundedRectangle(box, 14);
    g.setColour(pal.outline);
    g.drawRoundedRectangle(box.reduced(1), 14, 1);
    g.setColour(pal.secondary);
    g.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
    g.drawText(p.language == 0 ? "PERSONAGEM" : "CHARACTER", 14, 10, getWidth() - 28, 20,
               juce::Justification::left);
    const float strength = mode == 0 ? 0 : mode == 1 ? .22f : 1.f;
    const float energy = std::min(1.f, level * 7), impact = note * strength;
    const float activity = std::min(1.f, energy + note);
    float posture = (y - .5f) * 2 * activity * strength;
    float sway =
        (wobble * .17f + (x - .5f) * .28f * activity + swing * .13f + dance.hip * .12f) * strength;
    float pose = swing * strength;
    auto poseTransform =
        juce::AffineTransform::scale(1 - posture * .10f, 1 + posture * .12f).rotated(sway);
    auto actorBounds = juce::Rectangle<float>(-122, -112, 244, 248);
    auto stage = box.withTrimmedTop(34).reduced(8, 4);
    float scale = std::min(stage.getWidth() / actorBounds.getWidth(),
                           stage.getHeight() / actorBounds.getHeight());
    float cx = stage.getCentreX() - actorBounds.getCentreX() * scale;
    float cy = stage.getCentreY() - actorBounds.getCentreY() * scale - lift * 3 * strength;
    const float sceneEnergy = activity * strength;
    g.setGradientFill(juce::ColourGradient(pal.bg, box.getCentreX(), 34, pal.surface,
                                           box.getCentreX(), box.getBottom(), false));
    g.fillRoundedRectangle(stage, 10);
    // Six distinct scene cues. Their levels come from measured DSP activity, never an idle clock.
    for (int i = 3; i > 0; --i)
    {
        float aura = (tail + effects[2]) * .5f * strength;
        g.setColour(pal.highlight.withAlpha(std::min(.12f, aura * .035f)));
        g.fillEllipse(stage.reduced(float(8 + i * 3)).translated(0, -float(i * 2)));
    }
    for (int i = 0; i < 3; ++i)
    {
        float offset = (wobble * strength * 10 + dance.hip * sceneEnergy * 4) * (i + 1) / 3;
        juce::Path wave;
        float yy = stage.getY() + stage.getHeight() * (.3f + i * .22f);
        wave.startNewSubPath(stage.getX(), yy);
        wave.cubicTo(stage.getCentreX() - 35, yy + offset, stage.getCentreX() + 35, yy - offset,
                     stage.getRight(), yy);
        g.setColour(pal.control.withAlpha(
            std::min(.3f, (effects[3] + std::abs(wobble)) * .22f * strength)));
        g.strokePath(wave, juce::PathStrokeType(1.2f));
    }
    for (int i = 0; i < 9; ++i)
    {
        float xx = stage.getX() + stage.getWidth() * (i + .5f) / 9;
        g.setColour(pal.secondary.withAlpha(effects[1] * strength * .35f));
        g.drawLine(xx, stage.getBottom() - 5, xx + dance.shoulder * 8 * strength,
                   stage.getBottom() - 10 - (i % 3) * 4, 2);
        g.setColour(pal.highlight.withAlpha(effects[5] * strength * .35f));
        g.fillRoundedRectangle(
            xx, stage.getY() + 12 + (i * 29 % int(std::max(20.f, stage.getHeight() - 24))),
            3 + (i % 3) * 2, 2, 1);
    }
    g.setColour(pal.secondary.withAlpha(std::min(.25f, controlPulse * sceneEnergy * .25f)));
    g.drawEllipse(stage.reduced(9), 1.5f);
    g.setColour(pal.highlight.withAlpha(.07f));
    g.fillEllipse(stage.getCentreX() - scale * 75, stage.getBottom() - 7, scale * 150, 7);
    g.setColour(pal.highlight.withAlpha(.16f));
    g.drawLine(16, stage.getBottom(), getWidth() - 16.f, stage.getBottom(), 1);
    juce::Graphics::ScopedSaveState state(g);
    g.addTransform(poseTransform.scaled(scale).translated(cx, cy));
    const int skin = (int)p.value("avatarColour"), bodyStyle = (int)p.value("avatarBody"),
              presentation = int(p.value("avatarPresentation"));
    auto colour = skin ? visuals::accent(skin - 1) : pal.highlight;
    auto dark = juce::Colour(0xff191c2a), metal = juce::Colour(0xffe7dbc1);
    auto trim = pal.secondary;
    auto line = [&](float x1, float y1, float x2, float y2, float width, juce::Colour c)
    {
        g.setColour(c);
        g.drawLine(x1, y1, x2, y2, width);
    };
    // GHOST measured echo trails; BLOOM remains the broad stage aura above.
    for (int i = 3; i > 0; --i)
    {
        g.setColour(trim.withAlpha(effects[0] * .10f * strength));
        g.fillRoundedRectangle(-36 + i * 13, -61 + i * 3, 76, 119, 20);
    }
    int tailPart = (int)p.value("avatarTail");
    if (tailPart)
    {
        juce::Path cable;
        cable.startNewSubPath(29, 50);
        if (tailPart == 1)
            cable.cubicTo(106, 87, 110, -2, 74, 3);
        else
        {
            cable.lineTo(89, 56);
            cable.lineTo(69, 32);
            cable.lineTo(103, 15);
        }
        g.setColour(trim);
        g.strokePath(cable, juce::PathStrokeType(tailPart == 1 ? 3.f : 6.f));
        g.setColour(metal);
        g.fillRoundedRectangle(69, 0, 15, 7, 2);
        line(81, 3, 88, 3, 3, metal);
    }
    int wing = (int)p.value("avatarWings");
    if (wing)
        for (float side : {-1.f, 1.f})
        {
            juce::Path panel;
            panel.startNewSubPath(side * 29, 12);
            panel.lineTo(side * 76, -4 - impact * 7);
            panel.lineTo(side * (wing == 1 ? 88 : 72), 49);
            panel.lineTo(side * 31, 55);
            panel.closeSubPath();
            g.setColour(wing == 1 ? dark.brighter(.12f) : metal.darker(.2f));
            g.fillPath(panel);
            for (int i = 0; i < 4; ++i)
                line(side * (37 + i * 8), 14 - i * 2, side * (40 + i * 10), 48, 1,
                     trim.withAlpha(.65f));
        }
    // Articulated hips, knees and weighted platform soles. Each foot has its own gait.
    for (float side : {-1.f, 1.f})
    {
        float step = (side < 0 ? dance.footLeft : dance.footRight) * strength;
        float fx = side * 25 + step * 12, fy = 111 - std::max(0.f, step) * 14,
              kx = side * 23 + step * 5, ky = 78 - std::max(0.f, step) * 6;
        juce::Path leg;
        leg.startNewSubPath(side * 17, 48);
        leg.lineTo(kx, ky);
        leg.lineTo(fx, fy - 4);
        g.setColour(pal.text.withAlpha(.7f));
        g.strokePath(leg, juce::PathStrokeType(23, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        g.setGradientFill(juce::ColourGradient(dark.brighter(.35f), -20, 48, dark, 35, 111, false));
        g.strokePath(leg, juce::PathStrokeType(20, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        line(side * 17, 53, kx, ky, 1.2f, trim.withAlpha(.6f));
        line(kx, ky, fx, fy - 8, 1.2f, trim.withAlpha(.6f));
        g.setColour(metal);
        g.fillRoundedRectangle(fx - 15, fy - 5, 35, 10, 3);
        g.setColour(dark);
        g.fillRoundedRectangle(fx - 16, fy + 2, 37, 5, 2);
        line(fx - 9, fy - 1, fx + 11, fy - 1, 1, dark);
    }
    int instrument = (int)p.value("avatarInstrument");
    for (float side : {-1.f, 1.f})
    {
        float arm = (side < 0 ? dance.armLeft : dance.armRight) * strength;
        float shoulderY = 17 + side * dance.shoulder * 4 * strength;
        float handY = instrument ? 40 + arm * 3 : 22 - arm * 27 - impact * 3;
        float handX = instrument ? side * 48 : side * 66 + arm * 11;
        float elbowX = side * 48 + arm * 5, elbowY = 34 - arm * 12;
        line(side * 30, shoulderY, elbowX, elbowY, 15, pal.text.withAlpha(.75f));
        line(side * 30, shoulderY, elbowX, elbowY, 12, dark);
        line(elbowX, elbowY, handX, handY, 12, pal.text.withAlpha(.75f));
        line(elbowX, elbowY, handX, handY, 10, colour.darker(.15f));
        line(elbowX - 2, elbowY - 2, handX - 2, handY - 2, 1.5f, metal.withAlpha(.5f));
        g.setColour(metal);
        g.fillRoundedRectangle(handX - 6, handY - 5, 12, 11, 4);
        line(handX - 3, handY - 2, handX + 3, handY - 2, 1, dark);
    }
    juce::Path torso;
    float shoulders = presentation == 1 ? 35.f : presentation == 2 ? 28.f : 31.f;
    float waist = presentation == 2 ? 28.f : 36.f;
    torso.startNewSubPath(-shoulders, 11);
    torso.quadraticTo(0, 2, shoulders, 11);
    torso.lineTo(waist, 50);
    torso.quadraticTo(0, 64, -waist, 50);
    torso.closeSubPath();
    const int clothes = (int)p.value("avatarClothes");
    g.setGradientFill(juce::ColourGradient(clothes == 1 ? colour.darker(.1f) : dark.brighter(.27f),
                                           -30, 10, dark, 34, 57, false));
    g.fillPath(torso);
    g.setColour(colour);
    g.strokePath(torso, juce::PathStrokeType(1.5f));
    // Lapels, piping and hardware are costume pieces, not floating decoration.
    juce::Path lapel;
    lapel.startNewSubPath(-23, 11);
    lapel.lineTo(-4, 34);
    lapel.lineTo(-12, 13);
    lapel.startNewSubPath(23, 11);
    lapel.lineTo(4, 34);
    lapel.lineTo(12, 13);
    g.setColour(clothes == 1 ? metal : trim);
    g.strokePath(lapel, juce::PathStrokeType(3));
    line(0, 29, 0, 52, 1.5f, metal.withAlpha(.7f));
    line(-28, 49, 28, 49, 4, colour);
    g.setColour(metal);
    g.fillRoundedRectangle(-5, 46, 10, 6, 1);
    if (clothes == 2)
    {
        g.setColour(colour);
        g.fillRoundedRectangle(-24, 29, 15, 12, 2);
        g.fillRoundedRectangle(9, 29, 15, 12, 2);
        line(-22, 32, -11, 32, 1, metal);
        line(11, 32, 22, 32, 1, metal);
    }
    else
    {
        g.setColour(trim);
        g.fillEllipse(14, 27, 6, 6);
        line(-22, 36, -12, 36, 2, metal);
    }
    g.setColour(metal);
    g.fillRoundedRectangle(-7, -4, 14, 21, 4);
    {
        juce::Graphics::ScopedSaveState headPose(g);
        g.addTransform(juce::AffineTransform::translation(0, 40)
                           .scaled(.78f)
                           .rotated(dance.head * .13f * strength)
                           .translated(dance.shoulder * 2 * strength,
                                       -40 - std::abs(dance.head) * 2 * strength));
        juce::Path head;
        if (bodyStyle == 0)
        {
            head.startNewSubPath(-36, -51);
            head.quadraticTo(-39, -70, -10, -73);
            head.quadraticTo(36, -77, 39, -48);
            head.lineTo(33, -9);
            head.quadraticTo(2, 18, -29, -8);
            head.closeSubPath();
        }
        else if (bodyStyle == 1)
        {
            head.startNewSubPath(-38, -47);
            head.cubicTo(-36, -82, 25, -86, 38, -50);
            head.cubicTo(49, -13, 15, 9, -15, 4);
            head.quadraticTo(-43, -4, -38, -47);
            head.closeSubPath();
        }
        else
        {
            head.addRoundedRectangle(-39, -69, 78, 77, 12 - crush * strength * 7);
        }
        g.setGradientFill(juce::ColourGradient(colour.brighter(.28f), -31, -65, colour.darker(.22f),
                                               37, 5, false));
        g.fillPath(head);
        g.setColour(dark);
        g.strokePath(head, juce::PathStrokeType(2.5f));
        g.setColour(metal.withAlpha(.7f));
        g.strokePath(head, juce::PathStrokeType(1.2f));
        line(-29, -53, -29, -29, 2, metal.withAlpha(.45f));
        int horns = (int)p.value("avatarHorns");
        if (horns)
            for (float side : {-1.f, 1.f})
            {
                juce::Path fin;
                fin.startNewSubPath(side * 25, -61);
                fin.lineTo(side * (horns == 1 ? 36 : 52), -88);
                fin.lineTo(side * 44, -45);
                fin.closeSubPath();
                g.setGradientFill(juce::ColourGradient(metal, -40, -83, dark, 50, -45, false));
                g.fillPath(fin);
                line(side * 30, -63, side * 36, -77, 1, trim);
            }
        int hair = (int)p.value("avatarHair");
        if (hair)
        {
            juce::Path h;
            h.startNewSubPath(-37, -45);
            h.cubicTo(-50, -94, 27, -100, 40, -61);
            if (hair == 1)
                h.cubicTo(20, -70, -6, -57, -37, -45);
            else
            {
                h.lineTo(30, -26);
                h.lineTo(16, -61);
                h.quadraticTo(-6, -52, -37, -45);
            }
            h.closeSubPath();
            g.setColour(dark);
            g.fillPath(h);
            g.setColour(pal.text.withAlpha(.65f));
            g.strokePath(h, juce::PathStrokeType(1.2f));
            juce::Path glint;
            glint.startNewSubPath(-27, -69);
            glint.quadraticTo(-3, -88, 23, -72);
            g.setColour(trim.withAlpha(.75f));
            g.strokePath(glint, juce::PathStrokeType(2));
        }
        const int eyes = (int)p.value("avatarEyes");
        auto eye = [&](float ex, float width)
        {
            const float opening = 9 + impact * 4 + std::max(0.f, posture) * 3;
            g.setColour(dark);
            g.fillRoundedRectangle(ex - width / 2, -40, width, opening, eyes == 2 ? 1.f : 5.f);
            g.setColour(metal);
            g.fillRoundedRectangle(ex - 2 + (x - .5f) * 5, -39, 4, opening - 2, 1);
            line(ex - width / 2, -44, ex + width / 2, -46 + posture * 2, 2, dark);
        };
        if (eyes == 1)
            eye(0, 48);
        else
        {
            eye(-17, 22);
            eye(17, 22);
        }
        const int glasses = (int)p.value("avatarGlasses");
        if (glasses)
        {
            if (glasses == 1)
            {
                for (float side : {-1.f, 1.f})
                {
                    auto lens = juce::Rectangle<float>(side * 18 - 15, -44, 30, 20);
                    g.setColour(dark);
                    g.fillRoundedRectangle(lens, 8);
                    g.setColour(metal);
                    g.drawRoundedRectangle(lens, 8, 1.5f);
                    line(side * 18 - 7, -41, side * 18 + 5, -39, 1.5f, trim);
                }
                line(-3, -39, 3, -39, 2, metal);
            }
            else
            {
                g.setColour(dark);
                g.fillRoundedRectangle(-37, -46, 74, 22, 6);
                line(-29, -40, 24, -40, 2, trim);
                line(-25, -35, -12, -35, 1, metal);
            }
        }
        juce::Path smile;
        smile.startNewSubPath(-8, -12);
        smile.quadraticTo(3, -6 + energy * strength * 3, 18, -16 - posture * 2);
        g.setColour(dark);
        g.strokePath(smile, juce::PathStrokeType(2.5f));
        line(15, -19, 21, -16, 1.5f, dark);
        int accessory = (int)p.value("avatarAccessory");
        if (accessory == 0)
        {
            g.setColour(metal);
            g.drawEllipse(-32, -87, 65, 9, 2);
            line(-23, -82, -27, -69, 1, metal);
            line(24, -82, 28, -69, 1, metal);
        }
        else if (accessory == 1)
        {
            juce::Path band;
            band.startNewSubPath(-43, -28);
            band.cubicTo(-54, -92, 54, -92, 43, -28);
            g.setColour(dark);
            g.strokePath(band, juce::PathStrokeType(8));
            g.setColour(metal);
            g.strokePath(band, juce::PathStrokeType(2));
            for (float side : {-1.f, 1.f})
            {
                auto cup = juce::Rectangle<float>(side * 43 - 7, -44, 14, 28);
                g.setColour(dark);
                g.fillRoundedRectangle(cup.expanded(2), 5);
                g.setColour(metal);
                g.fillRoundedRectangle(cup.reduced(2), 4);
                line(side * 43, -37, side * 43, -24, 2, colour);
            }
        }
        else
        {
            line(-26, 10, 24, 10, 6, trim);
            juce::Path silk;
            silk.startNewSubPath(15, 13);
            silk.lineTo(32, 35);
            silk.lineTo(24, 40);
            silk.lineTo(12, 17);
            silk.closeSubPath();
            g.setColour(trim);
            g.fillPath(silk);
            line(17, 17, 26, 34, 1, metal);
        }
    } // articulated head and its freely combinable accessories
    if (instrument == 1)
    {
        line(-24, 16, 37, 49, 4, metal);
        juce::Graphics::ScopedSaveState keys(g);
        g.addTransform(juce::AffineTransform::rotation(-.20f));
        juce::Path keytar;
        keytar.addRoundedRectangle(-51, 31, 99, 29, 5);
        keytar.addRoundedRectangle(39, 24, 29, 17, 4);
        g.setColour(dark);
        g.fillPath(keytar);
        g.setColour(metal);
        g.strokePath(keytar, juce::PathStrokeType(1.5f));
        g.setColour(metal);
        g.fillRect(-42, 37, 70, 18);
        for (int i = 0; i <= 10; ++i)
        {
            line(-42 + i * 7, 37, -42 + i * 7, 55, 1, dark);
            if (i < 10 && i % 7 != 2 && i % 7 != 6)
                line(-38 + i * 7, 37, -38 + i * 7, 47, 4, dark);
        }
        g.setColour(trim);
        g.fillRoundedRectangle(34, 45, 9, 5, 1);
        line(48, 30, 58, 30, 2, colour);
        g.setColour(colour);
        g.fillEllipse(-51, 41, 9, 9);
        g.fillEllipse(54, 26, 9, 9);
    }
    else if (instrument == 2)
    {
        g.setColour(dark);
        g.fillRoundedRectangle(-43, 31, 86, 35, 5);
        g.setColour(metal);
        g.drawRoundedRectangle(-43, 31, 86, 35, 5, 1.5f);
        for (int row = 0; row < 2; ++row)
            for (int col = 0; col < 4; ++col)
            {
                g.setColour((row + col) % 2 ? trim : colour);
                g.fillRoundedRectangle(-21 + col * 14, 37 + row * 12, 10, 9, 2);
            }
        g.setColour(metal);
        g.fillRoundedRectangle(-36, 38, 8, 5, 1);
        g.fillEllipse(-36, 50, 6, 6);
    }
    if (crush * strength > .1f)
    {
        g.setColour(dark.withAlpha(crush * strength * .5f));
        for (int i = 0; i < 7; ++i)
            g.fillRect(-38.f + (i * 17 % 70), -48.f + i * 14, 8.f, 4.f);
    }
    g.setColour(visuals::text.withAlpha(std::min(.85f, dust * strength)));
    for (int i = 0; i < 18; ++i)
    {
        float a = i * 2.39996f + wobble * .5f + dance.phase * dust * strength;
        g.fillRect(std::cos(a) * (85 + i % 3 * 6), std::sin(a) * (78 + i % 3 * 7), 2.f + i % 2,
                   2.f);
    }
}
