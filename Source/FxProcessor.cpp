#include "FxProcessor.h"
#include "FxEditor.h"
using namespace rastro::fx;
namespace
{
const char *factoryNames[]{"INIT / Clear wire", "Ghost postcard", "Crumbling bloom",
                           "Dusty cassette",    "Talking moon",   "Orbital choir",
                           "Crystal staircase", "Velvet furnace", "Rubber postcard",
                           "Swing machine"};
juce::StringArray targetNames()
{
    juce::StringArray names{"None", "Input Gain", "Output Gain", "Global Dry/Wet"};
    for (const auto &m : modules)
    {
        for (int k = 0; k < m.controls; ++k)
            names.add(juce::String(m.name) + " / " + m.knob[k].name);
        names.add(juce::String(m.name) + " / Mix");
    }
    for (int k = 0; k < 8; ++k)
        names.add("SWING / Point " + juce::String(k + 1));
    return names;
}
const char *modIds[]{"fxXTarget",        "fxYTarget",  "fxXDepth",    "fxYDepth",
                     "fxLfoTarget",      "fxLfoDepth", "fxLfoRate",   "fxLfoShape",
                     "fxEnvTarget",      "fxEnvDepth", "fxEnvAttack", "fxEnvRelease",
                     "fxEnvSensitivity", "fxX",        "fxY",         "fxRouting"};
} // namespace
juce::String RastroFxProcessor::moduleId(int module, const juce::String &suffix)
{
    return "fxM" + juce::String(module) + suffix;
}
juce::AudioProcessorValueTreeState::ParameterLayout RastroFxProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    auto add = [&](juce::String id, juce::String name, float low, float high, float initial,
                   juce::String unit = "")
    {
        result.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>{low, high}, initial,
            juce::AudioParameterFloatAttributes()
                .withLabel(unit)
                .withStringFromValueFunction(
                    [unit](float v, int)
                    { return juce::String(unit == "%" ? v * 100 : v, unit == "dB" ? 1 : 2); })
                .withValueFromStringFunction(
                    [unit](const juce::String &text)
                    { return text.getFloatValue() / (unit == "%" ? 100.f : 1.f); })));
    };
    // Original FX parameter identities and ordering are retained.
    add("fxInputGain", "Input Gain", -24, 24, 0, "dB");
    add("fxOutputGain", "Output Gain", -24, 24, 0, "dB");
    result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"fxBypass", 1},
                                                          "Bypass", false));
    add("fxDryWet", "Global Dry/Wet", 0, 1, 1, "%");
    result.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"fxRouting", 1}, "Routing", juce::StringArray{"Series", "Parallel"}, 0));
    for (int i = 0; i < count; ++i)
    {
        juce::String name = modules[i].name;
        result.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{moduleId(i, "On"), 1}, name + " Enabled", false));
        add(moduleId(i, "Mix"), name + " Mix", 0, 1, .5f, "%");
        result.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{moduleId(i, "Position"), 1}, name + " Position", 0, count - 1, i));
        for (int k = 0; k < modules[i].controls; ++k)
        {
            const auto control = modules[i].knob[k];
            auto attributes =
                juce::AudioParameterFloatAttributes()
                    .withLabel(control.unit)
                    .withStringFromValueFunction(
                        [control](float v, int)
                        {
                            if (juce::String(control.unit) == "mode")
                            {
                                int mode = juce::jlimit(0, 2, int(std::round(v * 2)));
                                return juce::String(juce::String(control.name) ==
                                                            "Drive / Tube / Tape"
                                                        ? (mode == 0   ? "DRIVE"
                                                           : mode == 1 ? "TUBE"
                                                                       : "TAPE")
                                                        : (mode == 0   ? "LP"
                                                           : mode == 1 ? "BP"
                                                                       : "HP"));
                            }
                            return juce::String(control.physical(v), 2);
                        })
                    .withValueFromStringFunction(
                        [control](const juce::String &text)
                        {
                            float physical = text.getFloatValue();
                            if (juce::String(control.unit) == "mode")
                            {
                                if (text.containsIgnoreCase("tube") || text.equalsIgnoreCase("BP"))
                                    return .5f;
                                if (text.containsIgnoreCase("tape") || text.equalsIgnoreCase("HP"))
                                    return 1.f;
                                if (text.containsIgnoreCase("drive") || text.equalsIgnoreCase("LP"))
                                    return 0.f;
                            }
                            physical = juce::jlimit(control.low, control.high, physical);
                            return control.logarithmic
                                       ? std::log(physical / control.low) /
                                             std::log(control.high / control.low)
                                       : (physical - control.low) / (control.high - control.low);
                        });
            result.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{moduleId(i, "P" + juce::String(k)), 1}, name + " " + control.name,
                juce::NormalisableRange<float>{0, 1}, control.initial, attributes));
        }
    }
    for (int i = 0; i < 8; ++i)
        add("fxPan" + juce::String(i), "SWING Point " + juce::String(i + 1), 0, 1, Frame{}.pan[i]);
    for (auto prefix :
         {juce::String("X"), juce::String("Y"), juce::String("Lfo"), juce::String("Env")})
    {
        result.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fx" + prefix + "Target", 1}, prefix + " Target", targetNames(), 0));
        add("fx" + prefix + "Depth", prefix + " Amount", -1, 1,
            prefix == "X" || prefix == "Y" ? .5f : 0);
    }
    add("fxX", "Pad X", 0, 1, .5f);
    add("fxY", "Pad Y", 0, 1, .5f);
    add("fxLfoRate", "LFO Rate", .05f, 20, 1, "Hz");
    result.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"fxLfoShape", 1}, "LFO Shape",
        juce::StringArray{"Sine", "Triangle", "Saw", "Square"}, 0));
    add("fxEnvAttack", "Follower Attack", 1, 200, 10, "ms");
    add("fxEnvRelease", "Follower Release", 10, 2000, 200, "ms");
    add("fxEnvSensitivity", "Follower Sensitivity", -24, 24, 0, "dB");
    // Appended after existing parameters to preserve every legacy host parameter index.
    for (int i = 0; i < count; ++i)
        result.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{moduleId(i, "InRack"), 1},
            juce::String(modules[i].name) + " Inserted", false));
    return result;
}
RastroFxProcessor::RastroFxProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "RASTRO_FX_STATE", layout())
{
    registerTargets();
    for (int i = 0; i < count; ++i)
    {
        on[i] = params.getRawParameterValue(moduleId(i, "On"));
        inRack[i] = params.getRawParameterValue(moduleId(i, "InRack"));
        position[i] = params.getRawParameterValue(moduleId(i, "Position"));
    }
    for (int i = 0; i < 16; ++i)
        mod[i] = params.getRawParameterValue(modIds[i]);
    bypass = params.getRawParameterValue("fxBypass");
    routing = params.getRawParameterValue("fxRouting");
    setPresetName("INIT / Clear wire");
    banks[0] = snapshot();
    banks[1] = snapshot();
}
void RastroFxProcessor::registerTargets()
{
    auto add = [&](const juce::String &id, const juce::String &name, float low = 0, float high = 1)
    {
        int index = int(targets.size());
        targets.push_back({id, name, params.getRawParameterValue(id), low, high});
        return index;
    };
    add("fxInputGain", "Input Gain", -24, 24);
    add("fxOutputGain", "Output Gain", -24, 24);
    add("fxDryWet", "Global Dry/Wet");
    for (int i = 0; i < count; ++i)
    {
        knobIndex[i].fill(-1);
        for (int k = 0; k < modules[i].controls; ++k)
            knobIndex[i][k] = add(moduleId(i, "P" + juce::String(k)),
                                  juce::String(modules[i].name) + " / " + modules[i].knob[k].name);
        mixIndex[i] = add(moduleId(i, "Mix"), juce::String(modules[i].name) + " / Mix");
    }
    for (int i = 0; i < 8; ++i)
        panIndex[i] = add("fxPan" + juce::String(i), "SWING / Point " + juce::String(i + 1));
    jassert(targets.size() <= maxTargets);
}
bool RastroFxProcessor::isBusesLayoutSupported(const BusesLayout &b) const
{
    return b.inputBuses.size() == 1 && b.outputBuses.size() == 1 &&
           b.getMainInputChannelSet() == b.getMainOutputChannelSet() &&
           (b.getMainInputChannelSet() == juce::AudioChannelSet::mono() ||
            b.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}
void RastroFxProcessor::prepareToPlay(double sampleRate, int)
{
    rate = sampleRate;
    rack.prepare(rate);
    for (size_t i = 0; i < targets.size(); ++i)
        smooth[i].reset(rate, i < 2 ? .02 : .01);
    for (auto &s : enables)
        s.reset(rate, .01);
    for (auto &s : xy)
        s.reset(rate, .01);
    activeMix.reset(rate, .01);
    reset();
}
void RastroFxProcessor::reset()
{
    rack.reset();
    for (size_t i = 0; i < targets.size(); ++i)
        smooth[i].setCurrentAndTargetValue((targets[i].raw->load() - targets[i].low) /
                                           (targets[i].high - targets[i].low));
    for (int i = 0; i < count; ++i)
        enables[i].setCurrentAndTargetValue(on[i]->load() * inRack[i]->load());
    for (int i = 0; i < 2; ++i)
        xy[i].setCurrentAndTargetValue(mod[13 + i]->load());
    activeMix.setCurrentAndTargetValue(bypass->load() >= .5f ? 0. : 1.);
    activeOrder = order();
    parallel = routing->load() >= .5f;
    topology = 1;
    envelope = lfoSmooth = inputHeld = outputHeld = 0;
    lfoPhase = 0;
    modulationOffset = {};
}
std::array<int, count> RastroFxProcessor::order() const
{
    std::array<int, count> result{}, ranks{};
    for (int i = 0; i < count; ++i)
    {
        result[i] = i;
        ranks[i] = int(position[i]->load());
    }
    std::sort(result.begin(), result.end(),
              [&](int a, int b) { return ranks[a] == ranks[b] ? a < b : ranks[a] < ranks[b]; });
    return result;
}
template <typename Sample>
void RastroFxProcessor::process(juce::AudioBuffer<Sample> &buffer, bool hostBypass)
{
    for (size_t i = 0; i < targets.size(); ++i)
        smooth[i].setTargetValue((targets[i].raw->load() - targets[i].low) /
                                 (targets[i].high - targets[i].low));
    for (int i = 0; i < count; ++i)
        enables[i].setTargetValue(on[i]->load() * inRack[i]->load());
    for (int i = 0; i < 2; ++i)
        xy[i].setTargetValue(mod[13 + i]->load());
    activeMix.setTargetValue(hostBypass || bypass->load() >= .5f ? 0. : 1.);
    const auto desired = order();
    const bool desiredParallel = routing->load() >= .5f;
    float config[13];
    for (int i = 0; i < 13; ++i)
        config[i] = mod[i]->load();
    const float envGain = std::pow(10.f, config[12] / 20);
    const float attack = 1 - std::exp(float(-1 / (rate * config[10] * .001)));
    const float release = 1 - std::exp(float(-1 / (rate * config[11] * .001)));
    const float modulationSlew = 1 - std::exp(float(-1 / (rate * .005)));
    const bool mono = buffer.getNumChannels() == 1;
    float inMeter = 0, outMeter = 0;
    std::array<float, count> peaks{};
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const Sample rawL = buffer.getSample(0, sample),
                     rawR = mono ? rawL : buffer.getSample(1, sample);
        float sourcePeak = float(std::max(std::abs(rawL), std::abs(rawR)));
        if (!std::isfinite(sourcePeak))
            sourcePeak = 0;
        inMeter = std::max(inMeter, sourcePeak);
        float envTarget = std::min(1.f, sourcePeak * envGain);
        envelope += (envTarget - envelope) * (envTarget > envelope ? attack : release);
        double wave = std::sin(2 * pi * lfoPhase);
        if (int(config[7]) == 1)
            wave = 1 - 4 * std::abs(lfoPhase - .5);
        if (int(config[7]) == 2)
            wave = 2 * lfoPhase - 1;
        if (int(config[7]) == 3)
            wave = lfoPhase < .5 ? 1 : -1;
        lfoSmooth += (float(wave) - lfoSmooth) * modulationSlew;
        lfoPhase += config[6] / rate;
        lfoPhase -= std::floor(lfoPhase);
        std::array<double, maxTargets> values{}, offset{};
        for (size_t i = 0; i < targets.size(); ++i)
            values[i] = smooth[i].getNextValue();
        auto apply = [&](float selected, float amount, float source)
        {
            int index = int(selected) - 1;
            if (index >= 0 && index < int(targets.size()))
                offset[index] += amount * source;
        };
        apply(config[0], config[2], xy[0].getNextValue() * 2 - 1);
        apply(config[1], config[3], xy[1].getNextValue() * 2 - 1);
        apply(config[4], config[5], lfoSmooth);
        apply(config[8], config[9], envelope);
        for (size_t i = 0; i < targets.size(); ++i)
        {
            modulationOffset[i] += (offset[i] - modulationOffset[i]) * modulationSlew;
            if (std::abs(modulationOffset[i]) < 1e-10)
                modulationOffset[i] = 0;
            values[i] = std::clamp(values[i] + modulationOffset[i], 0., 1.);
        }
        bool any = false;
        for (int m = 0; m < count; ++m)
        {
            frame.enabled[m] = enables[m].getNextValue();
            frame.mix[m] = float(values[mixIndex[m]]);
            any |= frame.enabled[m] > 0 && frame.mix[m] > 0;
            for (int k = 0; k < modules[m].controls; ++k)
                frame.value[m][k] = float(values[knobIndex[m][k]]);
        }
        for (int i = 0; i < 8; ++i)
            frame.pan[i] = float(values[panIndex[i]]);
        if (desired != activeOrder || desiredParallel != parallel)
        {
            topology = std::max(0.f, topology - float(1 / (rate * .01)));
            if (topology == 0)
            {
                activeOrder = desired;
                parallel = desiredParallel;
            }
        }
        else
            topology = std::min(1.f, topology + float(1 / (rate * .01)));
        frame.order = activeOrder;
        frame.parallel = parallel;
        const double input = std::pow(10., (-24 + 48 * values[0]) / 20);
        const double output = std::pow(10., (-24 + 48 * values[1]) / 20);
        const double bypassMix = activeMix.getNextValue();
        const double wet = values[2] * topology;
        float l = float(rawL * input), r = float(rawR * input);
        const float dryL = l, dryR = r;
        rack.process(l, r, frame, mono);
        const double deltaL = any && std::isfinite(l) ? double(l - dryL) * wet : 0.;
        const double deltaR = any && std::isfinite(r) ? double(r - dryR) * wet : 0.;
        // Delta form avoids converting the dry signal through float and preserves signed zero.
        const double gain = 1 + bypassMix * (input * output - 1);
        const double effectL = bypassMix * output * deltaL, effectR = bypassMix * output * deltaR;
        if (gain != 1 || effectL != 0)
            buffer.setSample(0, sample, Sample(rawL * gain + effectL));
        if (!mono && (gain != 1 || effectR != 0))
            buffer.setSample(1, sample, Sample(rawR * gain + effectR));
        outMeter =
            std::max(outMeter, float(std::max(std::abs(buffer.getSample(0, sample)),
                                              std::abs(buffer.getSample(mono ? 0 : 1, sample)))));
        for (int m = 0; m < count; ++m)
            peaks[m] = std::max(peaks[m], frame.enabled[m] * frame.mix[m]);
    }
    const float releaseMeter = std::exp(float(-buffer.getNumSamples() / (rate * .25)));
    inputHeld = std::max(inMeter, inputHeld * releaseMeter);
    outputHeld = std::max(outMeter, outputHeld * releaseMeter);
    inputPeak.store(inputHeld, std::memory_order_relaxed);
    outputPeak.store(outputHeld, std::memory_order_relaxed);
    lfoMeter.store(lfoSmooth, std::memory_order_relaxed);
    envelopeMeter.store(envelope, std::memory_order_relaxed);
    for (int m = 0; m < count; ++m)
        activity[m].store(peaks[m], std::memory_order_relaxed);
}
void RastroFxProcessor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &)
{
    process(b, false);
}
void RastroFxProcessor::processBlock(juce::AudioBuffer<double> &b, juce::MidiBuffer &)
{
    process(b, false);
}
void RastroFxProcessor::processBlockBypassed(juce::AudioBuffer<float> &b, juce::MidiBuffer &)
{
    process(b, true);
}
void RastroFxProcessor::processBlockBypassed(juce::AudioBuffer<double> &b, juce::MidiBuffer &)
{
    process(b, true);
}
juce::AudioProcessorParameter *RastroFxProcessor::getBypassParameter() const
{
    return params.getParameter("fxBypass");
}
juce::AudioProcessorEditor *RastroFxProcessor::createEditor() { return new RastroFxEditor(*this); }
void RastroFxProcessor::setValue(const juce::String &id, float v)
{
    if (auto *p = params.getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}
float RastroFxProcessor::value(const juce::String &id) const
{
    if (auto *p = params.getRawParameterValue(id))
        return p->load();
    return 0;
}
void RastroFxProcessor::moveModule(int module, int delta)
{
    auto sorted = order();
    auto it = std::find(sorted.begin(), sorted.end(), module);
    int from = int(it - sorted.begin()), to = juce::jlimit(0, count - 1, from + delta);
    if (from < to)
        std::rotate(sorted.begin() + from, sorted.begin() + from + 1, sorted.begin() + to + 1);
    else if (to < from)
        std::rotate(sorted.begin() + to, sorted.begin() + from, sorted.begin() + from + 1);
    for (int i = 0; i < count; ++i)
        setValue(moduleId(sorted[i], "Position"), float(i));
}
juce::ValueTree RastroFxProcessor::snapshot()
{
    const juce::ScopedLock lock(stateLock);
    auto state = params.copyState();
    state.removeChild(state.getChildWithName("FX_AB"), nullptr);
    return state;
}
void RastroFxProcessor::compare()
{
    const juce::ScopedLock lock(stateLock);
    banks[currentBank] = snapshot();
    currentBank = 1 - currentBank;
    if (banks[currentBank].isValid())
        params.replaceState(banks[currentBank].createCopy());
    currentProgram =
        juce::jlimit(0, getNumPrograms() - 1, int(params.state.getProperty("factoryProgram", 0)));
}
void RastroFxProcessor::copyCompare()
{
    const juce::ScopedLock lock(stateLock);
    banks[1 - currentBank] = snapshot();
}
int RastroFxProcessor::getNumPrograms() { return int(std::size(factoryNames)); }
const juce::String RastroFxProcessor::getProgramName(int i)
{
    return factoryNames[juce::jlimit(0, getNumPrograms() - 1, i)];
}
void RastroFxProcessor::loadFactory(int index)
{
    const juce::ScopedLock lock(stateLock);
    index = juce::jlimit(0, getNumPrograms() - 1, index);
    currentProgram = index;
    for (auto *base : getParameters())
        if (auto *p = dynamic_cast<juce::RangedAudioParameter *>(base))
            p->setValueNotifyingHost(p->getDefaultValue());
    auto enable = [&](int m, float mix)
    {
        setValue(moduleId(m, "InRack"), 1);
        setValue(moduleId(m, "On"), 1);
        setValue(moduleId(m, "Mix"), mix);
    };
    if (index == 1)
    {
        enable(0, .4f);
        enable(2, .25f);
    }
    if (index == 2)
    {
        enable(1, .45f);
        enable(2, .4f);
    }
    if (index == 3)
    {
        enable(3, .5f);
        enable(4, .6f);
        enable(5, .3f);
        enable(11, .3f);
        setValue(moduleId(11, "P3"), 1);
    }
    if (index == 4)
    {
        enable(6, .8f);
        setValue("fxLfoTarget", float(knobIndex[6][0] + 1));
        setValue("fxLfoDepth", .3f);
        setValue("fxLfoRate", .4f);
    }
    if (index == 5)
    {
        enable(7, .35f);
        enable(9, .5f);
        setValue("fxRouting", 1);
    }
    if (index == 6)
    {
        enable(10, .5f);
        enable(2, .3f);
    }
    if (index == 7)
    {
        enable(11, .7f);
        setValue(moduleId(11, "P3"), .5f);
        enable(13, .3f);
    }
    if (index == 8)
    {
        enable(12, .6f);
        enable(8, .3f);
    }
    if (index == 9)
    {
        enable(14, 1);
        enable(9, .3f);
    }
    params.state.setProperty("factoryProgram", index, nullptr);
    setPresetName(factoryNames[index]);
    setAppearance("accent", index % 5);
    setAppearance("skin", index % 3);
}
juce::String RastroFxProcessor::presetName() const
{
    const juce::ScopedLock lock(stateLock);
    return params.state.getProperty("presetName", "Untitled orbit").toString();
}
void RastroFxProcessor::setPresetName(const juce::String &name)
{
    const juce::ScopedLock lock(stateLock);
    params.state.setProperty("presetName", name.substring(0, 80), nullptr);
}
int RastroFxProcessor::appearance(const char *key, int fallback) const
{
    const juce::ScopedLock lock(stateLock);
    return juce::jlimit(0, juce::String(key) == "skin" ? 2 : 4,
                        int(params.state.getProperty(key, fallback)));
}
void RastroFxProcessor::setAppearance(const char *key, int v)
{
    const juce::ScopedLock lock(stateLock);
    params.state.setProperty(key, juce::jlimit(0, juce::String(key) == "skin" ? 2 : 4, v), nullptr);
}
void RastroFxProcessor::getStateInformation(juce::MemoryBlock &out)
{
    const juce::ScopedLock lock(stateLock);
    banks[currentBank] = snapshot();
    auto state = snapshot();
    juce::ValueTree ab("FX_AB");
    ab.setProperty("selected", currentBank.load(), nullptr);
    for (int i = 0; i < 2; ++i)
    {
        juce::ValueTree slot("SLOT");
        slot.addChild(banks[i].createCopy(), -1, nullptr);
        ab.addChild(slot, -1, nullptr);
    }
    state.addChild(ab, -1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, out);
}
void RastroFxProcessor::setStateInformation(const void *data, int size)
{
    const juce::ScopedLock lock(stateLock);
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName("RASTRO_FX_STATE"))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        auto ab = state.getChildWithName("FX_AB");
        currentBank = juce::jlimit(0, 1, int(ab.getProperty("selected", 0)));
        for (int i = 0; i < 2; ++i)
            banks[i] = ab.getChild(i).getChild(0).createCopy();
        state.removeChild(ab, nullptr);
        auto completeState = [&](juce::ValueTree tree)
        {
            if (!tree.isValid())
                return tree;
            // Older FX states treated ON as both rack membership and bypass. Preserve their sound.
            for (int i = 0; i < count; ++i)
            {
                const auto insertId = moduleId(i, "InRack");
                bool foundInsert = false;
                float oldOn = 0.f;
                for (auto child : tree)
                {
                    auto id = child.getProperty("id").toString();
                    if (id == insertId)
                        foundInsert = true;
                    else if (id == moduleId(i, "On"))
                        oldOn = float(child.getProperty("value", 0));
                }
                if (!foundInsert)
                {
                    juce::ValueTree inserted("PARAM");
                    inserted.setProperty("id", insertId, nullptr);
                    inserted.setProperty("value", oldOn, nullptr);
                    tree.addChild(inserted, -1, nullptr);
                }
            }
            for (auto *base : getParameters())
                if (auto *parameter = dynamic_cast<juce::RangedAudioParameter *>(base))
                {
                    bool found = false;
                    for (auto child : tree)
                        found |= child.getProperty("id").toString() == parameter->paramID;
                    if (!found)
                    {
                        juce::ValueTree child("PARAM");
                        child.setProperty("id", parameter->paramID, nullptr);
                        child.setProperty("value",
                                          parameter->convertFrom0to1(parameter->getDefaultValue()), nullptr);
                        tree.addChild(child, -1, nullptr);
                    }
                }
            return tree;
        };
        state = completeState(state);
        for (auto &bank : banks)
            bank = completeState(bank);
        currentProgram =
            juce::jlimit(0, getNumPrograms() - 1, int(state.getProperty("factoryProgram", 0)));
        params.replaceState(state);
        for (auto &bank : banks)
            if (!bank.isValid())
                bank = snapshot();
    }
}
bool RastroFxProcessor::savePreset(const juce::File &file)
{
    juce::MemoryBlock data;
    getStateInformation(data);
    juce::TemporaryFile temporary(file);
    return temporary.getFile().replaceWithData(data.getData(), data.getSize()) &&
           temporary.overwriteTargetFileWithTemporary();
}
bool RastroFxProcessor::loadPreset(const juce::File &file)
{
    if (file.getSize() > 4 * 1024 * 1024 || file.getSize() <= 0)
        return false;
    juce::MemoryBlock data;
    if (!file.loadFileAsData(data))
        return false;
    auto xml = getXmlFromBinary(data.getData(), int(data.getSize()));
    if (!xml || !xml->hasTagName("RASTRO_FX_STATE"))
        return false;
    setStateInformation(data.getData(), int(data.getSize()));
    return true;
}
