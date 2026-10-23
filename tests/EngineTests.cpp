#include "Engine.h"
#include <algorithm>
#include <cstdlib>
#include <string>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>
static bool audio = false;
void *operator new(std::size_t n)
{
    if (audio)
        throw std::runtime_error("audio allocation");
    if (auto *p = std::malloc(n))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void check(bool ok, const char *m)
{
    if (!ok)
        throw std::runtime_error(m);
}
int main()
{
    for (double sr : {22050., 44100., 48000., 96000.})
    {
        rastro::Sample sample;
        sample.rate = sr;
        for (int c = 0; c < 2; ++c)
        {
            sample.data[c].resize((size_t)sr);
            for (int i = 0; i < (int)sr; ++i)
                sample.data[c][i] = .6f * std::sin(float(i * 440 * 6.283185307 / sr));
        }
        rastro::Engine engine;
        engine.prepare(sr);
        rastro::Settings s;
        s.enabled = {false, false, false};
        s.loop = 1;
        s.level = 1;
        engine.noteOn(60, 1, &sample, s);
        float energy = 0;
        audio = true;
        for (int i = 0; i < 5000; ++i)
        {
            float l, r;
            engine.frame(l, r, &sample, s);
            check(std::isfinite(l) && std::abs(l) <= 1, "finite bounded output");
            energy += l * l;
        }
        audio = false;
        check(energy > 1, "note must sound");
        engine.noteOff(60, s);
        audio = true;
        for (int i = 0; i < (int)sr; ++i)
        {
            float l, r;
            engine.frame(l, r, &sample, s);
        }
        audio = false;
        check(engine.meter < 1e-5, "note off releases");
        for (int effect = 0; effect < 3; ++effect)
        {
            engine.reset();
            s.enabled = {false, false, false};
            s.enabled[effect] = true;
            s.ghost[3] = 1;
            s.crumble[2] = 1;
            s.bloom[3] = 1;
            engine.noteOn(60, 1, &sample, s);
            float sum = 0;
            audio = true;
            for (int i = 0; i < (int)sr * 2; ++i)
            {
                float l, r;
                engine.frame(l, r, &sample, s);
                check(std::isfinite(l) && std::isfinite(r) && std::abs(l) <= 1, "effect stability");
                sum += l * l;
            }
            audio = false;
            check(sum > .01, "effect audible");
        }
        // Extreme trim, voice stealing, maximum feedback, reversed range and null source.
        s.start = .999f;
        s.end = .001f;
        s.ghost[1] = 1;
        s.bloom[2] = 1;
        s.enabled = {true, true, true};
        for (int i = 0; i < 32; ++i)
            engine.noteOn(i + 48, 1, &sample, s);
        audio = true;
        for (int i = 0; i < 10000; ++i)
        {
            float l, r;
            engine.frame(l, r, &sample, s);
            check(std::isfinite(l), "extreme trim finite");
        }
        engine.allOff();
        for (int i = 0; i < 100; ++i)
        {
            float l, r;
            engine.frame(l, r, nullptr, s);
        }
        audio = false;
    }
    // Serial vs parallel rack: parallel must sound different, stay bounded, and not depend on order.
    {
        const double sr = 48000;
        rastro::Sample sample;
        sample.rate = sr;
        for (int c = 0; c < 2; ++c)
        {
            sample.data[c].resize((size_t)sr);
            for (int i = 0; i < (int)sr; ++i)
                sample.data[c][i] = .5f * std::sin(float(i * 220 * 6.283185307 / sr));
        }
        auto render = [&](bool parallel, rastro::Order order, std::vector<float> &out)
        {
            rastro::Engine engine;
            engine.prepare(sr);
            rastro::Settings s;
            s.loop = 1;
            s.level = 1;
            s.parallel = parallel;
            s.order = order;
            s.enabled = {true, false, true, false, false, true}; // GHOST, BLOOM, CRUSH
            s.ghost[3] = .8f;
            s.bloom[3] = .8f;
            s.crush[0] = .8f;
            s.crush[1] = .8f;
            engine.noteOn(60, 1, &sample, s);
            out.resize(24000);
            audio = true;
            for (auto &v : out)
            {
                float l, r;
                engine.frame(l, r, &sample, s);
                check(std::isfinite(l) && std::abs(l) <= 1.5f, "rack bounded");
                v = l;
            }
            audio = false;
        };
        std::vector<float> serialA, serialB, parallelA, parallelB;
        render(false, {{0, 1, 2, 3, 4, 5}}, serialA);
        render(false, {{5, 4, 3, 2, 1, 0}}, serialB);
        render(true, {{0, 1, 2, 3, 4, 5}}, parallelA);
        render(true, {{5, 4, 3, 2, 1, 0}}, parallelB);
        double serialOrderDiff = 0, parallelOrderDiff = 0, topologyDiff = 0;
        for (size_t i = 4800; i < serialA.size(); ++i)
        {
            serialOrderDiff += std::abs(serialA[i] - serialB[i]);
            parallelOrderDiff += std::abs(parallelA[i] - parallelB[i]);
            topologyDiff += std::abs(serialA[i] - parallelA[i]);
        }
        check(serialOrderDiff > 1, "serial order changes the sound");
        check(parallelOrderDiff < 1e-3, "parallel does not depend on order");
        check(topologyDiff > 1, "parallel differs from serial");
    }
    // Rack audit: every control of every module changes the audio, bypass and Mix 0 are exactly dry,
    // bypass fades instead of jumping, and the new BLOOM diffusion / WOBBLE width do what they say.
    {
        const double sr = 48000;
        rastro::Sample sample;
        sample.rate = sr;
        unsigned seed = 7;
        for (int i = 0; i < (int)sr; ++i)
        {
            const float t = float(i % 12000) / float(sr); // a plucked burst every 250 ms
            seed = seed * 1664525u + 1013904223u;
            const float noise = float(seed >> 8) / 8388608.f - 1.f;
            const float v = std::exp(-t * 18) * (.45f * std::sin(float(i * 330 * 6.283185307 / sr)) +
                                                 .25f * noise);
            sample.data[0].push_back(v);
            sample.data[1].push_back(v);
        }
        using Edit = void (*)(rastro::Settings &, float);
        auto base = [](int module)
        {
            rastro::Settings s;
            s.loop = 1;
            s.level = 1;
            s.enabled.fill(false);
            if (module >= 0)
                s.enabled[module] = true;
            s.ghost[3] = s.crumble[2] = s.bloom[3] = s.wobble[2] = s.dust[2] = s.crush[1] = 1;
            return s;
        };
        auto render = [&](const rastro::Settings &s, int frames, std::vector<float> *right = nullptr,
                          int switchAt = -1, int switchModule = -1)
        {
            rastro::Engine engine;
            engine.prepare(sr);
            auto local = s;
            engine.noteOn(60, 1, &sample, local);
            std::vector<float> out((size_t)frames);
            if (right)
                right->assign((size_t)frames, 0);
            audio = true;
            for (int i = 0; i < frames; ++i)
            {
                if (i == switchAt)
                    local.enabled[switchModule] = !local.enabled[switchModule];
                float l, r;
                engine.frame(l, r, &sample, local);
                check(std::isfinite(l) && std::isfinite(r) && std::abs(l) <= 1.5f, "audit bounded");
                out[(size_t)i] = l;
                if (right)
                    (*right)[(size_t)i] = r;
            }
            audio = false;
            return out;
        };
        auto diff = [](const std::vector<float> &a, const std::vector<float> &b)
        {
            double d = 0;
            for (size_t i = 0; i < a.size(); ++i)
                d += std::abs(a[i] - b[i]);
            return d;
        };
        const int N = 48000;
        const auto dry = render(base(-1), N);
        struct Control
        {
            int module;
            const char *name;
            Edit edit;
        };
        const Control controls[]{
            {0, "GHOST time", [](rastro::Settings &s, float v) { s.ghost[0] = v; }},
            {0, "GHOST feedback", [](rastro::Settings &s, float v) { s.ghost[1] = v; }},
            {0, "GHOST tone", [](rastro::Settings &s, float v) { s.ghost[2] = v; }},
            {1, "CRUMBLE rate", [](rastro::Settings &s, float v) { s.crumble[0] = v; }},
            {1, "CRUMBLE variation", [](rastro::Settings &s, float v) { s.crumble[1] = v; }},
            {2, "BLOOM size", [](rastro::Settings &s, float v) { s.bloom[0] = v; }},
            {2, "BLOOM tone", [](rastro::Settings &s, float v) { s.bloom[1] = v; }},
            {2, "BLOOM decay", [](rastro::Settings &s, float v) { s.bloom[2] = v; }},
            {2, "BLOOM diffusion", [](rastro::Settings &s, float v) { s.bloomDiffusion = v; }},
            {3, "WOBBLE depth", [](rastro::Settings &s, float v) { s.wobble[0] = v; }},
            {3, "WOBBLE rate", [](rastro::Settings &s, float v) { s.wobble[1] = v; }},
            {4, "DUST level", [](rastro::Settings &s, float v) { s.dust[0] = v; }},
            {4, "DUST character", [](rastro::Settings &s, float v) { s.dust[1] = v; }},
            {5, "CRUSH amount", [](rastro::Settings &s, float v) { s.crush[0] = v; }},
        };
        for (auto &c : controls)
        {
            auto lo = base(c.module), hi = base(c.module);
            c.edit(lo, .15f);
            c.edit(hi, .9f);
            if (diff(render(lo, N), render(hi, N)) < 1)
                throw std::runtime_error(std::string("control has no audible effect: ") + c.name);
        }
        for (int m = 0; m < rastro::moduleCount; ++m)
        {
            // Bypassed with extreme settings: bit-identical to an empty rack.
            auto off = base(m);
            off.enabled[m] = false;
            off.ghost = {1, 1, 1, 1};
            off.bloom = {1, 1, 1, 1};
            off.bloomDiffusion = off.wobbleWidth = 1;
            check(render(off, N) == dry, "bypassed module is exactly dry");
            // Mix 0: bit-identical to dry as well.
            auto zero = base(m);
            zero.ghost[3] = zero.crumble[2] = zero.bloom[3] = zero.wobble[2] = zero.dust[2] =
                zero.crush[1] = 0;
            check(render(zero, N) == dry, "Mix 0 is exactly dry");
            // Turning the module off mid-note fades out (about 15 ms) instead of cutting:
            // right after the switch the output still differs from a hard cut, and it ends dry.
            const int at = 20000;
            const auto on = render(base(m), N);
            const auto sw = render(base(m), N, nullptr, at, m);
            double fade = 0, tail = 0, wetness = 0;
            for (int i = at; i < at + 1500; ++i)
            {
                fade += std::abs(sw[i] - dry[i]);     // a hard cut would be exactly dry here
                wetness += std::abs(on[i] - dry[i]);
            }
            for (int i = at + 15000; i < at + 16000; ++i)
                tail += std::abs(sw[i] - dry[i]);
            check(wetness < 1e-3 || fade > wetness * .1, "bypass crossfades instead of cutting");
            check(tail < 1e-2, "bypass ends exactly dry");
        }
        // WOBBLE width: 0 keeps left = right on a mono source; width opens the stereo image.
        {
            auto narrow = base(3), wide = base(3);
            narrow.wobble[0] = wide.wobble[0] = .9f;
            wide.wobbleWidth = 1;
            std::vector<float> rn, rw;
            const auto ln = render(narrow, N, &rn), lw = render(wide, N, &rw);
            check(ln == rn, "WOBBLE width 0 stays mono");
            check(diff(lw, rw) > 1, "WOBBLE width opens stereo");
        }
    }
    std::cout << "PASS: sample playback, release, all effects, extreme trims, polyphony, 4 sample "
                 "rates, serial/parallel rack, rack audit (every control, bypass, Mix 0, fades, width); "
                 "no allocations in frame\n";
}
