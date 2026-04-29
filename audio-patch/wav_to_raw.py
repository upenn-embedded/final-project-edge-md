#!/usr/bin/env python3
"""
WAV → raw PCM (no RIFF header).

Output: mono, 16-bit signed little-endian — same byte order as UART playback
to the STM32 (ring_pop_sample reads int16 LE).

Example:
  python3 wav_to_raw.py piper_out.wav clip.raw
  python3 wav_to_raw.py piper_out.wav clip.raw -r 16000
"""

from __future__ import annotations

import argparse
import struct
import sys
import wave

try:
    import numpy as np
except ImportError:
    np = None


def resample_pcm16_mono(samples: list[int], src_rate: int, dst_rate: int) -> list[int]:
    if src_rate == dst_rate:
        return list(samples)

    n = len(samples)
    if n == 0:
        return []

    new_n = max(1, int(round(n * dst_rate / float(src_rate))))

    if np is not None:
        x_old = np.arange(n, dtype=np.float64)
        x_new = np.linspace(0.0, n - 1.0, new_n)
        y = np.interp(x_new, x_old, np.asarray(samples, dtype=np.float64))
        y = np.clip(np.round(y), -32768, 32767).astype(np.int16)
        return y.tolist()

    out: list[int] = []
    for j in range(new_n):
        t = j * (n - 1) / (new_n - 1) if new_n > 1 else 0.0
        i0 = int(t)
        i1 = min(i0 + 1, n - 1)
        frac = t - i0
        s = samples[i0] * (1.0 - frac) + samples[i1] * frac
        v = int(round(s))
        out.append(max(-32768, min(32767, v)))
    return out


def wav_to_samples(path: str) -> tuple[list[int], int, int]:
    with wave.open(path, 'rb') as wf:
        ch = wf.getnchannels()
        rate = wf.getframerate()
        sw = wf.getsampwidth()
        n = wf.getnframes()
        raw = wf.readframes(n)

    if sw != 2:
        raise SystemExit(f"Need 16-bit PCM WAV, got {sw * 8}-bit")

    if ch == 2:
        pairs = struct.unpack(f'<{n * 2}h', raw)
        samples = [(pairs[i] + pairs[i + 1]) // 2 for i in range(0, len(pairs), 2)]
    else:
        samples = list(struct.unpack(f'<{n}h', raw))

    return samples, rate, ch


def main() -> None:
    p = argparse.ArgumentParser(description='Convert WAV to raw s16le mono PCM')
    p.add_argument('input_wav', help='Input .wav')
    p.add_argument('output_raw', help='Output .raw (no header)')
    p.add_argument(
        '-r', '--rate', type=int, default=None,
        help='Resample to this rate (Hz). Default: keep WAV rate.',
    )
    args = p.parse_args()

    samples, rate, ch = wav_to_samples(args.input_wav)
    out_rate = rate if args.rate is None else args.rate

    if out_rate != rate:
        samples = resample_pcm16_mono(samples, rate, out_rate)

    blob = struct.pack(f'<{len(samples)}h', *samples)
    with open(args.output_raw, 'wb') as f:
        f.write(blob)

    dur = len(samples) / out_rate
    print(
        f"Wrote {args.output_raw}: {len(blob)} bytes "
        f"({len(samples)} samples, {out_rate} Hz mono, ~{dur:.2f}s)",
        file=sys.stderr,
    )


if __name__ == '__main__':
    main()
