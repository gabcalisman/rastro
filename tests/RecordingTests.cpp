#include "RecordingImport.h"
#include <iostream>
#include <stdexcept>
void check(bool b, const char *message)
{
    if (!b)
        throw std::runtime_error(message);
}
int main()
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    for (auto name : {"voice-aac-mono.m4a", "voice-aac-mono48.m4a", "voice-aac-stereo.m4a",
                      "voice-alac-mono16.m4a", "voice-alac-mono24.m4a", "voice-alac-stereo16.m4a"})
    {
        juce::MemoryBlock data;
        check(juce::File(RASTRO_TEST_DATA_DIR).getChildFile(name).loadFileAsData(data),
              "fixture available");
        auto info = rastro::inspectM4a(data.getData(), data.getSize());
        check(info.error == rastro::ImportError::none, "container metadata inspected");
        rastro::ImportError error;
        auto sample = rastro::decodeM4a(data.getData(), data.getSize(), formats, error);
        check(sample && error == rastro::ImportError::none, "offline native M4A decoding");
        check(sample->rate >= 44100 && sample->rate <= 48000 && sample->size() > 10000 &&
                  sample->size() < 18000,
              "decoded rate and duration");
        float peak = 0;
        for (auto &c : sample->data)
            for (auto x : c)
            {
                check(std::isfinite(x) && std::abs(x) < .6f, "finite bounded decoded PCM");
                peak = std::max(peak, std::abs(x));
            }
        check(peak > .08f, "non-silent decoded content");
        auto restored = rastro::decodeM4a(data.getData(), data.getSize(), formats, error);
        check(restored && sample->data == restored->data,
              "embedded bytes decode deterministically");
        check(rastro::inspectM4a(data.getData(), data.getSize() / 2).error !=
                  rastro::ImportError::none,
              "truncated file rejected");
        std::cout << "PASS offline M4A " << name << " rate=" << sample->rate
                  << " frames=" << sample->size() << "\n";
    }
}
