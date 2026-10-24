#pragma once
#include "AntialiasedShape.h"
#include "CharacterEffects.h"
#include "EngineTypes.h"
#include <algorithm>
#include <atomic>
#include <cmath>
namespace rastro
{
// FX adaptation of EffectRack: same module algorithms, without PLAY output coloration.
// Keep EffectRack.h unchanged; regression tests compare each inherited algorithm.
class FxLegacyRack
{
    std::array<std::vector<float>, 2> delay, grain;
    std::array<std::array<std::vector<float>, 4>, 2> reverb;
    std::array<std::array<int, 4>, 2> rp{};
    float ghostTone[2]{}, bloomTone[2]{};
    std::array<float, 2> aaGhost{};
    std::array<std::array<float, 4>, 2> aaBloom{};
    size_t clearCursor = 0;
    float delayTime = 1, grainOutput[2]{}, grainSmoothing = .1f;
    bool wasClocked = false;
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
    std::array<float, moduleCount> activityPeaks{};
    float topologyGain = 1;
    float smooth(float &a, float b)
    {
        a += (b - a) * smoothing;
        return a;
    }

  public:
    int selected = 0;
    std::atomic<float> meter{0};
    std::array<std::atomic<float>, moduleCount> moduleActivity{{0, 0, 0, 0, 0, 0}};
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
            g.assign((size_t)(sr * 5) + 2, 0);
        for (auto &c : reverb)
            for (auto &d : c)
                d.assign((size_t)(sr * .16) + 2, 0);
        character.prepare(sr);
        grainSmoothing = 1 - std::exp(-1.f / float(sr * .0008));
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
        clearCursor = ~size_t(0);
        resetState();
    }
    void markDirty() { clearCursor = 0; }
    // Clears up to `budget` buffer samples per call (delay, grain, reverb lines in a fixed order);
    // true once no stale audio remains. Used while the FX module sleeps outside the rack.
    bool clearStep(size_t budget)
    {
        std::vector<float> *buffers[]{&delay[0],     &grain[0],     &delay[1],     &grain[1],
                                      &reverb[0][0], &reverb[0][1], &reverb[0][2], &reverb[0][3],
                                      &reverb[1][0], &reverb[1][1], &reverb[1][2], &reverb[1][3]};
        size_t offset = 0;
        for (auto *buffer : buffers)
        {
            const size_t size = buffer->size();
            while (budget > 0 && clearCursor >= offset && clearCursor < offset + size)
            {
                const size_t n = std::min(budget, offset + size - clearCursor);
                std::fill_n(buffer->begin() + std::ptrdiff_t(clearCursor - offset), n, 0.f);
                clearCursor += n;
                budget -= n;
            }
            offset += size;
        }
        return clearCursor >= offset;
    }
    // Everything except the large buffers.
    void resetState()
    {
        rp = {};
        write = grainWrite = phase = 0;
        random = 12345;
        fragment = 0;
        fragmentLength = 1;
        std::fill_n(ghostTone, 2, 0);
        std::fill_n(bloomTone, 2, 0);
        aaGhost = {};
        aaBloom = {};
        initialized = false;
        topologyGain = 1;
        delayTime = 1;
        grainOutput[0] = grainOutput[1] = 0;
        wasClocked = false;
        std::fill_n(activation, moduleCount, 1.f);
        character.reset();
        displayPeak = 0;
        activityPeaks = {};
        for (auto &v : moduleActivity)
            v.store(0, std::memory_order_relaxed);
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
        smooth(sm.ghostMs, p.ghostMs);
        smooth(sm.crumbleHz, p.crumbleHz);
        smooth(sm.wobbleHz, p.wobbleHz);
        float wobbleRate = sm.wobble[1];
        double reference = -1;
        bool run = true;
        if (p.timingMode[2] > 0)
        {
            float hz = p.timingMode[2] == 1
                           ? sm.wobbleHz
                           : std::clamp(p.tempo, 30.f, 300.f) /
                                 (60 * beatDivisions[std::clamp(p.division[2], 0, 3)]);
            wobbleRate = (hz - .15f) / 2.85f;
            if (p.timingMode[2] == 2 && p.transportValid)
            {
                double cycles = p.transportBeat / beatDivisions[std::clamp(p.division[2], 0, 3)];
                reference = cycles - std::floor(cycles);
                run = p.transportPlaying;
            }
        }
        character.advance(std::max(std::abs(x[0]), std::abs(x[1])), wobbleRate, reference, run);
        character.beginCrush(crushAmount);
        const int delaySize = (int)delay[0].size(), grainSize = (int)grain[0].size();
        int beatSamples = std::max(16, int(sr * 60 / std::clamp(p.tempo, 30.f, 300.f)));
        const float divisions[]{.125f, .25f, .5f, 1.f, 2.f};
        int dt = std::clamp(int(beatSamples * divisions[std::min(4, int(sm.ghost[0] * 4.99f))]), 1,
                            delaySize - 1);
        if (p.timingMode[0] == 1)
            dt = std::clamp(int(sr * sm.ghostMs * .001), 1, delaySize - 1);
        else if (p.timingMode[0] == 2)
            dt = std::clamp(int(beatSamples * beatDivisions[std::clamp(p.division[0], 0, 3)]), 1,
                            delaySize - 1);
        smooth(delayTime, (float)dt);
        int period = std::clamp(int(beatSamples * std::pow(2., -1 - int(sm.crumble[0] * 4.99f))),
                                16, (int(sr) + 2) / 2);
        if (p.timingMode[1] == 1)
            period = std::clamp(int(sr / std::max(.5f, sm.crumbleHz)), 16, grainSize / 2);
        else if (p.timingMode[1] == 2)
            period = std::clamp(int(beatSamples * beatDivisions[std::clamp(p.division[1], 0, 3)]),
                                16, grainSize / 2);
        bool clocked = p.timingMode[1] == 2 && p.transportValid;
        int previous = phase;
        if (clocked && p.transportPlaying)
        {
            double cycles = p.transportBeat / beatDivisions[std::clamp(p.division[1], 0, 3)];
            phase = int((cycles - std::floor(cycles)) * period);
        }
        bool wrap = clocked ? (phase < previous || !wasClocked) : phase >= period;
        wasClocked = clocked;
        if (wrap)
        {
            if (!clocked)
                phase = 0;
            random = random * 1664525u + 1013904223u;
            fragmentLength = std::max(16, int(period * (.25f + .7f * (1 - sm.crumble[1]))));
            fragment = (grainWrite - fragmentLength -
                        int((random % 1024) / 1024.f * sm.mutation * period) + grainSize * 2) %
                       grainSize;
        }
        for (int m = 0; m < moduleCount; ++m)
            smooth(activation[m], p.enabled[m] ? 1.f : 0.f);
        for (auto &v : activityPeaks)
            v *= peakRelease;
        for (int module : activeOrder)
            for (int c = 0; c < 2; ++c)
            {
                if (module != selected)
                    continue;
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
                    float feedback = ghostTone[c] * std::min(.88f, sm.ghost[1] * (.5f + sm.trail));
                    // Residual ADAA: exact while linear, antialiased only where the loop clips.
                    delay[c][write] = fx::antialiasedResidual(
                        x[c] + feedback, aaGhost[c], [](float u) { return std::tanh(u); }, 1.f);
                    wet = ghostTone[c];
                }
                if (module == 1)
                {
                    grain[c][grainWrite] = x[c];
                    float w = .5f - .5f * std::cos(6.2831853f * (phase % fragmentLength) /
                                                   fragmentLength);
                    wet = grain[c][(fragment + phase % fragmentLength) % grainSize] * w;
                    if (clocked && !p.transportPlaying)
                    {
                        wet = x[c];
                        grainOutput[c] = wet;
                    }
                    else if (p.timingMode[1] > 0)
                    {
                        grainOutput[c] += (wet - grainOutput[c]) * grainSmoothing;
                        wet = grainOutput[c];
                    }
                    else
                        grainOutput[c] = wet;
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
                        float feedback = z * std::min(.94f, .3f + .6f * sm.bloom[2] + .04f * sm.trail);
                        // One antialiasing state per line (each line has its own argument).
                        reverb[c][k][idx] = fx::antialiasedResidual(
                            x[c] * .3f + feedback, aaBloom[c][size_t(k)],
                            [](float u) { return std::tanh(u); }, 1.f);
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
                if (module == 1 && p.xyCrumble)
                    mix = std::clamp(mix * sm.playY * 2, 0.f, 1.f);
                // Bypass crossfades instead of abruptly switching the signal.
                float &active = activation[module];
                mix *= active;
                activityPeaks[module] =
                    std::max(activityPeaks[module], std::abs((wet - x[c]) * mix) * sm.drywet);
                x[c] += (wet - x[c]) * mix;
            }
        character.finish();
        write = (write + 1) % delaySize;
        grainWrite = (grainWrite + 1) % grainSize;
        if (!clocked)
            ++phase;
        for (int c = 0; c < 2; ++c)
            x[c] = original[c] + (x[c] - original[c]) * sm.drywet;
        l = x[0] * topologyGain;
        r = x[1] * topologyGain;
        displayPeak = std::max(std::max(std::abs(l), std::abs(r)), displayPeak * peakRelease);
        // Publish decimated measurements only. UI graphics never touch audio-owned storage.
        if (++telemetryCounter >= 64)
        {
            telemetryCounter = 0;
            for (int m = 0; m < moduleCount; ++m)
                moduleActivity[m].store(activityPeaks[m], std::memory_order_relaxed);
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
