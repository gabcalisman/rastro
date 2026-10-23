#pragma once
#include "DanceMotion.h"
#include "Processor.h"
#include "RastroStrings.h"
#include "Visuals.h"
// All animation, Paths and painting remain on JUCE's message thread.
class PlayPad : public juce::Component, public juce::SettableTooltipClient
{
    RastroProcessor &p;
    void position(juce::Point<float> point);

  public:
    explicit PlayPad(RastroProcessor &processor);
    ~PlayPad() override;
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseDoubleClick(const juce::MouseEvent &) override;
    void center();
    bool keyPressed(const juce::KeyPress &) override;

  private:
    bool dragging = false;
    void finishGesture();
};
class RastroAvatar : public juce::Component
{
    RastroProcessor &p;
    float level = 0, wobble = 0, dust = 0, crush = 0, tail = 0, note = 0, x = .5f, y = .5f;
    unsigned lastNote = 0;
    int mode = 2;
    float swing = 0, swingVelocity = 0, lift = 0, liftVelocity = 0, gesturePhase = 0,
          gestureAge = 1, directionTarget = 0;
    std::array<float, 32> lastFrame{};
    rastro::DanceMotion dance;
    std::array<float, 6> effects{};
    std::array<float, 64> controlValues{};
    bool observedControls = false;
    bool visualPreview = false, previewPaused = false;
    float previewClock = 0;
    unsigned seenStop = 0, seenPause = 0;
    std::array<float, std::size(visuals::visualIds)> seenAppearance{};
    float controlPulse = 0, staleTime = 0;
    unsigned lastBlock = 0;
    juce::Image roomLayer;
    std::array<juce::uint32, 10> roomKey{};

  public:
    explicit RastroAvatar(RastroProcessor &processor) : p(processor) {}
    struct Motion
    {
        float x, y, swing, lift, activity, footLeft, footRight, armLeft, armRight, head, phase,
            torso, bounce;
    };
    bool isPreviewing() const { return visualPreview; }
    bool isPreviewPaused() const { return previewPaused; }
    void setPreview(bool enabled)
    {
        visualPreview = enabled;
        previewPaused = false;
        previewClock = 0;
        seenStop = p.stopSerial.load();
        seenPause = p.pauseSerial.load();
        resume();
        repaint();
    }
    void pausePreview(bool paused)
    {
        previewPaused = paused;
        repaint();
    }
    void resume()
    {
        lastNote = p.engine.noteSerial.load();
        note = level = wobble = dust = crush = tail = swing = swingVelocity = lift = liftVelocity =
            0;
        effects = {};
        controlPulse = 0;
        dance.reset();
        staleTime = 1;
        lastBlock = p.engine.blockSerial.load();
    }
    Motion motion() const
    {
        return {x,
                y,
                swing,
                lift,
                std::min(1.f, level * 7 + note),
                dance.footLeft,
                dance.footRight,
                dance.armLeft,
                dance.armRight,
                dance.head,
                dance.phase,
                dance.torso,
                dance.bounce};
    }
    bool update(int animationMode);
    void paint(juce::Graphics &) override;
};
