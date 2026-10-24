#pragma once
#include "Processor.h"
#include "RastroStrings.h"
class SampleView : public juce::Component, public juce::SettableTooltipClient
{
    RastroProcessor &p;
    juce::String dragging;
    float viewStart = 0, viewEnd = 1;
    juce::TextButton minus{"-"}, plus{"+"}, fit{"ALL"}, selection{"SEL"}, editRegion{"TRIM"};
    bool editingLoop = false;
    const rastro::Sample *lastSample = nullptr;
    void finish();
    void move(float);
    juce::Rectangle<float> plot() const;
    void setView(float, float);

  public:
    explicit SampleView(RastroProcessor &);
    ~SampleView() override { finish(); }
    float visibleStart() const { return viewStart; }
    float visibleEnd() const { return viewEnd; }
    float normalized(float pixel) const;
    float pixelFor(float samplePosition) const;
    void zoomAt(float factor, float anchor);
    void fitSample() { setView(0, 1); }
    void fitSelection();
    void resized() override;
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override { finish(); }
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override;
    bool keyPressed(const juce::KeyPress &) override;
};
