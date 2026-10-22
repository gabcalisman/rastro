#include "Processor.h"
#include "Editor.h"
#include "Mp3Decoder.h"
#include "RastroStrings.h"
namespace
{
constexpr int orders[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
}
juce::AudioProcessorValueTreeState::ParameterLayout RastroProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto add = [&](const char *id, float lo, float hi, float def, float step = 0.f)
    {
        l.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, id, juce::NormalisableRange<float>(lo, hi, step), def));
    };
    add("root", 0, 127, 60, 1);
    add("start", 0, .999f, 0);
    add("end", .001f, 1, 1);
    add("loop", 0, 1, 0, 1);
    add("level", 0, 1, .7f);
    add("velocity", 0, 1, .5f);
    add("tempo", 30, 300, 120, 1);
    add("presence", 0, 1, .5f);
    add("trail", 0, 1, .5f);
    add("mutation", 0, 1, .3f);
    add("drywet", 0, 1, 1);
    add("order", 0, 5, 0, 1);
    for (auto id : {"gTime", "gFeedback", "gTone", "gMix", "cRate", "cVariation", "cMix", "bSize",
                    "bTone", "bDecay", "bMix"})
        add(id, 0, 1, juce::String(id).endsWith("Mix") ? .25f : .5f);
    for (auto id : {"gOn", "cOn", "bOn", "gPresent", "cPresent", "bPresent"})
        add(id, 0, 1, 1, 1);
    add("rackOrder", 0, 719, 0, 1);
    add("playX", 0, 1, .5f);
    add("playY", 0, 1, .5f);
    add("animation", 0, 2, 2, 1);
    // Visual-only parameters: persisted/automatable but deliberately absent from Settings.
    add("visualTheme", 0, 3, 0, 1);
    add("visualPalette", 0, 3, 0, 1);
    add("controlStyle", 0, 2, 0, 1);
    add("avatarBody", 0, 2, 0, 1);
    add("avatarEyes", 0, 2, 0, 1);
    add("avatarAccessory", 0, 2, 0, 1);
    add("wDepth", 0, 1, .25f);
    add("wRate", 0, 1, .3f);
    add("wMix", 0, 1, .5f);
    add("dLevel", 0, 1, .15f);
    add("dCharacter", 0, 1, .5f);
    add("dMix", 0, 1, .5f);
    add("kAmount", 0, 1, .3f);
    add("kMix", 0, 1, .3f);
    for (auto id : {"wOn", "dOn", "kOn", "wPresent", "dPresent", "kPresent"})
        add(id, 0, 1, 1, 1);
    add("playMode", 0, 4, 0, 1);
    add("sliceCount", 2, 16, 8, 1);
    for (int i = 1; i < 16; ++i)
    {
        auto id = "slice" + juce::String(i);
        add(id.toRawUTF8(), 0, 1, std::min(1.f, i / 8.f));
    }
    // Append visual parameters only: no range/ID changes to existing host automation.
    add("knobDesign", 0, 4, 0, 1);
    add("uiCompact", 0, 1, 0, 1);
    for (auto id : {"visualBackground", "visualAccent", "visualControls", "avatarColour"})
        add(id, 0, 4, 0, 1);
    for (auto id : {"avatarHair", "avatarHorns", "avatarClothes", "avatarGlasses", "avatarTail",
                    "avatarWings", "avatarInstrument"})
        add(id, 0, 2, 0, 1);
    add("colourSet", 0, 8, 0, 1);
    add("pitchMode", 0, 1, 0, 1);
    add("cosmicArcade", 0, 1, 0, 1);
    add("gTiming", 0, 2, 2, 1);
    add("cTiming", 0, 2, 2, 1);
    add("wTiming", 0, 2, 1, 1);
    for (auto id : {"gDivision", "cDivision", "wDivision"})
        add(id, 0, 3, 1, 1);
    add("gFreeMs", 5, 2000, 250);
    add("cFreeHz", .5f, 40, 8);
    add("wFreeHz", .05f, 12, 1.005f);
    add("lfoTiming", 0, 2, 1, 1);
    add("lfoDivision", 0, 3, 0, 1);
    add("avatarPresentation", 0, 2, 0, 1);
    add("xyCrumble", 0, 1, 1, 1);
    add("regionLoop", 0, 1, 0, 1);
    add("loopRegionSet", 0, 1, 0, 1);
    add("loopStart", 0, .999f, 0);
    add("loopEnd", .001f, 1, 1);
    add("loopCrossfade", 0, 50, 5);
    add("synthEnable", 0, 1, 0, 1);
    add("ampAttack", .001f, 4, .01f);
    add("ampDecay", .001f, 4, .2f);
    add("ampSustain", 0, 1, .75f);
    add("ampRelease", .001f, 8, .2f);
    add("filterCutoff", 20, 20000, 16000);
    add("filterResonance", 0, .95f, .1f);
    add("envAmount", -1, 1, 0);
    add("envTarget", 0, 1, 0, 1);
    add("lfoRate", .05f, 12, 1);
    add("lfoDepth", 0, 1, 0);
    add("lfoTarget", 0, 1, 0, 1);
    add("lfoSync", 0, 3, 0, 1);
    add("voiceMode", 0, 2, 0, 1);
    add("glide", 0, 2, 0);
    add("bendRange", 0, 12, 2);
    add("triggerMode", 0, 2, 0, 1);
    add("avatarHalo", 0, 1, 0, 1);
    return l;
}
RastroProcessor::RastroProcessor(bool directCaptureForTests)
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "RASTRO", layout())
{
    directInput = directCaptureForTests || wrapperType == wrapperType_Standalone;
    if (directInput)
    {
        addBus(true);
        enableAllBuses();
    }
    formats.registerBasicFormats();
    keyboard.addListener(this);
}
void RastroProcessor::prepareToPlay(double sr, int)
{
    engine.prepare(sr);
    if (directInput)
        capture.prepare(sr);
    monitorGain = 0;
    previousSource = nullptr;
}
bool RastroProcessor::isBusesLayoutSupported(const BusesLayout &b) const
{
    return (b.getMainInputChannelSet().isDisabled() ||
            (directInput && b.getMainInputChannelSet().size() <= 2)) &&
           (b.getMainOutputChannelSet() == juce::AudioChannelSet::mono() ||
            b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
rastro::Settings RastroProcessor::settings() const
{
    rastro::Settings p;
    for (int i = 0; i < 3; ++i)
    {
        p.timingMode[i] = int(value(rastro::timingIds[i]));
        p.division[i] = int(value(rastro::divisionIds[i]));
    }
    p.ghostMs = value("gFreeMs");
    p.crumbleHz = value("cFreeHz");
    p.wobbleHz = value("wFreeHz");
    p.lfoTiming = int(value("lfoTiming"));
    p.lfoDivision = int(value("lfoDivision"));
    p.xyCrumble = value("xyCrumble") > .5f;
    p.regionLoop = value("regionLoop") > .5f;
    p.loopRegionSet = value("loopRegionSet") > .5f;
    p.loopStart = value("loopStart");
    p.loopEnd = value("loopEnd");
    p.loopCrossfade = value("loopCrossfade");
    p.synth = value("synthEnable") > .5f;
    p.attack = value("ampAttack");
    p.decay = value("ampDecay");
    p.sustain = value("ampSustain");
    p.release = value("ampRelease");
    p.cutoff = value("filterCutoff");
    p.resonance = value("filterResonance");
    p.envAmount = value("envAmount");
    p.envTarget = (int)value("envTarget");
    p.lfoRate = value("lfoRate");
    p.lfoDepth = value("lfoDepth");
    p.lfoTarget = (int)value("lfoTarget");
    p.lfoSync = (int)value("lfoSync");
    p.voiceMode = (int)value("voiceMode");
    p.glide = value("glide");
    p.bendRange = value("bendRange");
    p.triggerMode = (int)value("triggerMode");
    p.root = value("root");
    p.start = value("start");
    p.end = value("end");
    p.loop = value("loop");
    p.level = value("level");
    p.velocity = value("velocity");
    p.tempo = value("tempo");
    p.presence = value("presence");
    p.trail = value("trail");
    p.mutation = value("mutation");
    p.drywet = value("drywet");
    p.ghost = {value("gTime"), value("gFeedback"), value("gTone"), value("gMix")};
    p.crumble = {value("cRate"), value("cVariation"), value("cMix")};
    p.bloom = {value("bSize"), value("bTone"), value("bDecay"), value("bMix")};
    p.wobble = {value("wDepth"), value("wRate"), value("wMix")};
    p.dust = {value("dLevel"), value("dCharacter"), value("dMix")};
    p.crush = {value("kAmount"), value("kMix")};
    p.mode = (int)value("playMode");
    p.pitchMode = (int)value("pitchMode");
    p.sliceCount = (int)value("sliceCount");
    // Static ID table avoids temporary Strings in the callback.
    const char *cuts[]{"slice1",  "slice2",  "slice3",  "slice4",  "slice5",
                       "slice6",  "slice7",  "slice8",  "slice9",  "slice10",
                       "slice11", "slice12", "slice13", "slice14", "slice15"};
    p.cuts[0] = 0;
    for (int i = 1; i < p.sliceCount; ++i)
        p.cuts[i] = std::clamp(value(cuts[i - 1]), p.cuts[i - 1] + .0001f,
                               1.f - (p.sliceCount - i) * .0001f);
    p.cuts[p.sliceCount] = 1;
    p.playX = value("playX");
    p.playY = value("playY");
    for (int m = 0; m < rastro::moduleCount; ++m)
        p.enabled[m] = value(rastro::onIds[m]) > .5f && value(rastro::presentIds[m]) > .5f;
    int legacy = juce::jlimit(0, 5, (int)value("order"));
    auto permutation = rastro::decodeOrder((int)value("rackOrder"));
    for (int i = 0; i < rastro::moduleCount; ++i)
    {
        int m = permutation[i];
        p.order[i] = m < 3 ? orders[legacy][m] : m;
    }
    return p;
}
void RastroProcessor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals no;
    const int captureInputs = std::min(getTotalNumInputChannels(), b.getNumChannels());
    if (directInput)
        capture.process(b.getArrayOfReadPointers(), captureInputs, b.getNumSamples());
    if (captureInputs == 0)
        b.clear();
    if (b.getNumChannels() == 0)
        return;
    auto p = settings();
    float *macros[]{&p.presence, &p.trail, &p.mutation, &p.playX, &p.playY};
    for (int j = 0; j < 5; ++j)
    {
        float v = ccPending[j].load();
        if (v >= 0)
            *macros[j] = v;
    }
    float bpm = 0;
    hostBeat = -1;
    hostPlaying = false;
    if (auto *ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (wasHostPlaying && !pos->getIsPlaying())
                engine.allOff();
            wasHostPlaying = pos->getIsPlaying();
            hostPlaying = wasHostPlaying;
            if (auto beat = pos->getPpqPosition())
            {
                hostBeat = float(std::fmod(*beat, 4.));
                p.transportBeat = *beat;
                p.transportValid = true;
                p.transportPlaying = wasHostPlaying;
            }
            if (auto t = pos->getBpm())
                bpm = (float)*t;
        }
    hostBpm = bpm;
    if (bpm > 0)
        p.tempo = bpm;
    auto *s =
        source.load(std::memory_order_acquire); // Retained by loader until processor destruction.
    if (s != previousSource)
    {
        engine.clearVoices();
        previousSource = s;
    }
    const bool queueOverflow = overflow.exchange(false);
    auto rd = keyRead.load();
    auto wr = keyWrite.load(std::memory_order_acquire);
    while (rd != wr)
    {
        auto e = keyEvents[rd++ % 256];
        if (e.on)
        {
            engine.noteOn(e.note, e.velocity, s, p);
            midiFlash = 8;
        }
        else
            engine.noteOff(e.note, p);
    }
    keyRead.store(rd, std::memory_order_release);
    if (queueOverflow || overflow.exchange(false))
        engine.allOff();
    int preview = previewRequest.exchange(0);
    if (preview > 0)
    {
        auto audition = p;
        audition.mode = 0;
        audition.loop = 0;
        audition.regionLoop = false;
        audition.triggerMode = 1;
        audition.root = 255;
        engine.noteOn(255, 1, s, audition);
    }
    else if (preview < 0)
        engine.noteOff(255, p);
    auto it = midi.cbegin();
    auto end = midi.cend();
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        while (it != end && (*it).samplePosition <= i)
        {
            auto meta = *it;
            if (meta.numBytes > 3)
            {
                ++it;
                continue;
            }
            auto m = meta.getMessage();
            if (m.isNoteOn())
            {
                engine.noteOn(m.getNoteNumber(), m.getFloatVelocity(), s, p, m.getChannel());
                midiFlash = 8;
            }
            else if (m.isNoteOff())
                engine.noteOff(m.getNoteNumber(), p, m.getChannel());
            else if (m.isPitchWheel())
                engine.pitchBend(m.getChannel(),
                                 (m.getPitchWheelValue() - 8192) /
                                     (m.getPitchWheelValue() < 8192 ? 8192.f : 8191.f));
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                engine.allOff();
            else if (m.isController())
            {
                if (m.getControllerNumber() == 64)
                    engine.sustainPedal(m.getChannel(), m.getControllerValue() >= 64, p);
                if (m.getControllerNumber() == 1)
                    engine.modWheel(m.getChannel(), m.getControllerValue() / 127.f);
                int learning = learn.exchange(-1);
                if (learning >= 0 && learning < 5)
                    cc[learning] = m.getControllerNumber();
                for (int j = 0; j < 5; ++j)
                    if (cc[j] == m.getControllerNumber())
                    {
                        float v = m.getControllerValue() /
                                  127.f; // Defer host notifications to non-realtime code.
                        ccPending[j].store(v);
                        if (j == 0)
                            p.presence = v;
                        if (j == 1)
                            p.trail = v;
                        if (j == 2)
                            p.mutation = v;
                        if (j == 3)
                            p.playX = v;
                        if (j == 4)
                            p.playY = v;
                    }
            }
            ++it;
        }
        float inputL = captureInputs > 0 ? b.getSample(0, i) : 0,
              inputR = captureInputs > 1 ? b.getSample(1, i) : inputL;
        if (!std::isfinite(inputL))
            inputL = 0;
        if (!std::isfinite(inputR))
            inputR = 0;
        monitorGain += (capture.monitor.load() ? .1f - monitorGain : -monitorGain) * .01f;
        float l = 0, r = 0;
        engine.frame(l, r, s, p);
        if (p.transportValid && p.transportPlaying)
            p.transportBeat += p.tempo / (60 * getSampleRate());
        if (captureInputs > 0)
        {
            l = std::clamp(l + inputL * monitorGain, -1.f, 1.f);
            r = std::clamp(r + inputR * monitorGain, -1.f, 1.f);
        }
        if (b.getNumChannels() == 1)
            b.setSample(0, i, (l + r) * .5f);
        else
        {
            b.setSample(0, i, l);
            b.setSample(1, i, r);
        }
    }
    engine.endBlock();
    midi.clear();
}
void RastroProcessor::importFileAsync(juce::File file,
                                      std::function<void(rastro::ImportError)> complete)
{
    loader.addJob(
        [this, file, complete = std::move(complete)]
        {
            auto result = importFile(file);
            importError = result;
            juce::MessageManager::callAsync([complete, result] { complete(result); });
        });
}
void RastroProcessor::loadFileAsync(juce::File file, std::function<void(bool)> complete)
{
    importFileAsync(file, [complete = std::move(complete)](rastro::ImportError result)
                    { complete(result == rastro::ImportError::none); });
}
bool RastroProcessor::loadFile(const juce::File &file)
{
    auto result = importFile(file);
    importError = result;
    return result == rastro::ImportError::none;
}
bool RastroProcessor::loadData(const void *data, size_t size, juce::String name)
{
    auto result = importData(data, size, std::move(name));
    importError = result;
    return result == rastro::ImportError::none;
}
rastro::ImportError RastroProcessor::importFile(const juce::File &file)
{
    using E = rastro::ImportError;
    if (rastro::editableRecording(file))
        return E::editableMovie;
    if (!file.existsAsFile())
        return E::unreadable;
    if (file.getSize() > 32 * 1024 * 1024)
        return E::encodedLimit;
    juce::MemoryBlock bytes;
    if (!file.loadFileAsData(bytes))
        return E::unreadable;
    return importData(bytes.getData(), bytes.getSize(), file.getFileName());
}
rastro::ImportError RastroProcessor::importData(const void *data, size_t size, juce::String name)
{
    using E = rastro::ImportError;
    if (name.endsWithIgnoreCase(".mov") || name.endsWithIgnoreCase(".qt"))
        return E::editableMovie;
    if (!data || size == 0)
        return E::corrupt;
    if (size > 32 * 1024 * 1024)
        return E::encodedLimit;
    std::unique_ptr<rastro::Sample> sample;
    auto *bytes = static_cast<const uint8_t *>(data);
    bool mp4 =
        name.endsWithIgnoreCase(".m4a") || (size >= 12 && std::memcmp(bytes + 4, "ftyp", 4) == 0);
    if (mp4)
    {
        E error;
        sample = rastro::decodeM4a(data, size, formats, error);
        if (!sample)
            return error;
    }
    else
    {
        std::unique_ptr<juce::AudioFormatReader> reader;
        if (!name.endsWithIgnoreCase(".mp3"))
            reader.reset(formats.createReaderFor(
                std::make_unique<juce::MemoryInputStream>(data, size, false)));
        if (reader)
        {
            if (!std::isfinite(reader->sampleRate) || reader->sampleRate < 8000 ||
                reader->sampleRate > 384000 || reader->numChannels < 1 ||
                reader->lengthInSamples < 2)
                return E::corrupt;
            if (reader->lengthInSamples > reader->sampleRate * 120 ||
                reader->lengthInSamples > 16000000)
                return E::durationLimit;
            juce::AudioBuffer<float> buffer(2, (int)reader->lengthInSamples);
            if (!reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true))
                return E::corrupt;
            sample = std::make_unique<rastro::Sample>();
            sample->rate = reader->sampleRate;
            for (int c = 0; c < 2; ++c)
            {
                auto *ptr = buffer.getReadPointer(reader->numChannels == 1 ? 0 : c);
                sample->data[c].assign(ptr, ptr + buffer.getNumSamples());
            }
        }
        else
            sample = rastro::decodeMp3(data, size);
        if (!sample)
            return (name.endsWithIgnoreCase(".wav") || name.endsWithIgnoreCase(".aiff") ||
                    name.endsWithIgnoreCase(".aif") || name.endsWithIgnoreCase(".mp3"))
                       ? E::corrupt
                       : E::unsupportedFormat;
    }
    for (auto &channel : sample->data)
        for (auto &f : channel)
            f = std::isfinite(f) ? juce::jlimit(-4.f, 4.f, f) : 0;
    sample->buildPeaks();
    const size_t decodedBytes = size_t(sample->size()) * 2 * sizeof(float) + sample->peakBytes();
    for (int i = 0; i < sample->size(); ++i)
    {
        int bucket = std::min(511, int((int64_t)i * 512 / sample->size()));
        sample->waveform[bucket] =
            std::max(sample->waveform[bucket],
                     std::max(std::abs(sample->data[0][i]), std::abs(sample->data[1][i])));
    }
    juce::MemoryBlock encoded(data, size);
    juce::ScopedLock lock(stateLock);
    if (ownedBytes + decodedBytes > 256 * 1024 * 1024)
        return E::memoryLimit;
    auto *ptr = sample.get();
    owned.push_back(std::move(sample));
    ownedBytes += decodedBytes;
    sampleBytes.swapWith(encoded);
    loadedName = name;
    source.store(ptr, std::memory_order_release);
    return E::none;
}
juce::String RastroProcessor::importMessage(rastro::ImportError error) const
{
    using E = rastro::ImportError;
    const char *key = error == E::unreadable           ? "importRead"
                      : error == E::corrupt            ? "importCorrupt"
                      : error == E::unsupportedFormat  ? "importFormat"
                      : error == E::incompatibleCodec  ? "importCodec"
                      : error == E::editableMovie      ? "importMovie"
                      : error == E::encodedLimit       ? "importEncoded"
                      : error == E::durationLimit      ? "importDuration"
                      : error == E::memoryLimit        ? "importMemory"
                      : error == E::decoderUnavailable ? "importBackend"
                      : error == E::decodeFailed       ? "importDecode"
                                                       : "importSuccess";
    return text::get(key, language);
}
juce::String RastroProcessor::sampleName()
{
    juce::ScopedLock lock(stateLock);
    return loadedName;
}
void RastroProcessor::set(const juce::String &id, float v)
{
    if (auto *p = params.getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}
void RastroProcessor::setPlaybackMode(int mode)
{
    set("playMode", (float)juce::jlimit(0, 4, mode));
    set("loop", mode == 1 ? 1.f : 0.f);
}
void RastroProcessor::equalSlices(int count)
{
    count = juce::jlimit(2, 16, count);
    set("sliceCount", (float)count);
    for (int i = 1; i < 16; ++i)
        set("slice" + juce::String(i), std::min(1.f, i / (float)count));
}
void RastroProcessor::moveModule(int from, int to)
{
    if (from < 0 || to < 0 || from >= rastro::moduleCount || to >= rastro::moduleCount ||
        from == to)
        return;
    auto order = settings().order;
    const int moved = order[from];
    if (from < to)
        for (int i = from; i < to; ++i)
            order[i] = order[i + 1];
    else
        for (int i = from; i > to; --i)
            order[i] = order[i - 1];
    order[to] = moved;
    int legacy = juce::jlimit(0, 5, (int)value("order"));
    for (auto &m : order)
        if (m < 3)
            for (int i = 0; i < 3; ++i)
                if (orders[legacy][i] == m)
                {
                    m = i;
                    break;
                }
    set("rackOrder", (float)rastro::encodeOrder(order));
}
void RastroProcessor::mutate()
{
    juce::ScopedLock lock(stateLock);
    float amount = value("mutation");
    if (amount <= 0)
        return;
    auto active = settings().enabled;
    if (std::none_of(active.begin(), active.end(), [](bool enabled) { return enabled; }))
        return;
    mutationAffected = {};
    bool any = false;
    auto &random = juce::Random::getSystemRandom();
    for (int m = 0; m < rastro::moduleCount; ++m)
    {
        if (value(rastro::onIds[m]) < .5f || value(rastro::presentIds[m]) < .5f)
            continue;
        any = mutationAffected[m] = true;
        for (int i = 0; i < 4; ++i)
            if (auto id = rastro::controlIds[m][i])
            {
                int slot = rastro::timingSlot(m);
                float before = value(id);
                mutationBefore[m][i] = before;
                if (slot >= 0 && ((m == 3 && i == 1) || (m != 3 && i == 0)) &&
                    value(rastro::timingIds[slot]) > 0)
                    continue;
                bool isMix = juce::String(id).endsWith("Mix");
                float max = isMix ? .65f : .8f;
                if (juce::String(id) == "dLevel")
                    max = .55f;
                float target = isMix ? .2f + random.nextFloat() * .4f
                                     : .12f + random.nextFloat() * (max - .12f);
                if (std::abs(target - before) < .12f)
                    target = before < max * .5f ? max : .12f;
                set(id, juce::jlimit(0.f, max, before + (target - before) * (.35f + .6f * amount)));
            }
    }
    mutationUndoAvailable = any;
}
void RastroProcessor::undoMutation()
{
    juce::ScopedLock lock(stateLock);
    if (!mutationUndoAvailable.exchange(false))
        return;
    for (int m = 0; m < rastro::moduleCount; ++m)
        if (mutationAffected[m])
            for (int i = 0; i < 4; ++i)
                if (auto id = rastro::controlIds[m][i])
                    set(id, mutationBefore[m][i]);
}
void RastroProcessor::resetEffects()
{
    juce::ScopedLock lock(stateLock);
    mutationUndoAvailable = false;
    for (auto id :
         {"gTime", "gFeedback", "gTone", "cRate", "cVariation", "bSize", "bTone", "bDecay"})
        set(id, .5f);
    for (auto id : {"gMix", "cMix", "bMix"})
        set(id, .25f);
    for (auto id : {"gTiming", "cTiming", "wTiming", "gDivision", "cDivision", "wDivision",
                    "gFreeMs", "cFreeHz", "wFreeHz"})
    {
        auto *param = params.getParameter(id);
        set(id, param->convertFrom0to1(param->getDefaultValue()));
    }
    set("wDepth", .25f);
    set("wRate", .3f);
    set("wMix", .5f);
    set("dLevel", .15f);
    set("dCharacter", .5f);
    set("dMix", .5f);
    set("kAmount", .3f);
    set("kMix", .3f);
    for (auto id : rastro::onIds)
        set(id, 1);
    set("presence", .5f);
    set("trail", .5f);
    set("mutation", .3f);
    set("drywet", 1);
    set("playX", .5f);
    set("playY", .5f);
}
void RastroProcessor::applyCC()
{
    const auto &ids = rastro::macroIds;
    for (int j = 0; j < 5; ++j)
    {
        float v = ccPending[j].exchange(-1);
        if (v >= 0)
            set(ids[j], v);
    }
}
void RastroProcessor::getStateInformation(juce::MemoryBlock &dest)
{
    applyCC();
    juce::ScopedLock lock(stateLock);
    auto tree = params.copyState();
    tree.setProperty("stateVersion", 4, nullptr);
    auto previous = tree.getChildWithName("MutationUndo");
    if (previous.isValid())
        tree.removeChild(previous, nullptr);
    juce::ValueTree undo("MutationUndo");
    undo.setProperty("available", mutationUndoAvailable.load(), nullptr);
    for (int m = 0; m < rastro::moduleCount; ++m)
        if (mutationAffected[m])
            for (int i = 0; i < 4; ++i)
                if (auto id = rastro::controlIds[m][i])
                    undo.setProperty(id, mutationBefore[m][i], nullptr);
    tree.addChild(undo, -1, nullptr);
    tree.setProperty("language", language.load(), nullptr);
    tree.setProperty("sampleName", loadedName, nullptr);
    tree.setProperty("sample", sampleBytes.toBase64Encoding(), nullptr);
    for (int j = 0; j < 5; ++j)
        tree.setProperty("cc" + juce::String(j), cc[j].load(), nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary(*xml, dest);
}
void RastroProcessor::setStateInformation(const void *data, int n) { restoreState(data, n); }
bool RastroProcessor::restoreState(const void *data, int n)
{
    if (!data || n <= 0 || n > 64 * 1024 * 1024)
        return false;
    auto xml = getXmlFromBinary(data, n);
    if (!xml || !xml->hasTagName("RASTRO"))
        return false;
    auto tree = juce::ValueTree::fromXml(*xml);
    juce::MemoryBlock bytes;
    bytes.fromBase64Encoding(tree["sample"].toString());
    if (bytes.getSize() > 0)
    {
        if (!loadData(bytes.getData(), bytes.getSize(), tree["sampleName"].toString()))
            return false;
    }
    else
    {
        source = nullptr;
        juce::ScopedLock lock(stateLock);
        sampleBytes.reset();
        loadedName = {};
    }
    // Explicit defaults make restoration deterministic even when this instance had edits.
    const bool oldVersion = (int)tree.getProperty("stateVersion", 1) < 2;
    for (auto *parameter : getParameters())
        if (auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(parameter))
        {
            auto id = ranged->paramID;
            if (!tree.getChildWithProperty("id", id).isValid())
            {
                juce::ValueTree node("PARAM");
                node.setProperty("id", id, nullptr);
                float v = ranged->convertFrom0to1(ranged->getDefaultValue());
                if (id == "avatarHalo")
                {
                    auto accessory = tree.getChildWithProperty("id", "avatarAccessory");
                    v = !accessory.isValid() || int(accessory["value"]) == 0 ? 1.f : 0.f;
                }
                if (id == "xyCrumble" || id == "gTiming" || id == "cTiming" || id == "wTiming" ||
                    id == "lfoTiming")
                    v = 0;
                if (oldVersion && (id == "wOn" || id == "dOn" || id == "kOn"))
                    v = 0;
                node.setProperty("value", v, nullptr);
                tree.addChild(node, -1, nullptr);
            }
        }
    params.replaceState(tree);
    for (auto &pending : ccPending)
        pending = -1;
    learn = -1;
    mutationAffected = {};
    auto undo = tree.getChildWithName("MutationUndo");
    mutationUndoAvailable = undo.isValid() && (bool)undo.getProperty("available", false);
    for (int m = 0; m < rastro::moduleCount; ++m)
        for (int i = 0; i < 4; ++i)
            if (auto id = rastro::controlIds[m][i])
                if (undo.hasProperty(id))
                {
                    mutationAffected[m] = true;
                    mutationBefore[m][i] = juce::jlimit(0.f, 1.f, (float)undo[id]);
                }
    language = juce::jlimit(0, 1, (int)tree["language"]);
    for (int j = 0; j < 5; ++j)
        cc[j] = juce::jlimit(-1, 127, (int)tree.getProperty("cc" + juce::String(j), -1));
    return true;
}
juce::AudioProcessorEditor *RastroProcessor::createEditor() { return new RastroEditor(*this); }
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new RastroProcessor(); }

void RastroProcessor::beginCapture(std::function<void(bool)> complete)
{
    if (!directInput || !capture.start())
    {
        if (complete)
            complete(false);
        return;
    }
    loader.addJob(
        [this, complete = std::move(complete)]
        {
            while (!shuttingDown && capture.state != rastro::CaptureRecorder::ready)
            {
                capture.finishStop();
                juce::Thread::sleep(5);
            }
            bool ok = false;
            if (!shuttingDown && capture.claim())
            {
                juce::MemoryBlock bytes;
                auto result = rastro::finishCapture(capture, bytes);
                ok = result == rastro::CaptureRecorder::success &&
                     loadData(bytes.getData(), bytes.getSize(), "Capture.wav");
                capture.outcome = ok ? rastro::CaptureRecorder::success
                                  : result == rastro::CaptureRecorder::success
                                      ? rastro::CaptureRecorder::encodeError
                                      : result;
                capture.release();
            }
            juce::MessageManager::callAsync(
                [complete, ok]
                {
                    if (complete)
                        complete(ok);
                });
        });
}
void RastroProcessor::listCaptures(std::function<void(std::vector<juce::File>)> complete)
{
    loader.addJob(
        [complete = std::move(complete)]
        {
            auto files = rastro::captureFiles();
            juce::MessageManager::callAsync(
                [complete, files = std::move(files)]() mutable
                {
                    if (complete)
                        complete(std::move(files));
                });
        });
}

rastro::PatchValues RastroProcessor::patchValues() const
{
    rastro::PatchValues values{};
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = value(rastro::patchIds[i]);
    return values;
}
void RastroProcessor::applyPatch(const rastro::PatchValues &values)
{
    for (size_t i = 0; i < values.size(); ++i)
        set(rastro::patchIds[i], values[i]);
}
void RastroProcessor::patchFileAsync(juce::File file, bool save, std::function<void(bool)> complete)
{
    loader.addJob(
        [this, file, save, complete = std::move(complete)]
        {
            bool ok = false;
            if (save)
            {
                juce::MemoryBlock data;
                getStateInformation(data);
                if (data.getSize() <= 64 * 1024 * 1024)
                    ok = file.replaceWithData(data.getData(), data.getSize());
            }
            else if (file.getSize() > 0 && file.getSize() <= 64 * 1024 * 1024)
            {
                juce::MemoryBlock data;
                if (file.loadFileAsData(data))
                    ok = restoreState(data.getData(), int(data.getSize()));
            }
            juce::MessageManager::callAsync(
                [complete, ok]
                {
                    if (complete)
                        complete(ok);
                });
        });
}

void RastroProcessor::pollCaptureLinks(
    std::function<void(std::vector<juce::File>, std::vector<rastro::CaptureLink>)> complete)
{
    if (bridgePollPending.exchange(true))
        return;
    loader.addJob(
        [this, complete = std::move(complete)]
        {
            auto files = rastro::captureFiles();
            auto links = rastro::readCaptureLinks();
            bridgePollPending = false;
            juce::MessageManager::callAsync(
                [complete, files = std::move(files), links = std::move(links)]() mutable
                {
                    if (complete)
                        complete(std::move(files), std::move(links));
                });
        });
}
void RastroProcessor::remoteCapture(juce::String id, bool start, std::function<void(bool)> complete)
{
    loader.addJob(
        [id, start, complete = std::move(complete)]
        {
            bool ok = rastro::requestCapture(id, start);
            juce::MessageManager::callAsync(
                [complete, ok]
                {
                    if (complete)
                        complete(ok);
                });
        });
}
