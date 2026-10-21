#pragma once
#include "Processor.h"
#include "Strings.h"
class SampleView : public juce::Component, public juce::SettableTooltipClient
{
    RastroProcessor &p;
    juce::String dragging;
    float normalized(float x) const
    {
        return juce::jlimit(0.f, 1.f, (x - 8) / std::max(1.f, getWidth() - 16.f));
    }
    void finish();
    void move(float);

  public:
    explicit SampleView(RastroProcessor &processor) : p(processor) {}
    ~SampleView() override { finish(); }
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override { finish(); }
};
