#include "FxWebEditor.h"

#if RASTRO_FX_WEB_UI
#include "RastroFxWeb.h"

namespace
{
const char *mimeFor(const juce::String &file)
{
    auto ext = file.fromLastOccurrenceOf(".", false, false).toLowerCase();
    if (ext == "html")
        return "text/html";
    if (ext == "js")
        return "text/javascript";
    if (ext == "css")
        return "text/css";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "png")
        return "image/png";
    if (ext == "woff")
        return "font/woff";
    return "application/octet-stream";
}
// Windows: Edge WebView2 (statically linked loader). It needs a writable user-data folder, and a
// DAW often runs from a protected location, so it lives in the user's temp folder.
juce::WebBrowserComponent::Options platformOptions(juce::WebBrowserComponent::Options options)
{
#if JUCE_WINDOWS
    options = options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                  .withWinWebView2Options(
                      juce::WebBrowserComponent::Options::WinWebView2{}.withUserDataFolder(
                          juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("RASTRO FX WebView2")));
#endif
    return options;
}
juce::File presetFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("RASTRO FX/Presets");
}
} // namespace

RastroFxWebEditor::RastroFxWebEditor(RastroFxProcessor &processor)
    : juce::AudioProcessorEditor(&processor), p(processor)
{
    for (auto *raw : p.getParameters())
        if (auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(raw))
        {
            parameters.push_back(ranged);
            sentValues.push_back(std::numeric_limits<float>::quiet_NaN());
        }

    auto options =
        platformOptions(juce::WebBrowserComponent::Options{})
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider([this](const juce::String &url) { return resource(url); })
            .withNativeFunction("ready",
                                [this](const Args &, Completion done)
                                {
                                    pageReady = true;
                                    std::fill(sentValues.begin(), sentValues.end(),
                                              std::numeric_limits<float>::quiet_NaN());
                                    done(initialState());
                                })
            .withNativeFunction("setParam",
                                [this](const Args &a, Completion done)
                                {
                                    setParameter(a);
                                    done(true);
                                })
            .withNativeFunction("loadFactory",
                                [this](const Args &a, Completion done)
                                {
                                    p.loadFactory(a.size() > 0 ? (int)a[0] : 0);
                                    done(true);
                                })
            .withNativeFunction("compare",
                                [this](const Args &, Completion done)
                                {
                                    p.compare();
                                    done(p.compareSlot());
                                })
            .withNativeFunction("copyCompare",
                                [this](const Args &, Completion done)
                                {
                                    p.copyCompare();
                                    done(true);
                                })
            .withNativeFunction("setPresetName",
                                [this](const Args &a, Completion done)
                                {
                                    p.setPresetName(a.size() > 0 ? a[0].toString()
                                                                 : juce::String());
                                    done(true);
                                })
            .withNativeFunction("savePreset", [this](const Args &, Completion done)
                                { choosePreset(true, std::move(done)); })
            .withNativeFunction("openPreset", [this](const Args &, Completion done)
                                { choosePreset(false, std::move(done)); });

    web = std::make_unique<Browser>(options);
    addAndMakeVisible(*web);
    web->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Same 16:10 canvas as the page (1440x900), scaled to the window.
    setResizable(true, true);
    if (auto *constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(1440.0 / 900.0);
    setResizeLimits(960, 600, 1920, 1200);
    setSize(1280, 800);
    startTimerHz(30);
}

RastroFxWebEditor::~RastroFxWebEditor() { stopTimer(); }

bool RastroFxWebEditor::available()
{
    return juce::WebBrowserComponent::areOptionsSupported(
        platformOptions(juce::WebBrowserComponent::Options{}.withNativeIntegrationEnabled()));
}

void RastroFxWebEditor::paint(juce::Graphics &g) { g.fillAll(juce::Colour(0xff12071c)); }

void RastroFxWebEditor::resized()
{
    if (web != nullptr)
        web->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
RastroFxWebEditor::resource(const juce::String &url)
{
    auto path = url.upToFirstOccurrenceOf("?", false, false);
    auto file = path == "/" || path.isEmpty() ? juce::String("index.html")
                                              : path.fromLastOccurrenceOf("/", false, false);
    for (int i = 0; i < RastroFxWeb::namedResourceListSize; ++i)
    {
        const char *name = RastroFxWeb::namedResourceList[i];
        if (file == RastroFxWeb::getNamedResourceOriginalFilename(name))
        {
            int size = 0;
            const auto *data = RastroFxWeb::getNamedResource(name, size);
            if (data == nullptr)
                return std::nullopt;
            std::vector<std::byte> bytes((size_t)size);
            std::memcpy(bytes.data(), data, (size_t)size);
            return juce::WebBrowserComponent::Resource{std::move(bytes), mimeFor(file)};
        }
    }
    return std::nullopt;
}

juce::var RastroFxWebEditor::initialState()
{
    auto *state = new juce::DynamicObject();
    juce::Array<juce::var> list;
    for (auto *parameter : parameters)
    {
        auto *info = new juce::DynamicObject();
        const auto &range = parameter->getNormalisableRange();
        info->setProperty("id", parameter->getParameterID());
        info->setProperty("v", parameter->convertFrom0to1(parameter->getValue()));
        info->setProperty("min", range.start);
        info->setProperty("max", range.end);
        info->setProperty("def", parameter->convertFrom0to1(parameter->getDefaultValue()));
        const bool discrete = dynamic_cast<juce::AudioParameterBool *>(parameter) != nullptr ||
                              dynamic_cast<juce::AudioParameterChoice *>(parameter) != nullptr ||
                              dynamic_cast<juce::AudioParameterInt *>(parameter) != nullptr;
        info->setProperty("step", discrete ? 1 : 0);
        list.add(info);
    }
    state->setProperty("params", list);

    juce::Array<juce::var> modules;
    for (const auto &m : rastro::fx::modules)
    {
        auto *module = new juce::DynamicObject();
        module->setProperty("name", m.name);
        module->setProperty("description", m.description);
        juce::Array<juce::var> controls;
        for (int k = 0; k < m.controls; ++k)
        {
            const auto &c = m.knob[(size_t)k];
            auto *control = new juce::DynamicObject();
            control->setProperty("name", c.name);
            control->setProperty("low", c.low);
            control->setProperty("high", c.high);
            control->setProperty("initial", c.initial);
            control->setProperty("unit", c.unit);
            control->setProperty("log", c.logarithmic);
            controls.add(control);
        }
        module->setProperty("controls", controls);
        modules.add(module);
    }
    state->setProperty("modules", modules);

    juce::Array<juce::var> targetNames;
    for (const auto &target : p.targets)
        targetNames.add(target.label);
    state->setProperty("targets", targetNames);

    juce::Array<juce::var> factory;
    for (int i = 0; i < p.getNumPrograms(); ++i)
        factory.add(p.getProgramName(i));
    state->setProperty("factory", factory);
    state->setProperty("program", p.getCurrentProgram());
    state->setProperty("preset", p.presetName());
    state->setProperty("bank", p.compareSlot());
    state->setProperty("latency", p.getLatencySamples());
    state->setProperty("version", juce::String(JucePlugin_VersionString));
    return state;
}

void RastroFxWebEditor::setParameter(const Args &a)
{
    if (a.isEmpty())
        return;
    auto *parameter = p.params.getParameter(a[0].toString());
    if (parameter == nullptr)
        return;
    const int phase = a.size() > 2 ? (int)a[2] : 0; // 1 begin gesture, 2 end gesture
    if (phase == 1)
        parameter->beginChangeGesture();
    if (a.size() > 1 && !a[1].isVoid())
        parameter->setValueNotifyingHost(parameter->convertTo0to1((float)(double)a[1]));
    if (phase == 2)
        parameter->endChangeGesture();
}

void RastroFxWebEditor::choosePreset(bool save, Completion done)
{
    auto directory = presetFolder();
    directory.createDirectory();
    chooser = std::make_unique<juce::FileChooser>(
        save ? "Salvar preset do RASTRO FX" : "Abrir preset do RASTRO FX",
        save ? directory.getChildFile(juce::File::createLegalFileName(p.presetName())) : directory,
        "*.rastrofx");
    juce::Component::SafePointer<RastroFxWebEditor> safe(this);
    chooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles |
            (save ? juce::FileBrowserComponent::saveMode |
                        juce::FileBrowserComponent::warnAboutOverwriting
                  : juce::FileBrowserComponent::openMode),
        [safe, save, done](const juce::FileChooser &result)
        {
            auto *reply = new juce::DynamicObject();
            reply->setProperty("ok", false);
            if (safe == nullptr || result.getResult() == juce::File{})
            {
                done(juce::var(reply));
                return;
            }
            auto file = result.getResult();
            if (save)
            {
                file = file.withFileExtension("rastrofx");
                safe->p.setPresetName(file.getFileNameWithoutExtension());
            }
            const bool ok = save ? safe->p.savePreset(file) : safe->p.loadPreset(file);
            reply->setProperty("ok", ok);
            reply->setProperty("name", safe->p.presetName());
            if (!ok)
                reply->setProperty("message", save ? "Não foi possível salvar nessa pasta."
                                                   : "Esse arquivo não é um preset do RASTRO FX.");
            done(juce::var(reply));
        });
}

void RastroFxWebEditor::timerCallback()
{
    if (!pageReady || web == nullptr || !web->isVisible())
        return;

    // Parameter changes from host automation, presets, A/B or this page.
    auto *changes = new juce::DynamicObject();
    bool anyChange = false;
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        const float v = parameters[i]->convertFrom0to1(parameters[i]->getValue());
        if (!(std::abs(v - sentValues[i]) < 1.0e-6f))
        {
            sentValues[i] = v;
            changes->setProperty(parameters[i]->getParameterID(), v);
            anyChange = true;
        }
    }
    if (anyChange)
        web->emitEventIfBrowserIsVisible("params", juce::var(changes));
    else
        delete changes;

    auto *t = new juce::DynamicObject();
    t->setProperty("in", p.inputPeak.load(std::memory_order_relaxed));
    t->setProperty("out", p.outputPeak.load(std::memory_order_relaxed));
    t->setProperty("lfo", p.lfoMeter.load(std::memory_order_relaxed));
    t->setProperty("env", p.envelopeMeter.load(std::memory_order_relaxed));
    juce::Array<juce::var> activity;
    for (auto &a : p.activity)
        activity.add(a.load(std::memory_order_relaxed));
    t->setProperty("act", activity);
    // last 256 scope points, oldest first (visualiser only)
    juce::Array<juce::var> sc;
    const int w = p.scopeWrite.load(std::memory_order_relaxed);
    for (int i = 0; i < 256; ++i)
    {
        const int k = (w - 256 + i + RastroFxProcessor::scopeSize) % RastroFxProcessor::scopeSize;
        sc.add(std::round(p.scope[size_t(k)].load(std::memory_order_relaxed) * 1000) / 1000);
    }
    t->setProperty("scope", sc);
    t->setProperty("bank", p.compareSlot());
    t->setProperty("preset", p.presetName());
    t->setProperty("program", p.getCurrentProgram());
    web->emitEventIfBrowserIsVisible("tick", juce::var(t));
}
#endif
