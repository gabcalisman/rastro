#include "Engine.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
void noteOffTests(double rate, int pitchMode = 0)
{
    rastro::Sample sample;
    sample.rate = rate;
    for (auto &channel : sample.data)
    {
        channel.resize((size_t)rate);
        for (int i = 0; i < (int)rate; ++i)
            channel[i] = .4f * std::sin(i * .07f);
    }
    rastro::Settings p;
    p.pitchMode = pitchMode;
    p.enabled.fill(false);
    p.drywet = 0;
    p.velocity = 0;
    p.start = .1f;
    p.end = .9f;
    p.sliceCount = 2;
    p.cuts = {0, .5f, 1};
    auto render = [&](rastro::Engine &engine, int count)
    {
        double energy = 0;
        for (int i = 0; i < count; ++i)
        {
            float l, r;
            engine.frame(l, r, &sample, p);
            energy += l * l + r * r;
        }
        return energy;
    };
    for (int mode = 0; mode < 5; ++mode)
    {
        p.mode = mode;
        rastro::Engine held, released;
        held.prepare(rate);
        released.prepare(rate);
        held.noteOn(60, 1, &sample, p);
        released.noteOn(60, 1, &sample, p);
        render(held, (int)(rate * .02));
        render(released, (int)(rate * .02));
        released.noteOff(61, p);
        check(render(released, 1) > 0, "unrelated note-off leaves voice playing");
        render(held, 1);
        released.noteOff(60, p);
        if (mode == 0)
        {
            // Early key release must not change a single sample of a One Shot.
            for (int i = 0; i < (int)rate; ++i)
            {
                float a, b, c, d;
                held.frame(a, b, &sample, p);
                released.frame(c, d, &sample, p);
                check(a == c && b == d, "One Shot ignores note-off, exact held-note audio");
            }
            check(released.position > .899f && released.position < .901f,
                  "One Shot reaches trimmed end, not full source end");
            check(render(released, 256) == 0, "One Shot naturally stops at end");
        }
        else
        {
            check(render(released, (int)(rate * .01)) > 0, "note-off has audible release");
            render(released, (int)(rate * .025));
            check(render(released, 256) == 0, "gated modes silent within 35ms of note-off");
            render(held, (int)(rate * .035));
            check(render(held, 256) > 0, "held control still playing in each gated mode");
        }
    }
    p.mode = 0;
    rastro::Engine e;
    e.prepare(rate);
    e.noteOn(60, 1, &sample, p);
    render(e, (int)(rate * .02));
    e.allOff();
    render(e, (int)(rate * .035));
    check(render(e, 256) == 0, "allOff releases One Shot for safety");
    p.loop = 1;
    e.reset();
    e.noteOn(60, 1, &sample, p);
    render(e, (int)(rate * .02));
    e.noteOff(60, p);
    render(e, (int)(rate * .035));
    check(render(e, 256) == 0, "legacy loop remains gated");
    p.loop = 0;
    p.root = 255;
    e.reset();
    e.noteOn(255, 1, &sample, p);
    render(e, (int)(rate * .02));
    e.noteOff(255, p);
    render(e, (int)(rate * .035));
    check(render(e, 256) == 0 && !e.previewActive, "explicit preview stop still works");
}
void pitchModeTests(double rate)
{
    rastro::Sample sample;
    sample.rate = 44100;
    for (auto &c : sample.data)
    {
        c.resize(44100);
        for (int i = 0; i < 44100; ++i)
            c[i] = .4f * std::sin(i * .07f);
    }
    rastro::Settings p;
    p.enabled.fill(false);
    p.drywet = 0;
    p.velocity = 0;
    p.start = .1f;
    p.end = .9f;
    auto render = [&](int mode, int pitchMode, int note)
    {
        p.mode = mode;
        p.pitchMode = pitchMode;
        rastro::Engine engine;
        engine.prepare(rate);
        engine.noteOn(note, 1, &sample, p);
        std::vector<float> pcm(2048);
        for (auto &v : pcm)
        {
            float r;
            engine.frame(v, r, &sample, p);
        }
        return pcm;
    };
    for (int mode = 0; mode < 4; ++mode)
    {
        auto original = render(mode, 0, 60);
        for (int note : {12, 48, 60, 72, 108, 127})
            check(render(mode, 1, note) == original, "FIXED PITCH: identical original-speed PCM on "
                                                     "every MIDI note in each playback mode");
        check(render(mode, 0, 72) != original,
              "REPITCH octave changes audio in each playback mode");
    }
    auto crossings = [](const std::vector<float> &pcm)
    {
        int n = 0;
        for (size_t i = 1; i < pcm.size(); ++i)
            n += pcm[i - 1] < 0 && pcm[i] >= 0;
        return n;
    };
    check(std::abs(crossings(render(0, 0, 72)) - 2 * crossings(render(0, 0, 60))) <= 2,
          "REPITCH octave doubles measured waveform frequency");
    auto positionAfter = [&](int pitch, int note)
    {
        p.mode = 0;
        p.pitchMode = pitch;
        rastro::Engine engine;
        engine.prepare(rate);
        engine.noteOn(note, 1, &sample, p);
        float l, r;
        engine.frame(l, r, &sample, p);
        float start = engine.position;
        for (int i = 0; i < 1000; ++i)
            engine.frame(l, r, &sample, p);
        return engine.position.load() - start;
    };
    float native = positionAfter(0, 60);
    check(std::abs(native - 1000.f / (float)rate) < .00001f,
          "root plays original source duration across host sample rates");
    check(std::abs(positionAfter(0, 72) - native * 2) < .00001f &&
              std::abs(positionAfter(0, 48) - native * .5f) < .00001f,
          "REPITCH octave doubles/halves sample traversal speed");
    check(positionAfter(1, 48) == native && positionAfter(1, 72) == native,
          "FIXED PITCH keeps sample traversal speed regardless of note");
    for (int note = 60; note < 68; ++note)
        check(render(4, 0, note) == render(4, 1, note),
              "Slice mapping/audio identical in both pitch modes");
    auto silence = render(4, 1, 59);
    for (float v : silence)
        check(v == 0, "FIXED PITCH does not bypass Slice note range");
}

int main()
{
    for (double rate : {22050., 44100., 48000., 96000.})
    {
        noteOffTests(rate);
        noteOffTests(rate, 1);
        pitchModeTests(rate);
        rastro::Sample sample;
        sample.rate = rate;
        for (auto &c : sample.data)
        {
            c.resize(4800);
            for (int i = 0; i < 4800; ++i)
                c[i] = .4f * std::sin(i * .07f);
        }
        rastro::Settings p;
        p.enabled.fill(false);
        p.start = .1f;
        p.end = .9f;
        p.velocity = 0;
        rastro::Engine e;
        e.prepare(rate);
        auto frame = [&](int count)
        {
            float l = 0, r = 0;
            double energy = 0;
            for (int i = 0; i < count; ++i)
            {
                e.frame(l, r, &sample, p);
                check(std::isfinite(l) && std::abs(l) <= 1, "all modes bounded");
                energy += l * l;
            }
            return energy;
        };
        for (int mode = 0; mode < 5; ++mode)
        {
            p.mode = mode;
            e.reset();
            e.noteOn(60, 1, &sample, p);
            check(frame(400) > 0, "mode audible");
            float a = e.position;
            check(a > p.start && a < p.end, "trim boundaries respected");
            if (mode == 2)
            {
                frame(200);
                check(e.position < a, "Reverse runs backwards");
            }
            if (mode == 0 || mode == 2 || mode == 4)
            {
                frame(5000);
                float stopped = e.position;
                frame(1000);
                check(e.position == stopped, "one pass modes stop");
            }
            else
            {
                frame(5000);
                float b = e.position;
                frame(500);
                check(e.position != b, "loop modes continue");
                e.noteOff(60, p);
                frame((int)rate);
                float stopped = e.position;
                frame(500);
                check(e.position == stopped, "note off releases loop modes");
            }
        }
        p.mode = 3;
        p.start = 0;
        p.end = 1;
        e.reset();
        e.noteOn(60, 1, &sample, p);
        frame(4700);
        float before = e.position;
        frame(500);
        check(e.position < before, "Ping-Pong reflects at end");
        p.mode = 4;
        p.sliceCount = 4;
        p.cuts = {0, .2f, .45f, .7f, 1};
        e.reset();
        e.noteOn(62, .8f, &sample, p);
        frame(1);
        check(std::abs(e.position - .45f) < .001f, "MIDI root+2 selects manual third slice");
        frame(1400);
        check(e.position < .701f, "slice does not cross its end");
        e.reset();
        e.noteOn(59, 1, &sample, p);
        check(frame(1000) == 0, "notes outside slice map ignored");
        p.mode = 0;
        p.loop = 1;
        e.reset();
        e.noteOn(60, 1, &sample, p);
        frame(10000);
        float pos = e.position;
        frame(200);
        check(e.position != pos, "legacy loop parameter preserved");
        p.loop = 0;
        p.mode = 4;
        p.start = .999f;
        p.end = .001f;
        e.reset();
        e.noteOn(60, 1, &sample, p);
        frame(2000);
    }
    std::cout << "PASS: One Shot, Loop, Reverse, Ping-Pong, manual MIDI Slices; trims, velocity, "
                 "release, legacy loop, four sample rates\n";
}
