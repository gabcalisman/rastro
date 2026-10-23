#include "Engine.h"
#include <cstdlib>
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
    std::cout << "PASS: sample playback, release, all effects, extreme trims, polyphony, 4 sample "
                 "rates, serial/parallel rack; no allocations in frame\n";
}
