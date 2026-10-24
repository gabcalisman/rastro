#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace rastro::fx
{
// First-order antiderivative antialiasing. Three-point Gauss-Legendre quadrature
// averages a memoryless transfer curve over each input-sample segment. It suppresses
// foldback without oversampling latency or affecting the plugin's exact-dry path.
template <typename Transfer>
inline float antialiasedShape(float input, float &previous, Transfer &&transfer) noexcept
{
    constexpr float nodes[]{-.7745966692f, 0.f, .7745966692f};
    constexpr float weights[]{5.f / 9.f, 8.f / 9.f, 5.f / 9.f};
    if (!std::isfinite(input))
    {
        previous = 0;
        return 0;
    }
    const float old = previous;
    previous = input;
    const float delta = input - old;
    if (std::abs(delta) < 1e-5f)
        return transfer(.5f * (input + old));
    const float midpoint = .5f * (input + old), half = .5f * delta;
    float average = 0;
    for (int i = 0; i < 3; ++i)
        average += weights[i] * transfer(midpoint + half * nodes[i]);
    return .5f * average;
}

// Residual ADAA for unity-slope safety curves (feedback clippers, Colour). Only the curve's
// deviation from a straight wire, f(u) - u, is averaged. Signals that stay in the linear region
// pass exactly, with no half-sample delay or treble loss per feedback round trip; clipping
// products keep the same antialiasing as antialiasedShape. `previous` stores the last u.
// `ceiling` restores the curve's bound for pathological jumps between samples (for example
// -8 to +8, where the averaged residual cancels); normal programme material never reaches it.
template <typename Transfer>
inline float antialiasedResidual(float input, float &previous, Transfer &&transfer,
                                 float ceiling = std::numeric_limits<float>::infinity()) noexcept
{
    if (!std::isfinite(input))
    {
        previous = 0;
        return 0;
    }
    const float y =
        input + antialiasedShape(input, previous, [&transfer](float u) { return transfer(u) - u; });
    return std::clamp(y, -ceiling, ceiling);
}

constexpr float softLimitCeiling = 1.5f;
// Transparent up to full scale, then a smooth knee (slope 1 at the knee) to a 1.5 (+3.5 dBFS)
// ceiling.
inline float softLimit(float u) noexcept
{
    const float magnitude = std::abs(u);
    if (magnitude <= 1)
        return u;
    return std::copysign(1 + .5f * std::tanh(2 * (magnitude - 1)), u);
}
} // namespace rastro::fx
