#include "SampleView.h"
namespace
{
const juce::Colour ink(0xff261e19), cream(0xfff2e3c5), orange(0xffdf682f), lime(0xffced968);
}
void SampleView::finish()
{
    if (dragging.isNotEmpty())
    {
        p.params.getParameter(dragging)->endChangeGesture();
        dragging = {};
    }
}
void SampleView::mouseDown(const juce::MouseEvent &e)
{
    if (!p.source.load())
        return;
    auto s = p.settings();
    float x = normalized(e.position.x);
    float tolerance = 10.f / std::max(1, getWidth() - 16);
    if (std::abs(x - s.start) < tolerance)
        dragging = "start";
    else if (std::abs(x - s.end) < tolerance)
        dragging = "end";
    else if (s.mode == 4)
        for (int i = 1; i < s.sliceCount; ++i)
            if (std::abs(x - (s.start + (s.end - s.start) * s.cuts[i])) < tolerance)
            {
                dragging = "slice" + juce::String(i);
                break;
            }
    if (dragging.isNotEmpty())
    {
        p.params.getParameter(dragging)->beginChangeGesture();
        move(e.position.x);
    }
}
void SampleView::move(float x)
{
    auto s = p.settings();
    float v = normalized(x);
    if (dragging == "start")
        v = juce::jlimit(0.f, std::max(0.f, s.end - .002f), v);
    else if (dragging == "end")
        v = juce::jlimit(std::min(1.f, s.start + .002f), 1.f, v);
    else
    {
        int i = dragging.substring(5).getIntValue();
        if (i <= 0 || i >= s.sliceCount)
            return;
        v = (v - s.start) / std::max(.002f, s.end - s.start);
        v = juce::jlimit(s.cuts[i - 1] + .0001f, s.cuts[i + 1] - .0001f, v);
    }
    auto *parameter = p.params.getParameter(dragging);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
    repaint();
}
void SampleView::mouseDrag(const juce::MouseEvent &e)
{
    if (dragging.isNotEmpty())
        move(e.position.x);
}
void SampleView::paint(juce::Graphics &g)
{
    g.setColour(ink);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6);
    auto *sample = p.source.load(std::memory_order_acquire);
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(11)));
    if (!sample)
    {
        g.drawFittedText(text::get("empty", p.language), getLocalBounds().reduced(12),
                         juce::Justification::centred, 2);
        return;
    }
    auto r = getLocalBounds().reduced(8);
    int labelsY = r.getBottom() - 13;
    r.removeFromBottom(14);
    auto s = p.settings();
    float width = (float)r.getWidth();
    auto px = [&](float v) { return r.getX() + v * width; };
    g.setColour(orange.withAlpha(.2f));
    g.fillRect(px(s.start), (float)r.getY(), std::max(0.f, (s.end - s.start) * width),
               (float)r.getHeight());
    g.setColour(cream);
    for (int x = 0; x < r.getWidth(); ++x)
    {
        float peak =
            std::min(1.f, sample->waveform[std::min(511, x * 512 / std::max(1, r.getWidth()))]);
        g.drawVerticalLine(r.getX() + x, r.getCentreY() - peak * r.getHeight() * .45f,
                           r.getCentreY() + peak * r.getHeight() * .45f);
    }
    if (s.mode == 4)
    {
        g.setColour(lime);
        for (int i = 0; i < s.sliceCount; ++i)
        {
            float a = px(s.start + (s.end - s.start) * s.cuts[i]),
                  b = px(s.start + (s.end - s.start) * s.cuts[i + 1]);
            if (i > 0)
                g.drawVerticalLine((int)a, (float)r.getY(), (float)r.getBottom());
            g.drawText(juce::String((int)s.root + i), (int)a, labelsY, std::max(1, (int)(b - a)),
                       13, juce::Justification::centred);
        }
    }
    for (int i = 0; i < 2; ++i)
    {
        float x = px(i ? s.end : s.start);
        g.setColour(orange);
        g.drawVerticalLine(juce::roundToInt(x), (float)r.getY(), (float)r.getBottom());
        juce::Path handle;
        handle.addTriangle(x - 6, (float)r.getY(), x + 6, (float)r.getY(), x, (float)r.getY() + 8);
        g.fillPath(handle);
        if (s.mode != 4)
            g.drawText(text::get(i ? "end" : "start", p.language),
                       juce::jlimit(0, getWidth() - 52, (int)x - 26), labelsY, 52, 13,
                       juce::Justification::centred);
    }
    if (p.value("animation") > 0 && p.engine.meter > .001f)
    {
        g.setColour(lime);
        g.drawVerticalLine(juce::jlimit(r.getX(), r.getRight(), (int)px(p.engine.position.load())),
                           (float)r.getY(), (float)r.getBottom());
    }
}
