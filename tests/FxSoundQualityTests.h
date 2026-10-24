#pragma once
// Sound-quality regression tests for the RASTRO FX DSP: oversampling, residual antialiasing,
// gain staging, latency alignment, sleeping modules, denormals, host tempo and rate independence.
#include "FxRackIntegration.h"
#include <cfenv>
#include <chrono>

namespace fxsound
{
using rastro::fx::pi;
constexpr int latency = rastro::fx::Rack::latency();

// Single-bin DFT magnitude (amplitude) at an arbitrary frequency, Hann-windowed.
static double tone(const std::vector<float> &x, double frequency, double rate)
{
    double re = 0, im = 0, norm = 0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        const double w = .5 - .5 * std::cos(2 * pi * double(i) / double(x.size() - 1));
        re += w * x[i] * std::cos(2 * pi * frequency * double(i) / rate);
        im += w * x[i] * std::sin(2 * pi * frequency * double(i) / rate);
        norm += w;
    }
    return 2 * std::hypot(re, im) / norm;
}
// Energy that is neither a harmonic of `f0` nor DC, as a ratio to the fundamental (dB).
static double aliasRatioDb(const std::vector<float> &x, double f0, double rate)
{
    const int n = int(x.size());
    std::vector<double> w(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
        w[size_t(i)] = .5 - .5 * std::cos(2 * pi * i / (n - 1));
    double alias = 0;
    const double binHz = rate / n;
    for (int bin = int(40 / binHz); bin < int(20000 / binHz); bin += 1)
    {
        const double f = bin * binHz;
        const double h = f / f0;
        if (std::abs(h - std::round(h)) * f0 < 6 * binHz)
            continue;
        double re = 0, im = 0;
        for (int i = 0; i < n; ++i)
        {
            re += w[size_t(i)] * x[size_t(i)] * std::cos(2 * pi * bin * i / n);
            im += w[size_t(i)] * x[size_t(i)] * std::sin(2 * pi * bin * i / n);
        }
        alias += re * re + im * im;
    }
    const double fundamental = tone(x, f0, rate) * n * .25;
    return 10 * std::log10(alias / (fundamental * fundamental) + 1e-30);
}
static rastro::fx::Frame single(int id, std::array<float, 4> value, float mix = 1)
{
    auto f = defaultFxFrame();
    for (int i = 0; i < rastro::fx::count; ++i)
    {
        f.enabled[i] = i == id ? 1.f : 0.f;
        f.mix[i] = i == id ? mix : 0.f;
    }
    for (int k = 0; k < rastro::fx::modules[id].controls; ++k)
        if (value[size_t(k)] >= 0)
            f.value[id][size_t(k)] = value[size_t(k)];
    return f;
}

static void oversamplerTests()
{
    // Exact integer latency, unity DC and a flat passband through the up/down round trip.
    rastro::fx::HalfbandOversampler os;
    std::vector<float> impulse;
    for (int i = 0; i < 200; ++i)
    {
        float a, b, unusedA, unusedB;
        os.up(0, i == 0 ? 1.f : 0.f, a, b);
        os.up(1, 0, unusedA, unusedB);
        float y = os.down(0, a, b);
        os.down(1, 0, 0);
        impulse.push_back(y);
    }
    const auto peak = std::max_element(impulse.begin(), impulse.end(),
                                       [](float x, float y) { return std::abs(x) < std::abs(y); });
    check(peak - impulse.begin() == rastro::fx::HalfbandOversampler::latency,
          "oversampler round-trip latency is exactly the declared integer");
    double dc = 0;
    for (float v : impulse)
        dc += v;
    check(std::abs(dc - 1) < 1e-5, "oversampler has unity DC gain");
    for (double f : {100., 1000., 10000., 18000.})
    {
        double re = 0, im = 0;
        for (size_t i = 0; i < impulse.size(); ++i)
        {
            re += impulse[i] * std::cos(2 * pi * f * double(i) / 48000);
            im -= impulse[i] * std::sin(2 * pi * f * double(i) / 48000);
        }
        check(std::abs(20 * std::log10(std::hypot(re, im))) < .02,
              "oversampler passband is flat to 18 kHz at 48 kHz");
    }
    // A hard tanh at 3011 Hz: the 2x stage removes most of the folded energy.
    const double rate = 48000, f0 = 3011, drive = 10.8;
    std::vector<float> direct, oversampled;
    rastro::fx::HalfbandOversampler stage;
    for (int i = 0; i < 16384 + 64; ++i)
    {
        const float x = float(.5 * std::sin(2 * pi * f0 * i / rate));
        float a, b, unusedA, unusedB;
        stage.up(0, x, a, b);
        stage.up(1, 0, unusedA, unusedB);
        a = std::tanh(float(a * drive));
        b = std::tanh(float(b * drive));
        float y = stage.down(0, a, b);
        stage.down(1, 0, 0);
        if (i >= 64)
        {
            direct.push_back(std::tanh(float(x * drive)));
            oversampled.push_back(y);
        }
    }
    const double plain = aliasRatioDb(direct, f0, rate), os2 = aliasRatioDb(oversampled, f0, rate);
    std::cout << "    alias/fundamental tanh x10.8 @3011 Hz: direct " << plain << " dB, 2x " << os2
              << " dB\n";
    check(os2 < plain - 20 && os2 < -50,
          "2x oversampled drive suppresses folded harmonics by 20 dB or more");
}

static void residualTests()
{
    // Linear region passes exactly (no half-sample delay or treble loss per feedback pass).
    float previous = 0, previousTanh = 0;
    unsigned seed = 7;
    for (int i = 0; i < 20000; ++i)
    {
        const float u = float((seed = seed * 1664525u + 1013904223u) >> 8) / 8388608.f - 1.f;
        check(rastro::fx::antialiasedResidual(u, previous, rastro::fx::softLimit) == u,
              "residual soft limit is bit-exact below full scale");
        check(rastro::fx::antialiasedResidual(u * .001f, previousTanh, [](float s)
                                              { return s + (std::tanh(s) - s) * 0; }) == u * .001f,
              "Colour 0 TRAIL shaper is an exact wire");
    }
    float state = 0;
    for (int i = 0; i < 4800; ++i)
    {
        const float y =
            rastro::fx::antialiasedResidual(float(8 * std::sin(i * .37)), state,
                                            rastro::fx::softLimit, rastro::fx::softLimitCeiling);
        check(std::abs(y) <= 1.5f + 1e-6f, "soft limit ceiling bounds feedback");
    }
    // TRAIL Colour 0: repeats keep their top end. Compare echo spectra of a click.
    rastro::fx::Node trail;
    trail.prepare(48000);
    auto f = single(8, {.2f, .5f, 1, 0}); // short time, feedback .45, tone 18 kHz, clean
    std::vector<float> out;
    for (int i = 0; i < 48000; ++i)
    {
        float l = i == 0 ? 1.f : 0.f, r = l;
        trail.process(8, l, r, f.value[8], f.pan, false);
        out.push_back(l);
    }
    // Locate first two echoes and compare their high/low energy balance.
    std::vector<int> peaks;
    for (int i = 1; i + 1 < int(out.size()) && peaks.size() < 2; ++i)
        if (std::abs(out[size_t(i)]) > .05f &&
            std::abs(out[size_t(i)]) >= std::abs(out[size_t(i - 1)]) &&
            std::abs(out[size_t(i)]) >= std::abs(out[size_t(i + 1)]))
        {
            peaks.push_back(i);
            i += 200;
        }
    check(peaks.size() == 2, "TRAIL produces repeats");
    auto width = [&](int at)
    {
        double sum = 0, weighted = 0;
        for (int k = -40; k <= 40; ++k)
        {
            const double v = out[size_t(at + k)] * out[size_t(at + k)];
            sum += v;
            weighted += v * k * k;
        }
        return weighted / sum;
    };
    // Each pass adds only the 18 kHz tone filter: the second repeat is barely wider than the first.
    check(width(peaks[1]) < width(peaks[0]) * 2.5 + .5, "clean TRAIL repeats stay bright");
}

static std::vector<float> sine(double frequency, double amplitude, int n, double rate)
{
    std::vector<float> x(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
        x[size_t(i)] = float(amplitude * std::sin(2 * pi * frequency * i / rate));
    return x;
}
static double peakAfter(const std::vector<float> &x, size_t from)
{
    double p = 0;
    for (size_t i = from; i < x.size(); ++i)
        p = std::max(p, double(std::abs(x[i])));
    return p;
}
static std::vector<float> renderNode(int id, const rastro::fx::Frame &f,
                                     const std::vector<float> &in, double rate = 48000)
{
    rastro::fx::Node node;
    node.prepare(rate);
    std::vector<float> out;
    for (float v : in)
    {
        float l = v, r = v;
        node.process(id, l, r, f.value[id], f.pan, false);
        out.push_back(l);
    }
    return out;
}

static void gainStagingTests()
{
    // EMBER: a -10 dBFS peak tone stays near -10 dBFS across drive and modes.
    const auto in = sine(1000, .316, 24000, 48000);
    for (float drive : {0.f, .5f, 1.f})
        for (float mode : {0.f, .5f, 1.f})
        {
            const auto out = renderNode(11, single(11, {drive, 1, .5f, mode}), in);
            const double db = 20 * std::log10(peakAfter(out, 12000));
            if (std::abs(db + 10) > 2.5)
                std::cerr << "EMBER drive " << drive << " mode " << mode << " peak " << db << '\n';
            check(std::abs(db + 10) < 2.5, "EMBER drive is gain-staged around the reference peak");
        }
    // SQUASH: same reference holds through threshold/ratio, and hot transients stay bounded.
    for (float threshold : {0.f, .5f, 1.f})
        for (float ratio : {0.f, 1.f})
        {
            const auto out = renderNode(12, single(12, {.2f, threshold, ratio, .5f}), in);
            const double db = 20 * std::log10(peakAfter(out, 12000));
            check(std::abs(db + 10) < 2.5, "SQUASH make-up keeps the reference level");
        }
    std::vector<float> burst(24000, 0.f);
    for (size_t i = 12000; i < burst.size(); ++i)
        burst[i] = float(std::sin(i * .21));
    const auto hot = renderNode(12, single(12, {1, 0, 1, 1}), burst);
    // 1.5 ceiling at 2x, plus at most the band-limiting (Gibbs) overshoot of a square wave.
    check(peakAfter(hot, 0) < 1.5f * 1.18f, "SQUASH transients stay under the soft ceiling");
}

static void rateIndependenceTests()
{
    // EMBER character no longer depends on the host rate (memory and DC blocker time constants).
    double level[2], third[2];
    int k = 0;
    for (double rate : {48000., 96000.})
    {
        const auto in = sine(500, .5, int(rate), rate);
        const auto out = renderNode(11, single(11, {.6f, 1, .7f, 1}), in, rate);
        std::vector<float> tail(out.begin() + int(rate / 2), out.end());
        level[k] = tone(tail, 500, rate);
        third[k] = tone(tail, 1000, rate) + tone(tail, 1500, rate);
        ++k;
    }
    check(std::abs(20 * std::log10(level[0] / level[1])) < .3 &&
              std::abs(20 * std::log10(third[0] / third[1])) < 1.5,
          "EMBER tape/tube character matches at 48 and 96 kHz");
}

static void alignmentAndSleepTests()
{
    // Every module outside the rack: the rack is a pure, bit-exact delay of `latency` samples.
    rastro::fx::Rack rack;
    rack.prepare(48000);
    auto f = defaultFxFrame();
    f.inserted.fill(0);
    f.enabled.fill(0);
    std::vector<float> in(4000), out;
    for (size_t i = 0; i < in.size(); ++i)
        in[i] = fixture(int(i));
    for (float v : in)
    {
        float l = v, r = -v;
        rack.process(l, r, f, false);
        out.push_back(l);
        check(r == -l, "sleeping rack keeps channels intact");
    }
    for (size_t i = 0; i < out.size(); ++i)
        check(out[i] == (i >= size_t(latency) ? in[i - size_t(latency)] : 0.f),
              "an empty rack is an exact delay of the reported latency");
    for (bool parallel : {false, true})
    {
        f.parallel = parallel;
        rack.reset();
        for (size_t i = 0; i < in.size(); ++i)
        {
            float l = in[i], r = in[i];
            rack.process(l, r, f, false);
            check(l == (i >= size_t(latency) ? in[i - size_t(latency)] : 0.f),
                  "series and parallel share the same alignment");
        }
    }
    // A module removed from the rack and inserted again never replays audio from before.
    f = single(0, {.5f, .8f, .6f, .8f});
    f.parallel = false;
    rack.reset();
    for (int i = 0; i < 9600; ++i)
    {
        float l = fixture(i), r = l;
        rack.process(l, r, f, false);
    }
    f.inserted[0] = 0;
    f.enabled[0] = 0;
    for (int i = 0; i < 480; ++i) // short sleep: buffers are not fully cleared yet
    {
        float l = 0, r = 0;
        rack.process(l, r, f, false);
    }
    f.inserted[0] = 1;
    f.enabled[0] = 1;
    double leak = 0;
    for (int i = 0; i < 48000; ++i)
    {
        float l = 0, r = 0;
        rack.process(l, r, f, false);
        leak = std::max(leak, double(std::abs(l)));
    }
    check(leak == 0, "re-inserted module starts clean (no stale echoes)");
    // Sleeping modules are cheap: an empty rack costs far less than all fifteen awake.
    auto time = [&](bool awake)
    {
        rastro::fx::Rack r;
        r.prepare(48000);
        auto frame = defaultFxFrame();
        if (!awake)
        {
            frame.inserted.fill(0);
            frame.enabled.fill(0);
        }
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 24000; ++i)
        {
            float l = fixture(i), rr = l;
            r.process(l, rr, frame, false);
        }
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    };
    const double awake = time(true), asleep = time(false);
    std::cout << "    rack CPU: all awake " << awake * 1e6 / 24000 << " us/sample, empty "
              << asleep * 1e6 / 24000 << " us/sample\n";
    check(asleep < awake * .25, "modules outside the rack do not consume DSP time");
}

static void tempoTests()
{
    // GHOST note divisions follow the host tempo.
    auto echo = [](float bpm)
    {
        rastro::fx::Rack rack;
        rack.prepare(48000);
        auto f = single(0, {.5f, .3f, 1, .5f}); // half-beat division
        f.tempo = bpm;
        for (int i = 0; i < 48000; ++i)
        {
            float l = i < 4 ? 1.f : 0.f, r = l;
            rack.process(l, r, f, false);
            if (i > 200 && std::abs(l) > .05f)
                return i - latency;
        }
        return -1;
    };
    const int at120 = echo(120), at90 = echo(90);
    check(std::abs(at120 - 12000) < 400 && std::abs(at90 - 16000) < 400,
          "GHOST echo time follows the host BPM");
    // The processor passes the host BPM to the rack.
    struct Head : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setBpm(90);
            return info;
        }
    } head;
    RastroFxProcessor p;
    set(p, "fxM0InRack", 1);
    set(p, "fxM0On", 1);
    set(p, "fxM0Mix", 1);
    set(p, "fxM0P1", .3f);
    p.setPlayHead(&head);
    p.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> b(2, 256);
    juce::MidiBuffer midi;
    int first = -1;
    for (int block = 0; block < 100 && first < 0; ++block)
    {
        b.clear();
        if (block == 0)
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 4; ++i)
                    b.setSample(c, i, 1);
        p.processBlock(b, midi);
        for (int i = 0; i < 256; ++i)
            if (block * 256 + i > 400 && std::abs(b.getSample(0, i)) > .02f)
            {
                first = block * 256 + i - latency;
                break;
            }
    }
    p.setPlayHead(nullptr);
    check(std::abs(first - 16000) < 400, "host playhead BPM reaches GHOST");
}

static void processorQualityTests()
{
    // Denormals are flushed inside the callback: a decaying ORBIT feedback tail never yields
    // subnormal output samples.
    RastroFxProcessor p;
    set(p, "fxM7InRack", 1);
    set(p, "fxM7On", 1);
    set(p, "fxM7Mix", 1);
    set(p, "fxM7P2", 1);
    p.prepareToPlay(48000, 512);
    juce::AudioBuffer<float> b(2, 512);
    juce::MidiBuffer midi;
    bool subnormal = false;
    for (int block = 0; block < 1500; ++block)
    {
        b.clear();
        if (block == 0)
            b.setSample(0, 0, .9f);
        p.processBlock(b, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
                subnormal |= std::fpclassify(b.getSample(c, i)) == FP_SUBNORMAL;
    }
    check(!subnormal, "feedback tails are flushed, never subnormal");
    // Oversampled stages are independent of the host block size.
    auto render = [](int block)
    {
        RastroFxProcessor q;
        for (int id : {11, 12, 7})
        {
            set(q, RastroFxProcessor::moduleId(id, "InRack").toRawUTF8(), 1);
            set(q, RastroFxProcessor::moduleId(id, "On").toRawUTF8(), 1);
        }
        set(q, "fxM11P0", .9f);
        q.prepareToPlay(44100, block);
        std::vector<float> out;
        juce::MidiBuffer m;
        for (int offset = 0; offset < 6000; offset += block)
        {
            const int n = std::min(block, 6000 - offset);
            juce::AudioBuffer<float> buffer(2, n);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < n; ++i)
                    buffer.setSample(c, i, fixture(offset + i, c));
            q.processBlock(buffer, m);
            for (int i = 0; i < n; ++i)
                out.push_back(buffer.getSample(0, i));
        }
        return out;
    };
    const auto a = render(1), b17 = render(17), c512 = render(512);
    check(a == b17 && a == c512, "oversampled modules are bit-identical across block sizes");
    // Automating EMBER drive is click-free on a low tone.
    RastroFxProcessor q;
    set(q, "fxM11InRack", 1);
    set(q, "fxM11On", 1);
    set(q, "fxM11Mix", 1);
    q.prepareToPlay(48000, 64);
    juce::AudioBuffer<float> buffer(2, 64);
    float previous = 0, jump = 0;
    for (int block = 0; block < 1500; ++block)
    {
        if (block % 50 == 0)
            set(q, "fxM11P0", block % 100 == 0 ? 1.f : 0.f);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 64; ++i)
                buffer.setSample(c, i,
                                 float(.3 * std::sin(2 * pi * 80 * (block * 64 + i) / 48000.)));
        q.processBlock(buffer, midi);
        for (int i = 0; i < 64; ++i)
        {
            if (block > 2)
                jump = std::max(jump, std::abs(buffer.getSample(0, i) - previous));
            previous = buffer.getSample(0, i);
        }
    }
    check(jump < .08f, "drive automation has no zipper steps");
}

static void run()
{
    oversamplerTests();
    residualTests();
    gainStagingTests();
    rateIndependenceTests();
    alignmentAndSleepTests();
    tempoTests();
    processorQualityTests();
    std::cout << "PASS FX sound: 2x drive oversampling, residual antialiasing, gain staging, "
                 "aligned latency, sleeping modules, denormals, host tempo, rate independence\n";
}
} // namespace fxsound
