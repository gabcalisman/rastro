#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace rastro
{
// Original DSP. Storage is prepared once; these functions own no changing audio-thread objects.
class CharacterEffects
{
    std::array<std::vector<float>, 2> tape;
    std::array<float, 2> noiseLow{}, noiseDC{}, noisePrevious{}, crackles{}, held{}, crushOutput{};
    double sr = 44100, phase = 0, flutter = 0, crushPhase = 0;
    float follower = 0, attack = .01f, release = .001f, slew = .2f;
    int write = 0;
    std::uint32_t seed = 0x52415354;
    float noise()
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return float(seed >> 8) * (2.f / 16777215.f) - 1.f;
    }

  public:
    float movement = 0, dustActivity = 0, crushActivity = 0, envelope = 0;
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto &c : tape)
            c.assign((size_t)(sr * .04) + 4, 0);
        attack = 1 - std::exp(-1.f / float(sr * .002));
        release = 1 - std::exp(-1.f / float(sr * .03));
        slew = 1 - std::exp(-1.f / float(sr * .00008));
        reset();
    }
    void reset()
    {
        for (auto &c : tape)
            std::fill(c.begin(), c.end(), 0);
        noiseLow = {};
        noiseDC = {};
        noisePrevious = {};
        crackles = {};
        held = {};
        crushOutput = {};
        phase = flutter = crushPhase = 0;
        write = 0;
        follower = movement = dustActivity = crushActivity = envelope = 0;
        seed = 0x52415354;
    }
    void advance(float sourcePeak, float rate)
    {
        follower += (sourcePeak - follower) * (sourcePeak > follower ? attack : release);
        if (follower < 1e-7f)
            follower = 0;
        envelope = follower;
        phase += (.15 + rate * 2.85) / sr;
        phase -= std::floor(phase);
        flutter += 17.3 / sr;
        flutter -= std::floor(flutter);
        movement =
            float(.84 * std::sin(phase * 6.283185307) + .16 * std::sin(flutter * 6.283185307));
        dustActivity = crushActivity = 0;
    }
    float wobble(float input, int c, float depth)
    {
        const int n = (int)tape[c].size();
        tape[c][write] = input;
        float delay = float(sr) * (.008f + .005f * depth * movement);
        float read = write - delay;
        if (read < 0)
            read += n;
        if (read >= n)
            read = 0;
        int a = (int)read;
        float wet = tape[c][a] + (read - a) * (tape[c][(a + 1) % n] - tape[c][a]);
        return input + (wet - input) * std::min(1.f, depth * 5.f);
    }
    float dust(float input, int c, float level, float character)
    {
        float white = noise();
        noiseLow[c] += .06f * (white - noiseLow[c]);
        crackles[c] *= .82f;
        if (std::abs(white) > .9988f)
            crackles[c] += white * .7f;
        float texture =
            (white - noiseLow[c]) * (1 - character) + (noiseLow[c] * 2 + crackles[c]) * character;
        float hp = texture - noisePrevious[c] + .995f * noiseDC[c];
        noisePrevious[c] = texture;
        noiseDC[c] = hp;
        float added = std::clamp(hp, -1.f, 1.f) * level * std::min(.6f, follower) * .45f;
        dustActivity = std::max(dustActivity, std::abs(added));
        return input + added;
    }
    void beginCrush(float amount) { crushPhase += 1. / (1 + amount * amount * 31); }
    float crush(float input, int c, float amount)
    {
        if (crushPhase >= 1)
        {
            float steps = std::exp2(15 - amount * 11);
            held[c] = std::round(input * steps) / steps;
        }
        crushOutput[c] += (held[c] - crushOutput[c]) * slew;
        float wet = input + (crushOutput[c] - input) * std::min(1.f, amount * 8.f);
        crushActivity = std::max(crushActivity, std::abs(wet - input));
        return wet;
    }
    void finish()
    {
        write = (write + 1) % (int)tape[0].size();
        if (crushPhase >= 1)
            crushPhase -= std::floor(crushPhase);
    }
};
} // namespace rastro
