#include "Mp3Decoder.h"
#include <algorithm>
#include <limits>
// Vendored CC0 decoder. Its upstream notice and full license are retained in third_party/minimp3.
#define MINIMP3_IMPLEMENTATION
#include "../third_party/minimp3/minimp3.h"
namespace rastro
{
std::unique_ptr<Sample> decodeMp3(const void *data, size_t bytes)
{
    if (!data || bytes == 0 || bytes > 32 * 1024 * 1024)
        return {};
    auto sample = std::make_unique<Sample>();
    mp3dec_t decoder{};
    mp3dec_init(&decoder);
    mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3dec_frame_info_t info{};
    auto *input = static_cast<const uint8_t *>(data);
    size_t pos = 0;
    int rate = 0;
    while (pos < bytes)
    {
        int count = mp3dec_decode_frame(
            &decoder, input + pos,
            int(std::min(bytes - pos, size_t(std::numeric_limits<int>::max()))), pcm, &info);
        if (info.frame_bytes <= 0)
            break;
        pos += size_t(info.frame_bytes);
        if (count <= 0)
            continue;
        if (info.channels < 1 || info.channels > 2 || info.hz < 8000 || info.hz > 48000 ||
            (rate != 0 && rate != info.hz))
            return {};
        rate = info.hz;
        if (sample->data[0].size() + size_t(count) > size_t(rate) * 120 ||
            sample->data[0].size() + size_t(count) > 16000000)
            return {};
        for (int i = 0; i < count; ++i)
            for (int c = 0; c < 2; ++c)
                sample->data[c].push_back(pcm[i * info.channels + (info.channels == 1 ? 0 : c)] /
                                          32768.f);
    }
    if (sample->size() < 2 || rate == 0)
        return {};
    sample->rate = rate;
    return sample;
}
} // namespace rastro
