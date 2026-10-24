#pragma once
#include <algorithm>
#include <cmath>
#include "Rack.h"
#include <array>
#include <vector>
namespace rastro
{
struct Sample
{
    struct Peak
    {
        float lo = 0, hi = 0;
    };
    std::array<std::vector<float>, 2> data;
    std::array<float, 512> waveform{};
    // Built once on the import worker. Painting queries bounded peak buckets, not full audio.
    std::array<std::vector<std::vector<Peak>>, 2> peaks;
    // Band-limited copies for high-quality playback: level k is the sample low-passed and
    // decimated by 2^k (k = 1..mipLevels). Built once on the import worker, never on the audio thread.
    static constexpr int mipLevels = 5;
    std::array<std::array<std::vector<float>, 2>, mipLevels> mips;
    bool stereo = false;
    void buildMips()
    {
        // 31-tap windowed-sinc half-band low-pass (cutoff = quarter of the source rate)
        constexpr int taps = 31, half = taps / 2;
        float h[taps];
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
        for (int c = 0; c < 2; ++c)
        {
            const std::vector<float> *src = &data[c];
            for (int k = 0; k < mipLevels; ++k)
            {
                auto &dst = mips[k][c];
                const int n = int(src->size());
                dst.assign(size_t(std::max(2, (n + 1) / 2)), 0.f);
                for (int j = 0; j < int(dst.size()); ++j)
                {
                    float acc = 0;
                    const int centre = j * 2;
                    for (int i = 0; i < taps; ++i)
                    {
                        const int idx = centre + i - half;
                        if (idx >= 0 && idx < n)
                            acc += h[i] * (*src)[size_t(idx)];
                    }
                    dst[size_t(j)] = acc;
                }
                src = &dst;
            }
        }
    }
    // 4-point Hermite read of one table
    static float hermite(const std::vector<float> &t, double pos)
    {
        const int n = int(t.size());
        if (n < 2)
            return n ? t[0] : 0.f;
        int i = int(pos);
        const float f = float(pos - i);
        auto at = [&](int k) { return t[size_t(std::clamp(k, 0, n - 1))]; };
        const float xm = at(i - 1), x0 = at(i), x1 = at(i + 1), x2 = at(i + 2);
        const float c1 = .5f * (x1 - xm), c2 = xm - 2.5f * x0 + 2 * x1 - .5f * x2,
                    c3 = .5f * (x2 - xm) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
    // High-quality read: Hermite on the band-limited level that matches the playback speed,
    // blended between the two nearest levels so changing pitch never jumps.
    float readHQ(int c, double pos, double step) const
    {
        const double s = std::abs(step);
        if (s <= 1 || mips[0][c].empty())
            return hermite(data[c], pos);
        const double l = std::min(double(mipLevels), std::log2(s));
        const int k = int(l);
        const float t = float(l - k);
        auto level = [&](int lv) {
            return lv == 0 ? hermite(data[c], pos) : hermite(mips[size_t(lv - 1)][c], pos / double(1 << lv));
        };
        const float a = level(k);
        if (t <= 0 || k >= mipLevels)
            return a;
        return a + (level(k + 1) - a) * t;
    }
    void buildPeaks()
    {
        buildMips();
        stereo = data[0] != data[1];
        for (int c = 0; c < 2; ++c)
        {
            peaks[c].clear();
            auto &first = peaks[c].emplace_back((size() + 63) / 64);
            for (int i = 0; i < size(); ++i)
            {
                auto &v = first[i / 64];
                v.lo = std::min(v.lo, data[c][i]);
                v.hi = std::max(v.hi, data[c][i]);
            }
            while (peaks[c].back().size() > 1)
            {
                const auto &from = peaks[c].back();
                std::vector<Peak> next((from.size() + 1) / 2);
                for (size_t i = 0; i < from.size(); ++i)
                {
                    next[i / 2].lo = std::min(next[i / 2].lo, from[i].lo);
                    next[i / 2].hi = std::max(next[i / 2].hi, from[i].hi);
                }
                peaks[c].push_back(std::move(next));
            }
        }
    }
    size_t peakBytes() const
    {
        size_t bytes = 0;
        for (auto &channel : peaks)
            for (auto &level : channel)
                bytes += level.size() * sizeof(Peak);
        return bytes;
    }
    double rate = 44100;
    int size() const { return (int)data[0].size(); }
};
struct Settings
{
    float root = 60, start = 0, end = 1, loop = 0, level = .7f, velocity = .5f, tempo = 120,
          presence = .5f, trail = .5f, mutation = .3f, drywet = 1;
    std::array<float, 4> ghost{{.5f, .35f, .6f, .3f}}, bloom{{.6f, .6f, .5f, .25f}};
    std::array<float, 3> crumble{{.5f, .3f, .2f}};
    std::array<float, 3> wobble{{.25f, .3f, .5f}}, dust{{.15f, .5f, .5f}};
    std::array<float, 2> crush{{.3f, .3f}};
    bool regionLoop = false, loopRegionSet = false;
    float loopStart = 0, loopEnd = 1, loopCrossfade = 5;
    bool synth = false;
    float attack = .01f, decay = .2f, sustain = .75f, release = .2f;
    float cutoff = 16000, resonance = .1f, envAmount = 0, lfoRate = 1, lfoDepth = 0, glide = 0,
          bendRange = 2;
    int envTarget = 0, lfoTarget = 0, lfoSync = 0, voiceMode = 0, triggerMode = 0;
    std::array<int, 3> timingMode{}, division{}; // 0 legacy, 1 FREE, 2 SYNC
    float ghostMs = 250, crumbleHz = 8, wobbleHz = 1.005f;
    int lfoTiming = 0, lfoDivision = 0;
    double transportBeat = 0;
    bool transportValid = false, transportPlaying = false;
    bool xyCrumble = false; // missing in old states: preserve legacy Y mapping
    float playX = .5f, playY = .5f;
    std::array<bool, moduleCount> enabled{{true, true, true, true, true, true}};
    Order order = defaultOrder;
    bool parallel = false; // false: modules in series (legacy); true: each module hears the dry input
    float bloomDiffusion = 0, wobbleWidth = 0; // 0 = exact legacy sound
    // Etapa 1 de som (24/10/2026). Every default below is the exact legacy behaviour; states saved
    // before these parameters existed restore them so old sessions sound the same.
    bool hq = false;           // high-quality playback (band-limited Hermite) + oversampled drive path
    int filterMode = 0;        // 0 LP (legacy), 1 HP, 2 BP, 3 Notch
    int filterSlope = 0;       // 0 12 dB (legacy), 1 24 dB
    float filterDrive = 0;     // 0 = no saturation
    bool fenvOwn = false;      // false: the filter follows the amp envelope (legacy)
    float fAttack = .005f, fDecay = .4f, fSustain = .3f, fRelease = .4f;
    int lfoShape = 0;          // 0 sine (legacy), 1 triangle, 2 square, 3 saw, 4 sample & hold
    float lfoFade = 0, lfoAmp = 0, lfoPan = 0;
    float sampleGainDb = 0;    // clip gain applied to the sample before the filter
    int mode = 0, sliceCount = 8, pitchMode = 0; // 0 REPITCH, 1 FIXED PITCH
    std::array<float, 17> cuts{
        {0, .125f, .25f, .375f, .5f, .625f, .75f, .875f, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
};
} // namespace rastro
