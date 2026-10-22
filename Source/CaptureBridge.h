#pragma once
#include "CaptureRecorder.h"
#include <juce_audio_formats/juce_audio_formats.h>
namespace rastro
{
// Only called by a loader worker after the PCM ownership hand-off; never by processBlock.
inline bool encodeCapture(CaptureRecorder &capture, juce::MemoryBlock &bytes)
{
    int count = capture.frames.load();
    if (count < 2)
        return false;
    auto stream = std::make_unique<juce::MemoryOutputStream>(bytes, false);
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        format.createWriterFor(stream.release(), capture.sampleRate(), 2, 24, {}, 0));
    const float *data[]{capture.channel(0), capture.channel(1)};
    return writer && writer->writeFromFloatArrays(data, 2, count);
}
inline juce::File captureDirectory()
{
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    base = base.getChildFile("Application Support");
#elif JUCE_LINUX
    auto xdg = juce::SystemStats::getEnvironmentVariable("XDG_CONFIG_HOME", {});
    if (xdg.isNotEmpty() && juce::File::isAbsolutePath(xdg))
        base = juce::File(xdg);
#endif
    return base.getChildFile("RASTRO").getChildFile("Captures");
}
inline juce::File publishCapture(const juce::MemoryBlock &bytes,
                                 juce::File dir = captureDirectory())
{
    if (!dir.createDirectory())
        return {};
    auto name =
        juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S") + "-" + juce::Uuid().toString();
    auto file = dir.getChildFile(name + ".wav");
    juce::TemporaryFile temp(file);
    if (!temp.getFile().replaceWithData(bytes.getData(), bytes.getSize()) ||
        !temp.overwriteTargetFileWithTemporary())
        return {};
    return file;
}
inline int finishCapture(CaptureRecorder &capture, juce::MemoryBlock &bytes)
{
    if (capture.frames < 2)
        return CaptureRecorder::empty;
    if (capture.takePeak <= 1e-7f)
        return CaptureRecorder::noSignal;
    return encodeCapture(capture, bytes) ? CaptureRecorder::success : CaptureRecorder::encodeError;
}
inline std::vector<juce::File> captureFiles()
{
    std::vector<juce::File> files;
    // Bounded scan, off the UI thread. Files are never auto-deleted.
    std::vector<juce::File> directories{captureDirectory()};
#if JUCE_MAC
    directories.push_back(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("RASTRO/Captures"));
#endif
    for (auto &directory : directories)
        for (const auto &entry :
             juce::RangedDirectoryIterator(directory, false, "*.wav", juce::File::findFiles))
        {
            if (files.size() >= 1000)
                break;
            files.push_back(entry.getFile());
        }
    std::sort(files.begin(), files.end(),
              [](auto &a, auto &b) { return a.getFileName() > b.getFileName(); });
    if (files.size() > 64)
        files.resize(64);
    return files;
}
} // namespace rastro
