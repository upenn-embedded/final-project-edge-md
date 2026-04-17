#!/usr/bin/env python3
"""
record_play_loop.py
Records 10 seconds from STM32 mic, saves WAV, plays back through STM32 speaker.
"""

import serial
import wave
import struct
import time
import os

# ══════════════════════════════════════════════════════════════════════════════
# USER SETTINGS
# ══════════════════════════════════════════════════════════════════════════════

RECORD_SECONDS = 10
OUTPUT_FILE    = "recording.wav"
PORT           = "/dev/ttyAMA0"
BAUD           = 921600
SAMPLE_RATE    = 16000

# ══════════════════════════════════════════════════════════════════════════════

def decode_samples(raw_bytes):
    samples = []
    i = 0
    while i < len(raw_bytes) and raw_bytes[i] == 0xFF:
        i += 1
    while i < len(raw_bytes) - 1:
        b0 = raw_bytes[i]
        b1 = raw_bytes[i + 1]
        if (b0 & 0x80) and not (b1 & 0x80):
            val12 = ((b0 & 0x3F) << 6) | (b1 & 0x3F)
            val16 = (val12 - 2048) << 4
            samples.append(val16)
            i += 2
        else:
            i += 1
    return samples


def encode_sample(val16):
    val12 = ((val16 >> 4) + 2048) & 0xFFF
    b0 = 0x80 | (val12 >> 6)
    b1 = val12 & 0x3F
    return bytes([b0, b1])


def record(ser):
    total_samples = SAMPLE_RATE * RECORD_SECONDS
    bytes_needed  = total_samples * 2 + 512

    print(f"  [REC] Recording {RECORD_SECONDS}s... speak now!")
    ser.reset_input_buffer()
    raw = ser.read(bytes_needed)
    print(f"  [REC] Got {len(raw)} bytes")

    samples = decode_samples(raw)
    samples = samples[:total_samples]

    if len(samples) == 0:
        print("  [REC] WARNING: No samples decoded")
        return False

    with wave.open(OUTPUT_FILE, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(struct.pack(f'<{len(samples)}h', *samples))

    print(f"  [REC] Saved {OUTPUT_FILE} ({len(samples)/SAMPLE_RATE:.1f}s)")
    return True


def playback(ser):
    if not os.path.exists(OUTPUT_FILE):
        print("  [PLAY] No file to play")
        return

    wf = wave.open(OUTPUT_FILE, 'r')
    channels = wf.getnchannels()
    total    = wf.getnframes()
    rate     = wf.getframerate()
    print(f"  [PLAY] Playing {OUTPUT_FILE} ({total/rate:.1f}s)...")

    CHUNK = 256
    sent  = 0

    while True:
        frames = wf.readframes(CHUNK)
        if not frames:
            break
        num = len(frames) // 2
        samples = struct.unpack(f'<{num}h', frames)
        if channels == 2:
            samples = [(samples[i] + samples[i+1]) // 2
                       for i in range(0, len(samples), 2)]
        ser.write(b''.join(encode_sample(s) for s in samples))
        sent += len(samples)

    wf.close()
    print(f"  [PLAY] Done. Sent {sent} samples.")


def main():
    print("=" * 50)
    print("  STM32 Record → Play Loop")
    print("=" * 50)
    print(f"  Port   : {PORT} @ {BAUD}")
    print(f"  Record : {RECORD_SECONDS}s per cycle")
    print("=" * 50)

    try:
        ser = serial.Serial(
            port     = PORT,
            baudrate = BAUD,
            bytesize = serial.EIGHTBITS,
            parity   = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            timeout  = RECORD_SECONDS + 3
        )
    except serial.SerialException as e:
        print(f"ERROR: {e}")
        print(f"Try: sudo chmod 666 {PORT}")
        return

    cycle = 0
    try:
        while True:
            cycle += 1
            print(f"\n{'─' * 50}")
            print(f"  Cycle {cycle}")
            print(f"{'─' * 50}")

            ok = record(ser)

            if ok:
                time.sleep(0.3)
                playback(ser)
                time.sleep(0.5)

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()