#include "Mp3Decoder.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
int main()
{
    for (auto name : {"mono.mp3", "stereo.mp3"})
    {
        std::ifstream file(std::string(RASTRO_TEST_DATA_DIR) + "/" + name, std::ios::binary);
        std::vector<char> bytes{std::istreambuf_iterator<char>(file), {}};
        check(!bytes.empty(), "fixture exists");
        auto sample = rastro::decodeMp3(bytes.data(), bytes.size());
        check(sample && sample->size() > 8000 && sample->size() < 16000, "MP3 decoded offline");
        check(sample->rate == (std::string(name) == "mono.mp3" ? 44100 : 48000), "MP3 sample rate");
        float energy = 0, difference = 0;
        for (int i = 0; i < sample->size(); ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                float v = sample->data[c][i];
                check(std::isfinite(v) && std::abs(v) <= 1, "finite PCM");
                energy += v * v;
            }
            difference += std::abs(sample->data[0][i] - sample->data[1][i]);
        }
        check(energy > 1, "MP3 signal audible");
        check(std::string(name) == "mono.mp3" ? difference == 0 : difference > 1,
              "mono duplication and stereo separation");
        for (size_t n : {0u, 1u, 7u, 32u, 96u})
        {
            auto truncated = rastro::decodeMp3(bytes.data(), n);
            if (truncated)
                check(truncated->size() > 1, "truncated input safely handled");
        }
    }
    char bad[]{1, 2, 3, 4};
    check(!rastro::decodeMp3(bad, sizeof bad), "bad MP3 rejected");
    std::cout << "PASS: offline MP3 mono/stereo decode, rates, PCM energy, channel layout, "
                 "invalid/truncated input\n";
}
