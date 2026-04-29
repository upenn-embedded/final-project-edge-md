#!/usr/bin/env python3
"""
loop_playback.py — convert WAV once, stream to STM32 in a loop.
Ctrl+C to stop.
"""

import subprocess
import time
import sys
from pathlib import Path

WAV_FILE = "spanish_20260417_020354.wav"
RAW_FILE = "/tmp/out.raw"
SERIAL_PORT = "/dev/serial0"
BAUD = 921600
GAP_SECONDS = 0.5  # pause between loops; set to 0 for back-to-back

def run(cmd, **kwargs):
    """Run a command, raise on non-zero exit."""
    result = subprocess.run(cmd, **kwargs)
    if result.returncode != 0:
        sys.exit(f"Command failed: {' '.join(cmd)}")
    return result

def main():
    wav = Path(WAV_FILE)
    if not wav.exists():
        sys.exit(f"File not found: {wav}")

    # Convert once
    print(f"Converting {wav} -> {RAW_FILE}")
    run([
        "ffmpeg", "-y", "-i", str(wav),
        "-f", "s16le", "-ar", "16000", "-ac", "1",
        "-acodec", "pcm_s16le", RAW_FILE,
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    # Configure UART once
    print(f"Configuring {SERIAL_PORT} @ {BAUD} baud")
    run([
        "stty", "-F", SERIAL_PORT, str(BAUD),
        "raw", "-echo", "-echoe", "-echok", "-echoctl", "-echoke",
    ])

    # Pre-read raw bytes so we're not touching disk in the loop
    raw_data = Path(RAW_FILE).read_bytes()
    print(f"Loaded {len(raw_data)} bytes (~{len(raw_data)/32000:.2f} sec audio)")

    # Open serial port once, stream in loop
    print("Streaming in loop. Ctrl+C to stop.\n")
    count = 0
    try:
        with open(SERIAL_PORT, "wb", buffering=0) as ser:
            while True:
                count += 1
                print(f"  [{count}] playing...", flush=True)
                ser.write(raw_data)
                ser.flush()
                if GAP_SECONDS > 0:
                    time.sleep(GAP_SECONDS)
    except KeyboardInterrupt:
        print(f"\nStopped after {count} loops.")

if __name__ == "__main__":
    main()