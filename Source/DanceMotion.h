#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace rastro
{
// Message-thread rig, in normalized joint units. No timer, random generator or audio dependency.
struct DanceMotion
{
    float phase = 0, footLeft = 0, footRight = 0, armLeft = 0, armRight = 0, shoulder = 0, hip = 0,
          head = 0, pulse = 0;
    std::array<float, 8> speed{};
    void reset() { *this = {}; }
    void step(float dt, float activity, float note, float playX, float playY, float bpm,
              float hostBeat, float change)
    {
        activity = std::clamp(activity, 0.f, 1.f);
        pulse += (change - pulse) * (1 - std::exp(-dt / .09f));
        if (std::abs(pulse - change) < .0001f)
            pulse = change;
        if (activity > .0001f)
        {
            phase += dt * std::clamp(bpm, 30.f, 300.f) / 120;
            if (hostBeat >= 0)
            {
                float target = std::fmod(hostBeat * .5f, 1.f), delta = target - phase;
                delta -= std::floor(delta + .5f);
                phase += delta * .12f;
            }
            phase -= std::floor(phase);
        }
        float a = phase * 6.28318530718f;
        float stride = activity * (.35f + .45f * playY + .2f * note);
        // Alternating foot placement, counter-rotating shoulders, delayed arms/head.
        std::array<float, 8> target{std::sin(a) * stride,
                                    -std::sin(a) * stride,
                                    std::sin(a - .65f) * stride,
                                    std::sin(a + 2.1f) * stride,
                                    std::sin(a + .35f) * activity,
                                    std::sin(a) * activity * (.45f + .55f * playX),
                                    std::sin(a - .9f) * activity * (.25f + .3f * playY),
                                    pulse * activity};
        float *joint[]{&footLeft, &footRight, &armLeft, &armRight, &shoulder, &hip, &head, &pulse};
        for (int sub = 0; sub < 8; ++sub)
            for (int i = 0; i < 7; ++i)
            {
                float h = dt / 8;
                speed[i] += (target[i] - *joint[i]) * 105 * h - speed[i] * 17 * h;
                *joint[i] = std::clamp(*joint[i] + speed[i] * h, -1.f, 1.f);
                if (std::abs(*joint[i] - target[i]) < .0001f && std::abs(speed[i]) < .0001f)
                {
                    *joint[i] = target[i];
                    speed[i] = 0;
                }
            }
    }
};
} // namespace rastro
