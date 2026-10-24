#pragma once
#include "EffectRack.h"
#include "FxEditor.h"
#include <fstream>

// Allocation guard is enabled only around the actual callback; setup/UI/preset I/O are excluded.
thread_local bool fxInCallback = false;
thread_local size_t fxAllocations = 0;
void *operator new(std::size_t n)
{
    if (fxInCallback)
        ++fxAllocations;
    if (auto *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

static rastro::fx::Frame defaultFxFrame()
{
    rastro::fx::Frame f;
    for (int i = 0; i < rastro::fx::count; ++i)
    {
        f.order[i] = i;
        f.mix[i] = 1;
        for (int k = 0; k < rastro::fx::modules[i].controls; ++k)
            f.value[i][k] = rastro::fx::modules[i].knob[k].initial;
    }
    return f;
}
static float fixture(int i, int channel = 0)
{
    return float((.23 * std::sin(i * .017 + channel * .2) + .15 * std::sin(i * .183) +
                  .11 * std::sin(i * .723)) *
                 (i % 5000 < 2500 ? 1 : .15));
}
static void inheritedFxTests()
{
    for (double rate : {22050., 48000.})
        for (int id = 0; id < 6; ++id)
        {
            rastro::EffectRack original;
            rastro::fx::Rack fx;
            original.prepare(rate);
            fx.prepare(rate);
            auto frame = defaultFxFrame();
            rastro::Settings settings;
            settings.enabled.fill(false);
            settings.enabled[id] = true;
            settings.ghost[3] = settings.crumble[2] = settings.bloom[3] = settings.wobble[2] =
                settings.dust[2] = settings.crush[1] = 1;
            settings.level = 1;
            settings.presence = .4f;
            double error = 0;
            for (int i = 0; i < int(rate * .7); ++i)
            {
                float l = fixture(i), r = fixture(i, 1), a = l, b = r;
                original.process(l, r, settings);
                fx.wet(id, a, b, frame, false);
                error = std::max(error,
                                 double(std::abs(l - std::tanh(a)) + std::abs(r - std::tanh(b))));
            }
            if (error > 2e-6)
                std::cerr << "legacy id " << id << " error " << error << '\n';
            check(error < 2e-6,
                  "inherited wet algorithms match PLAY before its global output saturator");
        }
    std::cout << "PASS FX inherited: six algorithms compared sample-by-sample with unchanged PLAY "
                 "at two rates\n";
}
static void inheritedControlTests()
{
    rastro::fx::Rack rack;
    rack.prepare(48000);
    auto render = [&](int id, int control, float amount)
    {
        rack.reset();
        auto f = defaultFxFrame();
        if (control >= 0)
            f.value[id][control] = amount;
        std::vector<float> out(42000);
        for (int i = 0; i < int(out.size()); ++i)
        {
            float l = fixture(i), r = fixture(i, 1);
            rack.wet(id, l, r, f, false);
            out[i] = l;
        }
        return out;
    };
    for (int id = 0; id < 6; ++id)
    {
        auto base = render(id, -1, 0);
        for (int k = 0; k < rastro::fx::modules[id].controls; ++k)
        {
            float amount = rastro::fx::modules[id].knob[k].initial > .5f ? .1f : .9f;
            auto changed = render(id, k, amount);
            double delta = 0;
            for (size_t i = 0; i < base.size(); ++i)
                delta += std::abs(base[i] - changed[i]);
            if (delta / base.size() < 1e-5)
                std::cerr << "inherited control " << id << " / " << k << " delta "
                          << delta / base.size() << '\n';
            check(delta / base.size() > 1e-5,
                  "every inherited FX rack control changes the wet audio");
        }
    }
    std::cout << "PASS FX inherited controls: every advanced control changes audio through the FX "
                 "adapter\n";
}
static std::vector<float> renderNative(int id, int changed = -1, float value = 0, bool mono = false)
{
    rastro::fx::Node node;
    node.prepare(48000);
    auto frame = defaultFxFrame();
    frame.pan = {{0, .1f, .2f, .3f, .6f, 1, .8f, .2f}};
    if (changed >= 0)
        frame.value[id][changed] = value;
    std::vector<float> result(84000);
    for (int i = 0; i < int(result.size() / 2); ++i)
    {
        float l = fixture(i), r = mono ? l : fixture(i, 1);
        node.process(id, l, r, frame.value[id], frame.pan, mono);
        check(std::isfinite(l) && std::isfinite(r) && std::abs(l) < 100,
              "new effects remain finite and bounded");
        result[2 * i] = l;
        result[2 * i + 1] = r;
    }
    return result;
}
static double difference(const std::vector<float> &a, const std::vector<float> &b)
{
    double sum = 0;
    for (size_t i = 0; i < a.size(); ++i)
        sum += std::abs(a[i] - b[i]);
    return sum / a.size();
}
static void nativeFxTests()
{
    for (int id = 6; id < rastro::fx::count; ++id)
    {
        auto baseline = renderNative(id);
        double diff = 0;
        for (size_t i = 0; i < baseline.size(); ++i)
            diff += std::abs(baseline[i] - fixture(int(i / 2), int(i % 2)));
        if (diff / baseline.size() <= .001)
            std::cerr << "native module " << id << " source delta " << diff / baseline.size()
                      << "\n";
        check(diff / baseline.size() > .001, "each native effect actually changes source audio");
        for (int k = 0; k < rastro::fx::modules[id].controls; ++k)
        {
            float initial = rastro::fx::modules[id].knob[k].initial;
            auto changed = renderNative(id, k, initial > .5f ? .1f : .9f);
            auto delta = difference(baseline, changed);
            if (delta < 1e-5)
                std::cerr << "module " << id << " control " << k << " delta " << delta << '\n';
            check(delta > 1e-5, "every advanced native control changes the actual sound");
        }
        auto mono = renderNative(id, -1, 0, true);
        check(std::any_of(mono.begin(), mono.end(), [](float x) { return std::abs(x) > .001; }),
              "native effects work in mono");
    }
    // Pitch-shifted echoes must move frequency, not just relabel an ordinary delay.
    rastro::fx::Node prism;
    prism.prepare(48000);
    auto f = defaultFxFrame();
    f.value[10][0] = 2.f / 3;
    f.value[10][1] = 0;
    f.value[10][2] = 0;
    f.value[10][3] = 1;
    double original = 0, octave = 0;
    for (int i = 0; i < 48000; ++i)
    {
        float l = float(.3 * std::sin(2 * rastro::fx::pi * 400 * i / 48000)), r = l;
        prism.process(10, l, r, f.value[10], f.pan, true);
        if (i > 12000)
        {
            original += l * std::sin(2 * rastro::fx::pi * 400 * i / 48000);
            octave += l * std::sin(2 * rastro::fx::pi * 800 * i / 48000);
        }
    }
    check(std::abs(octave) > std::abs(original) * 5, "PRISM creates an octave-up crystal echo");
    std::cout << "PASS FX native: nine real algorithms, every advanced control, mono, pitch-echo "
                 "spectral check\n";
}
static std::vector<float> renderFx(RastroFxProcessor &p, int samples = 8000, int channels = 2)
{
    std::vector<float> result(samples);
    juce::AudioBuffer<float> buffer(channels, 64);
    juce::MidiBuffer midi;
    for (int offset = 0; offset < samples;)
    {
        int length = std::min(64, samples - offset);
        buffer.setSize(channels, length, false, false, true);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < length; ++i)
                buffer.setSample(c, i, fixture(offset + i, c));
        fxInCallback = true;
        p.processBlock(buffer, midi);
        fxInCallback = false;
        for (int i = 0; i < length; ++i)
            result[offset + i] = buffer.getSample(0, i);
        offset += length;
    }
    return result;
}
static void routingFxTests()
{
    auto configureRack = [](RastroFxProcessor &p)
    {
        for (int id : {11, 13})
        {
            set(p, RastroFxProcessor::moduleId(id, "InRack").toRawUTF8(), 1);
            set(p, RastroFxProcessor::moduleId(id, "On").toRawUTF8(), 1);
        }
        set(p, "fxM11Mix", 1);
        set(p, "fxM13Mix", 1);
        set(p, "fxM11P0", .65f);
        set(p, "fxM13P0", .6f);
        p.prepareToPlay(48000, 64);
    };
    RastroFxProcessor serial;
    configureRack(serial);
    auto a = renderFx(serial);
    RastroFxProcessor reversed;
    configureRack(reversed);
    reversed.moveModule(13, -14);
    reversed.reset();
    auto b = renderFx(reversed);
    check(difference(a, b) > .001, "rack order changes the audio");
    RastroFxProcessor parallel;
    configureRack(parallel);
    set(parallel, "fxRouting", 1);
    parallel.reset();
    auto c = renderFx(parallel);
    check(difference(a, c) > .001, "parallel processing differs from series");
    // Parallel is the average of active branch outputs; verify against independent renders.
    RastroFxProcessor one, two;
    configureRack(one);
    configureRack(two);
    set(one, "fxM13On", 0);
    one.reset();
    set(two, "fxM11On", 0);
    two.reset();
    auto x = renderFx(one), y = renderFx(two);
    for (size_t i = 0; i < c.size(); ++i)
        check(std::abs(c[i] - (x[i] + y[i]) * .5f) < 2e-5, "parallel branch summation is correct");
    RastroFxProcessor dry;
    configureRack(dry);
    set(dry, "fxDryWet", 0);
    dry.reset();
    auto d = renderFx(dry);
    for (size_t i = 0; i < d.size(); ++i)
        check(d[i] == fixture(int(i)), "global wet zero is exact dry");
    RastroFxProcessor half;
    configureRack(half);
    set(half, "fxDryWet", .5f);
    half.reset();
    auto h = renderFx(half);
    for (size_t i = 0; i < h.size(); ++i)
        check(std::abs(h[i] - (a[i] + fixture(int(i))) * .5f) < 2e-5, "global wet interpolation");
    // Exercise all modules, host automation, order/routing transitions and sample-rate changes.
    RastroFxProcessor all;
    for (int id = 0; id < rastro::fx::count; ++id)
    {
        set(all, RastroFxProcessor::moduleId(id, "InRack").toRawUTF8(), 1);
        set(all, RastroFxProcessor::moduleId(id, "On").toRawUTF8(), 1);
    }
    for (double rate : {22050., 44100., 96000.})
    {
        all.prepareToPlay(rate, 64);
        auto output = renderFx(all, 2048);
        set(all, "fxRouting", 1);
        all.moveModule(14, -12);
        renderFx(all, 1024);
        for (float v : output)
            check(std::isfinite(v), "all modules/rates safe");
    }
    check(fxAllocations == 0,
          "no C++ allocations in audio callback with all modules and automation");
    std::cout
        << "PASS FX routing: order, exact parallel branch sum, dry/wet, rates, allocation guard\n";
}
static void modulationFxTests()
{
    auto init = [](RastroFxProcessor &p)
    {
        set(p, "fxM11InRack", 1);
        set(p, "fxM11On", 1);
        set(p, "fxM11Mix", 1);
        p.prepareToPlay(48000, 64);
    };
    RastroFxProcessor neutral;
    init(neutral);
    auto reference = renderFx(neutral);
    for (int route = 0; route < 4; ++route)
    {
        RastroFxProcessor p;
        init(p);
        int target = 0;
        for (size_t i = 0; i < p.targets.size(); ++i)
            if (p.targets[i].id == "fxM11P0")
                target = int(i) + 1;
        const char *prefix = route == 0 ? "X" : route == 1 ? "Y" : route == 2 ? "Lfo" : "Env";
        p.setValue(juce::String("fx") + prefix + "Target", float(target));
        p.setValue(juce::String("fx") + prefix + "Depth", .8f);
        if (route < 2)
            p.setValue(route == 0 ? "fxX" : "fxY", 1);
        if (route == 3)
            p.setValue("fxEnvSensitivity", 12);
        p.reset();
        auto result = renderFx(p);
        check(difference(result, reference) > .001,
              "XY/LFO/envelope routes measurably modulate audio");
        check(p.value("fxM11P0") == neutral.value("fxM11P0"),
              "modulation does not overwrite host parameter base");
    }
    std::cout << "PASS FX modulation: XY, LFO and envelope change audio without overwriting "
                 "automation bases\n";
}
static void transitionFxTests()
{
    for (bool host : {false, true})
    {
        RastroFxProcessor p;
        set(p, "fxM11InRack", 1);
        set(p, "fxM11On", 1);
        set(p, "fxM11Mix", 1);
        p.prepareToPlay(48000, 1);
        juce::AudioBuffer<float> b(2, 1);
        juce::MidiBuffer midi;
        auto sample = [&](bool bypassed)
        {
            b.setSample(0, 0, .25f);
            b.setSample(1, 0, .25f);
            fxInCallback = true;
            if (bypassed)
                p.processBlockBypassed(b, midi);
            else
                p.processBlock(b, midi);
            fxInCallback = false;
            return b.getSample(0, 0);
        };
        float previous = 0;
        for (int i = 0; i < 6000; ++i)
            previous = sample(false);
        check(std::abs(previous) < .001, "wet EMBER is actually processing the constant input");
        if (!host)
            set(p, "fxBypass", 1);
        for (int i = 0; i < 600; ++i)
        {
            float v = sample(host);
            check(std::abs(v - previous) < .0015, "wet global bypass transition has no jump");
            previous = v;
        }
        check(previous == .25f, "wet rack bypass settles to bit-exact dry");
        set(p, "fxBypass", 0);
        for (int i = 0; i < 600; ++i)
        {
            float v = sample(false);
            check(std::abs(v - previous) < .0015, "wet rack reenable has no jump");
            previous = v;
        }
        set(p, "fxM11On", 0);
        for (int i = 0; i < 600; ++i)
        {
            float v = sample(false);
            check(std::abs(v - previous) < .0015, "module bypass fades smoothly");
            previous = v;
        }
        check(previous == .25f, "module bypass settles to dry");
    }
    RastroFxProcessor fast, slow;
    fast.prepareToPlay(48000, 64);
    slow.prepareToPlay(48000, 64);
    set(fast, "fxEnvAttack", 1);
    set(slow, "fxEnvAttack", 200);
    juce::AudioBuffer<float> block(2, 64);
    juce::MidiBuffer midi;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 64; ++i)
            block.setSample(c, i, .5f);
    fast.processBlock(block, midi);
    slow.processBlock(block, midi);
    check(fast.envelopeMeter.load() > slow.envelopeMeter.load() * 10,
          "follower attack controls real response time");
    set(fast, "fxEnvRelease", 10);
    set(slow, "fxEnvRelease", 2000);
    auto beforeFast = fast.envelopeMeter.load(), beforeSlow = slow.envelopeMeter.load();
    block.clear();
    for (int i = 0; i < 16; ++i)
    {
        fast.processBlock(block, midi);
        slow.processBlock(block, midi);
    }
    check(fast.envelopeMeter.load() / beforeFast < slow.envelopeMeter.load() / beforeSlow * .3f,
          "follower release controls real decay");
    check(fxAllocations == 0, "no allocations during wet bypass toggles");
    std::cout << "PASS FX transitions: wet host/parameter/module bypass, exact dry settling, "
                 "envelope attack/release\n";
}
static void presetFxTests()
{
    RastroFxProcessor p;
    p.loadFactory(7);
    p.setPresetName("Minha lua / copper");
    p.setAppearance("accent", 3);
    p.setAppearance("skin", 2);
    auto a = p.params.copyState();
    p.copyCompare();
    p.compare();
    check(p.value("fxM11On") == 1 && p.value("fxM11InRack") == 1,
          "copy A/B includes module state and rack membership");
    set(p, "fxM11P0", .9f);
    p.setPresetName("B / burn");
    p.setAppearance("accent", 1);
    p.compare();
    check(p.value("fxM11P0") != .9f && p.presetName() == "Minha lua / copper" &&
              p.appearance("accent") == 3,
          "A recalls audio/name/appearance");
    juce::MemoryBlock state;
    p.getStateInformation(state);
    RastroFxProcessor restored;
    restored.setStateInformation(state.getData(), int(state.getSize()));
    check(restored.presetName() == p.presetName() && restored.appearance("skin") == 2,
          "host recall includes preset metadata");
    restored.compare();
    check(restored.value("fxM11P0") == .9f && restored.presetName() == "B / burn",
          "host recall preserves alternate B snapshot");
    juce::TemporaryFile file(".rastrofx");
    check(p.savePreset(file.getFile()), "atomic user preset save");
    restored.loadFactory(0);
    check(restored.loadPreset(file.getFile()), "user preset load");
    check(restored.presetName() == p.presetName(), "user preset restores name");
    file.getFile().replaceWithText("bad preset");
    check(!restored.loadPreset(file.getFile()) && restored.presetName() == p.presetName(),
          "invalid preset is rejected without changing sound");
    p.prepareToPlay(48000, 64);
    restored.prepareToPlay(48000, 64);
    check(difference(renderFx(p), renderFx(restored)) < 1e-7,
          "host/user preset recall yields identical audio");
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.loadFactory(i);
        p.reset();
        auto data = renderFx(p, 2000);
        check(p.presetName().isNotEmpty(), "factory program has a name");
        for (float value : data)
            check(std::isfinite(value), "factory presets render safe audio");
    }
    // Earlier three-parameter FX projects recall as a clean rack, even over a modified instance.
    juce::ValueTree old("RASTRO_FX_STATE");
    juce::ValueTree gain("PARAM");
    gain.setProperty("id", "fxInputGain", nullptr);
    gain.setProperty("value", -6, nullptr);
    old.addChild(gain, -1, nullptr);
    juce::ValueTree enabled("PARAM");
    enabled.setProperty("id", "fxM14On", nullptr);
    enabled.setProperty("value", 1, nullptr);
    old.addChild(enabled, -1, nullptr);
    juce::MemoryBlock legacy;
    juce::AudioProcessor::copyXmlToBinary(*old.createXml(), legacy);
    p.setStateInformation(legacy.getData(), int(legacy.getSize()));
    check(p.value("fxInputGain") == -6 && p.value("fxM14On") == 1 &&
              p.value("fxM14InRack") == 1,
          "legacy-state migration preserves active modules and adds rack membership");
    std::cout << "PASS FX presets: factory/user presets, A/B, both-bank host recall, metadata, "
                 "rendered recall and legacy migration\n";
}
static juce::Component *findFx(juce::Component &component, const juce::String &id)
{
    if (component.getComponentID() == id)
        return &component;
    for (auto *child : component.getChildren())
        if (auto *result = findFx(*child, id))
            return result;
    return nullptr;
}
static void editorFxTests()
{
    RastroFxProcessor p;
    p.loadFactory(6);
    for (int id = 0; id < rastro::fx::count; ++id)
        set(p, RastroFxProcessor::moduleId(id, "InRack").toRawUTF8(), 1);
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(p.createEditor());
    auto *fx = dynamic_cast<RastroFxEditor *>(editor.get());
    check(fx != nullptr, "FX has a custom editor");
    for (auto size :
         {juce::Point<int>(980, 720), juce::Point<int>(1180, 840), juce::Point<int>(1440, 1000)})
    {
        editor->setSize(size.x, size.y);
        fx->refresh();
        check(fx->controlsFit(), "rack and advanced controls fit supported window sizes");
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        auto output = juce::File::getCurrentWorkingDirectory()
                          .getChildFile("fx-editor-" + juce::String(size.x) + ".png")
                          .createOutputStream();
        check(output != nullptr, "FX screenshot output");
        output->setPosition(0);
        output->truncate();
        juce::PNGImageFormat png;
        check(png.writeImageToStream(image, *output), "custom FX editor renders");
    }
    for (int id = 0; id < rastro::fx::count; ++id)
    {
        check(findFx(*editor, "fxRow" + juce::String(id)) != nullptr,
              "all fifteen rack cards exist");
        for (int k = 0; k < rastro::fx::modules[id].controls; ++k)
        {
            auto parameterId = RastroFxProcessor::moduleId(id, "P" + juce::String(k));
            auto *slider = dynamic_cast<juce::Slider *>(findFx(*editor, parameterId));
            if (slider != nullptr)
            {
                slider->setValue(.71, juce::sendNotificationSync);
                check(std::abs(p.value(parameterId) - .71) < 1e-5,
                      "custom slider controls real host parameter");
            }
            else
            {
                auto *segmented = findFx(*editor, parameterId);
                check(segmented != nullptr, "unique module selector is visible");
                int option = id == 6 ? 3 : 2;
                auto *button = dynamic_cast<juce::Button *>(
                    findFx(*editor, parameterId + "Option" + juce::String(option)));
                check(button != nullptr, "unique module selector has accessible options");
                button->triggerClick();
                float expected = id == 6 ? .75f : 1.f;
                check(std::abs(p.value(parameterId) - expected) < 1e-5,
                      "unique module selector controls real host parameter");
            }
        }
    }
    auto *shelf = dynamic_cast<juce::Button *>(findFx(*editor, "fxModule0"));
    check(shelf != nullptr, "module bank exposes add/remove controls");
    shelf->triggerClick();
    check(p.value("fxM0InRack") == 0, "module bank removes an inserted effect");
    shelf->triggerClick();
    check(p.value("fxM0InRack") == 1, "module bank inserts an effect into the rack");
    auto *remove = dynamic_cast<juce::Button *>(findFx(*editor, "fxRemove0"));
    check(remove != nullptr, "rack row exposes a remove control");
    remove->triggerClick();
    check(p.value("fxM0InRack") == 0, "rack row removes its effect");
    auto *compare = dynamic_cast<juce::Button *>(findFx(*editor, "fxCompare"));
    check(compare != nullptr, "A/B UI");
    compare->onClick();
    check(p.compareSlot() == 1, "A/B button changes real comparison bank");
    check(findFx(*editor, "fxPattern") && findFx(*editor, "fxPad"),
          "editable panner and assignable XY UI");
    auto click = [&](juce::Component *component, juce::Point<float> point)
    {
        auto now = juce::Time::getCurrentTime();
        juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(), point,
                               juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, component,
                               component, now, point, now, 1, false);
        component->mouseDown(event);
        component->mouseUp(event);
    };
    auto *pad = findFx(*editor, "fxPad");
    click(pad, {float(pad->getWidth() - 8), 8.f});
    check(p.value("fxX") > .99f && p.value("fxY") > .99f,
          "XY mouse edits actual automatable coordinates");
    struct GestureWatch : juce::AudioProcessorParameter::Listener
    {
        int depth = 0, maximum = 0;
        void parameterValueChanged(int, float) override {}
        void parameterGestureChanged(int, bool begin) override
        {
            depth += begin ? 1 : -1;
            maximum = std::max(maximum, depth);
        }
    } watch;
    auto *xParameter = p.params.getParameter("fxX");
    xParameter->addListener(&watch);
    auto now = juce::Time::getCurrentTime();
    juce::Point<float> xyPoint(20, 20);
    juce::MouseEvent doubleClick(juce::Desktop::getInstance().getMainMouseSource(), xyPoint,
                                 juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, pad, pad,
                                 now, xyPoint, now, 2, false);
    pad->mouseDown(doubleClick);
    pad->mouseDoubleClick(doubleClick);
    pad->mouseUp(doubleClick);
    xParameter->removeListener(&watch);
    check(watch.depth == 0 && watch.maximum == 1 && p.value("fxX") == .5f && p.value("fxY") == .5f,
          "XY double-click centres within a single balanced host automation gesture");
    auto *pattern = findFx(*editor, "fxPattern");
    click(pattern, {14.f, 24.f});
    check(p.value("fxPan0") == 1, "panner point mouse edits real pattern parameter");
    auto capture = [&](const char *name)
    {
        fx->refresh();
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        auto file =
            juce::File::getCurrentWorkingDirectory().getChildFile(name).createOutputStream();
        check(file != nullptr, "additional FX capture output");
        file->setPosition(0);
        file->truncate();
        juce::PNGImageFormat png;
        check(png.writeImageToStream(image, *file), "additional actual FX capture");
    };
    p.loadFactory(9);
    p.moveModule(14, -14);
    fx->refresh();
    auto *modView = dynamic_cast<juce::Viewport *>(findFx(*editor, "fxModViewport"));
    check(modView != nullptr, "modulation scroll view exists");
    modView->setViewPosition(0, 450);
    capture("fx-editor-swing.png");
    p.loadFactory(4);
    p.moveModule(6, -14);
    p.moveModule(7, -13);
    p.moveModule(8, -12);
    fx->refresh();
    capture("fx-editor-new-modules.png");
    std::cout << "PASS FX editor: custom artwork, 15 cards, real advanced attachments, A/B UI and "
                 "min/normal/large layout renders\n";
}
static void fullFxTests()
{
    inheritedFxTests();
    inheritedControlTests();
    nativeFxTests();
    routingFxTests();
    modulationFxTests();
    transitionFxTests();
    presetFxTests();
    editorFxTests();
}
