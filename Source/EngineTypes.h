#pragma once
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
    bool stereo = false;
    void buildPeaks()
    {
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
    int mode = 0, sliceCount = 8, pitchMode = 0; // 0 REPITCH, 1 FIXED PITCH
    std::array<float, 17> cuts{
        {0, .125f, .25f, .375f, .5f, .625f, .75f, .875f, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
};
} // namespace rastro
