# Synthetic MP3 fixtures

Generated locally from mathematical sine waves using FFmpeg/libmp3lame. No recorded music or third-party audio is included. The project author dedicates these synthetic fixture files to CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/).

- mono.mp3: 440 Hz, 0.25 s, 44.1 kHz, mono, 96 kb/s.
- stereo.mp3: 440/770 Hz, 0.25 s, 48 kHz, stereo, 128 kb/s.

FFmpeg is a test-data authoring tool only; it is not linked into or required by the plugin. Decoder tests use the vendored minimp3 header, offline.
