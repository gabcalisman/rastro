#pragma once
#include "FxLegacyRack.h"
#include <array>
#include <cmath>
#include <vector>

namespace rastro::fx
{
constexpr int count = 15;
constexpr double pi = 3.14159265358979323846;
struct Control
{
    const char *name;
    float low, high, initial;
    const char *unit;
    bool logarithmic = false;
    float physical(float value) const
    {
        value = std::clamp(value, 0.f, 1.f);
        return logarithmic ? low * std::pow(high / low, value) : low + value * (high - low);
    }
};
struct Module
{
    const char *name, *description;
    int controls;
    std::array<Control, 4> knob;
};
inline const std::array<Module, count> modules{
    {{"GHOST",
      "PLAY filtered echoes",
      4,
      {{{"Time", 0, 1, .5f, ""},
        {"Feedback", 0, 1, .35f, ""},
        {"Tone", 0, 1, .6f, ""},
        {"Trail", 0, 1, .5f, ""}}}},
     {"CRUMBLE",
      "PLAY fragment repeater",
      3,
      {{{"Rate", 0, 1, .5f, ""}, {"Variation", 0, 1, .3f, ""}, {"Mutation", 0, 1, .3f, ""}}}},
     {"BLOOM",
      "PLAY diffuse space",
      4,
      {{{"Size", 0, 1, .6f, ""},
        {"Tone", 0, 1, .6f, ""},
        {"Decay", 0, 1, .5f, ""},
        {"Trail", 0, 1, .5f, ""}}}},
     {"WOBBLE",
      "PLAY wandering tape",
      2,
      {{{"Depth", 0, 1, .25f, ""}, {"Rate", .15f, 3, .3f, "Hz"}}}},
     {"DUST",
      "PLAY source-following texture",
      2,
      {{{"Amount", 0, 1, .15f, ""}, {"Character", 0, 1, .5f, ""}}}},
     {"CRUSH", "PLAY sample and bit reduction", 1, {{{"Amount", 0, 1, .3f, ""}}}},
     {"MOUTH",
      "Vowel resonators / formant",
      3,
      {{{"Vowel A E I O U", 0, 4, 0, ""},
        {"Formant shift", .5f, 2, .5f, "x", true},
        {"Resonance", 1, 12, .4f, "Q"}}}},
     {"ORBIT",
      "Through-space flanger",
      4,
      {{{"Rate", .03f, 8, .3f, "Hz", true},
        {"Depth", .1f, 6, .5f, "ms"},
        {"Feedback", -.85f, .85f, .65f, ""},
        {"Centre", .2f, 8, .4f, "ms"}}}},
     {"TRAIL",
      "Clean feedback echo",
      4,
      {{{"Time", 20, 1800, .45f, "ms", true},
        {"Feedback", 0, .9f, .4f, ""},
        {"Tone", 300, 18000, .7f, "Hz", true},
        {"Colour", 0, 1, 0, ""}}}},
     {"CHOIR",
      "Three-voice chorus",
      4,
      {{{"Rate", .05f, 5, .4f, "Hz", true},
        {"Depth", .1f, 10, .4f, "ms"},
        {"Delay", 8, 35, .4f, "ms"},
        {"Spread", 0, 1, .7f, ""}}}},
     {"PRISM",
      "Pitch echo / crystal",
      4,
      {{{"Pitch", -12, 24, 2.f / 3, "st"},
        {"Time", 40, 1500, .45f, "ms", true},
        {"Feedback", 0, .82f, .35f, ""},
        {"Tone", 500, 16000, .75f, "Hz", true}}}},
     {"EMBER",
      "Drive / tube / tape",
      4,
      {{{"Drive", 1, 24, .25f, "x", true},
        {"Tone", 300, 18000, .7f, "Hz", true},
        {"Bias", -.4f, .4f, .5f, ""},
        {"Drive / Tube / Tape", 0, 2, 0, "mode"}}}},
     {"SQUASH",
      "Elastic saturation + compression",
      4,
      {{{"Drive", 1, 12, .2f, "x", true},
        {"Threshold", -36, 0, .6f, "dB"},
        {"Ratio", 1, 16, .3f, ":1"},
        {"Release", 10, 1000, .5f, "ms", true}}}},
     {"SIEVE",
      "Resonant LP / BP / HP",
      3,
      {{{"Cutoff", 30, 18000, .65f, "Hz", true},
        {"Resonance", .5f, 8, .2f, "Q"},
        {"Low / Band / High", 0, 2, 0, "mode"}}}},
     {"SWING",
      "Editable eight-point panner",
      4,
      {{{"Rate", .02f, 10, .4f, "Hz", true},
        {"Depth", 0, 1, 1, ""},
        {"Slew", 1, 250, .25f, "ms", true},
        {"Phase", 0, 1, 0, ""}}}}}};

struct Frame
{
    std::array<std::array<float, 4>, count> value{};
    std::array<float, count> mix{}, enabled{};
    std::array<int, count> order{};
    std::array<float, 8> pan{{0, .2f, .5f, .8f, 1, .8f, .5f, .2f}};
    bool parallel = false;
};

// All rings and filters are owned by the audio thread after prepare().
class Node
{
    std::array<std::vector<float>, 2> ring;
    std::array<std::array<float, 2>, 2> formantState{}, filterState{};
    std::array<float, 2> tone{}, previous{}, dc{}, magnetization{};
    float follower = 0, panPosition = 0;
    double sr = 48000, phase = 0, pitchPhase = 0;
    int write = 0;
    float read(int channel, double delay) const
    {
        const int size = int(ring[channel].size());
        double position = write - std::clamp(delay, 1., double(size - 2));
        if (position < 0)
            position += size;
        int a = int(position);
        return ring[channel][a] +
               float(position - a) * (ring[channel][(a + 1) % size] - ring[channel][a]);
    }
    float lowpass(float input, int channel, float frequency)
    {
        const float coefficient =
            1 - std::exp(float(-2 * pi * std::min(frequency, float(sr * .45)) / sr));
        return tone[channel] += coefficient * (input - tone[channel]);
    }
    // Topology-preserving state-variable filter; stable below Nyquist.
    float filter(float input, std::array<float, 2> &state, float frequency, float q, float type)
    {
        float g = std::tan(float(pi * std::clamp(frequency, 20.f, float(sr * .4)) / sr));
        float k = 1 / std::max(.5f, q), a = 1 / (1 + g * (g + k));
        float v1 = a * (state[0] + g * (input - state[1]));
        float v2 = state[1] + g * v1;
        state[0] = 2 * v1 - state[0];
        state[1] = 2 * v2 - state[1];
        float high = input - k * v1 - v2;
        return type <= 1 ? v2 + (v1 - v2) * type : v1 + (high - v1) * (type - 1);
    }

  public:
    void prepare(double rate)
    {
        sr = rate;
        for (auto &channel : ring)
            channel.assign(size_t(sr * 2.2) + 4, 0);
        reset();
    }
    void reset()
    {
        for (auto &channel : ring)
            std::fill(channel.begin(), channel.end(), 0);
        formantState = {};
        filterState = {};
        tone = {};
        previous = {};
        dc = {};
        magnetization = {};
        follower = panPosition = 0;
        phase = pitchPhase = 0;
        write = 0;
    }
    void process(int id, float &left, float &right, const std::array<float, 4> &normalized,
                 const std::array<float, 8> &pattern, bool mono)
    {
        std::array<float, 4> p{};
        for (int i = 0; i < modules[id].controls; ++i)
            p[i] = modules[id].knob[i].physical(normalized[i]);
        float x[2]{left, right};
        float peak = std::max(std::abs(left), std::abs(right));
        if (id == 12)
        {
            float target = peak * p[0];
            float seconds = target > follower ? .005f : p[3] * .001f;
            follower += (target - follower) * (1 - std::exp(float(-1 / (sr * seconds))));
        }
        if (id == 14)
        {
            double at = std::fmod(phase + p[3], 1.) * 8;
            int a = int(at);
            float target =
                ((pattern[a] + float(at - a) * (pattern[(a + 1) % 8] - pattern[a])) * 2 - 1) * p[1];
            panPosition += (target - panPosition) * (1 - std::exp(float(-1 / (sr * p[2] * .001))));
        }
        for (int c = 0; c < 2; ++c)
        {
            if (id == 6)
            {
                static constexpr float vowels[5][2]{
                    {800, 1150}, {400, 1700}, {300, 2200}, {500, 800}, {350, 650}};
                int a = std::min(3, int(p[0]));
                float t = p[0] - a;
                float sum = 0;
                for (int band = 0; band < 2; ++band)
                {
                    float frequency =
                        (vowels[a][band] + t * (vowels[a + 1][band] - vowels[a][band])) * p[1];
                    // Each band has independent states per channel.
                    auto &state = band == 0 ? formantState[c] : filterState[c];
                    sum += filter(x[c], state, frequency, p[2], 1) / std::sqrt(p[2]);
                }
                x[c] = sum * 1.5f;
            }
            if (id == 7)
            {
                float z = read(c, sr * .001 * (p[3] + p[1] * (1 + std::sin(2 * pi * phase)) * .5));
                ring[c][write] = std::tanh(x[c] + z * p[2]);
                x[c] = z;
            }
            if (id == 8)
            {
                float z = lowpass(read(c, sr * p[0] * .001), c, p[2]);
                float input = x[c] + z * p[1];
                ring[c][write] = input + (std::tanh(input) - input) * p[3];
                x[c] = z;
            }
            if (id == 9)
            {
                ring[c][write] = x[c];
                float sum = 0;
                for (int voice = 0; voice < 3; ++voice)
                    sum +=
                        read(c, sr * .001 *
                                    (p[2] + p[1] * std::sin(2 * pi *
                                                            (phase + voice / 3. * (.5 + .5 * p[3]) +
                                                             (mono ? 0 : c * .23 * p[3]))))) /
                        3;
                x[c] = sum;
            }
            if (id == 10)
            {
                // Dual windowed moving taps pitch-shift the delayed signal in feedback.
                const double window = sr * .06;
                const double delay = sr * p[1] * .001;
                double second = std::fmod(pitchPhase + .5, 1.);
                double weight = .5 - .5 * std::cos(2 * pi * pitchPhase);
                float shifted = float(read(c, delay + pitchPhase * window) * weight +
                                      read(c, delay + second * window) * (1 - weight));
                shifted = lowpass(shifted, c, p[3]);
                ring[c][write] = std::tanh(x[c] + shifted * p[2]);
                x[c] = shifted;
            }
            if (id == 11)
            {
                float driven = (x[c] + p[2]) * p[0], bias = p[2] * p[0];
                magnetization[c] += .12f * (driven - magnetization[c]);
                float modes[]{std::tanh(driven) - std::tanh(bias),
                              (std::tanh(driven) + .2f * std::tanh(driven * driven)) -
                                  (std::tanh(bias) + .2f * std::tanh(bias * bias)),
                              std::atan(driven + .18f * magnetization[c]) * float(2 / pi) -
                                  std::atan(bias * 1.18f) * float(2 / pi)};
                int mode = std::min(1, int(p[3]));
                float shaped = modes[mode] + (modes[mode + 1] - modes[mode]) * (p[3] - mode);
                // DC blocker handles asymmetric tube distortion.
                float high = shaped - previous[c] + .995f * dc[c];
                previous[c] = shaped;
                dc[c] = high;
                x[c] = lowpass(high, c, p[1]);
            }
            if (id == 12)
            {
                float db = 20 * std::log10(std::max(follower, 1e-7f));
                float reduction = std::max(0.f, db - p[1]) * (1 - 1 / p[2]);
                x[c] = std::tanh(x[c] * p[0] * std::pow(10.f, -reduction / 20));
            }
            if (id == 13)
                x[c] = filter(x[c], filterState[c], p[0], p[1], p[2]);
            if (id == 14)
            {
                // Stereo balance; mono is audible amplitude movement, without channel loss.
                float gain = mono ? 1 - .5f * std::abs(panPosition)
                                  : (c == 0 ? 1 - std::max(0.f, panPosition)
                                            : 1 + std::min(0.f, panPosition));
                x[c] *= gain;
            }
        }
        const float rate = id == 7 || id == 9 ? p[0] : id == 14 ? p[0] : 0;
        phase += rate / sr;
        phase -= std::floor(phase);
        if (id == 10)
        {
            pitchPhase += (1 - std::pow(2., p[0] / 12)) / (sr * .06);
            pitchPhase -= std::floor(pitchPhase);
        }
        write = (write + 1) % int(ring[0].size());
        left = x[0];
        right = x[1];
    }
};

class Rack
{
    std::array<FxLegacyRack, 6> inherited;
    std::array<Node, 9> nodes;
    std::array<Settings, 6> legacySettings;

  public:
    void prepare(double rate)
    {
        for (int i = 0; i < 6; ++i)
        {
            inherited[i].selected = i;
            inherited[i].prepare(rate);
            auto &p = legacySettings[i];
            p.enabled.fill(false);
            p.enabled[i] = true;
            p.ghost[3] = p.crumble[2] = p.bloom[3] = p.wobble[2] = p.dust[2] = p.crush[1] = 1;
        }
        for (auto &node : nodes)
            node.prepare(rate);
    }
    void reset()
    {
        for (auto &node : nodes)
            node.reset();
        for (auto &node : inherited)
            node.reset();
    }
    void wet(int id, float &l, float &r, const Frame &frame, bool mono)
    {
        const auto &v = frame.value[id];
        if (id >= 6)
        {
            nodes[id - 6].process(id, l, r, v, frame.pan, mono);
            return;
        }
        auto &p = legacySettings[id];
        if (id == 0)
        {
            p.ghost[0] = v[0];
            p.ghost[1] = v[1];
            p.ghost[2] = v[2];
            p.trail = v[3];
        }
        if (id == 1)
        {
            p.crumble[0] = v[0];
            p.crumble[1] = v[1];
            p.mutation = v[2];
        }
        if (id == 2)
        {
            p.bloom[0] = v[0];
            p.bloom[1] = v[1];
            p.bloom[2] = v[2];
            p.trail = v[3];
        }
        if (id == 3)
        {
            p.wobble[0] = v[0];
            p.wobble[1] = v[1];
        }
        if (id == 4)
        {
            p.dust[0] = v[0];
            p.dust[1] = v[1];
        }
        if (id == 5)
            p.crush[0] = v[0];
        inherited[id].process(l, r, p);
    }
    void process(float &l, float &r, const Frame &frame, bool mono)
    {
        const float dryL = l, dryR = r;
        float sumL = 0, sumR = 0, branches = 0;
        for (int id : frame.order)
        {
            float a = frame.parallel ? dryL : l, b = frame.parallel ? dryR : r;
            const float beforeL = a, beforeR = b;
            // Bypassed nodes keep their clocks/tails alive, with no contribution to output.
            wet(id, a, b, frame, mono);
            float mix = frame.mix[id] * frame.enabled[id];
            if (frame.parallel)
            {
                sumL += (a - beforeL) * mix;
                sumR += (b - beforeR) * mix;
                branches += frame.enabled[id];
            }
            else if (mix != 0)
            {
                l += (a - l) * mix;
                r += (b - r) * mix;
            }
        }
        if (frame.parallel)
        {
            l = dryL + sumL / std::max(1.f, branches);
            r = dryR + sumR / std::max(1.f, branches);
        }
    }
};
} // namespace rastro::fx
