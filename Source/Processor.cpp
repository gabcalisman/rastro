#include "Processor.h"
#include "Editor.h"
#include "Mp3Decoder.h"
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
    return l;
}
RastroProcessor::RastroProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "RASTRO", layout())
{
    formats.registerBasicFormats();
    keyboard.addListener(this);
}
void RastroProcessor::prepareToPlay(double sr, int)
{
    engine.prepare(sr);
    previousSource = nullptr;
}
bool RastroProcessor::isBusesLayoutSupported(const BusesLayout &b) const
{
    return b.getMainInputChannelSet().isDisabled() &&
           (b.getMainOutputChannelSet() == juce::AudioChannelSet::mono() ||
            b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
rastro::Settings RastroProcessor::settings() const
{
    rastro::Settings p;
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
    if (auto *ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto t = pos->getBpm())
                bpm = (float)*t;
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
                engine.noteOn(m.getNoteNumber(), m.getFloatVelocity(), s, p);
                midiFlash = 8;
            }
            else if (m.isNoteOff())
                engine.noteOff(m.getNoteNumber(), p);
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                engine.allOff();
            else if (m.isController())
            {
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
        float l = 0, r = 0;
        engine.frame(l, r, s, p);
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
void RastroProcessor::loadFileAsync(juce::File f, std::function<void(bool)> complete)
{
    loader.addJob(
        [this, f, complete = std::move(complete)]
        {
            bool ok = loadFile(f);
            juce::MessageManager::callAsync([complete, ok] { complete(ok); });
        });
}
bool RastroProcessor::loadFile(const juce::File &f)
{
    juce::MemoryBlock data;
    if (f.getSize() > 32 * 1024 * 1024 || !f.loadFileAsData(data))
        return false;
    return loadData(data.getData(), data.getSize(), f.getFileName());
}
bool RastroProcessor::loadData(const void *data, size_t size, juce::String name)
{
    if (data == nullptr || size == 0 || size > 32 * 1024 * 1024)
        return false;
    std::unique_ptr<rastro::Sample> sample;
    std::unique_ptr<juce::AudioFormatReader> reader;
    if (!name.endsWithIgnoreCase(".mp3"))
        reader.reset(
            formats.createReaderFor(std::make_unique<juce::MemoryInputStream>(data, size, false)));
    if (reader)
    {
        if (reader->lengthInSamples < 2 || reader->lengthInSamples > reader->sampleRate * 120 ||
            reader->lengthInSamples > 16000000)
            return false;
        juce::AudioBuffer<float> buffer(2, (int)reader->lengthInSamples);
        if (!reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true))
            return false;
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
        return false;
    const size_t bytes = size_t(sample->size()) * 2 * sizeof(float);
    for (auto &channel : sample->data)
        for (auto &f : channel)
            f = std::isfinite(f) ? juce::jlimit(-4.f, 4.f, f) : 0;
    for (int i = 0; i < sample->size(); ++i)
    {
        int bucket = std::min(511, int((int64_t)i * 512 / sample->size()));
        sample->waveform[bucket] =
            std::max(sample->waveform[bucket],
                     std::max(std::abs(sample->data[0][i]), std::abs(sample->data[1][i])));
    }
    juce::MemoryBlock encoded(data, size);
    juce::ScopedLock lock(stateLock);
    if (ownedBytes + bytes > 256 * 1024 * 1024)
        return false;
    auto *ptr = sample.get();
    owned.push_back(std::move(sample));
    ownedBytes += bytes;
    sampleBytes.swapWith(encoded);
    loadedName = name;
    source.store(ptr, std::memory_order_release);
    return true;
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
                float before = value(id);
                mutationBefore[m][i] = before;
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
    tree.setProperty("stateVersion", 2, nullptr);
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
void RastroProcessor::setStateInformation(const void *data, int n)
{
    auto xml = getXmlFromBinary(data, n);
    if (!xml || !xml->hasTagName("RASTRO"))
        return;
    auto tree = juce::ValueTree::fromXml(*xml);
    juce::MemoryBlock bytes;
    bytes.fromBase64Encoding(tree["sample"].toString());
    if (bytes.getSize() > 0)
    {
        if (!loadData(bytes.getData(), bytes.getSize(), tree["sampleName"].toString()))
            return;
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
}
juce::AudioProcessorEditor *RastroProcessor::createEditor() { return new RastroEditor(*this); }
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new RastroProcessor(); }
