#include "Engine.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
static bool audio = false;
void *operator new(std::size_t n)
{
    if (audio)
        std::abort();
    if (auto *p = std::malloc(n))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void check(bool b, const char *message)
{
    if (!b)
        throw std::runtime_error(message);
}
int main()
{
    for (int rank = 0; rank < 720; ++rank)
    {
        auto p = rastro::decodeOrder(rank);
        check(rastro::encodeOrder(p) == rank, "six module permutation roundtrip");
        auto sorted = p;
        std::sort(sorted.begin(), sorted.end());
        check(sorted == rastro::defaultOrder, "unique rack slots");
    }
    for (double sr : {22050., 44100., 48000., 96000.})
    {
        rastro::Sample sample;
        sample.rate = sr;
        for (auto &c : sample.data)
        {
            c.resize((int)sr);
            for (int i = 0; i < (int)sr; ++i)
                c[i] = .5f * std::sin(float(i * 713 * 6.283185307 / sr));
        }
        for (int module = 3; module < 6; ++module)
        {
            rastro::Engine dry, wet;
            dry.prepare(sr);
            wet.prepare(sr);
            rastro::Settings p;
            p.enabled.fill(false);
            p.loop = 1;
            p.velocity = 0;
            auto effect = p;
            effect.enabled[module] = true;
            effect.wobble = {.9f, .8f, 1};
            effect.dust = {1, .4f, 1};
            effect.crush = {1, 1};
            dry.noteOn(60, 1, &sample, p);
            wet.noteOn(60, 1, &sample, effect);
            float difference = 0, energy = 0, max = 0;
            audio = true;
            for (int i = 0; i < (int)sr; ++i)
            {
                float a, b, l, r;
                dry.frame(a, b, &sample, p);
                wet.frame(l, r, &sample, effect);
                difference += std::abs(l - a);
                energy += l * l;
                max = std::max(max, std::abs(l));
                if (!std::isfinite(l) || !std::isfinite(r) || max > 1)
                    std::abort();
            }
            audio = false;
            check(difference > 1, "each new effect must change the audio");
            check(energy > .01f, "effect must remain audible");
            // Zero effect mix is a genuine dry comparison, not an arbitrary alternate preset.
            dry.reset();
            wet.reset();
            effect.wobble[2] = effect.dust[2] = effect.crush[1] = 0;
            dry.noteOn(60, 1, &sample, p);
            wet.noteOn(60, 1, &sample, effect);
            audio = true;
            for (int i = 0; i < 5000; ++i)
            {
                float a, b, l, r;
                dry.frame(a, b, &sample, p);
                wet.frame(l, r, &sample, effect);
                if (std::abs(l - a) > 1e-7f)
                    std::abort();
            }
            audio = false;
        }
        // DUST cannot self-oscillate on an empty source, silent sample or after release.
        rastro::Engine e;
        e.prepare(sr);
        rastro::Settings dust;
        dust.enabled.fill(false);
        dust.enabled[4] = true;
        dust.dust = {1, 1, 1};
        dust.loop = 1;
        audio = true;
        for (int i = 0; i < 1000; ++i)
        {
            float l, r;
            e.frame(l, r, nullptr, dust);
            if (l != 0 || r != 0)
                std::abort();
        }
        audio = false;
        e.noteOn(60, 1, &sample, dust);
        audio = true;
        for (int i = 0; i < 5000; ++i)
        {
            float l, r;
            e.frame(l, r, &sample, dust);
        }
        e.noteOff(60, dust);
        for (int i = 0; i < (int)sr; ++i)
        {
            float l, r;
            e.frame(l, r, &sample, dust);
            if (i > (int)sr - 100 && (l != 0 || r != 0))
                std::abort();
        }
        audio = false;
        check(e.visualDust == 0, "DUST telemetry must return to silence");
        e.reset();
        rastro::Settings motion;
        motion.enabled = {false, false, false, true, true, true};
        motion.loop = 1;
        e.noteOn(60, 1, &sample, motion);
        float deltaX = 0, deltaY = 0;
        audio = true;
        for (int i = 0; i < (int)sr; ++i)
        {
            motion.playX = (i / 256) % 2;
            motion.playY = (i / 512) % 2;
            motion.wobble[0] = motion.crush[0] = motion.dust[0] = 1;
            float l, r;
            e.frame(l, r, &sample, motion);
            if (!std::isfinite(l) || std::abs(l) > 1 || !std::isfinite(e.visualWobble.load()))
                std::abort();
            deltaX += std::abs(e.visualWobble.load());
            deltaY += e.visualDust.load() + e.visualCrush.load();
        }
        audio = false;
        check(deltaX > 0 && deltaY > 0, "XY must reach audible DSP and its telemetry");
    }
    std::cout << "PASS: 720 rack orders; WOBBLE/DUST/CRUSH audible, bounded, zero-mix dry; DUST "
                 "silence/release; XY automation/telemetry; 4 sample rates; no audio allocations\n";
}
