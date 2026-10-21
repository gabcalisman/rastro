#pragma once
#include "CharacterEffects.h"
#include "EngineTypes.h"
#include <algorithm>
#include <atomic>
#include <cmath>
namespace rastro
{
class EffectRack
{
    std::array<std::vector<float>, 2> delay, grain;
    std::array<std::array<std::vector<float>, 4>, 2> reverb;
    std::array<std::array<int, 4>, 2> rp{};
    float ghostTone[2]{}, bloomTone[2]{};
    float delayTime = 1;
    int write = 0, grainWrite = 0, phase = 0, fragment = 0, fragmentLength = 1;
    unsigned random = 12345;
    double sr = 44100;
    float smoothing = .001f;
    Settings sm;
    bool initialized = false;
    Order activeOrder = defaultOrder;
    CharacterEffects character;
    float displayPeak = 0, peakRelease = .999f;
    unsigned telemetryCounter = 0;
    float topologyGain = 1;
    float smooth(float &a, float b)
    {
        a += (b - a) * smoothing;
        return a;
    }

  public:
    std::atomic<float> meter{0};
    std::atomic<float> visualWobble{0}, visualDust{0}, visualCrush{0}, visualBloom{0},
        visualEnvelope{0};
    std::atomic<float> visualX{.5f}, visualY{.5f};
    void endBlock() { meter.store(displayPeak, std::memory_order_relaxed); }
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        smoothing = float(1 - std::exp(-1 / (sr * .015)));
        for (auto &d : delay)
            d.assign((size_t)(sr * 4) + 2, 0);
        for (auto &g : grain)
            g.assign((size_t)sr + 2, 0);
        for (auto &c : reverb)
            for (auto &d : c)
                d.assign((size_t)(sr * .16) + 2, 0);
        character.prepare(sr);
        peakRelease = std::exp(-1.f / float(sr * .075));
        reset();
    }
    void reset()
    {
        for (auto &d : delay)
            std::fill(d.begin(), d.end(), 0);
        for (auto &d : grain)
            std::fill(d.begin(), d.end(), 0);
        for (auto &c : reverb)
            for (auto &d : c)
                std::fill(d.begin(), d.end(), 0);
        rp = {};
        write = grainWrite = phase = 0;
        std::fill_n(ghostTone, 2, 0);
        std::fill_n(bloomTone, 2, 0);
        initialized = false;
        topologyGain = 1;
        delayTime = 1;
        std::fill_n(activation, moduleCount, 1.f);
        character.reset();
        displayPeak = 0;
        telemetryCounter = 0;
        visualWobble = 0;
        visualDust = 0;
        visualCrush = 0;
        visualBloom = 0;
        visualEnvelope = 0;
        visualX = .5f;
        visualY = .5f;
        meter = 0;
    }
    // All storage is prepared off the audio thread. No allocation, ownership, locks, or file I/O
    // here.
    void process(float &l, float &r, const Settings &p)
    {
        if (!initialized)
        {
            sm = p;
            activeOrder = p.order;
            for (int m = 0; m < moduleCount; ++m)
                activation[m] = p.enabled[m] ? 1.f : 0.f;
            initialized = true;
        }
        if (activeOrder != p.order)
        {
            topologyGain = std::max(0.f, topologyGain - float(1 / (sr * .005)));
            if (topologyGain == 0)
                activeOrder = p.order;
        }
        else
            topologyGain = std::min(1.f, topologyGain + float(1 / (sr * .005)));
        smooth(sm.level, p.level);
        smooth(sm.presence, p.presence);
        smooth(sm.trail, p.trail);
        smooth(sm.mutation, p.mutation);
        smooth(sm.drywet, p.drywet);
        smooth(sm.velocity, p.velocity);
        for (int j = 0; j < 4; ++j)
        {
            smooth(sm.ghost[j], p.ghost[j]);
            smooth(sm.bloom[j], p.bloom[j]);
            if (j < 3)
                smooth(sm.crumble[j], p.crumble[j]);
        }
        smooth(sm.playX, p.playX);
        smooth(sm.playY, p.playY);
        for (int j = 0; j < 3; ++j)
        {
            smooth(sm.wobble[j], p.wobble[j]);
            smooth(sm.dust[j], p.dust[j]);
            if (j < 2)
                smooth(sm.crush[j], p.crush[j]);
        }
        float x[2]{l, r};
        float original[2]{x[0], x[1]};
        // Center is neutral. XY scales the individual settings, never their enable flags.
        const float depth = std::clamp(sm.wobble[0] * 2 * sm.playX, 0.f, 1.f);
        const float dustLevel = std::clamp(sm.dust[0] * 2 * sm.playY, 0.f, 1.f);
        const float crushAmount = std::clamp(sm.crush[0] * 2 * sm.playY, 0.f, 1.f);
        character.advance(std::max(std::abs(x[0]), std::abs(x[1])), sm.wobble[1]);
        character.beginCrush(crushAmount);
        const int delaySize = (int)delay[0].size(), grainSize = (int)grain[0].size();
        int beatSamples = std::max(16, int(sr * 60 / std::clamp(p.tempo, 30.f, 300.f)));
        const float divisions[]{.125f, .25f, .5f, 1.f, 2.f};
        int dt = std::clamp(int(beatSamples * divisions[std::min(4, int(sm.ghost[0] * 4.99f))]), 1,
                            delaySize - 1);
        smooth(delayTime, (float)dt);
        int period = std::clamp(int(beatSamples * std::pow(2., -1 - int(sm.crumble[0] * 4.99f))),
                                16, grainSize / 2);
        if (phase >= period)
        {
            phase = 0;
            random = random * 1664525u + 1013904223u;
            fragmentLength = std::max(16, int(period * (.25f + .7f * (1 - sm.crumble[1]))));
            fragment = (grainWrite - fragmentLength -
                        int((random % 1024) / 1024.f * sm.mutation * period) + grainSize * 2) %
                       grainSize;
        }
        for (int m = 0; m < moduleCount; ++m)
            smooth(activation[m], p.enabled[m] ? 1.f : 0.f);
        for (int module : activeOrder)
            for (int c = 0; c < 2; ++c)
            {
                float wet = x[c];
                if (module == 0)
                {
                    float read = write - delayTime;
                    if (read < 0)
                        read += delaySize;
                    if (read >= delaySize)
                        read = 0;
                    int a = (int)read;
                    float z =
                        delay[c][a] + (read - a) * (delay[c][(a + 1) % delaySize] - delay[c][a]);
                    ghostTone[c] += (.02f + .7f * sm.ghost[2]) * (z - ghostTone[c]);
                    delay[c][write] = std::tanh(
                        x[c] + ghostTone[c] * std::min(.88f, sm.ghost[1] * (.5f + sm.trail)));
                    wet = ghostTone[c];
                }
                if (module == 1)
                {
                    grain[c][grainWrite] = x[c];
                    float w = .5f - .5f * std::cos(6.2831853f * (phase % fragmentLength) /
                                                   fragmentLength);
                    wet = grain[c][(fragment + phase % fragmentLength) % grainSize] * w;
                }
                if (module == 2)
                {
                    float sum = 0;
                    for (int k = 0; k < 4; ++k)
                    {
                        int len = (int)reverb[c][k].size();
                        float time = std::clamp(
                            float(sr * (.027 + .019 * k) * (.5f + sm.bloom[0]) + (c * 17)), 2.f,
                            float(len - 1));
                        int &idx = rp[c][k];
                        float read = idx - time;
                        if (read < 0)
                            read += len;
                        if (read >= len)
                            read = 0;
                        int a = (int)read;
                        float z = reverb[c][k][a] +
                                  (read - a) * (reverb[c][k][(a + 1) % len] - reverb[c][k][a]);
                        reverb[c][k][idx] =
                            std::tanh(x[c] * .3f + z * std::min(.94f, .3f + .6f * sm.bloom[2] +
                                                                          .04f * sm.trail));
                        idx = (idx + 1) % len;
                        sum += z * .25f;
                    }
                    bloomTone[c] += (.015f + .65f * sm.bloom[1]) * (sum - bloomTone[c]);
                    wet = bloomTone[c];
                }
                if (module == 3)
                    wet = character.wobble(x[c], c, depth);
                if (module == 4)
                    wet = character.dust(x[c], c, dustLevel, sm.dust[1]);
                if (module == 5)
                    wet = character.crush(x[c], c, crushAmount);
                const float mixes[]{sm.ghost[3],  sm.crumble[2], sm.bloom[3],
                                    sm.wobble[2], sm.dust[2],    sm.crush[1]};
                float mix = mixes[module];
                // Bypass crossfades instead of abruptly switching the signal.
                float &active = activation[module];
                mix *= active;
                x[c] += (wet - x[c]) * mix;
            }
        character.finish();
        write = (write + 1) % delaySize;
        grainWrite = (grainWrite + 1) % grainSize;
        ++phase;
        for (int c = 0; c < 2; ++c)
            x[c] = std::tanh((original[c] + (x[c] - original[c]) * sm.drywet) * sm.level *
                             (.6f + sm.presence));
        l = x[0] * topologyGain;
        r = x[1] * topologyGain;
        displayPeak = std::max(std::max(std::abs(l), std::abs(r)), displayPeak * peakRelease);
        // Publish decimated measurements only. UI graphics never touch audio-owned storage.
        if (++telemetryCounter >= 64)
        {
            telemetryCounter = 0;
            meter.store(displayPeak, std::memory_order_relaxed);
            visualWobble.store(character.movement * depth * sm.wobble[2] * activation[3] *
                                   sm.drywet,
                               std::memory_order_relaxed);
            visualDust.store(character.dustActivity * sm.dust[2] * activation[4] * sm.drywet,
                             std::memory_order_relaxed);
            visualCrush.store(character.crushActivity * sm.crush[1] * activation[5] * sm.drywet,
                              std::memory_order_relaxed);
            visualBloom.store(std::max(std::abs(bloomTone[0]), std::abs(bloomTone[1])) *
                                  sm.bloom[3] * activation[2] * sm.drywet,
                              std::memory_order_relaxed);
            visualEnvelope.store(character.envelope, std::memory_order_relaxed);
            visualX = sm.playX;
            visualY = sm.playY;
        }
    }

  private:
    float activation[moduleCount]{1, 1, 1, 1, 1, 1};
};
} // namespace rastro
