#include "Processor.h"
#include "Editor.h"
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
    p.enabled = {value("gOn") > .5f && value("gPresent") > .5f,
                 value("cOn") > .5f && value("cPresent") > .5f,
                 value("bOn") > .5f && value("bPresent") > .5f};
    int o = juce::jlimit(0, 5, (int)value("order"));
    for (int i = 0; i < 3; ++i)
        p.order[i] = orders[o][i];
    return p;
}
void RastroProcessor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals no;
    b.clear();
    if (b.getNumChannels() == 0)
        return;
    auto p = settings();
    float *macros[]{&p.presence, &p.trail, &p.mutation};
    for (int j = 0; j < 3; ++j)
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
    if (overflow.exchange(false))
        engine.allOff();
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
            engine.noteOff(e.note);
    }
    keyRead.store(rd, std::memory_order_release);
    if (overflow.exchange(false))
        engine.allOff();
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
                engine.noteOff(m.getNoteNumber());
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                engine.allOff();
            else if (m.isController())
            {
                int learning = learn.exchange(-1);
                if (learning >= 0 && learning < 3)
                    cc[learning] = m.getControllerNumber();
                for (int j = 0; j < 3; ++j)
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
    midi.clear();
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
    juce::ScopedLock lock(stateLock);
    if (size == 0 || size > 32 * 1024 * 1024)
        return false;
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(std::make_unique<juce::MemoryInputStream>(data, size, false)));
    if (!reader || reader->lengthInSamples < 2 ||
        reader->lengthInSamples > reader->sampleRate * 120 || reader->lengthInSamples > 16000000)
        return false;
    auto bytes = (size_t)reader->lengthInSamples * 2 * sizeof(float);
    if (ownedBytes + bytes > 256 * 1024 * 1024)
        return false;
    juce::AudioBuffer<float> buffer(2, (int)reader->lengthInSamples);
    if (!reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true))
        return false;
    auto sample = std::make_unique<rastro::Sample>();
    sample->rate = reader->sampleRate;
    for (int c = 0; c < 2; ++c)
    {
        const float *ptr = buffer.getReadPointer(reader->numChannels == 1 ? 0 : c);
        sample->data[c].assign(ptr, ptr + buffer.getNumSamples());
        for (auto &f : sample->data[c])
            f = std::isfinite(f) ? juce::jlimit(-4.f, 4.f, f) : 0;
    }
    auto *ptr = sample.get();
    owned.push_back(std::move(sample));
    ownedBytes += bytes;
    sampleBytes.replaceAll(data, size);
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
void RastroProcessor::moveModule(int from, int to)
{
    auto p = settings();
    auto order = p.order;
    std::swap(order[from], order[to]);
    for (int o = 0; o < 6; ++o)
        if (std::equal(order.begin(), order.end(), orders[o]))
            set("order", (float)o);
}
void RastroProcessor::mutate()
{
    auto &rng = juce::Random::getSystemRandom();
    float amt = value("mutation");
    for (auto id :
         {"gTime", "gFeedback", "gTone", "cRate", "cVariation", "bSize", "bTone", "bDecay"})
        set(id, juce::jlimit(.1f, .8f, value(id) + (rng.nextFloat() - .5f) * amt * .5f));
}
void RastroProcessor::resetEffects()
{
    for (auto id :
         {"gTime", "gFeedback", "gTone", "cRate", "cVariation", "bSize", "bTone", "bDecay"})
        set(id, .5f);
    for (auto id : {"gMix", "cMix", "bMix"})
        set(id, .25f);
    for (auto id : {"gOn", "cOn", "bOn"})
        set(id, 1);
    set("presence", .5f);
    set("trail", .5f);
    set("mutation", .3f);
    set("drywet", 1);
}
void RastroProcessor::applyCC()
{
    const char *ids[]{"presence", "trail", "mutation"};
    for (int j = 0; j < 3; ++j)
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
    tree.setProperty("language", language.load(), nullptr);
    tree.setProperty("sampleName", loadedName, nullptr);
    tree.setProperty("sample", sampleBytes.toBase64Encoding(), nullptr);
    for (int j = 0; j < 3; ++j)
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
    params.replaceState(tree);
    language = juce::jlimit(0, 1, (int)tree["language"]);
    for (int j = 0; j < 3; ++j)
        cc[j] = juce::jlimit(-1, 127, (int)tree.getProperty("cc" + juce::String(j), -1));
}
juce::AudioProcessorEditor *RastroProcessor::createEditor() { return new RastroEditor(*this); }
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new RastroProcessor(); }
