#!/usr/bin/env python3
"""Original CC0 sine fixtures for rendered Voice Memos codecs, not Apple app recordings.
Requires local ffmpeg and ffprobe for authoring only. No network access.
"""
import argparse
import json
import math
from pathlib import Path
import subprocess
import tempfile
import wave

VARIANTS = [
    ('voice-aac-mono', 44100, 1, 16, 'aac'),
    ('voice-aac-mono48', 48000, 1, 16, 'aac'),
    ('voice-aac-stereo', 48000, 2, 16, 'aac'),
    ('voice-alac-mono16', 44100, 1, 16, 'alac'),
    ('voice-alac-mono24', 48000, 1, 24, 'alac'),
    ('voice-alac-stereo16', 44100, 2, 16, 'alac'),
]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, default=Path(__file__).resolve().parents[1] / 'tests/fixtures')
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory() as scratch:
    for name, rate, channels, bits, codec in VARIANTS:
        source = Path(scratch) / (name + '.wav')
        frames = rate // 3
        pcm = bytearray()
        for i in range(frames):
            fade = min(1., i / (rate * .005), (frames - 1 - i) / (rate * .005))
            for c in range(channels):
                v = .35 * fade * math.sin(2 * math.pi * (310 if c == 0 else 500) * i / rate)
                pcm.extend(round(v * (2**(bits - 1) - 1)).to_bytes(bits // 8, 'little', signed=True))
        with wave.open(str(source), 'wb') as out:
            out.setparams((channels, bits // 8, rate, frames, 'NONE', 'not compressed'))
            out.writeframes(pcm)
        target = args.out / (name + '.m4a')
        extra = ['-b:a', '96k'] if codec == 'aac' else ['-sample_fmt', 's32p' if bits == 24 else 's16p']
        subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-i', str(source),
                        '-map_metadata', '-1', '-c:a', codec, *extra, str(target)], check=True)
        probe = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_streams', '-of', 'json', str(target)]))['streams'][0]
        assert probe['codec_name'] == codec
        assert probe['sample_rate'] == str(rate) and probe['channels'] == channels
        assert codec != 'aac' or probe['profile'] == 'LC'
        assert codec != 'alac' or probe['bits_per_raw_sample'] == str(bits)
        print(name, {k: probe[k] for k in ('codec_name', 'profile', 'sample_rate', 'channels', 'bits_per_raw_sample') if k in probe})
