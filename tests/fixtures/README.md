# Original synthetic codec fixtures

Sine waves generated locally; no music, speech, Apple recording or third-party audio is included.
These original synthetic fixture files are dedicated to **CC0 1.0**:
https://creativecommons.org/publicdomain/zero/1.0/.

## MP3

- `mono.mp3`: 440 Hz, 0.25 s, 44.1 kHz, mono, 96 kb/s.
- `stereo.mp3`: 440/770 Hz, 0.25 s, 48 kHz, stereo, 128 kb/s.

Originally encoded with FFmpeg/libmp3lame; decoded by the vendored CC0 minimp3 header,
with no runtime dependency for MP3.

## Rendered M4A

Generated with `python scripts/generate_recordings.py` using Python's standard library,
FFmpeg 7.1.5 and ffprobe. The script checks the actual codec/profile/rate/channels/depth.
Each file contains one third of a second of 310 Hz (left/mono) and 500 Hz (right),
peak amplitude .35, with 5 ms edge fades. AAC is lossy and may have encoder padding.

| File | Codec | Rate | Channels | PCM depth before encoding |
| --- | --- | --- | --- | --- |
| voice-aac-mono.m4a | AAC-LC, 96 kb/s target | 44.1 kHz | 1 | 16 bit |
| voice-aac-mono48.m4a | AAC-LC, 96 kb/s target | 48 kHz | 1 | 16 bit |
| voice-aac-stereo.m4a | AAC-LC, 96 kb/s target | 48 kHz | 2 | 16 bit |
| voice-alac-mono16.m4a | ALAC | 44.1 kHz | 1 | 16 bit |
| voice-alac-mono24.m4a | ALAC | 48 kHz | 1 | 24 bit |
| voice-alac-stereo16.m4a | ALAC | 44.1 kHz | 2 | 16 bit |

AAC-LC and ALAC cover compressed and lossless rendered audio codecs relevant to Voice Memos.
These are codec examples, **not recordings exported from the Apple app**. HE-AAC, HE-AACv2,
xHE-AAC, protected files, spatial/multichannel audio and editable QuickTime are not validated.

M4A runtime decoding uses the existing JUCE CoreAudioFormat on macOS (not run here),
or a separately installed local FFmpeg executable on Linux. No FFmpeg code/binary is
vendored, linked or packaged. See [import details and licensing](../../docs/AUDIO-IMPORT.md).
