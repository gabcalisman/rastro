#pragma once
#include "Engine.h"
#include <chrono>
#include <mutex>
#include <thread>
namespace rastro
{
inline constexpr const char *patchIds[]{
    "synthEnable",  "ampAttack",       "ampDecay",      "ampSustain",   "ampRelease",
    "filterCutoff", "filterResonance", "envAmount",     "envTarget",    "lfoRate",
    "lfoDepth",     "lfoTarget",       "voiceMode",     "glide",        "triggerMode",
    "regionLoop",   "lfoTiming",       "loopRegionSet", "loopCrossfade"};
using PatchValues = std::array<float, std::size(patchIds)>;
struct SamplePatch
{
    PatchValues values{1, .01f, .2f, .75f, .2f, 7000, .1f, 0, 0, 1, 0, 0, 0, 0, 2, 1, 1, 0, 5};
    float distance = 0, rank = 0;
};
inline Settings patchSettings(const SamplePatch &patch, Settings base)
{
    const auto &v = patch.values;
    base.synth = v[0] > .5f;
    base.attack = v[1];
    base.decay = v[2];
    base.sustain = v[3];
    base.release = v[4];
    base.cutoff = v[5];
    base.resonance = v[6];
    base.envAmount = v[7];
    base.envTarget = int(v[8]);
    base.lfoRate = v[9];
    base.lfoDepth = v[10];
    base.lfoTarget = int(v[11]);
    base.voiceMode = int(v[12]);
    base.glide = v[13];
    base.triggerMode = int(v[14]);
    base.regionLoop = v[15] > .5f;
    base.lfoTiming = int(v[16]);
    base.loopRegionSet = v[17] > .5f;
    base.loopCrossfade = v[18];
    return base;
}
class PatchSearch
{
    std::thread worker;
    mutable std::mutex resultMutex;
    std::vector<SamplePatch> result;
    std::atomic<const Sample *> resultSource{nullptr};
    std::atomic<bool> cancelled{false};
    using Features = std::array<float, 16>;
    static Features features(const std::vector<float> &audio)
    {
        Features out{};
        std::array<double, 5> state{}, energy{};
        double high = 0, total = 0, peak = 0;
        int crossings = 0;
        constexpr double cutoff[]{200, 600, 1500, 3500, 6000};
        std::array<double, 5> alpha{};
        for (int j = 0; j < 5; ++j)
            alpha[j] = 1 - std::exp(-6.28318530718 * cutoff[j] / 16000);
        for (size_t i = 0; i < audio.size(); ++i)
        {
            double x = audio[i];
            total += x * x;
            peak = std::max(peak, std::abs(x));
            out[std::min<size_t>(7, i * 8 / audio.size())] += float(x * x);
            double previous = 0;
            for (int j = 0; j < 5; ++j)
            {
                state[j] += (x - state[j]) * alpha[j];
                double band = state[j] - previous;
                energy[j] += band * band;
                previous = state[j];
            }
            high += (x - previous) * (x - previous);
            if (i && ((audio[i - 1] < 0) != (x < 0)))
                ++crossings;
        }
        float largest = 0;
        for (int i = 0; i < 8; ++i)
            largest = std::max(largest, out[i]);
        for (int i = 0; i < 8; ++i)
            out[i] = std::sqrt(out[i] / std::max(1e-15f, largest));
        double bands = high;
        for (double v : energy)
            bands += v;
        for (int j = 0; j < 5; ++j)
            out[8 + j] = float(std::sqrt(energy[j] / std::max(1e-15, bands)));
        out[13] = float(std::sqrt(high / std::max(1e-15, bands)));
        out[14] = float(crossings) / std::max<size_t>(1, audio.size());
        double rms = std::sqrt(total / std::max<size_t>(1, audio.size()));
        out[15] = float(std::min(1., std::max(0., peak / std::max(1e-12, rms) - 1) / 8));
        return out;
    }
    static float compare(const Features &a, const Features &b)
    {
        float sum = 0;
        for (size_t i = 0; i < a.size(); ++i)
            sum += (a[i] - b[i]) * (a[i] - b[i]);
        return std::sqrt(sum / a.size());
    }
    static float diversity(const SamplePatch &a, const SamplePatch &b)
    {
        float sum = std::abs(std::log2(a.values[5] / b.values[5])) / 7;
        for (int i : {1, 2, 3, 6, 7, 10})
            sum += std::abs(a.values[i] - b.values[i]);
        return sum / 7;
    }
    bool waitForPermission()
    {
        while (paused.load() && !cancelled.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return !cancelled.load();
    }
    void run(const Sample *source, Settings base, bool transformed, unsigned seed)
    {
        constexpr int count = 48, length = 6400;
        constexpr double sr = 16000;
        std::vector<float> reference(length), rendered(length);
        double begin = std::clamp(base.start, 0.f, .999f) * (source->size() - 1),
               end = std::clamp(base.end, .001f, 1.f) * (source->size() - 1);
        end = std::max(begin + 1, end);
        double energy = 0;
        std::array<double, 2> channelEnergy{};
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < length; ++i)
            {
                auto pos = begin + i * source->rate / sr;
                if (pos < end)
                {
                    auto at = std::clamp(int(pos), 0, source->size() - 1);
                    auto x = source->data[c][at];
                    channelEnergy[c] += x * x;
                }
            }
        const int channel = channelEnergy[1] > channelEnergy[0] ? 1 : 0;
        // Reference is a bounded excerpt of the selected source, not a guessed MIDI transcription.
        for (int i = 0; i < length; ++i)
        {
            double position = begin + i * source->rate / sr;
            float value = 0;
            if (position < end)
            {
                int at = std::clamp(int(position), 0, source->size() - 2);
                float f = float(position - at);
                value = source->data[channel][at] +
                        f * (source->data[channel][at + 1] - source->data[channel][at]);
            }
            reference[i] = value;
            energy += value * value;
        }
        if (energy < 1e-10)
        {
            status = silent;
            busy = false;
            return;
        }
        auto target = features(reference);
        Engine engine;
        engine.prepare(sr);
        std::vector<SamplePatch> pool;
        pool.reserve(count);
        auto random = [&]()
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            return float(seed >> 8) / 16777215.f;
        };
        for (int candidate = 0; candidate < count; ++candidate)
        {
            if (!waitForPermission())
            {
                status = canceled;
                busy = false;
                return;
            }
            auto started = std::chrono::steady_clock::now();
            SamplePatch patch;
            if (candidate)
            {
                auto &v = patch.values;
                v[1] = .001f + random() * random() * (transformed ? .3f : .08f);
                v[2] = .03f + random() * .8f;
                v[3] = .08f + random() * .92f;
                v[4] = .025f + random() * 1.5f;
                v[5] = std::exp2(7 + random() * 5.6f);
                v[6] = random() * (transformed ? .85f : .45f);
                v[7] = (random() * 2 - 1) * (transformed ? .85f : .35f);
                v[9] = .2f + random() * 5;
                v[10] = random() * (transformed ? .65f : .16f);
                v[11] = transformed && random() > .65f ? 1.f : 0.f;
            }
            auto settings = patchSettings(patch, base);
            settings.enabled.fill(false);
            settings.transportValid = false;
            settings.mode = 0;
            settings.loop = 0;
            engine.reset();
            engine.noteOn(int(base.root), .9f, source, settings);
            bool valid = true;
            for (int i = 0; i < length; ++i)
            {
                if (i % 256 == 0 && !waitForPermission())
                {
                    status = canceled;
                    busy = false;
                    return;
                }
                if (i == length * 2 / 3)
                    engine.noteOff(int(base.root), settings);
                float l, r;
                engine.frame(l, r, source, settings);
                rendered[i] = channel ? r : l;
                valid &= std::isfinite(rendered[i]);
            }
            if (valid)
            {
                patch.distance = compare(target, features(rendered));
                patch.rank = transformed ? std::abs(patch.distance - .32f) : patch.distance;
                pool.push_back(patch);
            }
            progress.store(float(candidate + 1) / count, std::memory_order_release);
            // Approximately half a core: sleep for the measured compute time in cancelable slices.
            auto rest = std::chrono::steady_clock::now() +
                        std::max(std::chrono::steady_clock::now() - started,
                                 std::chrono::steady_clock::duration(std::chrono::milliseconds(1)));
            while (std::chrono::steady_clock::now() < rest && !cancelled.load())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (cancelled)
        {
            status = canceled;
            busy = false;
            return;
        }
        std::sort(pool.begin(), pool.end(), [](auto &a, auto &b) { return a.rank < b.rank; });
        std::vector<SamplePatch> chosen;
        chosen.reserve(6);
        for (auto &patch : pool)
        {
            bool distinct = true;
            for (auto &other : chosen)
                distinct &= diversity(patch, other) > .045f;
            if (distinct)
                chosen.push_back(patch);
            if (chosen.size() == 6)
                break;
        }
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            result = std::move(chosen);
            resultSource.store(source, std::memory_order_release);
        }
        revision.fetch_add(1, std::memory_order_release);
        status = complete;
        busy = false;
    }

  public:
    enum Status
    {
        idle,
        searching,
        complete,
        canceled,
        silent,
        failed
    };
    std::atomic<bool> busy{false}, paused{false};
    std::atomic<float> progress{0};
    std::atomic<int> status{idle};
    std::atomic<unsigned> revision{0};
    ~PatchSearch() { stop(); }
    void stop()
    {
        cancelled = true;
        paused = false;
        if (worker.joinable())
            worker.join();
        busy = false;
    }
    bool start(const Sample *source, Settings base, bool transformed, unsigned seed = 0x52415354)
    {
        if (busy || !source || source->size() < 2)
            return false;
        if (worker.joinable())
            worker.join();
        cancelled = false;
        paused = false;
        progress = 0;
        status = searching;
        busy = true;
        try
        {
            worker = std::thread(
                [this, source, base, transformed, seed]
                {
                    try
                    {
                        run(source, base, transformed, seed ? seed : 1);
                    }
                    catch (...)
                    {
                        status = failed;
                        busy = false;
                    }
                });
        }
        catch (...)
        {
            status = failed;
            busy = false;
            return false;
        }
        return true;
    }
    void cancel()
    {
        cancelled = true;
        paused = false;
    }
    std::vector<SamplePatch> results() const
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        return result;
    }
    const Sample *sourceUsed() const { return resultSource.load(std::memory_order_acquire); }
};
} // namespace rastro
