#include "Performance.h"
#include "CharacterArtwork.h"
#include "ConceptCreatures.h"
#include "SceneArtwork.h"
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
    bool appearanceChanged = false;
    for (size_t i = 0; i < seenAppearance.size(); ++i)
    {
        float value = p.value(visuals::visualIds[i]);
        appearanceChanged |= value != seenAppearance[i];
        seenAppearance[i] = value;
    }
    if (mode == 0)
        visualPreview = previewPaused = false;
    auto stop = p.stopSerial.load();
    if (seenStop != stop)
    {
        seenStop = stop;
        setPreview(false);
        modeChanged = true;
    }
    auto pause = p.pauseSerial.load();
    if (seenPause != pause)
    {
        seenPause = pause;
        visualPreview = false;
        previewPaused = false;
    }
    // Pause is a hold, not a slow decay. Off always shows the natural pose.
    if (mode > 0 && (p.playbackTransport == 1 || previewPaused))
    {
        if (appearanceChanged)
            repaint();
        return appearanceChanged;
    }
    if (p.playbackTransport == 2 && !visualPreview)
    {
        resume();
        bool changed = modeChanged || appearanceChanged || lastFrame != std::array<float, 32>{};
        lastFrame = {};
        if (changed)
            repaint();
        return changed;
    }
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
    if (visualPreview && mode > 0)
    {
        // Synthetic visual beat only: no MIDI, engine calls, audio parameters or DSP writes.
        float bpm = p.hostBpm > 0 ? p.hostBpm.load() : p.value("tempo");
        previewClock += dt * std::clamp(bpm, 30.f, 300.f) / 60;
        float beat = previewClock - std::floor(previewClock);
        level = .1f;
        note = .6f * std::exp(-beat * 9);
        x = p.value("playX");
        y = p.value("playY");
    }
    if (mode > 0)
        dance.step(dt, std::min(1.f, level * 7 + note), note, x, y,
                   p.hostBpm > 0 ? p.hostBpm.load() : p.value("tempo"),
                   p.hostPlaying ? p.hostBeat.load() : -1.f, controlPulse,
                   int(p.value("danceStyle")), p.value("danceAmount"), mode == 1);
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
    frame[27] = dance.torso;
    frame[28] = dance.bounce;
    bool changed = modeChanged || appearanceChanged || frame != lastFrame;
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

    g.setColour(pal.secondary);
    g.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
    g.drawText(p.language == 0 ? "PERSONAGEM" : "CHARACTER", 12, 4, getWidth() - 24, 18,
               juce::Justification::left);
    const float strength = (mode == 0   ? 0
                            : mode == 1 ? .22f
                                        : 1.f) *
                           std::min(1.3f, p.value("danceAmount"));
    const float energy = std::min(1.f, level * 7), impact = note * strength;
    if (visualPreview)
    {
        g.setColour(pal.secondary);
        g.drawText(previewPaused ? "PREVIEW II" : "PREVIEW", getWidth() - 104, 4, 90, 18,
                   juce::Justification::right);
    }
    const float activity = std::min(1.f, energy + note);
    float posture = (y - .5f) * 2 * activity * strength;
    float sway =
        (wobble * .17f + (x - .5f) * .28f * activity + swing * .13f + dance.hip * .12f) * strength;
    float pose = swing * strength;
    auto poseTransform =
        juce::AffineTransform::scale(1 - posture * .10f, 1 + posture * .12f).rotated(sway);
    auto actorBounds = p.value("creatureModel") > .5f
                           ? juce::Rectangle<float>(-110, -140, 220, 278)
                           : juce::Rectangle<float>(-104, -122, 208, 254);
    auto stage = box.withTrimmedTop(24).reduced(4, 2);
    float scale = std::min(stage.getWidth() / actorBounds.getWidth(),
                           stage.getHeight() / actorBounds.getHeight());
    float cx = stage.getCentreX() - actorBounds.getCentreX() * scale;
    float cy = stage.getCentreY() - actorBounds.getCentreY() * scale - lift * 3 * strength;
    const float sceneEnergy = activity * strength;
    g.setGradientFill(juce::ColourGradient(pal.bg, box.getCentreX(), 34, pal.surface,
                                           box.getCentreX(), box.getBottom(), false));
    g.fillRoundedRectangle(stage, 10);
    if (p.value("sceneKind") > .5f)
    {
        std::array<juce::uint32, 10> key{(juce::uint32)p.value("sceneKind"),
                                         (juce::uint32)p.value("sceneColour"),
                                         (juce::uint32)p.value("sceneLight"),
                                         (juce::uint32)p.value("sceneDecor"),
                                         (juce::uint32)getWidth(),
                                         (juce::uint32)getHeight(),
                                         pal.bg.getARGB(),
                                         pal.surface.getARGB(),
                                         pal.highlight.getARGB(),
                                         pal.secondary.getARGB()};
        if (!roomLayer.isValid() || key != roomKey)
        {
            roomKey = key;
            roomLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
            juce::Graphics cached(roomLayer);
            drawRastroRoom(cached, p, stage, 0, 0);
        }
        g.drawImageAt(roomLayer, 0, 0);
        float light = std::min(.25f, (sceneEnergy + controlPulse * sceneEnergy) *
                                         p.value("sceneReaction") * .12f);
        auto lightColour = p.value("sceneLight") > 1.5f  ? juce::Colour(0xffffbe70)
                           : p.value("sceneLight") > .5f ? pal.secondary
                                                         : pal.highlight;
        g.setColour(lightColour.withAlpha(light));
        g.fillEllipse(stage.getX() + stage.getWidth() * .66f,
                      stage.getY() + stage.getHeight() * .09f, stage.getWidth() * .25f,
                      stage.getHeight() * .63f);
    }
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
    if (p.value("creatureModel") > .5f)
        drawConceptCreature(g, p, dance, strength, impact, energy, x);
    else
        drawRastroCharacter(g, p, dance, strength, impact, energy, posture, x);
    auto dark = juce::Colour(0xff191c2a);
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
