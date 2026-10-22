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
void xyCorners()
{
    rastro::Sample sample;
    sample.rate = 48000;
    for (auto &c : sample.data)
    {
        c.resize(48000);
        for (int i = 0; i < 48000; ++i)
            c[i] = .3f * std::sin(i * .09f) + .15f * std::sin(i * .17f);
    }
    auto render = [&](float x, float y, int only)
    {
        rastro::Engine e;
        e.prepare(48000);
        rastro::Settings p;
        p.enabled.fill(false);
        p.xyCrumble = true;
        if (only == 6)
        {
            p.enabled[1] = p.enabled[3] = p.enabled[4] = p.enabled[5] = true;
        }
        else if (only >= 0)
            p.enabled[only] = true;
        p.loop = 1;
        p.playX = x;
        p.playY = y;
        p.crumble = {.5f, .7f, .45f};
        p.dust = {.5f, .5f, 1};
        p.crush = {.5f, 1};
        p.wobble = {.4f, .5f, 1};
        e.noteOn(60, 1, &sample, p);
        std::vector<float> out(24000);
        for (auto &v : out)
        {
            float r;
            e.frame(v, r, &sample, p);
        }
        return out;
    };
    auto difference = [](const auto &a, const auto &b)
    {
        double sum = 0;
        for (size_t i = 0; i < a.size(); ++i)
            sum += std::abs(a[i] - b[i]);
        return sum;
    };
    auto dry = render(0, 0, -1);
    check(render(0, 0, 6) == dry, "XY clean/stable corner really removes its four destinations");
    for (int m : {1, 4, 5})
    {
        check(render(.2f, 0, m) == dry, "Y zero is dry for each degradation module");
        auto centre = render(.5f, .5f, m), top = render(.5f, 1, m);
        check(difference(dry, centre) > 1 && difference(centre, top) > .1,
              "Y centre/top make audible distinct degradation");
        check(render(0, 1, m) == render(1, 1, m), "X never changes degradation-only audio");
    }
    check(render(1, 0, 3) == render(1, 1, 3), "Y never changes WOBBLE-only audio");
    check(difference(render(0, 0, 3), render(1, 0, 3)) > 1, "X makes audible WOBBLE movement");
    for (auto point : {std::array<float, 2>{0, 0}, {1, 0}, {0, 1}, {1, 1}, {.5f, .5f}})
    {
        auto audio = render(point[0], point[1], 6);
        for (float value : audio)
            check(std::isfinite(value) && std::abs(value) <= 1,
                  "all XY corners and centre remain bounded");
    }
    std::cout << "PASS XY: four corners/centre, actual CRUMBLE/DUST/CRUSH and WOBBLE audio, "
                 "independent axes, no bypass changes\n";
}
int main()
{
    xyCorners();
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
