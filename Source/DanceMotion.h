#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace rastro
{
// Message-thread choreography. Joint springs model anticipation, weight and recovery.
struct DanceMotion
{
    float phase = 0, footLeft = 0, footRight = 0, armLeft = 0, armRight = 0, shoulder = 0, hip = 0,
          head = 0, pulse = 0, torso = 0, bounce = 0;
    std::array<float, 9> speed{};
    void reset() { *this = {}; }
    void step(float dt, float activity, float note, float playX, float playY, float bpm,
              float hostBeat, float change, int style = 0, float amount = 1, bool reduced = false)
    {
        activity = std::clamp(activity * std::clamp(amount, 0.f, 1.3f), 0.f, 1.f);
        pulse += (change - pulse) * (1 - std::exp(-dt / .09f));
        if (std::abs(pulse - change) < .0001f)
            pulse = change;
        if (activity > .0001f)
        {
            phase += dt * std::clamp(bpm, 30.f, 300.f) / (style == 2 ? 240 : 120);
            if (hostBeat >= 0)
            {
                float target = std::fmod(hostBeat * (style == 2 ? .25f : .5f), 1.f),
                      delta = target - phase;
                delta -= std::floor(delta + .5f);
                phase += delta * .12f;
            }
            phase -= std::floor(phase);
        }
        float a = phase * 6.28318530718f;
        float stride = activity * (.42f + .4f * playY + .18f * note);
        float s = std::sin(a), c = std::cos(a), accent = std::sin(2 * a - .55f);
        std::array<float, 9> target{};
        if (style ==
            1) // Disco: out/in steps, diagonal pointing arms, open chest and head counterturn.
            target = {(s * .7f + std::sin(2 * a) * .3f) * stride,
                      (-s * .7f + std::sin(2 * a) * .3f) * stride,
                      std::sin(a - .3f) * activity,
                      -std::sin(a - .3f) * activity,
                      c * activity,
                      s * activity * .75f,
                      -c * activity * .6f,
                      accent * activity * .45f,
                      -std::abs(s) * activity * .65f};
        else if (style ==
                 2) // Half-time: wide planted weight shifts, deep knee drop, slow rolling arms.
            target = {std::max(0.f, s) * stride * .6f,
                      std::max(0.f, -s) * stride * .6f,
                      std::sin(a - 1.2f) * stride * .7f,
                      std::sin(a - 1.9f) * stride * .7f,
                      -s * activity * .45f,
                      s * activity,
                      std::sin(a - .8f) * activity * .7f,
                      std::sin(a - .6f) * activity * .8f,
                      -(.5f + .5f * c) * activity};
        else // Soul step: alternating heel/toe, relaxed swinging arms, shoulder/backbeat
             // syncopation.
            target = {s * stride,
                      -s * stride,
                      std::sin(a - .65f) * stride,
                      std::sin(a + 2.1f) * stride,
                      std::sin(a + .35f) * activity,
                      s * activity * (.45f + .55f * playX),
                      std::sin(a - .9f) * activity * .5f,
                      std::sin(a + .7f) * activity * .55f,
                      accent * activity * .5f};
        if (reduced)
        {
            // Keep the weighted steps and head nod; remove secondary arm/chest articulation.
            target[2] = target[3] = target[4] = target[7] = 0;
            target[8] *= .3f;
        }
        float *joint[]{&footLeft, &footRight, &armLeft, &armRight, &shoulder,
                       &hip,      &head,      &torso,   &bounce};
        for (int sub = 0; sub < 8; ++sub)
            for (int i = 0; i < 9; ++i)
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
