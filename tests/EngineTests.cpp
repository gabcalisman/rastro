#include "Engine.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
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
        engine.noteOff(60);
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
    std::cout << "PASS: sample playback, release, all effects, extreme trims, polyphony, 4 sample "
                 "rates; no allocations in frame\n";
}
