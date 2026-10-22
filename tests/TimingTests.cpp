#include "Engine.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
thread_local bool inAudio = false;
void *operator new(size_t n)
{
    if (inAudio)
        throw std::runtime_error("timing audio allocation");
    if (auto *p = std::malloc(n))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
std::vector<float> render(int module, int mode, float bpm, int division = 0, bool clock = false)
{
    rastro::EffectRack rack;
    rack.prepare(48000);
    rastro::Settings p;
    p.enabled.fill(false);
    p.enabled[module] = true;
    p.timingMode.fill(mode);
    p.division.fill(division);
    p.ghostMs = 150;
    p.crumbleHz = 7;
    p.wobbleHz = 2;
    p.tempo = bpm;
    p.ghost = {.5f, .4f, .6f, 1};
    p.crumble = {.5f, .7f, 1};
    p.wobble = {.6f, .5f, 1};
    p.transportValid = clock;
    p.transportPlaying = clock;
    std::vector<float> out(48000);
    inAudio = true;
    for (size_t i = 0; i < out.size(); ++i)
    {
        float l = .25f * std::sin(float(i) * .13f) + .12f * std::sin(float(i) * .031f), r = l;
        p.transportBeat = double(i) * bpm / (60 * 48000);
        rack.process(l, r, p);
        check(std::isfinite(l) && std::abs(l) <= 1, "timed effects remain bounded");
        out[i] = l;
    }
    inAudio = false;
    return out;
}
int main()
{
    for (int module : {0, 1, 3})
    {
        check(render(module, 1, 60) == render(module, 1, 180),
              "FREE ms/Hz audio is independent of host BPM");
        auto a = render(module, 2, 60), b = render(module, 2, 120);
        double difference = 0;
        for (size_t i = 0; i < a.size(); ++i)
            difference += std::abs(a[i] - b[i]);
        check(difference > 1, "SYNC BPM changes actual audible timing");
        for (int division = 0; division < 4; ++division)
        {
            auto c = render(module, 2, 120, division, true);
            check(!c.empty(), "all musical divisions render with PPQ");
        }
    }
    for (double sr : {22050., 44100., 48000., 96000.})
    {
        rastro::Sample sample;
        sample.rate = sr;
        for (auto &c : sample.data)
        {
            c.resize(int(sr));
            for (int i = 0; i < int(sr); ++i)
                c[i] = .4f * std::sin(i * .07f);
        }
        rastro::Settings p;
        p.enabled.fill(false);
        p.synth = true;
        p.regionLoop = true;
        p.loopRegionSet = true;
        p.loopStart = .2f;
        p.loopEnd = .6f;
        p.loopCrossfade = 10;
        p.triggerMode = 1;
        p.release = .05f;
        rastro::Engine e;
        e.prepare(sr);
        auto frames = [&](int count)
        {
            inAudio = true;
            for (int i = 0; i < count; ++i)
            {
                float l, r;
                e.frame(l, r, &sample, p);
                check(std::isfinite(l) && std::abs(l) <= 1, "all region loop modes are stable");
            }
            inAudio = false;
        };
        for (int mode = 0; mode < 5; ++mode)
        {
            e.reset();
            p.mode = mode;
            p.triggerMode = 1;
            e.noteOn(60, 1, &sample, p);
            frames(int(sr * 1.2));
            e.noteOff(60, p);
            frames(int(sr * .2));
            check(e.activeVoiceCount() == 1, "One Shot LOOP ignores note-off in every mode");
            float lo = mode == 4 ? 0 : .2f, hi = mode == 4 ? .125f : .6f;
            check(e.position >= lo - .001f && e.position <= hi + .001f,
                  "loop respects own region and Slice intersection");
            if (mode == 3)
            {
                float first = e.position;
                frames(int(sr * .1));
                check(std::abs(e.position - first) > .02f,
                      "Ping-Pong lower reflection resumes traversal rather than jittering");
            }
            e.allOff();
            frames(int(sr * .03));
            check(e.activeVoiceCount() == 0, "all-off stops infinite One Shot loop");
            e.reset();
            p.triggerMode = 2;
            e.noteOn(60, 1, &sample, p);
            frames(int(sr * .2));
            e.noteOff(60, p);
            frames(int(sr * .06));
            check(e.activeVoiceCount() == 0,
                  "Gate/Hold releases looping source in every playback mode");
        }
        rastro::Sample ramp;
        ramp.rate = sr;
        for (auto &c : ramp.data)
        {
            c.resize(int(sr));
            for (int i = 0; i < int(sr); ++i)
                c[i] = float(i / sr - .5) * .8f;
        }
        p.synth = false;
        p.mode = 0;
        p.triggerMode = 1;
        auto seam = [&](float fade)
        {
            e.reset();
            p.loopCrossfade = fade;
            e.noteOn(60, 1, &ramp, p);
            float previous = 0, maximum = 0;
            for (int i = 0; i < int(sr); ++i)
            {
                float l, r;
                e.frame(l, r, &ramp, p);
                if (i > sr * .1)
                    maximum = std::max(maximum, std::abs(l - previous));
                previous = l;
            }
            return maximum;
        };
        check(seam(10) < seam(0) * .4f, "overlap crossfade reduces measured loop seam jump");
    }
    std::cout << "PASS timing: FREE BPM independence, audible SYNC, four divisions/PPQ, "
                 "independent LOOP all five modes/One Shot/Gate, Ping-Pong direction, reduced seam "
                 "discontinuity, four sample rates; no audio allocations\n";
}
