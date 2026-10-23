#pragma once
#include "CaptureBridge.h"
#include <mutex>
#include <thread>
namespace rastro
{
struct CaptureLink
{
    juce::String id, request;
    juce::File file;
    double updated = 0, lastAudio = 0, rate = 48000;
    int state = CaptureRecorder::idle, outcome = CaptureRecorder::none, frames = 0;
    float peak = 0;
    bool hasAudioFlow() const { return lastAudio > 0 && updated - lastAudio < 1500; }
};
inline double captureNow() { return double(juce::Time::getCurrentTime().toMilliseconds()); }
inline juce::File captureLinksDirectory()
{
    return captureDirectory().getSiblingFile("Capture Links");
}
inline bool captureIdValid(const juce::String &id)
{
    return id.length() == 32 && id.containsOnly("0123456789abcdef");
}
inline bool writeCaptureXml(const juce::File &file, const juce::XmlElement &xml)
{
    if (!file.getParentDirectory().createDirectory())
        return false;
    juce::TemporaryFile temp(file);
    return temp.getFile().replaceWithText(xml.toString()) &&
           temp.overwriteTargetFileWithTemporary();
}
inline std::vector<CaptureLink> readCaptureLinks()
{
    std::vector<CaptureLink> links;
    int scanned = 0;
    auto now = captureNow();
    for (auto &entry : juce::RangedDirectoryIterator(captureLinksDirectory(), false, "*.status.xml",
                                                     juce::File::findFiles))
    {
        if (++scanned > 128)
            break;
        auto file = entry.getFile();
        if (file.getSize() > 16384)
            continue;
        auto xml = juce::XmlDocument::parse(file);
        if (!xml || !xml->hasTagName("RASTRO_LINK"))
            continue;
        CaptureLink link;
        link.id = xml->getStringAttribute("id");
        link.updated = xml->getDoubleAttribute("updated");
        if (!captureIdValid(link.id) || now - link.updated > 2500 || link.updated > now + 1000 ||
            file.getFileName() != link.id + ".status.xml")
            continue;
        link.rate = xml->getDoubleAttribute("rate", 48000);
        link.lastAudio = xml->getDoubleAttribute("audio");
        link.request = xml->getStringAttribute("request");
        link.state = xml->getIntAttribute("state");
        link.outcome = xml->getIntAttribute("outcome");
        link.frames = xml->getIntAttribute("frames");
        link.peak = float(xml->getDoubleAttribute("peak"));
        auto name = xml->getStringAttribute("file");
        if (name.isNotEmpty() && name == juce::File::createLegalFileName(name) &&
            !name.containsAnyOf("/\\") && name.endsWithIgnoreCase(".wav"))
            link.file = captureDirectory().getChildFile(name);
        links.push_back(link);
        if (links.size() == 32)
            break;
    }
    std::sort(links.begin(), links.end(), [](auto &a, auto &b) { return a.id < b.id; });
    return links;
}
inline bool requestCapture(const juce::String &id, bool start)
{
    if (!captureIdValid(id))
        return false;
    juce::XmlElement xml("RASTRO_REQUEST");
    xml.setAttribute("token", juce::Uuid().toString());
    xml.setAttribute("time", captureNow());
    xml.setAttribute("start", start);
    return writeCaptureXml(captureLinksDirectory().getChildFile(id + ".request.xml"), xml);
}
// File-based IPC survives separate AU host processes. All scanning, encoding and publication
// occur on this bounded worker, never in the UI or processBlock. PCM ownership remains in recorder.
class CaptureLinkServer
{
    CaptureRecorder &rec;
    std::atomic<bool> closing{false};
    juce::String id = juce::Uuid().toString().removeCharacters("-");
    std::thread worker;
    void run()
    {
        auto folder = captureLinksDirectory();
        auto status = folder.getChildFile(id + ".status.xml"),
             command = folder.getChildFile(id + ".request.xml");
        juce::String token;
        juce::File take;
        unsigned serial = rec.blockSerial;
        double audioTime = 0;
        while (!closing)
        {
            if (command.existsAsFile() && command.getSize() < 16384)
            {
                if (auto xml = juce::XmlDocument::parse(command);
                    xml && xml->hasTagName("RASTRO_REQUEST"))
                {
                    auto next = xml->getStringAttribute("token");
                    auto age = captureNow() - xml->getDoubleAttribute("time");
                    if (next != token && age >= -1000 && age < 5000)
                    {
                        token = next;
                        if (xml->getBoolAttribute("start"))
                        {
                            if (rec.state == CaptureRecorder::idle)
                            {
                                take = juce::File{};
                                rec.start();
                            }
                        }
                        else
                            rec.stop();
                    }
                }
            }
            rec.finishStop();
            if (rec.claim())
            {
                juce::MemoryBlock bytes;
                int result = finishCapture(rec, bytes);
                if (result == CaptureRecorder::success)
                {
                    take = publishCapture(bytes);
                    if (take == juce::File{})
                        result = CaptureRecorder::saveError;
                }
                rec.outcome = result;
                rec.release();
            }
            auto block = rec.blockSerial.load();
            if (block != serial)
            {
                serial = block;
                audioTime = captureNow();
            }
            juce::XmlElement xml("RASTRO_LINK");
            xml.setAttribute("id", id);
            xml.setAttribute("request", token);
            xml.setAttribute("updated", captureNow());
            xml.setAttribute("audio", audioTime);
            xml.setAttribute("state", rec.state.load());
            xml.setAttribute("outcome", rec.outcome.load());
            xml.setAttribute("frames", rec.frames.load());
            xml.setAttribute("rate", rec.sampleRate());
            xml.setAttribute("peak", double(rec.peak.load()));
            xml.setAttribute("file", take.getFileName());
            writeCaptureXml(status, xml);
            for (int i = 0; i < 10 && !closing; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        status.deleteFile();
        command.deleteFile();
    }

  public:
    explicit CaptureLinkServer(CaptureRecorder &r)
        : rec(r), worker(
                      [this]
                      {
                          try
                          {
                              run();
                          }
                          catch (...)
                          {
                              rec.outcome = CaptureRecorder::saveError;
                          }
                      })
    {
    }
    ~CaptureLinkServer() { stop(); }
    void stop()
    {
        closing = true;
        if (worker.joinable())
            worker.join();
    }
    juce::String identifier() const { return id; }
};
} // namespace rastro
