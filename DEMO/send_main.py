"""
UART playback: stream a 16-bit mono WAV to the STM32 (same framing as main_code).

Default file: ./Output/playback.wav

  python3 send_main.py
  python3 send_main.py Output/spanish_20260424_120000.wav
"""

import os
import struct
import sys
import time
from pathlib import Path

import serial
import wave

_DEMO_ROOT = Path(__file__).resolve().parent
OUTPUT_DIR = _DEMO_ROOT / "Output"
DEFAULT_WAV = OUTPUT_DIR / "playback.wav"

PORT = "/dev/ttyAMA0"
BAUD = 921600
SAMPLE_RATE = 16000


def encode_sample(val16):
    val12 = ((val16 >> 4) + 2048) & 0xFFF
    b0 = 0x80 | (val12 >> 6)
    b1 = val12 & 0x3F
    return bytes([b0, b1])


def playback(ser, wav_path):
    wav_path = Path(wav_path)
    if not wav_path.is_file():
        print(f"  [PLAY] File not found: {wav_path}")
        return

    wf = wave.open(str(wav_path), 'r')
    channels = wf.getnchannels()
    total = wf.getnframes()
    rate = wf.getframerate()

    print(f"  [PLAY] Playing {wav_path} ({total/rate:.1f}s)...")

    CHUNK = 256
    sent = 0
    t0 = time.time()

    while True:
        frames = wf.readframes(CHUNK)
        if not frames:
            break

        num = len(frames) // 2
        samples = struct.unpack(f'<{num}h', frames)

        if channels == 2:
            samples = [(samples[i] + samples[i + 1]) // 2
                       for i in range(0, len(samples), 2)]

        packet = b''.join(encode_sample(s) for s in samples)
        ser.write(packet)
        sent += len(samples)

        due = sent / rate
        elapsed = time.time() - t0
        if elapsed < due:
            time.sleep(due - elapsed)

    wf.close()
    print(f"  [PLAY] Done. Sent {sent} samples.")


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    wav = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_WAV
    if not wav.is_file() and len(sys.argv) < 2:
        alt = OUTPUT_DIR / "spanish_16k.wav"
        if alt.is_file():
            wav = alt

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
        print(f"ERROR: {e}")
        return

    try:
        playback(ser, wav)
    finally:
        ser.close()


if __name__ == '__main__':
    main()
