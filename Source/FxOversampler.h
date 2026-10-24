#pragma once
#include <array>
#include <cstddef>

namespace rastro::fx
{
// Linear-phase 2x oversampler used only by the FX drive stages (EMBER, SQUASH).
// 47-tap equiripple halfband: passband 0..0.208 of the 2x rate (20 kHz at 48 kHz),
// stopband from 0.292 (28 kHz at 48 kHz), about -69 dB, passband ripple below 0.01 dB.
// Polyphase up/down; the round trip has an exact, integer latency of 23 base-rate samples,
// so dry paths can be aligned bit for bit. Fixed-size state, no allocation.
class HalfbandOversampler
{
  public:
    static constexpr int latency = 23;

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            input[c].fill(0);
            even[c].fill(0);
            odd[c].fill(0);
        }
        inputPos = evenPos = oddPos = 0;
    }
    // Clears only the shaped (2x-rate) history; keeps the input history so an aligned dry
    // delay continues without a gap when a sleeping stage wakes up.
    void resetOutput()
    {
        for (int c = 0; c < 2; ++c)
        {
            even[c].fill(0);
            odd[c].fill(0);
        }
    }
    // Stores one base-rate input sample without filtering work (used while a stage sleeps).
    void push(int c, float x)
    {
        if (c == 0)
            inputPos = (inputPos + taps - 1) % taps;
        input[c][std::size_t(inputPos)] = input[c][std::size_t(inputPos + taps)] = x;
    }
    // Base-rate input delayed by the oversampler latency, after push()/up() for this sample.
    float delayed(int c) const { return input[c][std::size_t(inputPos + latency)]; }
    // One base-rate sample in, two 2x-rate samples out (first, second in time).
    void up(int c, float x, float &first, float &second)
    {
        push(c, x);
        const float *h = &input[c][std::size_t(inputPos)];
        float sum = 0;
        for (int j = 0; j < taps / 2; ++j)
            sum += coefficient[std::size_t(j)] * (h[j] + h[taps - 1 - j]);
        first = 2 * sum;
        second = h[(taps - 2) / 2]; // centre tap of the zero-stuffed stream: x[n - 11]
    }
    // Two 2x-rate samples in, one band-limited base-rate sample out.
    float down(int c, float first, float second)
    {
        if (c == 0)
        {
            evenPos = (evenPos + taps - 1) % taps;
            oddPos = (oddPos + oddSize - 1) % oddSize;
        }
        even[c][std::size_t(evenPos)] = even[c][std::size_t(evenPos + taps)] = first;
        odd[c][std::size_t(oddPos)] = odd[c][std::size_t(oddPos + oddSize)] = second;
        const float *h = &even[c][std::size_t(evenPos)];
        float sum = 0;
        for (int j = 0; j < taps / 2; ++j)
            sum += coefficient[std::size_t(j)] * (h[j] + h[taps - 1 - j]);
        return sum + .5f * odd[c][std::size_t(oddPos + oddSize - 1)];
    }

  private:
    static constexpr int taps = 24, oddSize = 13;
    // Non-zero even-index taps (symmetric, first half), normalized for exact unity DC gain.
    static constexpr std::array<float, 12> coefficient{
        {-5.077390006902e-04f, 1.093747685862e-03f, -2.193130511628e-03f, 3.936008530154e-03f,
         -6.556648878025e-03f, 1.037030872098e-02f, -1.584552018085e-02f, 2.379308843761e-02f,
         -3.589349291221e-02f, 5.650401346233e-02f, -1.017130860255e-01f, 3.170124506719e-01f}};
    std::array<std::array<float, taps * 2>, 2> input{}, even{};
    std::array<std::array<float, oddSize * 2>, 2> odd{};
    int inputPos = 0, evenPos = 0, oddPos = 0;
};
} // namespace rastro::fx
