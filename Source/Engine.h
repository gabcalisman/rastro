#pragma once
#include "EffectRack.h"
#include "SynthVoice.h"
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
        bool release = false, legacyMode = false, synth = false, keyDown = true, forced = false,
             looping = false;
        double loopBegin = 0, loopEnd = 1, fade = 0;
        int direction = 1;
        int channel = 0, trigger = 0;
        unsigned modCounter = 0;
        double readStep = 1, modPitch = 1, modPitchTarget = 1;
        float glideCoefficient = 1, cutoff = 16000, resonance = .1f, last[2]{};
        SynthEnvelope adsr;
        LowPass lowpass[2];
    };
    std::array<Voice, 16> voices{};
    double sr = 44100;
    float velocity = .5f, smoothing = .001f;
    struct Held
    {
        int note = -1, channel = 0;
        float velocity = 0;
    };
    std::array<Held, 128> held{};
    int heldCount = 0;
    unsigned controlCounter = 0;
    std::array<bool, 16> pedals{};
    std::array<float, 16> bendRaw{}, bendTarget{}, bend{}, wheel{};
    float lfoPhase = 0, lfo = 0, lfoSpeed = 1, stealTail[2]{};
    bool gated(const Voice &v, const Settings &p) const
    {
        if (v.note == 255 || v.trigger == 2)
            return true;
        if (v.trigger == 1)
            return false;
        return (v.legacyMode ? (p.loop > .5f ? 1 : 0) : v.mode) != 0;
    }
    void forget(int note, int channel)
    {
        for (int i = 0; i < heldCount;)
            if (held[i].note == note && held[i].channel == channel)
            {
                for (int j = i + 1; j < heldCount; ++j)
                    held[j - 1] = held[j];
                --heldCount;
            }
            else
                ++i;
    }

  public:
    const Sample *currentSample = nullptr; // audio-thread only, retained by processor
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
        heldCount = controlCounter = 0;
        pedals = {};
        bendRaw = {};
        wheel = {};
        bend.fill(1);
        bendTarget.fill(1);
        lfoPhase = lfo = 0;
        lfoSpeed = 1;
        stealTail[0] = stealTail[1] = 0;
        position = 0;
        previewActive = false;
        playbackActive = false;
    }
    void reset()
    {
        voices = {};
        heldCount = controlCounter = 0;
        pedals = {};
        bendRaw = {};
        wheel = {};
        bend.fill(1);
        bendTarget.fill(1);
        lfoPhase = lfo = 0;
        lfoSpeed = 1;
        stealTail[0] = stealTail[1] = 0;
        position = 0;
        previewActive = false;
        playbackActive = false;
        rack.reset();
    }
    void clearVoices()
    {
        voices = {};
        heldCount = 0;
        previewActive = false;
        playbackActive = false;
    }
    void noteOn(int note, float vel, const Sample *sample, const Settings &p, int channel = 1,
                bool remember = true)
    {
        noteVelocity.store(std::clamp(vel, 0.f, 1.f), std::memory_order_relaxed);
        noteSerial.fetch_add(1, std::memory_order_relaxed);
        if (!sample || sample->size() < 2)
            return;
        currentSample = sample;
        int mode = p.mode;
        if (mode == 0 && p.loop > .5f)
            mode = 1;
        int slice = note - (int)p.root;
        if (mode == 4 && (slice < 0 || slice >= p.sliceCount))
            return;
        channel = std::clamp(channel - 1, 0, 15);
        if (remember && note < 128)
        {
            forget(note, channel);
            if (heldCount == 128)
            {
                for (int i = 1; i < 128; ++i)
                    held[i - 1] = held[i];
                --heldCount;
            }
            held[heldCount++] = {note, channel, vel};
        }
        // Free voice first; when all are busy, steal the quietest one (released voices first).
        Voice *v = nullptr;
        for (auto &x : voices)
            if (x.note < 0)
            {
                v = &x;
                break;
            }
        if (!v)
        {
            v = &voices[0];
            float quietest = 1e9f;
            for (auto &x : voices)
            {
                if (x.note == 255)
                    continue;
                const float level = x.env * x.gain * (x.release ? .25f : 1.f);
                if (level < quietest)
                {
                    quietest = level;
                    v = &x;
                }
            }
        }
        if (note == 255)
            for (auto &x : voices)
                if (x.note == 255)
                {
                    v = &x;
                    break;
                }
        if (p.voiceMode > 0 && note != 255)
        {
            v = &voices[0];
            if (v->note == 255)
                v = &voices[1];
            for (auto &other : voices)
                if (&other != v && other.note >= 0 && other.note != 255)
                {
                    for (int c = 0; c < 2; ++c)
                        stealTail[c] += other.last[c];
                    other = {};
                }
        }
        auto before = *v;
        bool legato = p.voiceMode == 2 && before.note >= 0 && !before.release && note != 255 &&
                      before.synth == p.synth && (before.trigger == 2) == (p.triggerMode == 2);
        // Any voice that was still sounding hands its last output to a short tail (no hard cut).
        if (before.note >= 0 && !legato)
            for (int c = 0; c < 2; ++c)
                stealTail[c] = std::clamp(stealTail[c] + before.last[c], -2.f, 2.f);
        *v = {};
        v->synth = p.synth;
        v->channel = channel;
        v->trigger = p.triggerMode;
        v->adsr.start();
        if (legato && before.synth == p.synth)
        {
            v->adsr = before.adsr;
            v->env = before.env;
            for (int c = 0; c < 2; ++c)
            {
                v->filter[c] = before.filter[c];
                v->lowpass[c] = before.lowpass[c];
            }
        }
        v->cutoff = p.cutoff;
        v->resonance = p.resonance;
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
        v->looping = p.regionLoop && note != 255;
        v->loopBegin = begin;
        v->loopEnd = end;
        if (p.loopRegionSet && v->looping)
        {
            double a = std::max(begin, double(p.loopStart) * (n - 1)),
                   b = std::min(end, double(p.loopEnd) * (n - 1));
            if (b - a >= 1)
            {
                v->loopBegin = a;
                v->loopEnd = b;
            }
        }
        v->fade = std::clamp(double(p.loopCrossfade) * .001 * sample->rate, 0.,
                             (v->loopEnd - v->loopBegin) * .25);
        v->direction = mode == 2 ? -1 : 1;
        v->begin = begin;
        v->end = end;
        v->pos = mode == 2 ? std::max(begin, end - 1) : begin;
        v->step = sample->rate / sr *
                  (mode == 4 || p.pitchMode == 1 ? 1. : std::pow(2., (note - p.root) / 12.));
        v->readStep =
            before.note >= 0 && p.voiceMode > 0 && p.glide > 0 ? before.readStep : v->step;
        v->glideCoefficient =
            p.voiceMode > 0 && p.glide > 0 ? float(1 - std::exp(-1 / (sr * p.glide))) : 1;
        if (legato && mode != 4)
        {
            v->pos = std::clamp(before.pos, begin, end - .0001);
            v->travel = before.travel;
        }
    }
    void noteOff(int note, const Settings &p, int channel = 1)
    {
        channel = std::clamp(channel - 1, 0, 15);
        forget(note, channel);
        for (auto &v : voices)
            if (v.note == note && (note == 255 || v.channel == channel))
            {
                v.keyDown = false;
                if (note != 255 && p.voiceMode > 0 && heldCount > 0 && currentSample)
                {
                    auto h = held[heldCount - 1];
                    noteOn(h.note, h.velocity, currentSample, p, h.channel + 1, false);
                    return;
                }
                if (gated(v, p) && (!pedals[channel] || note == 255))
                    v.release = true;
                if (note == 255)
                    v.forced = true;
            }
    }
    void sustainPedal(int channel, bool down, const Settings &p)
    {
        channel = std::clamp(channel - 1, 0, 15);
        pedals[channel] = down;
        if (!down)
            for (auto &v : voices)
                if (v.channel == channel && !v.keyDown && gated(v, p))
                    v.release = true;
    }
    void pitchBend(int channel, float amount)
    {
        bendRaw[std::clamp(channel - 1, 0, 15)] = std::clamp(amount, -1.f, 1.f);
    }
    void modWheel(int channel, float amount)
    {
        wheel[std::clamp(channel - 1, 0, 15)] = std::clamp(amount, 0.f, 1.f);
    }
    int activeVoiceCount() const
    {
        int n = 0;
        for (auto &v : voices)
            n += v.note >= 0;
        return n;
    }
    void allOff()
    {
        heldCount = 0;
        pedals = {};
        for (auto &v : voices)
        {
            v.release = true;
            v.forced = true;
        }
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
        currentSample = sample;
        if (controlCounter++ % 32 == 0)
            for (int c = 0; c < 16; ++c)
                bendTarget[c] = std::pow(2.f, bendRaw[c] * p.bendRange / 12.f);
        for (int c = 0; c < 16; ++c)
            bend[c] += (bendTarget[c] - bend[c]) * smoothing;
        float rate = p.lfoSync == 0 ? p.lfoRate
                                    : p.tempo / 60 *
                                          (p.lfoSync == 1   ? 1
                                           : p.lfoSync == 2 ? 2
                                                            : .5f);
        if (p.lfoTiming == 1)
            rate = p.lfoRate;
        else if (p.lfoTiming == 2)
            rate = std::clamp(p.tempo, 30.f, 300.f) /
                   (60 * beatDivisions[std::clamp(p.lfoDivision, 0, 3)]);
        if (p.lfoTiming == 2 && p.transportValid && !p.transportPlaying)
            rate = 0;
        lfoSpeed += (rate - lfoSpeed) * smoothing;
        lfoPhase += lfoSpeed / float(sr);
        if (lfoPhase >= 1)
            lfoPhase -= 1;
        if (p.lfoTiming == 2 && p.transportValid)
        {
            double cycles = p.transportBeat / beatDivisions[std::clamp(p.lfoDivision, 0, 3)];
            float target = float(cycles - std::floor(cycles));
            float delta = target - lfoPhase;
            delta -= std::floor(delta + .5f);
            lfoPhase += std::clamp(delta, -.002f, .002f);
            if (lfoPhase < 0)
                lfoPhase += 1;
        }
        lfo = std::sin(lfoPhase * 6.28318530718f);
        float signal[2]{};
        for (int c = 0; c < 2; ++c)
        {
            signal[c] = stealTail[c];
            stealTail[c] *= float(1 - 1 / (sr * .005));
            if (std::abs(stealTail[c]) < 1e-8f)
                stealTail[c] = 0;
        }
        bool preview = false;
        if (sample && sample->size() > 1)
            for (auto &v : voices)
                if (v.note >= 0)
                {
                    int mode = v.legacyMode ? (p.loop > .5f ? 1 : 0) : v.mode;
                    double span = v.end - v.begin;
                    if (v.looping)
                    {
                        double loopSpan = v.loopEnd - v.loopBegin;
                        if (mode == 3)
                        {
                            // Reflected interpolated position: continuous value at both
                            // turnarounds.
                            if (v.pos >= v.loopEnd || (v.pos < v.loopBegin && v.direction < 0))
                            {
                                double unfolded = v.direction < 0
                                                      ? 2 * loopSpan - (v.pos - v.loopBegin)
                                                      : v.pos - v.loopBegin;
                                double t = std::fmod(unfolded, 2 * loopSpan);
                                if (t < 0)
                                    t += 2 * loopSpan;
                                v.pos = v.loopBegin + (t <= loopSpan ? t : 2 * loopSpan - t);
                                v.direction = t < loopSpan ? 1 : -1;
                            }
                            v.pos = std::min(v.pos, v.end - .0001);
                        }
                        else if (mode == 2 && v.pos < v.loopBegin)
                            v.pos = v.loopEnd - v.fade -
                                    std::fmod(v.loopBegin - v.pos, loopSpan - v.fade);
                        else if (mode != 2 && v.pos >= v.loopEnd)
                            v.pos = v.loopBegin + v.fade +
                                    std::fmod(v.pos - v.loopEnd, loopSpan - v.fade);
                    }
                    else if (mode == 3)
                    {
                        double t = std::fmod(v.travel, span * 2);
                        v.pos = v.begin + (t <= span ? t : 2 * span - t);
                        v.pos = std::min(v.pos, v.end - .0001);
                    }
                    else if (v.pos >= v.end || v.pos < v.begin)
                    {
                        if (mode == 1 && !v.release)
                            v.pos = v.fade > 0 && span > v.fade
                                        ? v.begin + v.fade + std::fmod(v.pos - v.end, span - v.fade)
                                        : v.begin + std::fmod(v.pos - v.begin, span);
                        else
                        {
                            v.note = -1;
                            continue;
                        }
                    }
                    if (v.synth || v.trigger == 2)
                        v.env = v.adsr.tick(sr, p.attack, p.decay, p.sustain,
                                            v.forced ? .02f : p.release, !v.release);
                    else
                        v.env += v.release ? -float(1 / (sr * .03)) : float(1 / (sr * .004));
                    v.env = std::min(1.f, v.env);
                    if (v.env <= 0)
                    {
                        v.note = -1;
                        continue;
                    }
                    int a = std::clamp((int)v.pos, 0, sample->size() - 2);
                    float f = float(v.pos - a);
                    // Short fades only where the sound really starts or stops. A wrapping loop or a
                    // ping-pong turn is continuous, so it must not dip to silence on every pass.
                    const bool wraps = v.looping || mode == 3 || (mode == 1 && !v.release);
                    float edge =
                        wraps ? float(std::min(1., v.travel / std::max(1., sr * .002)))
                              : float(std::min({1., (v.pos - v.begin) / std::max(1., sr * .002),
                                                (v.end - v.pos) / std::max(1., sr * .003)}));
                    if (v.looping)
                        edge = 1;
                    float cutoff =
                        .035f + .85f * (1 - velocity + velocity * std::min(1.f, v.gain / .23f));
                    if (v.synth && v.modCounter++ % 16 == 0)
                    {
                        float amount = std::clamp(p.lfoDepth + wheel[v.channel], 0.f, 1.f);
                        float filterOctaves = (p.envTarget == 0 ? p.envAmount * v.env * 4 : 0) +
                                              (p.lfoTarget == 0 ? lfo * amount * 2 : 0);
                        float target = p.cutoff * std::pow(2.f, filterOctaves);
                        float follow = float(1 - std::exp(-16 / (sr * .015)));
                        v.cutoff += (std::clamp(target, 20.f, float(sr * .45)) - v.cutoff) * follow;
                        v.resonance += (p.resonance - v.resonance) * follow;
                        for (auto &filter : v.lowpass)
                            filter.coefficients(sr, v.cutoff, v.resonance);
                        float semitones = (p.envTarget == 1 ? p.envAmount * v.env * 12 : 0) +
                                          (p.lfoTarget == 1 ? lfo * amount * 2 : 0);
                        v.modPitchTarget = std::pow(2., semitones / 12);
                    }
                    for (int c = 0; c < 2; ++c)
                    {
                        float z =
                            sample->data[c][a] + f * (sample->data[c][a + 1] - sample->data[c][a]);
                        if ((v.looping || (mode == 1 && !v.release)) && v.fade > 0 && mode != 3)
                        {
                            double other = v.pos;
                            float blend = 0;
                            if (mode == 2 && v.pos < v.loopBegin + v.fade)
                            {
                                blend = float((v.loopBegin + v.fade - v.pos) / v.fade);
                                other = v.loopEnd - v.fade + v.pos - v.loopBegin;
                            }
                            else if (mode != 2 && v.pos > v.loopEnd - v.fade)
                            {
                                blend = float((v.pos - v.loopEnd + v.fade) / v.fade);
                                other = v.loopBegin + v.pos - (v.loopEnd - v.fade);
                            }
                            if (blend > 0)
                            {
                                other = std::clamp(other, 0., double(sample->size() - 1) - .0001);
                                int a2 = std::clamp(int(other), 0, sample->size() - 2);
                                float f2 = float(other - a2);
                                float next = sample->data[c][a2] +
                                             f2 * (sample->data[c][a2 + 1] - sample->data[c][a2]);
                                z += (next - z) * std::clamp(blend, 0.f, 1.f);
                            }
                        }
                        v.filter[c] += cutoff * (z - v.filter[c]);
                        float filtered = v.synth ? v.lowpass[c].process(v.filter[c]) : v.filter[c];
                        v.last[c] = filtered * v.env * v.gain * edge;
                        signal[c] += v.last[c];
                    }
                    position.store(float(v.pos / sample->size()), std::memory_order_relaxed);
                    if (v.note == 255)
                        preview = true;
                    v.modPitch += (v.modPitchTarget - v.modPitch) * smoothing;
                    v.readStep += (v.step - v.readStep) * v.glideCoefficient;
                    double step = v.readStep * (v.note == 255 ? 1 : bend[v.channel] * v.modPitch);
                    v.travel += step;
                    v.pos += v.looping && mode == 3 ? v.direction * step : mode == 2 ? -step : step;
                }
        previewActive.store(preview, std::memory_order_relaxed);
        l = signal[0];
        r = signal[1];
        rack.process(l, r, p);
    }
};
} // namespace rastro
