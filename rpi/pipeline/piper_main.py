#!/usr/bin/env python3
"""
piper_main.py
Takes a .txt file containing Spanish text, runs it through Piper TTS
(es_MX-claude-high, 22050 Hz), then streams the audio to the STM32
over UART using the same 2-byte framing as record_main.py.

STM32 I2S must be configured at 22050 Hz in CubeMX to match Piper output:
    hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_22K;

Download the voice model once on the RPi:
    mkdir -p ~/piper/models
    wget -P ~/piper/models \
      https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_MX/claude/high/es_MX-claude-high.onnx
    wget -P ~/piper/models \
      https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_MX/claude/high/es_MX-claude-high.onnx.json

Usage:
    python3 piper_main.py transcription.txt
"""

import os
import struct
import subprocess
import sys
import tempfile
import time
import wave

import serial

# ══════════════════════════════════════════════════════════════════════════════
# USER SETTINGS
# ══════════════════════════════════════════════════════════════════════════════

PORT        = '/dev/ttyAMA0'
BAUD        = 921600          # must match STM32 USART config
PIPER_BIN   = os.path.expanduser('~/piper/piper')
PIPER_MODEL = os.path.expanduser('~/piper/models/es_MX-claude-high.onnx')

# ══════════════════════════════════════════════════════════════════════════════
# DO NOT CHANGE — must match STM32 firmware framing (same as record_main.py)
# ══════════════════════════════════════════════════════════════════════════════

PIPER_SAMPLE_RATE = 22050   # es_MX-claude-high native output rate


# ── Framing (identical to record_main.py encode_sample) ───────────────────────

def encode_sample(val16: int) -> bytes:
    """Encode 16-bit signed PCM → 2-byte UART frame.
    Bit7=1 on byte0 (sync), Bit7=0 on byte1 (data).
    Converts via 12-bit unsigned midpoint (2048 = silence).
    """
    val12 = ((val16 >> 4) + 2048) & 0xFFF
    b0 = 0x80 | (val12 >> 6)   # bit7=1, bits 11:6
    b1 = val12 & 0x3F           # bit7=0, bits  5:0
    return bytes([b0, b1])


# ── Step 1: Piper TTS ──────────────────────────────────────────────────────────

def synthesize(text: str, out_wav: str) -> None:
    """Run Piper on Spanish text, write mono 16-bit WAV to out_wav."""
    if not os.path.exists(PIPER_BIN):
        raise FileNotFoundError(f"Piper binary not found: {PIPER_BIN}")
    if not os.path.exists(PIPER_MODEL):
        raise FileNotFoundError(
            f"Piper model not found: {PIPER_MODEL}\n"
            "Download it with the wget commands in this file's header."
        )

    result = subprocess.run(
        [PIPER_BIN, '--model', PIPER_MODEL, '--output_file', out_wav],
        input=text.encode('utf-8'),
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Piper failed:\n{result.stderr.decode(errors='ignore')}")

    with wave.open(out_wav) as wf:
        rate = wf.getframerate()
        n    = wf.getnframes()
    print(f"  [TTS] Generated {n/rate:.1f}s of audio at {rate} Hz → {out_wav}")


# ── Step 2: Send WAV to STM32 ──────────────────────────────────────────────────

def send_to_stm32(wav_path: str) -> None:
    """Stream WAV over UART using the same 2-byte framing as record_main.py."""
    with wave.open(wav_path) as wf:
        channels   = wf.getnchannels()
        sampwidth  = wf.getsampwidth()
        rate       = wf.getframerate()
        n_frames   = wf.getnframes()
        raw        = wf.readframes(n_frames)

    if sampwidth != 2:
        raise ValueError(f"Expected 16-bit WAV, got {sampwidth*8}-bit")

    # Mix stereo to mono if Piper ever outputs stereo
    if channels == 2:
        pairs   = struct.unpack(f'<{n_frames*2}h', raw)
        samples = [(pairs[i] + pairs[i+1]) // 2 for i in range(0, len(pairs), 2)]
    else:
        samples = list(struct.unpack(f'<{n_frames}h', raw))

    print(f"  [TX]  Sending {len(samples)} samples to STM32 @ {BAUD} baud...")

    try:
        ser = serial.Serial(
            port     = PORT,
            baudrate = BAUD,
            bytesize = serial.EIGHTBITS,
            parity   = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            timeout  = 5,
        )
    except serial.SerialException as e:
        raise RuntimeError(f"Could not open {PORT}: {e}\nTry: sudo chmod 666 {PORT}")

    CHUNK = 256   # samples per write — matches STM32 I2S DMA buffer half
    t0    = time.time()
    sent  = 0

    for i in range(0, len(samples), CHUNK):
        chunk   = samples[i:i + CHUNK]
        packet  = b''.join(encode_sample(s) for s in chunk)
        ser.write(packet)
        sent   += len(chunk)

        # Pace to playback rate so STM32 UART buffer doesn't overflow
        due     = sent / rate
        elapsed = time.time() - t0
        if elapsed < due:
            time.sleep(due - elapsed)

    ser.close()
    elapsed = time.time() - t0
    print(f"  [TX]  Done. Sent {sent} samples in {elapsed:.1f}s.")


# ── Main ───────────────────────────────────────────────────────────────────────

def main(txt_path: str) -> None:
    txt_path = os.path.abspath(txt_path)
    if not os.path.exists(txt_path):
        raise FileNotFoundError(f"Text file not found: {txt_path}")

    with open(txt_path, encoding='utf-8') as f:
        text = f.read().strip()

    if not text:
        raise ValueError("Text file is empty.")

    print(f"Input text: {text!r}")

    with tempfile.TemporaryDirectory() as tmp:
        wav_path = os.path.join(tmp, 'piper_out.wav')

        print("\n[1/2] Synthesizing Spanish speech...")
        synthesize(text, wav_path)

        print("\n[2/2] Sending to STM32 via UART → I2S...")
        send_to_stm32(wav_path)

    print("\nDone.")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: python3 {os.path.basename(__file__)} <spanish_text.txt>")
        sys.exit(1)
    main(sys.argv[1])
