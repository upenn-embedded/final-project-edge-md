#!/usr/bin/env python3
"""
stream_audio.py  –  Capture mic audio and stream raw PCM over UART to STM32.

Hardware:
  MAX9814 mic  →  RPi ADC pin (or USB audio dongle)
  RPi TX (GPIO14 / pin 8)  →  STM32 PA3 (USART2 RX)
  Common GND

Requirements:
  pip install pyserial sounddevice numpy
  (or: sudo apt install python3-serial python3-numpy python3-sounddevice)

Usage:
  python3 stream_audio.py                  # list devices then stream
  python3 stream_audio.py --device 1       # pick a specific input device
  python3 stream_audio.py --loopback       # stream a test tone instead of mic
"""

import argparse
import struct
import sys
import time

import numpy as np
import serial
import sounddevice as sd

# ── Configuration ─────────────────────────────────────────────────────────────
SAMPLE_RATE   = 16000      # Hz  – must match STM32 I2S clock config
CHANNELS      = 1          # mono
DTYPE         = 'int16'    # 16-bit signed PCM
BLOCK_SIZE    = 256        # samples per callback (latency vs. stability)
UART_PORT     = '/dev/ttyS0'   # or /dev/ttyAMA0  – check with: ls /dev/tty*
UART_BAUDRATE = 921600

# ── Argument parsing ──────────────────────────────────────────────────────────
parser = argparse.ArgumentParser(description='Stream mic audio to STM32 over UART')
parser.add_argument('--device',   type=int, default=None,  help='Input device index')
parser.add_argument('--port',     type=str, default=UART_PORT)
parser.add_argument('--loopback', action='store_true',     help='Send 1 kHz test tone')
parser.add_argument('--list',     action='store_true',     help='List audio devices and exit')
args = parser.parse_args()

if args.list:
    print(sd.query_devices())
    sys.exit(0)

# ── Open UART ─────────────────────────────────────────────────────────────────
print(f'Opening UART  {args.port}  @  {UART_BAUDRATE} baud …')
try:
    ser = serial.Serial(
        port      = args.port,
        baudrate  = UART_BAUDRATE,
        bytesize  = serial.EIGHTBITS,
        parity    = serial.PARITY_NONE,
        stopbits  = serial.STOPBITS_ONE,
        timeout   = 1,
    )
except serial.SerialException as e:
    print(f'ERROR: could not open serial port: {e}')
    print('Try:  sudo raspi-config  → Interface Options → Serial → disable console, enable hardware')
    sys.exit(1)

print('UART open.')

# ── Test tone mode ────────────────────────────────────────────────────────────
if args.loopback:
    print('Sending 1 kHz test tone … (Ctrl-C to stop)')
    t      = 0
    freq   = 1000
    amp    = 16000
    period = SAMPLE_RATE // BLOCK_SIZE
    try:
        while True:
            samples = (amp * np.sin(
                2 * np.pi * freq * np.arange(t, t + BLOCK_SIZE) / SAMPLE_RATE
            )).astype(np.int16)
            ser.write(samples.tobytes())
            t += BLOCK_SIZE
    except KeyboardInterrupt:
        print('\nStopped.')
        ser.close()
    sys.exit(0)

# ── Live microphone streaming ─────────────────────────────────────────────────
print(f'Audio device : {args.device if args.device is not None else "default"}')
print(f'Sample rate  : {SAMPLE_RATE} Hz')
print(f'Block size   : {BLOCK_SIZE} samples  ({1000*BLOCK_SIZE//SAMPLE_RATE} ms)')
print('Streaming mic → UART → STM32 … (Ctrl-C to stop)\n')

# Track stats
total_bytes  = 0
dropped      = 0
start_time   = time.time()

def audio_callback(indata, frames, time_info, status):
    """Called by sounddevice for every BLOCK_SIZE samples."""
    global total_bytes, dropped
    if status:
        print(f'  sounddevice warning: {status}')
    # indata shape: (frames, channels) – int16
    raw = indata[:, 0].astype(np.int16).tobytes()
    # Non-blocking write; if UART TX buffer is full, drop the block
    waiting = ser.out_waiting
    if waiting > UART_BAUDRATE // 8:   # > ~1 s of data queued
        dropped += 1
        return
    ser.write(raw)
    total_bytes += len(raw)

try:
    with sd.InputStream(
        samplerate = SAMPLE_RATE,
        blocksize  = BLOCK_SIZE,
        device     = args.device,
        channels   = CHANNELS,
        dtype      = DTYPE,
        callback   = audio_callback,
    ):
        while True:
            elapsed = time.time() - start_time
            kbps    = (total_bytes * 8 / 1000) / max(elapsed, 1)
            print(
                f'\r  {elapsed:6.1f} s   {total_bytes/1024:8.1f} KB sent'
                f'   {kbps:6.1f} kbps   dropped={dropped}',
                end='', flush=True
            )
            time.sleep(0.5)

except KeyboardInterrupt:
    print('\n\nStopped by user.')
except Exception as e:
    print(f'\nERROR: {e}')
finally:
    ser.close()
    print('Serial port closed.')



"""
The RPi likely won't have all of them by default.

**Already on RPi OS:**
- `serial` (pyserial) — sometimes pre-installed
- `numpy` — sometimes pre-installed
- `struct`, `sys`, `time`, `argparse` — 

**Almost certainly missing:**
- `sounddevice` — needs manual install

So run this:

```bash
# Update first
sudo apt update

# Install system-level audio library sounddevice depends on
sudo apt install -y libportaudio2 portaudio19-dev

# Then install Python packages
pip3 install pyserial sounddevice numpy
```



```bash
# Option 1 - use apt instead
sudo apt install -y python3-serial python3-numpy

# sounddevice has no apt package so force pip for just that one
pip3 install sounddevice --break-system-packages

# Option 2 - use a venv (cleaner)
python3 -m venv audioenv
source audioenv/bin/activate
pip install pyserial sounddevice numpy
# then run your script inside the venv
python3 stream_audio.py
```

**Also verify you actually have an audio input device:**

```bash
# List ALSA capture devices
arecord -l

# If nothing shows, your MAX9814 is analog and the RPi
# has no ADC — you'd need a USB audio dongle
# Check if one is connected:
lsusb | grep -i audio
```

"""