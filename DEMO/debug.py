"""
Play a WAV on the Pi via SPI → STM32 I2S → speaker.

All demo WAVs live in ./Output/ — pass a filename or use the default.

  python3 debug.py
  python3 debug.py Output/spanish_20260424_120000.wav
  python3 debug.py spanish_16k.wav
"""

import spidev
import wave
import sys
import time
from pathlib import Path

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 500000
spi.mode = 0

SAMPLE_RATE = 16000

_DEMO_ROOT = Path(__file__).resolve().parent
OUTPUT_DIR = _DEMO_ROOT / "Output"


def resolve_wav_path(arg: str) -> Path:
    p = Path(arg)
    if p.is_file():
        return p.resolve()
    in_output = OUTPUT_DIR / arg
    if in_output.is_file():
        return in_output.resolve()
    return p.resolve()


def stream_wav(filename):
    wf = wave.open(str(filename), 'rb')
    n_frames = wf.getnframes()
    framerate = wf.getframerate()
    print(f'Playing: {filename} {n_frames/framerate:.2f}s')

    start = time.time()
    samples_sent = 0

    while True:
        raw = wf.readframes(256)
        if not raw:
            break

        i2s_frames = []
        for i in range(0, len(raw), 2):
            lo = raw[i]
            hi = raw[i + 1]
            i2s_frames.append(hi)
            i2s_frames.append(lo)
            i2s_frames.append(hi)
            i2s_frames.append(lo)

        spi.writebytes2(bytes(i2s_frames))

        samples_sent += len(raw) // 2
        elapsed = time.time() - start
        drift = (samples_sent / SAMPLE_RATE) - elapsed
        if drift > 0:
            time.sleep(drift)

    wf.close()
    print('Done.')


if __name__ == '__main__':
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    default_wav = OUTPUT_DIR / 'spanish_16k.wav'
    if len(sys.argv) > 1:
        wav = resolve_wav_path(sys.argv[1])
    else:
        wav = default_wav if default_wav.is_file() else Path('spanish_16k.wav')

    try:
        while True:
            if not wav.is_file():
                print(f'WAV not found: {wav}')
                print(f'Put a file in {OUTPUT_DIR} or pass its path.')
                break
            stream_wav(wav)
    except KeyboardInterrupt:
        print('\nStopped.')
    finally:
        spi.close()
