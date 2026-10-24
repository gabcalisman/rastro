#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
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
// Multi-mode state-variable filter (same topology as LowPass, all outputs available).
struct MultiFilter
{
    float ic1 = 0, ic2 = 0, g = 1, k = 1, a = 1;
    void coefficients(double sr, float cutoff, float resonance)
    {
        g = std::tan(float(3.141592653589793 * std::clamp(double(cutoff), 20., sr * .45) / sr));
        k = 2 - 1.9f * std::clamp(resonance, 0.f, 1.f);
        a = 1 / (1 + g * (g + k));
    }
    // mode: 0 low-pass, 1 high-pass, 2 band-pass (unity peak), 3 notch
    float process(float input, int mode)
    {
        float v1 = (ic1 + g * (input - ic2)) * a, v2 = ic2 + g * v1;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        switch (mode)
        {
        case 1: return input - k * v1 - v2;
        case 2: return k * v1;
        case 3: return input - k * v1;
        default: return v2;
        }
    }
};
// Drive: soft saturation run at twice the sample rate (polyphase half-band FIR up and down),
// so the harmonics it creates do not fold back as aliasing. Fixed size, no allocation.
struct Saturator2x
{
    static constexpr int taps = 23;
    float up[taps]{}, down[taps]{};
    int upPos = 0, downPos = 0;
    static const float *kernel()
    {
        struct K
        {
            float h[taps];
            K()
            {
                const int half = taps / 2;
                float sum = 0;
                for (int i = 0; i < taps; ++i)
                {
                    const double n = i - half, x = n * .5;
                    const double sinc = n == 0 ? 1 : std::sin(3.141592653589793 * x) / (3.141592653589793 * x);
                    const double w = .42 - .5 * std::cos(2 * 3.141592653589793 * i / (taps - 1)) +
                                     .08 * std::cos(4 * 3.141592653589793 * i / (taps - 1));
                    h[i] = float(sinc * w);
                    sum += h[i];
                }
                for (auto &t : h)
                    t /= sum;
            }
        };
        static const K k;
        return k.h;
    }
    static float shape(float x, float drive)
    {
        if (drive <= 0)
            return x;
        const float pre = 1 + drive * 9;
        // gentle asymmetry adds warmth (even harmonics); output kept near unity gain
        const float y = std::tanh(pre * x + .08f * drive) - std::tanh(.08f * drive);
        return y / (1 + drive * 1.6f) * (1 + drive * .35f);
    }
    float fir(const float *buf, int pos) const
    {
        const float *h = kernel();
        float acc = 0;
        for (int i = 0; i < taps; ++i)
            acc += h[i] * buf[(pos - i + taps * 2) % taps];
        return acc;
    }
    float process(float x, float drive)
    {
        float out = 0;
        for (int phase = 0; phase < 2; ++phase)
        {
            // zero-stuffed upsampling (×2 gain compensates the inserted zeros)
            up[upPos] = phase == 0 ? x * 2 : 0;
            const float u = fir(up, upPos);
            upPos = (upPos + 1) % taps;
            down[downPos] = shape(u, drive);
            if (phase == 1)
                out = fir(down, downPos);
            downPos = (downPos + 1) % taps;
        }
        return out;
    }
};
// LFO waveforms. phase 0..1; returns -1..1. Sample & hold value is supplied by the caller.
inline float lfoWave(int shape, float phase, float held)
{
    switch (shape)
    {
    case 1: return phase < .5f ? phase * 4 - 1 : 3 - phase * 4;
    case 2: return phase < .5f ? 1.f : -1.f;
    case 3: return phase * 2 - 1;
    case 4: return held;
    default: return std::sin(phase * 6.28318530718f);
    }
}
} // namespace rastro
