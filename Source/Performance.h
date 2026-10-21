#pragma once
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

  public:
    explicit RastroAvatar(RastroProcessor &processor) : p(processor) {}
    void update(int animationMode);
    void paint(juce::Graphics &) override;
};
