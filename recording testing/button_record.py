#!/usr/bin/env python3
"""
button_record.py
Hold the USER button on the STM32 to record; release to stop and save.

Record path: STM32 MAX9814 mic → UART TX (PA2) → RPi /dev/ttyAMA0

Control protocol (sent by STM32 button_record.c):
  START  →  0xFE 0xFE 0x53  ('S')
  STOP   →  0xFE 0xFE 0x45  ('E')

These 3-byte sequences are unambiguous in the 2-byte audio frame format
because two consecutive 0xFE bytes cannot appear in a valid audio frame.
"""

import os
import struct
import time
import wave

import serial

# ── Settings ─────────────────────────────────────────────────────────────────

PORT        = "/dev/ttyAMA0"
BAUD        = 921600
SAMPLE_RATE = 16000
CHANNELS    = 1
SAMPLE_WIDTH = 2

OUTPUT_DIR  = os.path.dirname(os.path.abspath(__file__))

# ── Control sequences ─────────────────────────────────────────────────────────

START_SEQ = bytes([0xFE, 0xFE, 0x53])
STOP_SEQ  = bytes([0xFE, 0xFE, 0x45])

# ── Audio frame decoder ───────────────────────────────────────────────────────

def decode_audio_frames(raw_bytes):
    """Decode 2-byte framed UART stream into 16-bit signed PCM samples."""
    samples = []
    i = 0
    # Skip startup 0xFF marker bytes
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
            i += 1  # resync on bad frame
    return samples


def strip_control_seqs(data):
    """Remove START and STOP control sequences from a byte buffer."""
    data = data.replace(START_SEQ, b'')
    data = data.replace(STOP_SEQ, b'')
    return data

# ── Main loop ─────────────────────────────────────────────────────────────────

def main():
    print("=" * 52)
    print("  STM32 Button-Triggered Recorder")
    print("  Hold USER button to record. Release to save.")
    print("=" * 52)
    print(f"  Port : {PORT} @ {BAUD}")
    print(f"  Save : {OUTPUT_DIR}")
    print("=" * 52)

    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUD,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.05,   # short timeout for non-blocking reads while waiting
        )
    except serial.SerialException as e:
        print(f"ERROR opening port: {e}")
        print(f"Try: sudo chmod 666 {PORT}")
        return

    try:
        while True:
            # ── WAITING: look for START sequence ─────────────────────────────
            print("\nWaiting for button press…")
            buf = bytearray()

            while True:
                chunk = ser.read(64)
                if chunk:
                    buf.extend(chunk)
                    # Keep only the last 6 bytes for sequence detection to
                    # avoid unbounded growth while waiting
                    if len(buf) > 256:
                        buf = buf[-6:]
                    if START_SEQ in buf:
                        break

            # ── RECORDING: accumulate until STOP sequence ────────────────────
            print("Recording… (release button to stop)")
            ser.timeout = None      # block until bytes arrive while recording
            ser.reset_input_buffer()
            audio_buf = bytearray()
            tail = bytearray()     # rolling tail to catch sequence across reads

            while True:
                chunk = ser.read(256)
                if not chunk:
                    continue

                # Check combined tail+chunk for the STOP sequence so we don't
                # miss a sequence that straddles two reads
                combined = tail + chunk
                stop_idx = combined.find(STOP_SEQ)

                if stop_idx != -1:
                    # Only keep audio bytes before the STOP sequence
                    audio_portion = combined[:stop_idx]
                    audio_buf.extend(audio_portion)
                    break

                # No STOP yet — keep last 2 bytes as tail to bridge next read
                audio_buf.extend(combined[:-2] if len(combined) > 2 else b'')
                tail = bytearray(combined[-2:])

            # ── SAVING ───────────────────────────────────────────────────────
            ser.timeout = 0.05  # restore non-blocking timeout for WAITING phase

            # Strip any stray control sequences from captured audio
            clean = strip_control_seqs(bytes(audio_buf))
            samples = decode_audio_frames(clean)

            if len(samples) == 0:
                print("No audio samples decoded — discarding.")
                continue

            duration = len(samples) / SAMPLE_RATE
            timestamp = time.strftime('%Y%m%d_%H%M%S')
            wav_path = os.path.join(OUTPUT_DIR, f"recording_{timestamp}.wav")

            with wave.open(wav_path, 'w') as wf:
                wf.setnchannels(CHANNELS)
                wf.setsampwidth(SAMPLE_WIDTH)
                wf.setframerate(SAMPLE_RATE)
                wf.writeframes(struct.pack(f'<{len(samples)}h', *samples))

            print(f"Saved {os.path.basename(wav_path)}  ({duration:.1f}s, {len(samples)} samples)")

    except KeyboardInterrupt:
        print("\n\nStopped by user.")
    finally:
        ser.close()
        print("Port closed.")


if __name__ == "__main__":
    main()
