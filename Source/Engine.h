#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>
namespace rastro
{
struct Sample
{
    std::array<std::vector<float>, 2> data;
    double rate = 44100;
    int size() const { return (int)data[0].size(); }
};
struct Settings
{
    float root = 60, start = 0, end = 1, loop = 0, level = .7f, velocity = .5f, tempo = 120,
          presence = .5f, trail = .5f, mutation = .3f, drywet = 1;
    std::array<float, 4> ghost{{.5f, .35f, .6f, .3f}}, bloom{{.6f, .6f, .5f, .25f}};
    std::array<float, 3> crumble{{.5f, .3f, .2f}};
    std::array<bool, 3> enabled{{true, true, true}};
    std::array<int, 3> order{{0, 1, 2}};
};
class Engine
{
    struct Voice
    {
        double pos = 0, step = 1, begin = 0, end = 1;
        float gain = 0, env = 0, filter[2]{};
        int note = -1;
        bool release = false;
    };
    std::array<Voice, 16> voices{};
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
    std::array<int, 3> activeOrder{{0, 1, 2}};
    float topologyGain = 1;
    float smooth(float &a, float b)
    {
        a += (b - a) * smoothing;
        return a;
    }

  public:
    std::atomic<float> meter{0}, position{0};
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
        reset();
    }
    void reset()
    {
        voices = {};
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
        std::fill_n(activation, 3, 1.f);
        meter = 0;
        position = 0;
    }
    void noteOn(int note, float velocity, const Sample *s, const Settings &p)
    {
        if (!s || s->size() < 2)
            return;
        auto *v = &voices[0];
        for (auto &x : voices)
            if (x.note < 0)
            {
                v = &x;
                break;
            }
        *v = {};
        v->note = note;
        v->pos = std::clamp(p.start, 0.f, .999f) * (s->size() - 1);
        v->step = s->rate / sr * std::pow(2., (note - p.root) / 12.);
        v->gain = velocity * .23f;
        v->begin = v->pos;
        v->end = std::max(v->begin + 1., double(std::clamp(p.end, .001f, 1.f) * (s->size() - 1)));
    }
    void noteOff(int note)
    {
        for (auto &v : voices)
            if (v.note == note)
                v.release = true;
    }
    void clearVoices() { voices = {}; }
    void allOff()
    {
        for (auto &v : voices)
            v.release = true;
    }
    // All storage is prepared off the audio thread. No allocation, ownership, locks, or file I/O
    // here.
    void frame(float &l, float &r, const Sample *source, const Settings &p)
    {
        if (!initialized)
        {
            sm = p;
            activeOrder = p.order;
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
        float x[2]{};
        if (source && source->size() > 1)
            for (auto &v : voices)
                if (v.note >= 0)
                {
                    const int n = source->size();
                    double begin = v.begin, end = v.end;
                    if (v.pos >= end)
                    {
                        if (p.loop > .5f && !v.release)
                            v.pos = begin + std::fmod(v.pos - begin, end - begin);
                        else
                        {
                            v.note = -1;
                            continue;
                        }
                    }
                    if (v.pos < begin)
                        v.pos = begin;
                    v.env += v.release ? -float(1 / (sr * .03)) : float(1 / (sr * .004));
                    v.env = std::min(1.f, v.env);
                    if (v.env <= 0)
                    {
                        v.note = -1;
                        continue;
                    }
                    int a = std::clamp((int)v.pos, 0, n - 2);
                    float f = float(v.pos - a);
                    float edge = float(std::min({1., (v.pos - begin) / std::max(1., sr * .002),
                                                 (end - v.pos) / std::max(1., sr * .003)}));
                    float cutoff = .035f + .85f * (1 - sm.velocity +
                                                   sm.velocity * std::min(1.f, v.gain / .23f));
                    for (int c = 0; c < 2; ++c)
                    {
                        float z =
                            source->data[c][a] + f * (source->data[c][a + 1] - source->data[c][a]);
                        v.filter[c] += cutoff * (z - v.filter[c]);
                        x[c] += v.filter[c] * v.env * v.gain * edge;
                    }
                    v.pos += v.step;
                    position.store(float(v.pos / n), std::memory_order_relaxed);
                }
        float original[2]{x[0], x[1]};
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
        for (int m = 0; m < 3; ++m)
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
                float mix = module == 0 ? sm.ghost[3] : (module == 1 ? sm.crumble[2] : sm.bloom[3]);
                // Bypass crossfades instead of abruptly switching the signal.
                float &active = activation[module];
                mix *= active;
                x[c] += (wet - x[c]) * mix;
            }
        write = (write + 1) % delaySize;
        grainWrite = (grainWrite + 1) % grainSize;
        ++phase;
        for (int c = 0; c < 2; ++c)
            x[c] = std::tanh((original[c] + (x[c] - original[c]) * sm.drywet) * sm.level *
                             (.6f + sm.presence));
        l = x[0] * topologyGain;
        r = x[1] * topologyGain;
        meter.store(std::max(std::abs(l), std::abs(r)), std::memory_order_relaxed);
    }

  private:
    float activation[3]{1, 1, 1};
};
} // namespace rastro
