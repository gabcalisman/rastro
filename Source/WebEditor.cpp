#include "WebEditor.h"

#if RASTRO_WEB_UI
#include "CapturePanel.h"
#include "RastroWeb.h"

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
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "woff2")
        return "font/woff2";
    if (ext == "json")
        return "application/json";
    return "application/octet-stream";
}
} // namespace

RastroWebEditor::RastroWebEditor(RastroProcessor &processor)
    : juce::AudioProcessorEditor(&processor), p(processor)
{
    for (auto *raw : p.getParameters())
        if (auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(raw))
        {
            parameters.push_back(ranged);
            sentValues.push_back(std::numeric_limits<float>::quiet_NaN());
        }

    auto options =
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider([this](const juce::String &url) { return resource(url); })
            .withNativeFunction("ready",
                                [this](const Args &, Completion done)
                                {
                                    pageReady = true;
                                    std::fill(sentValues.begin(), sentValues.end(),
                                              std::numeric_limits<float>::quiet_NaN());
                                    lastOrder.fill(-1);
                                    lastSample = p.source.load();
                                    done(initialState());
                                })
            .withNativeFunction("setParam",
                                [this](const Args &a, Completion done)
                                {
                                    setParameter(a);
                                    done(true);
                                })
            .withNativeFunction("note",
                                [this](const Args &a, Completion done)
                                {
                                    const int note = juce::jlimit(0, 127, (int)a[0]);
                                    const float velocity =
                                        juce::jlimit(0.f, 1.f, (float)(double)a[1]);
                                    if ((bool)a[2])
                                        p.keyboard.noteOn(1, note, juce::jmax(.05f, velocity));
                                    else
                                        p.keyboard.noteOff(1, note, 0);
                                    done(true);
                                })
            .withNativeFunction("allNotesOff",
                                [this](const Args &, Completion done)
                                {
                                    p.keyboard.allNotesOff(1);
                                    done(true);
                                })
            .withNativeFunction(
                "importDialog",
                [this](const Args &, Completion done)
                {
                    chooser = std::make_unique<juce::FileChooser>(
                        p.language == 0 ? "Escolha um áudio" : "Choose audio", juce::File{},
                        "*.wav;*.aif;*.aiff;*.mp3;*.m4a");
                    juce::Component::SafePointer<RastroWebEditor> safe(this);
                    chooser->launchAsync(
                        juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles,
                        [safe, done](const juce::FileChooser &fc)
                        {
                            if (safe == nullptr)
                                return;
                            auto file = fc.getResult();
                            if (file == juce::File{})
                            {
                                done(juce::var());
                                return;
                            }
                            safe->p.importFileAsync(
                                file,
                                [safe, done](rastro::ImportError result)
                                {
                                    if (safe != nullptr)
                                        done(safe->importResult(result));
                                });
                        });
                })
            .withNativeFunction("importData",
                                [this](const Args &a, Completion done)
                                {
                                    juce::MemoryOutputStream decoded;
                                    if (!juce::Base64::convertFromBase64(decoded,
                                                                         a[1].toString()))
                                    {
                                        done(importResult(rastro::ImportError::corrupt));
                                        return;
                                    }
                                    juce::Component::SafePointer<RastroWebEditor> safe(this);
                                    p.importDataAsync(decoded.getMemoryBlock(), a[0].toString(),
                                                      [safe, done](rastro::ImportError result)
                                                      {
                                                          if (safe != nullptr)
                                                              done(safe->importResult(result));
                                                      });
                                })
            .withNativeFunction("waveform", [this](const Args &a, Completion done)
                                { done(waveform(juce::jlimit(64, 8192, (int)a[0]))); })
            .withNativeFunction("playbackMode",
                                [this](const Args &a, Completion done)
                                {
                                    p.setPlaybackMode((int)a[0]);
                                    done(true);
                                })
            .withNativeFunction("slices",
                                [this](const Args &a, Completion done)
                                {
                                    p.equalSlices((int)a[0]);
                                    done(true);
                                })
            .withNativeFunction("moveModule",
                                [this](const Args &a, Completion done)
                                {
                                    p.moveModule((int)a[0], (int)a[1]);
                                    done(true);
                                })
            .withNativeFunction("jam",
                                [this](const Args &, Completion done)
                                {
                                    p.mutate();
                                    done(p.canUndoMutation());
                                })
            .withNativeFunction("undoJam",
                                [this](const Args &, Completion done)
                                {
                                    p.undoMutation();
                                    done(p.canUndoMutation());
                                })
            .withNativeFunction("resetEffects",
                                [this](const Args &, Completion done)
                                {
                                    p.resetEffects();
                                    done(true);
                                })
            /* ---------- patches ---------- */
            .withNativeFunction("patchValues",
                                [this](const Args &, Completion done)
                                {
                                    juce::Array<juce::var> list;
                                    for (auto v : p.patchValues())
                                        list.add(v);
                                    done(list);
                                })
            .withNativeFunction("patchApply",
                                [this](const Args &a, Completion done)
                                {
                                    auto values = p.patchValues();
                                    if (auto *list = a[0].getArray())
                                        for (int i = 0; i < (int)values.size() && i < list->size(); ++i)
                                            values[(size_t)i] = (float)(double)(*list)[i];
                                    p.applyPatch(values);
                                    done(true);
                                })
            .withNativeFunction(
                "patchFile",
                [this](const Args &a, Completion done)
                {
                    const bool save = (bool)a[0], soundOnly = a.size() > 1 && (bool)a[1];
                    juce::Component::SafePointer<RastroWebEditor> safe(this);
                    chooseFile(save, "*.rastro",
                               [safe, save, soundOnly, done](juce::File file)
                               {
                                   if (safe == nullptr)
                                       return;
                                   if (file == juce::File{})
                                   {
                                       done(juce::var());
                                       return;
                                   }
                                   if (save)
                                       file = file.withFileExtension("rastro");
                                   safe->p.patchFileAsync(
                                       file, save,
                                       [done, name = file.getFileNameWithoutExtension()](bool ok)
                                       {
                                           auto *o = new juce::DynamicObject();
                                           o->setProperty("ok", ok);
                                           o->setProperty("name", name);
                                           done(o);
                                       },
                                       soundOnly);
                               });
                })
            .withNativeFunction("patchSearch",
                                [this](const Args &a, Completion done)
                                {
                                    done(p.patchSearch.start(p.source.load(), p.settings(),
                                                             (bool)a[0],
                                                             (unsigned)juce::Random::getSystemRandom().nextInt()));
                                })
            .withNativeFunction("patchSearchPause",
                                [this](const Args &, Completion done)
                                {
                                    p.patchSearch.paused = !p.patchSearch.paused.load();
                                    done(p.patchSearch.paused.load());
                                })
            .withNativeFunction("patchSearchCancel",
                                [this](const Args &, Completion done)
                                {
                                    p.patchSearch.cancel();
                                    done(true);
                                })
            .withNativeFunction("patchResults",
                                [this](const Args &, Completion done)
                                {
                                    searchResults = p.patchSearch.results();
                                    const bool valid = p.patchSearch.sourceUsed() == p.source.load();
                                    juce::Array<juce::var> list;
                                    for (auto &r : searchResults)
                                    {
                                        auto *o = new juce::DynamicObject();
                                        juce::Array<juce::var> values;
                                        for (auto v : r.values)
                                            values.add(v);
                                        o->setProperty("values", values);
                                        o->setProperty("distance", r.distance);
                                        list.add(o);
                                    }
                                    auto *o = new juce::DynamicObject();
                                    o->setProperty("valid", valid);
                                    o->setProperty("results", list);
                                    done(o);
                                })
            /* ---------- capture ---------- */
            .withNativeFunction(
                "captureInfo",
                [this](const Args &, Completion done)
                {
                    auto *o = new juce::DynamicObject();
                    o->setProperty("direct", p.getTotalNumInputChannels() > 0);
                    o->setProperty("standalone", juce::PluginHostType::getPluginLoadedAs() ==
                                                     juce::AudioProcessor::wrapperType_Standalone);
                    o->setProperty("inputAvailable",
                                   p.standaloneInputAvailable ? p.standaloneInputAvailable() : false);
                    o->setProperty("canOpenSettings", (bool)p.standaloneAudioSettings);
                    done(o);
                })
            .withNativeFunction("captureRecord",
                                [this](const Args &, Completion done)
                                {
                                    p.beginCapture([done](bool ok) { done(ok); });
                                })
            .withNativeFunction("captureStop",
                                [this](const Args &, Completion done)
                                {
                                    p.capture.stop();
                                    done(true);
                                })
            .withNativeFunction("captureMonitor",
                                [this](const Args &a, Completion done)
                                {
                                    p.capture.monitor = (bool)a[0];
                                    done(true);
                                })
            .withNativeFunction("captureGain",
                                [this](const Args &a, Completion done)
                                {
                                    p.capture.gain = juce::jlimit(0.f, 4.f, (float)(double)a[0]);
                                    done(true);
                                })
            .withNativeFunction("captureSettings",
                                [this](const Args &, Completion done)
                                {
                                    if (p.standaloneAudioSettings)
                                        p.standaloneAudioSettings();
                                    done(true);
                                })
            .withNativeFunction(
                "capturePoll",
                [this](const Args &, Completion done)
                {
                    p.pollCaptureLinks(
                        [done, lang = p.language.load()](std::vector<juce::File> files,
                                                         std::vector<rastro::CaptureLink> links)
                        {
                            auto *o = new juce::DynamicObject();
                            juce::Array<juce::var> f, l;
                            for (auto &file : files)
                            {
                                auto *x = new juce::DynamicObject();
                                x->setProperty("path", file.getFullPathName());
                                x->setProperty("name", file.getFileName());
                                x->setProperty("time", (double)file.getLastModificationTime().toMilliseconds());
                                f.add(x);
                            }
                            for (auto &link : links)
                            {
                                auto *x = new juce::DynamicObject();
                                x->setProperty("id", link.id);
                                x->setProperty("state", link.state);
                                x->setProperty("outcome", link.outcome);
                                x->setProperty("outcomeText", CapturePanel::outcomeText(link.outcome, lang == 0));
                                x->setProperty("frames", link.frames);
                                x->setProperty("rate", link.rate);
                                x->setProperty("peak", link.peak);
                                x->setProperty("flow", link.hasAudioFlow());
                                l.add(x);
                            }
                            o->setProperty("files", f);
                            o->setProperty("links", l);
                            done(o);
                        });
                })
            .withNativeFunction("captureRemote",
                                [this](const Args &a, Completion done)
                                {
                                    p.remoteCapture(a[0].toString(), (bool)a[1],
                                                    [done](bool ok) { done(ok); });
                                })
            .withNativeFunction("captureLoad",
                                [this](const Args &a, Completion done)
                                {
                                    juce::Component::SafePointer<RastroWebEditor> safe(this);
                                    p.importFileAsync(juce::File(a[0].toString()),
                                                      [safe, done](rastro::ImportError result)
                                                      {
                                                          if (safe != nullptr)
                                                              done(safe->importResult(result));
                                                      });
                                })
            .withNativeFunction("preview", [this](const Args &a, Completion done)
                                {
                                    const bool start = (bool)a[0];
                                    if (start && a.size() > 1 && !a[1].isVoid())
                                        p.previewFrom = juce::jlimit(0.f, 1.f, (float)(double)a[1]);
                                    p.previewRequest = start ? 1 : -1;
                                    done(true);
                                });

    web = std::make_unique<Browser>(options);
    addAndMakeVisible(*web);
    web->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable(true, true);
    setResizeLimits(960, 624, 1800, 1170);
    if (auto *constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(1200.0 / 780.0);
    setSize(1200, 780);
    startTimerHz(60);
}

RastroWebEditor::~RastroWebEditor() { stopTimer(); }

void RastroWebEditor::paint(juce::Graphics &g) { g.fillAll(juce::Colour(0xff2a1530)); }

void RastroWebEditor::resized()
{
    if (web != nullptr)
        web->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
RastroWebEditor::resource(const juce::String &url)
{
    auto path = url.upToFirstOccurrenceOf("?", false, false);
    auto file = path == "/" || path.isEmpty() ? juce::String("index.html")
                                              : path.fromLastOccurrenceOf("/", false, false);
    for (int i = 0; i < RastroWeb::namedResourceListSize; ++i)
    {
        const char *name = RastroWeb::namedResourceList[i];
        if (file == RastroWeb::getNamedResourceOriginalFilename(name))
        {
            int size = 0;
            const auto *data = RastroWeb::getNamedResource(name, size);
            if (data == nullptr)
                return std::nullopt;
            std::vector<std::byte> bytes((size_t)size);
            std::memcpy(bytes.data(), data, (size_t)size);
            return juce::WebBrowserComponent::Resource{std::move(bytes), mimeFor(file)};
        }
    }
    return std::nullopt;
}

juce::var RastroWebEditor::parameterInfo(const juce::RangedAudioParameter &parameter) const
{
    auto *o = new juce::DynamicObject();
    const auto &range = parameter.getNormalisableRange();
    o->setProperty("id", parameter.getParameterID());
    o->setProperty("v", parameter.convertFrom0to1(parameter.getValue()));
    o->setProperty("min", range.start);
    o->setProperty("max", range.end);
    o->setProperty("step", range.interval);
    o->setProperty("def", parameter.convertFrom0to1(parameter.getDefaultValue()));
    return o;
}

juce::var RastroWebEditor::sampleInfo() const
{
    auto *o = new juce::DynamicObject();
    const auto *s = p.source.load();
    o->setProperty("loaded", s != nullptr && s->size() > 0);
    o->setProperty("name", p.sampleName());
    o->setProperty("frames", s != nullptr ? s->size() : 0);
    o->setProperty("rate", s != nullptr ? s->rate : 44100.0);
    o->setProperty("seconds", s != nullptr && s->rate > 0 ? s->size() / s->rate : 0.0);
    o->setProperty("stereo", s != nullptr && s->stereo);
    return o;
}

juce::var RastroWebEditor::initialState()
{
    auto *o = new juce::DynamicObject();
    juce::Array<juce::var> list;
    for (auto *parameter : parameters)
        list.add(parameterInfo(*parameter));
    o->setProperty("params", list);
    o->setProperty("language", p.language.load());
    o->setProperty("sample", sampleInfo());
    o->setProperty("canUndoJam", p.canUndoMutation());
    o->setProperty("host", juce::PluginHostType().getHostDescription());
    o->setProperty("version", JucePlugin_VersionString);
    return o;
}

juce::var RastroWebEditor::waveform(int columns) const
{
    auto *o = new juce::DynamicObject();
    const auto *s = p.source.load();
    if (s == nullptr || s->size() == 0)
        return o;
    for (int c = 0; c < 2; ++c)
    {
        const auto &levels = s->peaks[(size_t)c];
        if (levels.empty())
            continue;
        size_t level = 0;
        while (level + 1 < levels.size() && levels[level].size() > (size_t)columns)
            ++level;
        juce::Array<juce::var> values;
        values.ensureStorageAllocated((int)levels[level].size() * 2);
        for (const auto &peak : levels[level])
        {
            values.add(std::round(peak.lo * 1000.f) / 1000.f);
            values.add(std::round(peak.hi * 1000.f) / 1000.f);
        }
        o->setProperty(c == 0 ? "l" : "r", values);
    }
    o->setProperty("stereo", s->stereo);
    return o;
}

juce::var RastroWebEditor::importResult(rastro::ImportError result) const
{
    auto *o = new juce::DynamicObject();
    o->setProperty("ok", result == rastro::ImportError::none);
    o->setProperty("message", p.importMessage(result));
    return o;
}

void RastroWebEditor::chooseFile(bool save, juce::String pattern,
                                 std::function<void(juce::File)> complete)
{
    chooser = std::make_unique<juce::FileChooser>(
        save ? (p.language == 0 ? "Salvar patch" : "Save patch")
             : (p.language == 0 ? "Abrir patch" : "Open patch"),
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), pattern);
    auto flags = save ? juce::FileBrowserComponent::saveMode |
                            juce::FileBrowserComponent::warnAboutOverwriting
                      : juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags, [complete](const juce::FileChooser &fc) { complete(fc.getResult()); });
}

void RastroWebEditor::setParameter(const Args &a)
{
    auto *parameter = p.params.getParameter(a[0].toString());
    if (parameter == nullptr)
        return;
    const int phase = a.size() > 2 ? (int)a[2] : 0; // 1 begin gesture, 2 end gesture
    if (phase == 1)
        parameter->beginChangeGesture();
    if (a.size() > 1 && !a[1].isVoid())
    {
        const float value = (float)(double)a[1];
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
    if (phase == 2)
        parameter->endChangeGesture();
}

void RastroWebEditor::timerCallback()
{
    if (!pageReady || web == nullptr || !web->isVisible())
        return;

    // Parameter changes from host automation, MIDI Learn, JAM, presets or this page.
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
        web->emitEventIfBrowserIsVisible("params", changes);
    else
        delete changes;

    const auto *sample = p.source.load();
    if (sample != lastSample)
    {
        lastSample = sample;
        web->emitEventIfBrowserIsVisible("sample", sampleInfo());
    }

    const auto order = p.settings().order;
    bool orderChanged = false;
    for (int i = 0; i < rastro::moduleCount; ++i)
        orderChanged |= order[(size_t)i] != lastOrder[(size_t)i];

    auto &engine = p.engine;
    auto *t = new juce::DynamicObject();
    juce::Array<juce::var> activity;
    for (auto &a : engine.rack.moduleActivity)
        activity.add(a.load(std::memory_order_relaxed));
    juce::Array<juce::var> notes;
    for (auto &word : p.heldNotes)
        notes.add((double)word.load(std::memory_order_relaxed));
    t->setProperty("meter", engine.meter.load());
    t->setProperty("act", activity);
    t->setProperty("env", engine.visualEnvelope.load());
    t->setProperty("wob", engine.visualWobble.load());
    t->setProperty("dust", engine.visualDust.load());
    t->setProperty("crush", engine.visualCrush.load());
    t->setProperty("bloom", engine.visualBloom.load());
    t->setProperty("pos", engine.position.load());
    t->setProperty("playing", engine.playbackActive.load());
    t->setProperty("preview", engine.previewActive.load());
    t->setProperty("vel", engine.noteVelocity.load());
    t->setProperty("serial", (double)engine.noteSerial.load());
    t->setProperty("bpm", p.hostBpm.load());
    t->setProperty("beat", p.hostBeat.load());
    t->setProperty("hostPlaying", p.hostPlaying.load());
    t->setProperty("notes", notes);
    t->setProperty("canUndoJam", p.canUndoMutation());
    auto *search = new juce::DynamicObject();
    search->setProperty("busy", p.patchSearch.busy.load());
    search->setProperty("paused", p.patchSearch.paused.load());
    search->setProperty("progress", p.patchSearch.progress.load());
    search->setProperty("status", p.patchSearch.status.load());
    search->setProperty("revision", (double)p.patchSearch.revision.load());
    t->setProperty("search", search);
    auto *cap = new juce::DynamicObject();
    cap->setProperty("state", p.capture.state.load());
    cap->setProperty("outcome", p.capture.outcome.load());
    cap->setProperty("frames", p.capture.frames.load());
    cap->setProperty("seconds", p.capture.seconds());
    cap->setProperty("peak", p.capture.peak.load());
    cap->setProperty("monitor", p.capture.monitor.load());
    t->setProperty("capture", cap);
    if (orderChanged)
    {
        juce::Array<juce::var> list;
        for (int i = 0; i < rastro::moduleCount; ++i)
        {
            lastOrder[(size_t)i] = order[(size_t)i];
            list.add(order[(size_t)i]);
        }
        t->setProperty("order", list);
    }
    web->emitEventIfBrowserIsVisible("tick", t);
}
#endif
