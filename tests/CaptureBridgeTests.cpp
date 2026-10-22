#include "CaptureLink.h"
#include <iostream>
#include <stdexcept>
void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
template <class F> void waitFor(F predicate, const char *message)
{
    for (int i = 0; i < 600; ++i)
    {
        if (predicate())
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    check(false, message);
}
int main()
{
    using R = rastro::CaptureRecorder;
#if JUCE_MAC
    check(rastro::captureDirectory().getFullPathName().contains(
              "/Library/Application Support/RASTRO/Captures"),
          "Mac canonical capture folder matches installation guide");
#endif
    R rec;
    rec.prepare(44100);
    rastro::CaptureLinkServer server(rec);
    auto id = server.identifier();
    waitFor(
        [&]
        {
            for (auto &link : rastro::readCaptureLinks())
                if (link.id == id)
                    return true;
            return false;
        },
        "insert is discovered through actual file IPC");
    check(rastro::requestCapture(id, true), "PLAY remote start sent");
    waitFor([&] { return rec.state == R::recording; }, "insert starts from PLAY command");
    check(rastro::requestCapture(id, false), "PLAY remote stop sent");
    waitFor([&] { return rec.outcome == R::empty && rec.state == R::idle; },
            "no callback is an EMPTY recording, not a write error");
    check(rastro::requestCapture(id, true), "retry silent take");
    waitFor([&] { return rec.state == R::recording; }, "retry starts");
    std::array<float, 256> left{}, right{};
    const float *input[]{left.data(), right.data()};
    rec.process(input, 2, 256);
    rastro::requestCapture(id, false);
    waitFor([&] { return rec.outcome == R::noSignal && rec.state == R::idle; },
            "silent frames are NO SIGNAL, not empty or save error");
    check(rastro::requestCapture(id, true), "start real take");
    waitFor([&] { return rec.state == R::recording; }, "remote recording ready");
    for (int i = 0; i < 256; ++i)
    {
        left[i] = .3f * std::sin(i * .11f);
        right[i] = .2f * std::cos(i * .07f);
    }
    for (int n = 0; n < 8; ++n)
    {
        rec.process(input, 2, 256);
        rec.prepare(44100);
    }
    check(rec.state == R::recording && rec.frames == 2048,
          "same-rate host reprepare does not abort capture");
    check(rastro::requestCapture(id, false), "finish real take");
    waitFor([&] { return rec.outcome == R::success && rec.state == R::idle; },
            "real signal saved successfully");
    juce::File take;
    waitFor(
        [&]
        {
            for (auto &link : rastro::readCaptureLinks())
                if (link.id == id)
                {
                    take = link.file;
                    return link.outcome == R::success && take.existsAsFile() && link.rate == 44100;
                }
            return false;
        },
        "PLAY receives acknowledged valid take at source rate");
    auto files = rastro::captureFiles();
    check(std::find(files.begin(), files.end(), take) != files.end(),
          "automatic refresh discovers finalized take");
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader(
        format.createReaderFor(take.createInputStream().release(), true));
    check(reader && reader->lengthInSamples == 2048 && reader->sampleRate == 44100 &&
              reader->bitsPerSample == 24,
          "published WAV metadata is valid");
    juce::AudioBuffer<float> decoded(2, 2048);
    check(reader->read(&decoded, 0, 2048, 0, true, true), "WAV decodes fully");
    for (int i = 0; i < 2048; ++i)
    {
        check(std::abs(decoded.getSample(0, i) - left[i % 256]) < 2e-7, "left PCM preserved");
        check(std::abs(decoded.getSample(1, i) - right[i % 256]) < 2e-7, "right PCM preserved");
    }
    juce::TemporaryFile blocking(".file");
    blocking.getFile().replaceWithText("not a directory");
    juce::MemoryBlock bytes;
    take.loadFileAsData(bytes);
    check(rastro::publishCapture(bytes, blocking.getFile()) == juce::File{},
          "directory/write failure is reported without a fake take");
    check(!rastro::requestCapture("../invalid", true), "IPC identifier validated");
    reader.reset();
    take.deleteFile();
    server.stop();
    waitFor(
        [&]
        {
            for (auto &link : rastro::readCaptureLinks())
                if (link.id == id)
                    return false;
            return true;
        },
        "disconnected insert disappears");
    std::cout << "PASS capture bridge: actual remote start/stop/ack, no frames vs no signal, "
                 "same-rate reprepare, finalized PCM24 WAV, canonical folder/discovery, write "
                 "failure and disconnect\n";
}
