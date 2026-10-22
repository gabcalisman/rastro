#include "SampleView.h"
#include "Visuals.h"
SampleView::SampleView(RastroProcessor &processor) : p(processor)
{
    setWantsKeyboardFocus(true);
    for (auto *b : {&minus, &plus, &fit, &selection, &editRegion})
        addAndMakeVisible(b);
    editRegion.setComponentID("waveEditRegion");
    editRegion.onClick = [this]
    {
        editingLoop = !editingLoop;
        editRegion.setButtonText(editingLoop ? "LOOP" : "TRIM");
        repaint();
    };
    minus.setComponentID("waveZoomOut");
    plus.setComponentID("waveZoomIn");
    fit.setComponentID("waveFitAll");
    selection.setComponentID("waveFitSelection");
    minus.onClick = [this] { zoomAt(.5f, (viewStart + viewEnd) * .5f); };
    plus.onClick = [this] { zoomAt(2, (viewStart + viewEnd) * .5f); };
    fit.onClick = [this] { fitSample(); };
    selection.onClick = [this] { fitSelection(); };
}
juce::Rectangle<float> SampleView::plot() const
{
    return getLocalBounds().toFloat().reduced(8).withTrimmedTop(24).withTrimmedBottom(18);
}
void SampleView::resized()
{
    int x = getWidth() - 226;
    for (auto *b : {&minus, &plus, &fit, &selection, &editRegion})
    {
        b->setBounds(x, 3, 40, 24);
        x += 44;
    }
}
float SampleView::normalized(float x) const
{
    return juce::jlimit(0.f, 1.f,
                        viewStart + (x - plot().getX()) / std::max(1.f, plot().getWidth()) *
                                        (viewEnd - viewStart));
}
float SampleView::pixelFor(float v) const
{
    return plot().getX() + (v - viewStart) / (viewEnd - viewStart) * plot().getWidth();
}
void SampleView::setView(float a, float b)
{
    float span = juce::jlimit(1.f / 4096, 1.f, b - a);
    viewStart = juce::jlimit(0.f, 1.f - span, a);
    viewEnd = viewStart + span;
    repaint();
}
void SampleView::zoomAt(float factor, float anchor)
{
    float fraction = juce::jlimit(0.f, 1.f, (anchor - viewStart) / (viewEnd - viewStart));
    float span = (viewEnd - viewStart) / std::max(.1f, factor);
    setView(anchor - fraction * span, anchor + (1 - fraction) * span);
}
void SampleView::fitSelection()
{
    auto s = p.settings();
    float pad = std::max(.001f, (s.end - s.start) * .04f);
    setView(s.start - pad, s.end + pad);
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
    grabKeyboardFocus();
    finish();
    auto s = p.settings();
    float closest = 15;
    auto consider = [&](const juce::String &id, float v)
    {
        float distance = std::abs(pixelFor(v) - e.position.x);
        if (distance < closest)
        {
            closest = distance;
            dragging = id;
        }
    };
    if (editingLoop)
    {
        consider("loopStart", s.loopStart);
        consider("loopEnd", s.loopEnd);
    }
    else
    {
        consider("start", s.start);
        consider("end", s.end);
    }
    if (s.mode == 4 && !editingLoop)
        for (int i = 1; i < s.sliceCount; ++i)
            consider("slice" + juce::String(i), s.start + (s.end - s.start) * s.cuts[i]);
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
    if (dragging == "loopStart")
    {
        v = std::min(v, s.loopEnd - .0001f);
        p.set("loopRegionSet", 1);
    }
    else if (dragging == "loopEnd")
    {
        v = std::max(v, s.loopStart + .0001f);
        p.set("loopRegionSet", 1);
    }
    else if (dragging == "start")
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
void SampleView::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w)
{
    if (e.mods.isShiftDown() || std::abs(w.deltaX) > std::abs(w.deltaY))
    {
        float d = (w.deltaX != 0 ? w.deltaX : w.deltaY) * (viewEnd - viewStart);
        setView(viewStart - d, viewEnd - d);
    }
    else
        zoomAt(std::exp(w.deltaY * 3), normalized(e.position.x));
}
bool SampleView::keyPressed(const juce::KeyPress &key)
{
    if (key.getKeyCode() == '+' || key.getKeyCode() == '=')
        zoomAt(2, (viewStart + viewEnd) * .5f);
    else if (key.getKeyCode() == '-')
        zoomAt(.5f, (viewStart + viewEnd) * .5f);
    else if (key == juce::KeyPress::homeKey)
        fitSample();
    else if (key.getTextCharacter() == 'f' || key.getTextCharacter() == 'F')
        fitSelection();
    else
        return false;
    return true;
}
void SampleView::paint(juce::Graphics &g)
{
    auto pal = visuals::palette(p);
    g.setColour(pal.bg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6);
    auto *sample = p.source.load(std::memory_order_acquire);
    if (sample != lastSample)
    {
        lastSample = sample;
        viewStart = 0;
        viewEnd = 1;
    }
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(11)));
    if (!sample)
    {
        g.drawText(text::get("empty", p.language), plot(), juce::Justification::centred);
        return;
    }
    auto s = p.settings();
    auto r = plot();
    g.drawText(juce::String(1 / (viewEnd - viewStart), 1) + "x  " +
                   (sample->stereo ? "L / R" : "MONO"),
               10, 4, std::max(0, getWidth() - 240), 22, juce::Justification::left);
    {
        juce::Graphics::ScopedSaveState clip(g);
        g.reduceClipRegion(r.toNearestInt());
        g.setColour(pal.highlight.withAlpha(.15f));
        g.fillRect(pixelFor(s.start), r.getY(),
                   (s.end - s.start) / (viewEnd - viewStart) * r.getWidth(), r.getHeight());
        int channels = sample->stereo ? 2 : 1;
        float framesPerPixel = (viewEnd - viewStart) * sample->size() / std::max(1.f, r.getWidth());
        for (int c = 0; c < channels; ++c)
        {
            float height = r.getHeight() / channels, cy = r.getY() + height * (c + .5f);
            g.setColour(pal.text.withAlpha(.22f));
            g.drawHorizontalLine(juce::roundToInt(cy), r.getX(), r.getRight());
            g.setColour(c ? pal.secondary : pal.text);
            for (int x = 0; x < (int)r.getWidth(); ++x)
            {
                int begin = std::clamp((int)(normalized(r.getX() + x) * sample->size()), 0,
                                       sample->size() - 1);
                int end = std::clamp((int)(normalized(r.getX() + x + 1) * sample->size()) + 1,
                                     begin + 1, sample->size());
                rastro::Sample::Peak peak;
                if (framesPerPixel < 64 || sample->peaks[c].empty())
                {
                    // Zoomed ranges are bounded to 65 samples per pixel; fallback for test sources.
                    for (int i = begin; i < std::min(end, begin + 65); ++i)
                    {
                        peak.lo = std::min(peak.lo, sample->data[c][i]);
                        peak.hi = std::max(peak.hi, sample->data[c][i]);
                    }
                }
                else
                {
                    int level = 0, stride = 64;
                    while (level + 1 < (int)sample->peaks[c].size() && stride * 2 <= framesPerPixel)
                    {
                        ++level;
                        stride *= 2;
                    }
                    auto &buckets = sample->peaks[c][level];
                    for (int i = begin / stride;
                         i <= std::min((int)buckets.size() - 1, (end - 1) / stride); ++i)
                    {
                        peak.lo = std::min(peak.lo, buckets[i].lo);
                        peak.hi = std::max(peak.hi, buckets[i].hi);
                    }
                }
                g.drawVerticalLine((int)r.getX() + x, cy - peak.hi * height * .44f,
                                   cy - peak.lo * height * .44f + 1);
            }
        }
        auto marker = [&](float v, const juce::String &label, juce::Colour colour)
        {
            float x = pixelFor(v);
            g.setColour(colour);
            g.drawLine(x, r.getY(), x, r.getBottom(), 2);
            g.fillRoundedRectangle(x - 8, r.getY(), 16, 10, 2);
            g.drawText(label, (int)x + 9, (int)r.getY() + 2, 75, 16, juce::Justification::left);
        };
        if (s.mode == 4)
            for (int i = 1; i < s.sliceCount; ++i)
                marker(s.start + (s.end - s.start) * s.cuts[i], juce::String((int)s.root + i),
                       pal.secondary);
        marker(s.start, "IN", pal.highlight);
        marker(s.end, "OUT", pal.highlight);
        if (s.loopRegionSet || editingLoop)
        {
            g.setColour(pal.control.withAlpha(.10f));
            g.fillRect(pixelFor(s.loopStart), r.getY(),
                       (s.loopEnd - s.loopStart) / (viewEnd - viewStart) * r.getWidth(),
                       r.getHeight());
            marker(s.loopStart, "L IN", pal.control);
            marker(s.loopEnd, "L OUT", pal.control);
        }
        if (p.engine.playbackActive.load() && p.value("animation") > 0)
        {
            g.setColour(pal.control);
            float x = pixelFor(p.engine.position.load());
            g.drawLine(x, r.getY(), x, r.getBottom(), 1.5f);
        }
    }
    g.setColour(pal.text);
    float duration = (float)(sample->size() / sample->rate * 1000);
    juce::String label = "IN " + juce::String(s.start * duration, 1) + " ms  |  OUT " +
                         juce::String(s.end * duration, 1) + " ms";
    if (editingLoop)
        label = "LOOP " + juce::String(s.loopStart * duration, 1) + " - " +
                juce::String(s.loopEnd * duration, 1) + " ms | TRIM " +
                juce::String(s.start * duration, 1) + " - " + juce::String(s.end * duration, 1) +
                " ms";
    if (dragging.startsWith("slice"))
    {
        int i = dragging.substring(5).getIntValue();
        label += "  |  " + dragging + " " +
                 juce::String((s.start + (s.end - s.start) * s.cuts[i]) * duration, 1) + " ms";
    }
    g.drawText(label, 8, getHeight() - 20, getWidth() - 16, 18, juce::Justification::left);
    if (hasKeyboardFocus(true))
    {
        g.setColour(pal.control);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 5, 1.5f);
    }
}
