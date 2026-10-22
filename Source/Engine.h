#pragma once
#include "EffectRack.h"
namespace rastro
{
// MIDI sampler only. EffectRack is separately reusable by a future audio-input product.
class Engine
{
    struct Voice
    {
        double pos = 0, step = 1, begin = 0, end = 1, travel = 0;
        float gain = 0, env = 0, filter[2]{};
        int note = -1, mode = 0;
        bool release = false, legacyMode = false;
    };
    std::array<Voice, 16> voices{};
    double sr = 44100;
    float velocity = .5f, smoothing = .001f;

  public:
    EffectRack rack;
    std::atomic<float> &meter = rack.meter;
    std::atomic<float> &visualWobble = rack.visualWobble;
    std::atomic<float> &visualDust = rack.visualDust;
    std::atomic<float> &visualCrush = rack.visualCrush;
    std::atomic<float> &visualBloom = rack.visualBloom;
    std::atomic<float> &visualEnvelope = rack.visualEnvelope;
    std::atomic<float> &visualX = rack.visualX;
    std::atomic<float> &visualY = rack.visualY;
    std::atomic<float> position{0};
    std::atomic<float> noteVelocity{0};
    std::atomic<unsigned> noteSerial{0};
    std::atomic<bool> previewActive{false}, playbackActive{false};
    std::atomic<unsigned> blockSerial{0};
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        smoothing = 1 - std::exp(-1.f / float(sr * .015));
        rack.prepare(sr);
        voices = {};
        position = 0;
        previewActive = false;
        playbackActive = false;
    }
    void reset()
    {
        voices = {};
        position = 0;
        previewActive = false;
        playbackActive = false;
        rack.reset();
    }
    void clearVoices()
    {
        voices = {};
        previewActive = false;
        playbackActive = false;
    }
    void noteOn(int note, float vel, const Sample *sample, const Settings &p)
    {
        noteVelocity.store(std::clamp(vel, 0.f, 1.f), std::memory_order_relaxed);
        noteSerial.fetch_add(1, std::memory_order_relaxed);
        if (!sample || sample->size() < 2)
            return;
        int mode = p.mode;
        if (mode == 0 && p.loop > .5f)
            mode = 1;
        int slice = note - (int)p.root;
        if (mode == 4 && (slice < 0 || slice >= p.sliceCount))
            return;
        Voice *v = &voices[0];
        for (auto &x : voices)
            if (x.note < 0)
            {
                v = &x;
                break;
            }
        if (note == 255)
            for (auto &x : voices)
                if (x.note == 255)
                {
                    v = &x;
                    break;
                }
        *v = {};
        v->mode = mode;
        v->legacyMode = p.mode == 0 && note != 255;
        v->note = note;
        v->gain = vel * .23f;
        double n = sample->size();
        double begin = std::clamp(p.start, 0.f, .999f) * (n - 1);
        double end = std::clamp(p.end, .001f, 1.f) * (n - 1);
        begin = std::min(begin, n - 2);
        end = std::clamp(end, begin + 1, n - 1);
        if (mode == 4)
        {
            double range = end - begin;
            double a = begin + range * p.cuts[slice], b = begin + range * p.cuts[slice + 1];
            begin = std::min(a, n - 2);
            end = std::clamp(b, begin + 1, n - 1);
        }
        v->begin = begin;
        v->end = end;
        v->pos = mode == 2 ? std::max(begin, end - 1) : begin;
        v->step = sample->rate / sr *
                  (mode == 4 || p.pitchMode == 1 ? 1. : std::pow(2., (note - p.root) / 12.));
    }
    void noteOff(int note, const Settings &p)
    {
        for (auto &v : voices)
            if (v.note == note)
            {
                // One Shot is triggered, not gated. Legacy loop automation still determines
                // the effective mode; preview stop and allOff always release explicitly.
                const int mode = v.legacyMode ? (p.loop > .5f ? 1 : 0) : v.mode;
                if (mode != 0 || note == 255)
                    v.release = true;
            }
    }
    void allOff()
    {
        for (auto &v : voices)
            v.release = true;
    }
    void endBlock()
    {
        rack.endBlock();
        bool active = false;
        for (const auto &v : voices)
            active |= v.note >= 0 && v.env > 0;
        playbackActive.store(active, std::memory_order_relaxed);
        blockSerial.fetch_add(1, std::memory_order_relaxed);
    }
    void frame(float &l, float &r, const Sample *sample, const Settings &p)
    {
        velocity += (p.velocity - velocity) * smoothing;
        float signal[2]{};
        bool preview = false;
        if (sample && sample->size() > 1)
            for (auto &v : voices)
                if (v.note >= 0)
                {
                    int mode = v.legacyMode ? (p.loop > .5f ? 1 : 0) : v.mode;
                    double span = v.end - v.begin;
                    if (mode == 3)
                    {
                        double t = std::fmod(v.travel, span * 2);
                        v.pos = v.begin + (t <= span ? t : 2 * span - t);
                        v.pos = std::min(v.pos, v.end - .0001);
                    }
                    else if (v.pos >= v.end || v.pos < v.begin)
                    {
                        if (mode == 1 && !v.release)
                            v.pos = v.begin + std::fmod(v.pos - v.begin, span);
                        else
                        {
                            v.note = -1;
                            continue;
                        }
                    }
                    v.env += v.release ? -float(1 / (sr * .03)) : float(1 / (sr * .004));
                    v.env = std::min(1.f, v.env);
                    if (v.env <= 0)
                    {
                        v.note = -1;
                        continue;
                    }
                    int a = std::clamp((int)v.pos, 0, sample->size() - 2);
                    float f = float(v.pos - a);
                    float edge = float(std::min({1., (v.pos - v.begin) / std::max(1., sr * .002),
                                                 (v.end - v.pos) / std::max(1., sr * .003)}));
                    float cutoff =
                        .035f + .85f * (1 - velocity + velocity * std::min(1.f, v.gain / .23f));
                    for (int c = 0; c < 2; ++c)
                    {
                        float z =
                            sample->data[c][a] + f * (sample->data[c][a + 1] - sample->data[c][a]);
                        v.filter[c] += cutoff * (z - v.filter[c]);
                        signal[c] += v.filter[c] * v.env * v.gain * edge;
                    }
                    position.store(float(v.pos / sample->size()), std::memory_order_relaxed);
                    if (v.note == 255)
                        preview = true;
                    v.travel += v.step;
                    v.pos += mode == 2 ? -v.step : v.step;
                }
        previewActive.store(preview, std::memory_order_relaxed);
        l = signal[0];
        r = signal[1];
        rack.process(l, r, p);
    }
};
} // namespace rastro
