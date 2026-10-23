#pragma once
#include "Processor.h"
// Playback hardware is a passive view. It owns no timer and never touches voice storage.
class SampleDeck : public juce::Component, public juce::SettableTooltipClient
{
    RastroProcessor &p;
    float angle = 0, level = 0, idleTime = .3f;
    unsigned lastBlock = 0;
    int mode = 2;
    bool playing = false;

  public:
    explicit SampleDeck(RastroProcessor &processor) : p(processor)
    {
        lastBlock = p.engine.blockSerial.load(std::memory_order_relaxed);
        setComponentID("sampleDeck");
        setInterceptsMouseClicks(true, false);
    }
    struct Motion
    {
        float angle, level;
        bool playing;
    };
    Motion motion() const { return {angle, level, playing}; }
    bool update(int animationMode);
    void resume()
    {
        lastBlock = p.engine.blockSerial.load(std::memory_order_relaxed);
        idleTime = .3f;
    }
    void paint(juce::Graphics &) override;
};
