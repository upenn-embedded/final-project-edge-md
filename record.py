#!/usr/bin/env python3
"""
capture.py — Record audio from STM32 MAX9814 mic and save as WAV
RPi receives 16kHz 16-bit mono audio over UART at 921600 baud

Usage:
    python3 capture.py
"""

import serial
import wave
import struct
import time
import os

# ══════════════════════════════════════════════════════════════════════════════
# USER SETTINGS — adjust these
# ══════════════════════════════════════════════════════════════════════════════

RECORD_SECONDS = 5
OUTPUT_FILE    = "recording.wav"
PORT           = "/dev/ttyAMA0"
BAUD           = 921600

# ══════════════════════════════════════════════════════════════════════════════
# DO NOT CHANGE BELOW — must match STM32 firmware
# ══════════════════════════════════════════════════════════════════════════════

SAMPLE_RATE  = 16000
CHANNELS     = 1
SAMPLE_WIDTH = 2


def decode_samples(raw_bytes):
    samples = []
    resync_count = 0
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
            resync_count += 1
            i += 1

    if resync_count > 0:
        print(f"  Warning: {resync_count} resync events")

    return samples


def main():
    total_samples = SAMPLE_RATE * RECORD_SECONDS
    bytes_needed  = total_samples * 2 + 1024

    print("=" * 50)
    print("  STM32 Audio Capture")
    print("=" * 50)
    print(f"  Port       : {PORT}")
    print(f"  Baud       : {BAUD}")
    print(f"  Duration   : {RECORD_SECONDS}s")
    print(f"  Sample rate: {SAMPLE_RATE}Hz")
    print(f"  Output     : {OUTPUT_FILE}")
    print("=" * 50)

    try:
        ser = serial.Serial(
            port     = PORT,
            baudrate = BAUD,
            bytesize = serial.EIGHTBITS,
            parity   = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            timeout  = RECORD_SECONDS + 5
        )
    except serial.SerialException as e:
        print(f"\nERROR: Could not open {PORT}")
        print(f"  {e}")
        print(f"\nFix: sudo chmod 666 {PORT}")
        print(f"  or change PORT to /dev/ttyS0")
        return

    ser.reset_input_buffer()
    time.sleep(0.1)
    ser.reset_input_buffer()

    print(f"\nRecording... speak into the mic")
    start = time.time()
    raw = ser.read(bytes_needed)
    elapsed = time.time() - start
    ser.close()

    print(f"Done. Received {len(raw)} bytes in {elapsed:.1f}s")

    if len(raw) < 100:
        print("\nERROR: Not enough data.")
        print("  Check STM32 PA9 -> RPi Pin 10")
        print("  Check STM32 GND -> RPi Pin 6")
        return

    print("Decoding...")
    samples = decode_samples(raw)

    if len(samples) == 0:
        print(f"\nERROR: 0 samples decoded.")
        print(f"First 20 bytes: {raw[:20].hex()}")
        return

    if len(samples) < total_samples:
        print(f"  Warning: got {len(samples)/SAMPLE_RATE:.2f}s (wanted {RECORD_SECONDS}s)")
    else:
        samples = samples[:total_samples]

    with wave.open(OUTPUT_FILE, 'w') as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(SAMPLE_WIDTH)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(struct.pack(f'<{len(samples)}h', *samples))

    print(f"\n{'=' * 50}")
    print(f"  Saved    : {OUTPUT_FILE}")
    print(f"  Duration : {len(samples)/SAMPLE_RATE:.2f}s")
    print(f"  Samples  : {len(samples)}")
    print(f"  Size     : {os.path.getsize(OUTPUT_FILE)/1024:.1f} KB")
    print(f"{'=' * 50}")
    print(f"\nPlay:    aplay {OUTPUT_FILE}")
    print(f"Copy:    scp pi@<rpi-ip>:~/{OUTPUT_FILE} .")


if __name__ == "__main__":
    main()