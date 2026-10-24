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
    // Click detector: a smooth low sine must stay smooth through every playback mode, loop wraps,
    // ping-pong turns, voice stealing (more notes than voices) and the CRUMBLE fragment windows.
    // A click is a jump in the second difference far above what the music itself produces.
    {
        const double sr = 48000;
        rastro::Sample sample;
        sample.rate = sr;
        const int n = 30011; // not a whole number of cycles: the loop seam does not line up by itself
        for (int c = 0; c < 2; ++c)
        {
            sample.data[c].resize((size_t)n);
            for (int i = 0; i < n; ++i)
                sample.data[c][i] = .6f * std::sin(float(i * 110 * 6.283185307 / sr));
        }
        float lastDip = 1;
        auto worstClick = [&](rastro::Settings s, int notes, int noteLen, int frames, bool hold)
        {
            rastro::Engine engine;
            engine.prepare(sr);
            float p1 = 0, p2 = 0, worst = 0, winPeak = 0, minPeak = 1e9f, maxPeak = 0;
            int nextOn = 0, k = 0;
            audio = true;
            for (int i = 0; i < frames; ++i)
            {
                if (i == nextOn && k < notes)
                {
                    engine.noteOn(48 + (k * 5) % 24, .9f, &sample, s);
                    if (!hold)
                        ; // released below
                    ++k;
                    nextOn += noteLen;
                }
                if (!hold && k > 0 && i == (k - 1) * noteLen + noteLen - 1)
                    engine.noteOff(48 + ((k - 1) * 5) % 24, s);
                float l, r;
                engine.frame(l, r, &sample, s);
                if (i > 2)
                    worst = std::max(worst, std::abs(l - 2 * p1 + p2));
                winPeak = std::max(winPeak, std::abs(l));
                if (i % 600 == 599)
                {
                    if (i > 6000) { minPeak = std::min(minPeak, winPeak); maxPeak = std::max(maxPeak, winPeak); }
                    winPeak = 0;
                }
                p2 = p1;
                p1 = l;
            }
            audio = false;
            lastDip = maxPeak > 0 ? minPeak / maxPeak : 1;
            return worst;
        };
        rastro::Settings base;
        base.enabled.fill(false);
        base.level = 1;
        base.root = 60;
        struct Case { const char *name; int mode; bool region; bool synth; int voice; int notes; int len; bool hold; };
        const Case cases[]{
            {"one shot", 0, false, false, 0, 6, 9000, true},
            {"loop mode wrap", 1, false, false, 0, 1, 1, true},
            {"reverse", 2, false, false, 0, 4, 9000, true},
            {"ping-pong turns", 3, false, false, 0, 1, 1, true},
            {"region LOOP wrap", 0, true, false, 0, 1, 1, true},
            {"voice stealing (24 notes on 16 voices)", 1, false, false, 0, 24, 700, true},
            {"synth voice stealing", 1, false, true, 0, 24, 700, true},
            {"mono retrigger", 1, false, false, 1, 12, 2000, false},
            {"gate release", 1, false, true, 0, 8, 3000, false},
        };
        float reference = 0;
        int clicks = 0;
        {
            auto s = base;
            s.mode = 1;
            s.regionLoop = true;
            s.loopCrossfade = 20;
            reference = worstClick(s, 1, 1, 4000, true); // inside the sample: pure sine curvature
        }
        for (auto &c : cases)
        {
            auto s = base;
            s.mode = c.mode;
            s.regionLoop = c.region;
            s.loopCrossfade = 5;
            s.synth = c.synth;
            s.voiceMode = c.voice;
            const float w = worstClick(s, c.notes, c.len, 120000, c.hold);
            const bool dipCase = c.notes == 1; // one held looping note: its level must stay steady
            const bool bad = w > std::max(.012f, reference * 6) || (dipCase && lastDip < .8f);
            std::cerr << (bad ? "CLICK " : "ok    ") << c.name << ": jump " << w << " (smooth " << reference << "), level steadiness " << lastDip << std::endl;
            clicks += bad;
        }
        // Level dips: a constant (DC) sample held in every looping mode must keep a constant level,
        // so any fade-out/fade-in at a loop wrap or ping-pong turn shows up directly.
        {
            rastro::Sample dc;
            dc.rate = sr;
            for (int c = 0; c < 2; ++c)
                dc.data[c].assign(9001, .5f);
            const char *names[]{"loop mode wrap", "ping-pong turns", "region LOOP wrap"};
            const int modes[]{1, 3, 0};
            for (int m = 0; m < 3; ++m)
            {
                auto s = base;
                s.mode = modes[m];
                s.regionLoop = m == 2;
                rastro::Engine engine;
                engine.prepare(sr);
                engine.noteOn(60, .9f, &dc, s);
                float lo = 1e9f, hi = 0;
                audio = true;
                for (int i = 0; i < 60000; ++i)
                {
                    float l, r;
                    engine.frame(l, r, &dc, s);
                    if (i > 2000) { lo = std::min(lo, std::abs(l)); hi = std::max(hi, std::abs(l)); }
                }
                audio = false;
                const bool bad = lo < hi * .97f;
                std::cerr << (bad ? "DIP   " : "ok    ") << names[m] << ": level min/max " << lo / std::max(1e-9f, hi) << std::endl;
                clicks += bad;
            }
        }
        // CRUMBLE fragments must be windowed whole: no cut-off window at the end of each period
        {
            auto s = base;
            s.mode = 1;
            s.enabled[1] = true;
            s.crumble = {.3f, .1f, 1};
            const float w = worstClick(s, 1, 1, 96000, true);
            std::cerr << (w > .02f ? "CLICK " : "ok    ") << "CRUMBLE fragments: " << w << std::endl;
            clicks += w > .02f;
        }
        check(clicks == 0, "click detected");
    }
    // ===== Etapa 1 de som: HQ playback, filter modes/slope/drive, filter ADSR, LFO shapes/fade/amp/pan =====
    {
        const double sr = 48000;
        auto sine = [&](double hz, float amp = .6f) {
            rastro::Sample smp;
            smp.rate = sr;
            for (int c = 0; c < 2; ++c)
            {
                smp.data[c].resize(96000);
                for (int i = 0; i < 96000; ++i)
                    smp.data[c][i] = amp * float(std::sin(i * hz * 6.283185307 / sr));
            }
            smp.buildPeaks(); // also builds the band-limited levels
            return smp;
        };
        struct Out { std::vector<float> l, r; float rms = 0, peak = 0, maxStep = 0; };
        auto render = [&](const rastro::Sample &smp, rastro::Settings s, int note, int frames, int skip = 2400) {
            s.enabled = {false, false, false, false, false, false};
            s.level = 1;
            s.mode = 1; // loop: steady tone
            rastro::Engine engine;
            engine.prepare(sr);
            engine.noteOn(note, 1, &smp, s);
            Out o;
            o.l.resize(size_t(frames)); o.r.resize(size_t(frames));
            double sum = 0;
            float prev = 0;
            audio = true;
            for (int i = 0; i < frames; ++i)
            {
                float l, r;
                engine.frame(l, r, &smp, s);
                check(std::isfinite(l) && std::isfinite(r), "etapa1: finite output");
                if (i >= skip)
                {
                    sum += double(l) * l;
                    o.peak = std::max(o.peak, std::abs(l));
                    o.maxStep = std::max(o.maxStep, std::abs(l - prev));
                }
                o.l[size_t(i)] = l; o.r[size_t(i)] = r;
                prev = l;
            }
            audio = false;
            o.rms = float(std::sqrt(sum / std::max(1, frames - skip)));
            return o;
        };
        rastro::Settings base;
        base.velocity = 0; // no velocity tone filter: measure the engine itself
        // 1. HQ playback removes aliasing: a 9 kHz tone played 2 octaves up (36 kHz) must vanish
        {
            auto hi = sine(9000);
            auto lin = base, hq = base;
            hq.hq = true;
            const float a = render(hi, lin, 84, 24000).rms, b = render(hi, hq, 84, 24000).rms;
            std::cerr << "HQ aliasing: linear " << a << " vs HQ " << b << std::endl;
            check(b < a * .1f, "HQ playback must remove aliasing");
            auto lo = sine(1000);
            const float c = render(lo, lin, 72, 24000).rms, d = render(lo, hq, 72, 24000).rms;
            check(std::abs(c - d) < c * .1f, "HQ keeps the level of in-band content");
        }
        auto synth = base;
        synth.synth = true;
        synth.attack = .001f; synth.sustain = 1;
        auto tone200 = sine(200), tone4k = sine(4000), tone3k = sine(3000);
        // 2. filter modes
        {
            auto lp = synth, hp = synth, bp = synth, nt = synth;
            lp.cutoff = hp.cutoff = 2000;
            hp.filterMode = 1;
            bp.cutoff = nt.cutoff = 200; bp.filterMode = 2; nt.filterMode = 3; bp.resonance = nt.resonance = .3f;
            const float L = render(tone200, lp, 60, 24000).rms, H = render(tone200, hp, 60, 24000).rms,
                        B = render(tone200, bp, 60, 24000).rms, N = render(tone200, nt, 60, 24000).rms;
            std::cerr << "filter 200 Hz: LP " << L << " HP " << H << " BP " << B << " notch " << N << std::endl;
            check(H < L * .1f, "HP removes lows");
            check(B > L * .5f, "BP passes its centre");
            check(N < L * .3f, "notch rejects its centre");
            auto s12 = synth, s24 = synth;
            s12.cutoff = s24.cutoff = 1000; s24.filterSlope = 1;
            const float a = render(tone4k, s12, 60, 24000).rms, b = render(tone4k, s24, 60, 24000).rms;
            std::cerr << "slope at 2 oct: 12dB " << a << " 24dB " << b << std::endl;
            check(b < a * .4f, "24 dB slope is steeper");
        }
        // 3. drive: adds harmonics, stays bounded, controlled level
        {
            auto clean = synth, hot = synth;
            hot.filterDrive = 1; hot.hq = clean.hq = true;
            auto a = render(tone200, clean, 60, 24000), b = render(tone200, hot, 60, 24000);
            float diff = 0;
            for (size_t i = 4800; i < a.l.size(); ++i) diff += std::abs(a.l[i] - b.l[i]);
            std::cerr << "drive: rms " << a.rms << " -> " << b.rms << ", peak " << b.peak << std::endl;
            check(diff > 50, "drive changes the sound");
            check(b.peak < 1.2f && b.rms < a.rms * 2.5f && b.rms > a.rms * .5f, "drive level controlled");
            // HQ path with drive 0 is transparent (only a few samples of latency)
            auto t0 = synth; t0.hq = true;
            auto t1 = synth;
            check(std::abs(render(tone200, t0, 60, 24000).rms - render(tone200, t1, 60, 24000).rms) < .002f,
                  "HQ drive stage at 0 is transparent");
        }
        // 4. filter envelope with its own ADSR
        {
            auto amp = synth, own = synth;
            amp.cutoff = own.cutoff = 150; amp.envAmount = own.envAmount = 1;
            own.fenvOwn = true; own.fAttack = 1.5f; own.fSustain = 1;
            const float a = render(tone3k, amp, 60, 4800, 0).rms, b = render(tone3k, own, 60, 4800, 0).rms;
            std::cerr << "filter env: follows amp " << a << " own slow attack " << b << std::endl;
            check(b < a * .5f, "own filter ADSR shapes the filter independently");
        }
        // 5. LFO shapes, fade, volume and pan (no clicks with square tremolo)
        {
            auto plain = synth, sq = synth, pan = synth, fade = synth, sh = synth;
            sq.lfoShape = 2; sq.lfoAmp = 1; sq.lfoRate = 4;
            pan.lfoPan = 1; pan.lfoRate = 3;
            sh.lfoShape = 4; sh.lfoAmp = 1; sh.lfoRate = 6;
            fade.lfoAmp = 1; fade.lfoRate = 8; fade.lfoFade = 2; fade.lfoShape = 2;
            auto a = render(tone200, plain, 60, 48000), b = render(tone200, sq, 60, 48000);
            std::cerr << "square tremolo: max step " << b.maxStep << " vs plain " << a.maxStep << std::endl;
            check(b.rms < a.rms * .85f, "LFO to volume works");
            check(b.maxStep < a.maxStep * 1.3f, "square LFO never clicks");
            auto c = render(tone200, pan, 60, 48000);
            float lr = 0;
            for (size_t i = 0; i < c.l.size(); ++i) lr += std::abs(c.l[i] - c.r[i]);
            check(lr > 100, "LFO to pan moves the sound");
            auto d = render(tone200, sh, 60, 48000);
            check(d.maxStep < a.maxStep * 1.3f, "sample & hold never clicks");
            // fade-in: the first 200 ms barely move, later the tremolo is full
            auto e = render(tone200, fade, 60, 96000, 0);
            auto depth = [&](int from, int to) { float lo = 9, hi = 0; for (int i = from; i < to; i += 64) { float m = 0; for (int k = 0; k < 64; ++k) m = std::max(m, std::abs(e.l[size_t(i + k)])); lo = std::min(lo, m); hi = std::max(hi, m); } return hi > 0 ? lo / hi : 1; };
            check(depth(2400, 9600) > depth(86000, 95000) + .3f, "LFO fade-in");
        }
        // 6. sample gain (clip gain): +6 dB doubles the level, 0 dB is exactly the same sound
        {
            auto g0 = base, g6 = base; g6.sampleGainDb = 6.0206f;
            const float a = render(tone200, g0, 60, 48000, 24000).rms, b = render(tone200, g6, 60, 48000, 24000).rms;
            std::cerr << "sample gain: 0 dB " << a << " +6 dB " << b << std::endl;
            check(std::abs(b / a - 2) < .06f, "sample gain +6 dB doubles the level (output stage is gently non-linear)");
        }
        std::cerr << "ok    etapa 1 (HQ, filter modes, slope, drive, filter ADSR, LFO shapes/fade/vol/pan)" << std::endl;
    }
    std::cout << "PASS: sample playback, release, all effects, extreme trims, polyphony, 4 sample "
                 "rates, serial/parallel rack, rack audit (every control, bypass, Mix 0, fades, width); "
                 "no allocations in frame\n";
}
