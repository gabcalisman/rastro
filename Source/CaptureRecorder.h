#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>
namespace rastro
{
// Ownership: idle -> starting -> recording <-> writing -> ready -> exporting -> idle.
// Only writing owns mutable PCM. The worker can claim PCM only after ready (acquire).
class CaptureRecorder
{
    static constexpr int maxFrames = 2 * 1024 * 1024;
    std::array<std::vector<float>, 2> pcm;
    int writePosition = 0;
    std::atomic<int> limit{0};
    std::atomic<double> currentRate{48000};
    double takeRate = 48000;
    std::atomic<bool> stopRequested{false};

  public:
    enum State
    {
        idle,
        recording,
        writing,
        ready,
        exporting,
        starting
    };
    std::atomic<int> state{idle}, frames{0}, inputChannels{0}, route{0};
    std::atomic<float> peak{0}, gain{1};
    std::atomic<bool> monitor{false};
    void prepare(double sr)
    {
        stop();
        finishStop();
        // Allocate once outside the audio callback. Device/sample-rate changes never resize a take.
        if (pcm[0].empty())
            for (auto &c : pcm)
                c.resize(maxFrames);
        currentRate = sr;
        limit = std::min(maxFrames, int(std::clamp(sr, 8000., 384000.) * 30));
    }
    bool start()
    {
        if (pcm[0].empty())
            return false;
        int expected = idle;
        if (!state.compare_exchange_strong(expected, starting))
            return false;
        writePosition = 0;
        frames = 0;
        takeRate = currentRate.load();
        stopRequested = false;
        state.store(recording, std::memory_order_release);
        return true;
    }
    void stop() { stopRequested.store(true, std::memory_order_release); }
    void finishStop()
    {
        if (!stopRequested.load(std::memory_order_acquire))
            return;
        int expected = recording;
        state.compare_exchange_strong(expected, ready, std::memory_order_acq_rel);
    }
    void process(const float *const *input, int channels, int count)
    {
        inputChannels.store(channels, std::memory_order_relaxed);
        float level = 0;
        int r = std::clamp(route.load(std::memory_order_relaxed), 0, 2);
        float scale = std::clamp(gain.load(std::memory_order_relaxed), 0.f, 2.f);
        for (int i = 0; i < count; ++i)
            for (int c = 0; c < std::min(2, channels); ++c)
                if (input[c])
                {
                    float value = input[c][i] * scale;
                    level = std::max(level, std::isfinite(value) ? std::abs(value) : 1.f);
                }
        peak.store(std::max(level, peak.load(std::memory_order_relaxed) * .85f),
                   std::memory_order_relaxed);
        finishStop();
        int expected = recording;
        if (!state.compare_exchange_strong(expected, writing, std::memory_order_acq_rel))
            return;
        if (channels > 0 && input[0])
            for (int i = 0; i < count && writePosition < limit.load(std::memory_order_relaxed);
                 ++i, ++writePosition)
                for (int c = 0; c < 2; ++c)
                {
                    int source = r == 1 ? 0 : r == 2 ? 1 : c;
                    source = std::min(channels - 1, source);
                    float value = input[source] ? input[source][i] * scale : 0;
                    pcm[c][writePosition] = std::isfinite(value) ? std::clamp(value, -1.f, 1.f) : 0;
                }
        frames.store(writePosition, std::memory_order_release);
        bool done = stopRequested.load(std::memory_order_acquire) || writePosition >= limit.load();
        state.store(done ? ready : recording, std::memory_order_release);
    }
    bool claim()
    {
        int expected = ready;
        return state.compare_exchange_strong(expected, exporting, std::memory_order_acq_rel);
    }
    void release() { state.store(idle, std::memory_order_release); }
    double sampleRate() const
    {
        return takeRate;
    } // only read after claim, or on the message thread while not starting
    double seconds() const { return frames.load() / currentRate.load(); }
    double maxSeconds() const { return limit.load() / currentRate.load(); }
    const float *channel(int c) const { return pcm[c].data(); } // immutable while exporting
};
} // namespace rastro
