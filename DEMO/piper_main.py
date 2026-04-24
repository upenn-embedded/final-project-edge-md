#!/usr/bin/env python3
"""
Demo copy — Spanish text file → Piper TTS → UART → STM32 speaker.

Synthesized WAV is kept in ./Output/piper_tts_out.wav for your demo folder.

  python3 piper_main.py Output/translation_20260424_120000.txt
"""

import os
import struct
import subprocess
import sys
import time
import wave
from pathlib import Path

import serial

_DEMO_ROOT = Path(__file__).resolve().parent
OUTPUT_DIR = _DEMO_ROOT / "Output"

PORT = '/dev/ttyAMA0'
BAUD = 921600
PIPER_BIN = os.path.expanduser('~/piper/piper')
PIPER_MODEL = os.path.expanduser('~/piper/models/es_MX-claude-high.onnx')

PIPER_SAMPLE_RATE = 22050


def encode_sample(val16: int) -> bytes:
    val12 = ((val16 >> 4) + 2048) & 0xFFF
    b0 = 0x80 | (val12 >> 6)
    b1 = val12 & 0x3F
    return bytes([b0, b1])


def synthesize(text: str, out_wav: str) -> None:
    if not os.path.exists(PIPER_BIN):
        raise FileNotFoundError(f"Piper binary not found: {PIPER_BIN}")
    if not os.path.exists(PIPER_MODEL):
        raise FileNotFoundError(f"Piper model not found: {PIPER_MODEL}")

    result = subprocess.run(
        [PIPER_BIN, '--model', PIPER_MODEL, '--output_file', out_wav],
        input=text.encode('utf-8'),
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Piper failed:\n{result.stderr.decode(errors='ignore')}")

    with wave.open(out_wav) as wf:
        rate = wf.getframerate()
        n = wf.getnframes()
    print(f"  [TTS] Generated {n/rate:.1f}s of audio at {rate} Hz → {out_wav}")


def send_to_stm32(wav_path: str) -> None:
    with wave.open(wav_path) as wf:
        channels = wf.getnchannels()
        sampwidth = wf.getsampwidth()
        rate = wf.getframerate()
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    if sampwidth != 2:
        raise ValueError(f"Expected 16-bit WAV, got {sampwidth*8}-bit")

    if channels == 2:
        pairs = struct.unpack(f'<{n_frames*2}h', raw)
        samples = [(pairs[i] + pairs[i+1]) // 2 for i in range(0, len(pairs), 2)]
    else:
        samples = list(struct.unpack(f'<{n_frames}h', raw))

    print(f"  [TX]  Sending {len(samples)} samples to STM32 @ {BAUD} baud...")

    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUD,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=5,
        )
    except serial.SerialException as e:
        raise RuntimeError(f"Could not open {PORT}: {e}\nTry: sudo chmod 666 {PORT}")

    CHUNK = 256
    t0 = time.time()
    sent = 0

    try:
        for i in range(0, len(samples), CHUNK):
            chunk = samples[i:i + CHUNK]
            packet = b''.join(encode_sample(s) for s in chunk)
            ser.write(packet)
            sent += len(chunk)

            due = sent / rate
            elapsed = time.time() - t0
            if elapsed < due:
                time.sleep(due - elapsed)
    finally:
        ser.close()

    elapsed = time.time() - t0
    print(f"  [TX]  Done. Sent {sent} samples in {elapsed:.1f}s.")


def main(txt_path: str) -> None:
    txt_path = os.path.abspath(txt_path)
    if not os.path.exists(txt_path):
        raise FileNotFoundError(f"Text file not found: {txt_path}")

    with open(txt_path, encoding='utf-8') as f:
        text = f.read().strip()

    if not text:
        raise ValueError("Text file is empty.")

    print(f"Input text: {text!r}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    wav_path = str(OUTPUT_DIR / 'piper_tts_out.wav')

    print("\n[1/2] Synthesizing Spanish speech...")
    synthesize(text, wav_path)

    print("\n[2/2] Sending to STM32 via UART → I2S...")
    send_to_stm32(wav_path)

    print("\nDone.")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: python3 {os.path.basename(__file__)} <spanish_or_mixed_text.txt>")
        sys.exit(1)
    main(sys.argv[1])
