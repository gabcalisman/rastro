#pragma once
#include <algorithm>
#include <cmath>

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
} // namespace rastro::fx
