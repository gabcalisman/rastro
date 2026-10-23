#include "RecordingImport.h"
#include <cstring>
namespace rastro
{
namespace
{
uint32_t be32(const uint8_t *p)
{
    return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}
uint64_t be64(const uint8_t *p) { return uint64_t(be32(p)) << 32 | be32(p + 4); }
constexpr uint32_t tag(char a, char b, char c, char d)
{
    return uint32_t(a) << 24 | uint32_t(b) << 16 | uint32_t(c) << 8 | uint32_t(d);
}
struct Inspector
{
    const uint8_t *data;
    bool ftyp = false, moov = false, mdat = false, quickTime = false, codec = false,
         badCodec = false;
    int audioTracks = 0, videoTracks = 0, atoms = 0;
    RecordingInfo info;
    bool scan(size_t begin, size_t end, int depth)
    {
        if (depth > 8)
            return false;
        for (size_t at = begin; at < end;)
        {
            if (++atoms > 100000 || end - at < 8)
                return false;
            uint64_t size = be32(data + at);
            size_t header = 8;
            if (size == 1)
            {
                if (end - at < 16)
                    return false;
                size = be64(data + at + 8);
                header = 16;
            }
            if (size == 0)
                size = end - at;
            if (size < header || size > end - at)
                return false;
            auto type = be32(data + at + 4);
            size_t payload = at + header, length = size - header;
            if (type == tag('f', 't', 'y', 'p'))
            {
                if (length < 8)
                    return false;
                ftyp = true;
                quickTime |= be32(data + payload) == tag('q', 't', ' ', ' ');
            }
            else if (type == tag('m', 'd', 'a', 't'))
                mdat |= length > 0;
            else if (type == tag('h', 'd', 'l', 'r'))
            {
                if (length < 12)
                    return false;
                auto handler = be32(data + payload + 8);
                if (handler == tag('s', 'o', 'u', 'n'))
                    ++audioTracks;
                if (handler == tag('v', 'i', 'd', 'e'))
                    ++videoTracks;
            }
            else if (type == tag('s', 't', 's', 'd'))
            {
                if (length < 8)
                    return false;
                uint32_t count = be32(data + payload + 4);
                size_t entry = payload + 8;
                if (count == 0 || count > 128)
                    return false;
                for (uint32_t i = 0; i < count; ++i)
                {
                    if (at + size - entry < 8)
                        return false;
                    uint32_t bytes = be32(data + entry), format = be32(data + entry + 4);
                    if (bytes < 8 || bytes > at + size - entry)
                        return false;
                    if (bytes < 36)
                        return false;
                    auto channels = (uint16_t(data[entry + 24]) << 8) | data[entry + 25];
                    if (channels < 1 || channels > 2)
                        badCodec = true;
                    if (format == tag('m', 'p', '4', 'a'))
                        info.aac = codec = true;
                    else if (format == tag('a', 'l', 'a', 'c'))
                        info.alac = codec = true;
                    else
                        badCodec = true;
                    entry += bytes;
                }
            }
            else if (type == tag('m', 'd', 'h', 'd'))
            {
                if (length < 20)
                    return false;
                int version = data[payload];
                uint32_t timescale = 0;
                uint64_t duration = 0;
                if (version == 0)
                {
                    timescale = be32(data + payload + 12);
                    duration = be32(data + payload + 16);
                }
                else if (version == 1)
                {
                    if (length < 32)
                        return false;
                    timescale = be32(data + payload + 20);
                    duration = be64(data + payload + 24);
                }
                else
                    return false;
                if (timescale)
                    info.duration = std::max(info.duration, double(duration) / timescale);
            }
            if (type == tag('m', 'o', 'o', 'v') || type == tag('t', 'r', 'a', 'k') ||
                type == tag('m', 'd', 'i', 'a') || type == tag('m', 'i', 'n', 'f') ||
                type == tag('s', 't', 'b', 'l'))
            {
                if (type == tag('m', 'o', 'o', 'v'))
                    moov = true;
                if (!scan(payload, at + size, depth + 1))
                    return false;
            }
            at += size;
        }
        return true;
    }
};
std::unique_ptr<Sample> readNative(juce::AudioFormatReader &reader, ImportError &error)
{
    if (!std::isfinite(reader.sampleRate) || reader.sampleRate < 8000 ||
        reader.sampleRate > 384000 || reader.numChannels < 1 || reader.numChannels > 2 ||
        reader.lengthInSamples < 2)
    {
        error = ImportError::corrupt;
        return {};
    }
    if (reader.lengthInSamples > 16000000 || reader.lengthInSamples > reader.sampleRate * 120)
    {
        error = ImportError::durationLimit;
        return {};
    }
    juce::AudioBuffer<float> pcm(2, (int)reader.lengthInSamples);
    if (!reader.read(&pcm, 0, pcm.getNumSamples(), 0, true, true))
    {
        error = ImportError::decodeFailed;
        return {};
    }
    auto sample = std::make_unique<Sample>();
    sample->rate = reader.sampleRate;
    for (int c = 0; c < 2; ++c)
    {
        auto *channel = pcm.getReadPointer(reader.numChannels == 1 ? 0 : c);
        sample->data[c].assign(channel, channel + pcm.getNumSamples());
    }
    error = ImportError::none;
    return sample;
}
} // namespace
RecordingInfo inspectM4a(const void *bytes, size_t size)
{
    if (!bytes || size < 16 || size > 32 * 1024 * 1024)
        return {};
    Inspector p{static_cast<const uint8_t *>(bytes)};
    if (!p.scan(0, size, 0) || !p.moov || !p.mdat)
        return p.info;
    if (p.quickTime || p.videoTracks || p.audioTracks > 1)
    {
        p.info.error = ImportError::editableMovie;
        return p.info;
    }
    if (!p.ftyp || p.audioTracks != 1)
    {
        p.info.error = ImportError::corrupt;
        return p.info;
    }
    if (!p.codec || p.badCodec)
    {
        p.info.error = ImportError::incompatibleCodec;
        return p.info;
    }
    p.info.error = p.info.duration > 120.05 ? ImportError::durationLimit : ImportError::none;
    return p.info;
}
std::unique_ptr<Sample> decodeM4a(const void *data, size_t size, juce::AudioFormatManager &formats,
                                  ImportError &error)
{
    auto info = inspectM4a(data, size);
    error = info.error;
    if (error != ImportError::none)
        return {};
#if JUCE_MAC || JUCE_IOS
    // JUCE's existing CoreAudioFormat delegates AAC/ALAC to Apple's local system framework.
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(std::make_unique<juce::MemoryInputStream>(data, size, false)));
    if (reader)
        return readNative(*reader, error);
    error = ImportError::decodeFailed;
    return {};
#elif JUCE_LINUX
    juce::ignoreUnused(formats);
    juce::File ffmpeg(RASTRO_FFMPEG_EXECUTABLE);
    if (!ffmpeg.existsAsFile())
    {
        error = ImportError::decoderUnavailable;
        return {};
    }
    juce::TemporaryFile input(".m4a"), output(".f32");
    if (!input.getFile().replaceWithData(data, size))
    {
        error = ImportError::unreadable;
        return {};
    }
    juce::StringArray args{ffmpeg.getFullPathName(),
                           "-nostdin",
                           "-v",
                           "error",
                           "-xerror",
                           "-err_detect",
                           "explode",
                           "-protocol_whitelist",
                           "file",
                           "-f",
                           "mov",
                           "-i",
                           input.getFile().getFullPathName(),
                           "-map",
                           "0:a:0",
                           "-vn",
                           "-t",
                           "121",
                           "-af",
                           "pan=stereo|FL=FL+FC|FR=FR+FC",
                           "-ac",
                           "2",
                           "-ar",
                           "48000",
                           "-f",
                           "f32le",
                           "-y",
                           output.getFile().getFullPathName()};
    juce::ChildProcess process;
    if (!process.start(args, 0))
    {
        error = ImportError::decoderUnavailable;
        return {};
    }
    auto deadline = juce::Time::getMillisecondCounterHiRes() + 30000;
    while (process.isRunning())
    {
        if (juce::Time::getMillisecondCounterHiRes() > deadline)
        {
            process.kill();
            process.waitForProcessToFinish(1000);
            error = ImportError::decodeFailed;
            return {};
        }
        juce::Thread::sleep(5);
    }
    if (process.getExitCode() != 0)
    {
        error = ImportError::decodeFailed;
        return {};
    }
    auto outputBytes = output.getFile().getSize();
    if (outputBytes > int64_t(48000) * 120 * 8)
    {
        error = ImportError::durationLimit;
        return {};
    }
    if (outputBytes < 16 || outputBytes % 8 != 0)
    {
        error = ImportError::decodeFailed;
        return {};
    }
    juce::MemoryBlock pcm;
    if (!output.getFile().loadFileAsData(pcm))
    {
        error = ImportError::unreadable;
        return {};
    }
    auto sample = std::make_unique<Sample>();
    sample->rate = 48000;
    size_t frames = pcm.getSize() / 8;
    for (auto &c : sample->data)
        c.resize(frames);
    auto *src = static_cast<const uint8_t *>(pcm.getData());
    for (size_t i = 0; i < frames; ++i)
        for (int c = 0; c < 2; ++c)
        {
            uint32_t bits = juce::ByteOrder::littleEndianInt(src + i * 8 + c * 4);
            float value;
            std::memcpy(&value, &bits, 4);
            sample->data[c][i] = value;
        }
    error = ImportError::none;
    return sample;
#else
    juce::ignoreUnused(data, size, formats);
    error = ImportError::decoderUnavailable;
    return {};
#endif
}
} // namespace rastro
