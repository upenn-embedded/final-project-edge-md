#!/usr/bin/env python3
"""
UART record + SPI playback loop (demo paths).

Artifacts under ./Output/: raw.wav, out_16k.wav

  python3 pipeline.py
"""

import subprocess
import time
import wave
from pathlib import Path

import serial
import spidev

_DEMO_ROOT = Path(__file__).resolve().parent
OUTPUT_DIR = _DEMO_ROOT / "Output"

UART_PORT = '/dev/ttyAMA0'
UART_BAUD = 921600
SAMPLE_RATE = 16000
RECORD_SECS = 5
RECORD_SAMPLES = SAMPLE_RATE * RECORD_SECS

RAW_WAV = str(OUTPUT_DIR / 'raw.wav')
OUT_WAV = str(OUTPUT_DIR / 'out_16k.wav')

START_MARKER = bytes([0xAA, 0xBB])
END_MARKER = bytes([0xFF, 0xFE])

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 500000
spi.mode = 0

ser = serial.Serial(UART_PORT, UART_BAUD, timeout=15)

print('Ready. Waiting for STM32...')


def wait_for_marker(marker, timeout=30):
    buf = bytearray()
    deadline = time.time() + timeout
    while time.time() < deadline:
        b = ser.read(1)
        if b:
            buf.append(b[0])
            if len(buf) >= 2 and bytes(buf[-2:]) == marker:
                return True
    return False


def receive_and_save():
    print('\n[RX] Waiting for START...')
    ser.reset_input_buffer()
    if not wait_for_marker(START_MARKER, timeout=30):
        print('  No START received')
        return False

    print(f'  START received. Reading {RECORD_SECS}s of PCM...')
    expected = RECORD_SAMPLES * 2
    data = bytearray()
    deadline = time.time() + RECORD_SECS + 3

    while len(data) < expected and time.time() < deadline:
        chunk = ser.read(expected - len(data))
        if chunk:
            data.extend(chunk)

    wait_for_marker(END_MARKER, timeout=3)
    print(f'  Got {len(data)} bytes ({len(data)//2} samples)')

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    with wave.open(RAW_WAV, 'wb') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(bytes(data))

    subprocess.run([
        'ffmpeg', '-y', '-i', RAW_WAV,
        '-ar', '16000', '-ac', '1', '-sample_fmt', 's16',
        OUT_WAV
    ], capture_output=True)

    print(f'  Saved: {OUT_WAV}')
    return True


def stream_back():
    print('[TX] Streaming back via SPI...')
    wf = wave.open(OUT_WAV, 'rb')
    n_frames = wf.getnframes()
    print(f'  {n_frames / SAMPLE_RATE:.2f}s')

    start = time.time()
    samples_sent = 0

    while True:
        raw = wf.readframes(256)
        if not raw:
            break

        frames = []
        for i in range(0, len(raw), 2):
            lo = raw[i]
            hi = raw[i + 1]
            frames += [hi, lo, hi, lo]

        spi.writebytes2(bytes(frames))

        samples_sent += len(raw) // 2
        drift = (samples_sent / SAMPLE_RATE) - (time.time() - start)
        if drift > 0:
            time.sleep(drift)

    spi.writebytes2(bytes(512))
    wf.close()
    print(f'  Done in {time.time()-start:.2f}s')


try:
    cycle = 0
    while True:
        cycle += 1
        print(f'\n{"═"*40}\nCYCLE {cycle}\n{"═"*40}')

        if not receive_and_save():
            time.sleep(1)
            continue

        stream_back()

except KeyboardInterrupt:
    print('\nStopped.')
finally:
    ser.close()
    spi.close()
