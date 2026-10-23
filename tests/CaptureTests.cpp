#include "CaptureRecorder.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
thread_local bool inCallback = false;
void *operator new(size_t n)
{
    if (inCallback)
        throw std::runtime_error("capture audio allocation");
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
int main()
{
    for (double sr : {22050., 44100., 48000., 96000., 192000., 384000.})
    {
        rastro::CaptureRecorder rec;
        rec.prepare(sr);
        std::array<float, 256> l{}, r{};
        for (int i = 0; i < 256; ++i)
        {
            l[i] = .3f * std::sin(i * .11f);
            r[i] = .2f * std::cos(i * .17f);
        }
        const float *input[]{l.data(), r.data()};
        check(!rec.monitor && rec.start(), "capture starts with monitoring off");
        for (int i = 0; i < 8; ++i)
        {
            inCallback = true;
            rec.process(input, 2, 256);
            inCallback = false;
        }
        rec.stop();
        rec.finishStop();
        check(rec.claim(), "worker claims completed take");
        check(rec.frames == 2048 && rec.sampleRate() == sr,
              "capture frame count and source sample rate");
        check(!rec.start(), "cannot overwrite take being exported");
        for (int i = 0; i < 2048; ++i)
            check(rec.channel(0)[i] == l[i % 256] && rec.channel(1)[i] == r[i % 256],
                  "recorded stereo PCM matches input");
        rec.prepare(sr * .5);
        check(rec.sampleRate() == sr && rec.channel(0)[123] == l[123],
              "device change does not resize or corrupt an exporting take");
        rec.release();
        rec.prepare(sr);
        rec.route = 2;
        rec.gain = 2;
        check(rec.start(), "second capture starts");
        inCallback = true;
        rec.process(input, 2, 256);
        inCallback = false;
        rec.stop();
        rec.finishStop();
        check(rec.claim(), "mono take claim");
        for (int i = 0; i < 256; ++i)
            check(rec.channel(0)[i] == r[i] * 2 && rec.channel(1)[i] == r[i] * 2,
                  "right-channel mono routing and gain are real");
        rec.release();
        rec.route = 0;
        rec.gain = 1;
        rec.start();
        while (rec.state == rastro::CaptureRecorder::recording)
        {
            inCallback = true;
            rec.process(input, 2, 256);
            inCallback = false;
        }
        check(rec.frames <= 2 * 1024 * 1024 && rec.seconds() <= 30.001 && rec.claim(),
              "automatic stop respects prepared memory and duration limits");
        rec.release();
        rec.start();
        l[0] = std::numeric_limits<float>::quiet_NaN();
        inCallback = true;
        rec.process(input, 2, 256);
        inCallback = false;
        rec.stop();
        rec.finishStop();
        rec.claim();
        check(rec.channel(0)[0] == 0 && std::isfinite(rec.peak.load()),
              "nonfinite input is sanitized");
        rec.release();
    }
    std::cout << "PASS capture: exact stereo PCM, gain/channel routing, stop/claim ownership, "
                 "device-rate change, bounded automatic stop, six sample rates, no callback "
                 "allocations\n";
}
