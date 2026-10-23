#pragma once
#include <algorithm>
#include <cmath>
namespace rastro
{
// Original bounded per-voice DSP. No allocation, locks or framework dependency.
struct SynthEnvelope
{
    enum Stage
    {
        idle,
        attack,
        decay,
        sustain,
        release
    } stage = idle;
    float value = 0, releaseStep = 0;
    void start()
    {
        stage = attack;
        value = releaseStep = 0;
    }
    float tick(double sr, float a, float d, float s, float r, bool gate)
    {
        s = std::clamp(s, 0.f, 1.f);
        if (!gate && stage != release && stage != idle)
        {
            stage = release;
            releaseStep = value / (float(sr) * std::max(.001f, r));
        }
        if (stage == attack)
        {
            value += 1 / (float(sr) * std::max(.001f, a));
            if (value >= 1)
            {
                value = 1;
                stage = decay;
            }
        }
        else if (stage == decay)
        {
            value -= (1 - s) / (float(sr) * std::max(.001f, d));
            if (value <= s)
            {
                value = s;
                stage = sustain;
            }
        }
        else if (stage == sustain)
            value += (s - value) * float(1 - std::exp(-1 / (sr * .015)));
        else if (stage == release)
        {
            value = std::max(0.f, value - releaseStep);
            if (value == 0)
                stage = idle;
        }
        return value;
    }
};
struct LowPass
{
    float ic1 = 0, ic2 = 0, g = 1, k = 1, a = 1;
    void coefficients(double sr, float cutoff, float resonance)
    {
        g = std::tan(float(3.141592653589793 * std::clamp(double(cutoff), 20., sr * .45) / sr));
        k = 2 - 1.9f * std::clamp(resonance, 0.f, 1.f);
        a = 1 / (1 + g * (g + k));
    }
    float process(float input)
    {
        float v1 = (ic1 + g * (input - ic2)) * a, v2 = ic2 + g * v1;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        return v2;
    }
};
} // namespace rastro
