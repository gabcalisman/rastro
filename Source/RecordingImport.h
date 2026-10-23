#pragma once
#include "EngineTypes.h"
#include <juce_audio_formats/juce_audio_formats.h>
namespace rastro
{
enum class ImportError
{
    none,
    unreadable,
    corrupt,
    unsupportedFormat,
    incompatibleCodec,
    editableMovie,
    encodedLimit,
    durationLimit,
    memoryLimit,
    decoderUnavailable,
    decodeFailed
};
struct RecordingInfo
{
    ImportError error = ImportError::corrupt;
    bool aac = false, alac = false;
    double duration = 0;
};
// Original bounded ISO-BMFF inspection. Only a single rendered AAC/ALAC audio track.
RecordingInfo inspectM4a(const void *, size_t);
std::unique_ptr<Sample> decodeM4a(const void *, size_t, juce::AudioFormatManager &, ImportError &);
inline bool offeredAudioFile(const juce::File &f)
{
    return f.hasFileExtension("wav;aiff;aif;mp3;m4a");
}
inline bool editableRecording(const juce::File &f) { return f.hasFileExtension("mov;qt"); }
} // namespace rastro
