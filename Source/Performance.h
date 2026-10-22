#pragma once
#include "DanceMotion.h"
#include "Processor.h"
#include "Strings.h"
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
    float controlPulse = 0, staleTime = 0;
    unsigned lastBlock = 0;

  public:
    explicit RastroAvatar(RastroProcessor &processor) : p(processor) {}
    struct Motion
    {
        float x, y, swing, lift, activity, footLeft, footRight, armLeft, armRight, head;
    };
    void resume()
    {
        lastNote = p.engine.noteSerial.load();
        note = level = swing = swingVelocity = lift = liftVelocity = 0;
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
                dance.head};
    }
    bool update(int animationMode);
    void paint(juce::Graphics &) override;
};
