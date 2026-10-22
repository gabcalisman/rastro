#include "DanceMotion.h"
#include "Engine.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
thread_local bool inAudio = false;
void *operator new(size_t n)
{
    if (inAudio)
        throw std::runtime_error("synth audio allocation");
    if (auto *p = std::malloc(n))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void check(bool ok, const char *m)
{
    if (!ok)
        throw std::runtime_error(m);
}
void chordAndModulationTests()
{
    rastro::Sample sample;
    sample.rate = 48000;
    for (auto &c : sample.data)
    {
        c.resize(48000);
        for (int i = 0; i < 48000; ++i)
            c[i] = .4f * std::sin(i * .17f);
    }
    rastro::Settings p;
    p.enabled.fill(false);
    p.synth = true;
    p.mode = 1;
    p.velocity = 0;
    p.attack = .01f;
    auto render = [&](bool chord, bool wheel)
    {
        rastro::Engine e;
        e.prepare(48000);
        if (wheel)
            e.modWheel(1, 1);
        e.noteOn(60, 1, &sample, p);
        if (chord)
        {
            e.noteOn(64, .7f, &sample, p);
            e.noteOn(67, .6f, &sample, p);
        }
        std::vector<float> out(12000);
        for (auto &v : out)
        {
            float r;
            e.frame(v, r, &sample, p);
        }
        return out;
    };
    std::vector<double> individual(12000);
    for (auto key : {std::array<int, 2>{60, 100}, {64, 70}, {67, 60}})
    {
        rastro::Engine e;
        e.prepare(48000);
        e.noteOn(key[0], key[1] / 100.f, &sample, p);
        for (auto &sum : individual)
        {
            float l, r;
            e.frame(l, r, &sample, p);
            sum += std::atanh(double(l));
        }
    }
    auto chord = render(true, false);
    for (size_t i = 0; i < chord.size(); ++i)
        check(std::abs(std::atanh(double(chord[i])) - individual[i]) < .000001,
              "chord is the sum of independent pitched voices before master saturation");
    p.cutoff = 900;
    p.lfoDepth = 0;
    p.lfoRate = 3;
    auto dry = render(false, false), modulated = render(false, true);
    double difference = 0;
    for (size_t i = 0; i < dry.size(); ++i)
        difference += std::abs(dry[i] - modulated[i]);
    check(difference > 1, "CC1 produces actual audible cutoff modulation");
    p.envAmount = 1;
    auto envelope = render(false, false);
    difference = 0;
    for (size_t i = 0; i < dry.size(); ++i)
        difference += std::abs(dry[i] - envelope[i]);
    check(difference > 1, "envelope routing changes audible filter output");
}
int main()
{
    chordAndModulationTests();
    {
        rastro::DanceMotion rig;
        float last = 0;
        bool left = false, right = false;
        for (int i = 0; i < 240; ++i)
        {
            rig.step(1.f / 24, .9f, .6f, .7f, .7f, 120, -1, .2f);
            left |= rig.footLeft > .3f;
            right |= rig.footRight > .3f;
            check(std::abs(rig.footLeft - last) < .7f, "dance joint continuity");
            last = rig.footLeft;
        }
        check(left && right, "dance steps with both feet");
        for (int i = 0; i < 200; ++i)
            rig.step(1.f / 12, 0, 0, .5f, .5f, 120, -1, 0);
        check(rig.footLeft == 0 && rig.armLeft == 0 && rig.head == 0,
              "dance naturally settles at silence");
        rastro::DanceMotion soul, disco, slow, still;
        float difference = 0;
        for (int i = 0; i < 36; ++i)
        {
            soul.step(1.f / 24, .9f, .6f, .7f, .7f, 120, -1, .2f, 0, 1);
            disco.step(1.f / 24, .9f, .6f, .7f, .7f, 120, -1, .2f, 1, 1);
            slow.step(1.f / 24, .9f, .6f, .7f, .7f, 120, -1, .2f, 2, 1);
            still.step(1.f / 24, .9f, .6f, .7f, .7f, 120, -1, .2f, 1, 0);
            difference += std::abs(soul.footLeft - disco.footLeft);
        }
        check(difference > .5f, "disco shuffle differs from soul step");
        check(std::abs(soul.phase - .5f) < .001f && std::abs(slow.phase - .75f) < .001f,
              "half-time follows half the beat rate");
        check(still.footLeft == 0 && still.armRight == 0 && still.phase == 0,
              "zero dance intensity freezes the rig, even with notes");
        for (int i = 0; i < 240; ++i)
            for (auto *d : {&soul, &disco, &slow})
                d->step(1.f / 12, 0, 0, .5f, .5f, 120, -1, 0, 1, 2);
        check(soul.head == 0 && disco.footLeft == 0 && slow.armRight == 0,
              "every dance style settles completely in silence");
    }

    for (double sr : {22050., 44100., 48000., 96000.})
    {
        rastro::Sample sample;
        sample.rate = sr;
        for (auto &c : sample.data)
        {
            c.resize(int(sr));
            for (int i = 0; i < int(sr); ++i)
                c[i] = .4f * std::sin(float(i * 6.283185307 * 3000 / sr));
        }
        rastro::Settings p;
        p.enabled.fill(false);
        p.velocity = 0;
        p.synth = true;
        p.mode = 1;
        p.triggerMode = 2;
        p.attack = .02f;
        p.decay = .03f;
        p.sustain = .6f;
        p.release = .05f;
        rastro::Engine e;
        e.prepare(sr);
        auto render = [&](int n)
        {
            double energy = 0;
            inAudio = true;
            for (int i = 0; i < n; ++i)
            {
                float l, r;
                e.frame(l, r, &sample, p);
                check(std::isfinite(l) && std::abs(l) <= 1, "finite bounded synth output");
                energy += l * l;
            }
            inAudio = false;
            return energy;
        };
        rastro::SynthEnvelope env;
        env.start();
        for (int i = 0; i < int(sr * .01); ++i)
            env.tick(sr, .02f, .03f, .6f, .05f, true);
        check(env.value > .48f && env.value < .52f, "ADSR attack measured at half time");
        for (int i = 0; i < int(sr * .05) + 3; ++i)
            env.tick(sr, .02f, .03f, .6f, .05f, true);
        check(std::abs(env.value - .6f) < .002f, "ADSR reaches sustain");
        for (int i = 0; i < int(sr * .051); ++i)
            env.tick(sr, .02f, .03f, .6f, .05f, false);
        check(env.stage == rastro::SynthEnvelope::idle, "ADSR release reaches zero on time");
        e.noteOn(60, 1, &sample, p);
        e.noteOn(64, .7f, &sample, p);
        e.noteOn(67, .6f, &sample, p);
        check(e.activeVoiceCount() == 3 && render(int(sr * .1)) > .01,
              "independent chord voices sound");
        for (int n = 0; n < 32; ++n)
            e.noteOn(40 + n, .6f, &sample, p);
        check(e.activeVoiceCount() == 16, "polyphony bounded at sixteen voices");
        e.allOff();
        render(int(sr * .06));
        check(e.activeVoiceCount() == 0, "all-off releases every voice");
        e.reset();
        p.voiceMode = 1;
        e.noteOn(60, 1, &sample, p);
        render(100);
        e.noteOn(64, 1, &sample, p);
        check(e.activeVoiceCount() == 1, "Mono replaces voice");
        render(100);
        e.noteOff(64, p);
        check(e.activeVoiceCount() == 1, "Mono returns to held note");
        e.noteOff(60, p);
        render(int(sr * .06));
        check(e.activeVoiceCount() == 0, "Mono releases last key");
        e.reset();
        p.voiceMode = 2;
        p.glide = .1f;
        e.noteOn(60, 1, &sample, p);
        render(1000);
        float before = e.position;
        e.noteOn(72, 1, &sample, p);
        render(1);
        check(e.position > before, "Legato retains traversal on overlapping notes");
        e.noteOff(72, p);
        e.noteOff(60, p);
        render(int(sr * .06));
        check(e.activeVoiceCount() == 0, "Legato held stack clears");
        e.reset();
        p.voiceMode = 0;
        p.glide = 0;
        e.noteOn(60, 1, &sample, p);
        render(1000);
        e.sustainPedal(1, true, p);
        e.noteOff(60, p);
        render(int(sr * .1));
        check(e.activeVoiceCount() == 1, "CC64 sustains released gate");
        e.sustainPedal(2, false, p);
        render(100);
        check(e.activeVoiceCount() == 1, "pedal channels are independent");
        e.sustainPedal(1, false, p);
        render(int(sr * .06));
        check(e.activeVoiceCount() == 0, "pedal up starts release");
        e.reset();
        p.cutoff = 16000;
        e.noteOn(60, 1, &sample, p);
        double bright = render(int(sr * .15));
        e.reset();
        p.cutoff = 200;
        e.noteOn(60, 1, &sample, p);
        double dark = render(int(sr * .15));
        check(dark < bright * .05, "low-pass attenuates high frequency");
        e.reset();
        p.cutoff = 20000;
        p.bendRange = 12;
        e.pitchBend(1, 1);
        e.noteOn(60, 1, &sample, p);
        render(int(sr * .05));
        float bent = e.position;
        e.reset();
        e.noteOn(60, 1, &sample, p);
        render(int(sr * .05));
        check(bent > e.position * 1.6f, "pitch bend changes traversal speed");
        e.reset();
        p.triggerMode = 1;
        p.mode = 0;
        e.noteOn(60, 1, &sample, p);
        render(100);
        e.noteOff(60, p);
        render(int(sr * .1));
        check(e.activeVoiceCount() == 1, "synth One Shot ignores key-up");
        e.allOff();
        render(int(sr * .03));
        check(e.activeVoiceCount() == 0, "forced stop overrides One Shot");
        e.reset();
        p.triggerMode = 2;
        p.mode = 1;
        p.lfoDepth = 1;
        p.lfoRate = 12;
        p.envAmount = 1;
        p.resonance = .95f;
        e.noteOn(60, 1, &sample, p);
        render(int(sr * .2));
        e.modWheel(1, 1);
        p.lfoTarget = 1;
        render(int(sr * .2));
    }
    std::cout
        << "PASS synth: ADSR timing, resonant filter, 16-voice chords, Mono/Legato, sustain "
           "channels, pitch bend, LFO, One Shot/Gate, four sample rates; no audio allocations\n";
}
