#!/usr/bin/env python3
"""
Patched pipeline (sample rate + playback UART format).

Changes vs rpi/pipeline/main_code.py:
  - Piper WAV may be 22050 / 24000 / etc. Resample to SAMPLE_RATE (16 kHz) before UART.
  - Playback uses raw little-endian int16 frames (2 bytes/sample), matching
    uart-pi/Core/Src/main.c ring_pop_sample — not the 12-bit mic uplink framing.

STM32 I2S is configured for 16 kHz; UART stream must match that rate.
"""

import os
import struct
import subprocess
import time
import wave
from pathlib import Path

import serial

try:
    import numpy as np
except ImportError:
    np = None

# ══════════════════════════════════════════════════════════════════════════════
# SETTINGS
# ══════════════════════════════════════════════════════════════════════════════

PORT           = "/dev/ttyAMA0"
BAUD           = 921600
RECORD_SECONDS = 5
SAMPLE_RATE    = 16000  # Must match STM32 I2S / TIM2 ADC path
CHANNELS       = 1
SAMPLE_WIDTH   = 2

WHISPER_BIN    = os.path.expanduser('~/whisper.cpp/build/bin/whisper-cli')
WHISPER_MODEL  = os.path.expanduser('~/whisper.cpp/models/ggml-small.en.bin')

PIPER_BIN      = os.path.expanduser('~/piper/piper/piper')
PIPER_MODEL    = os.path.expanduser('~/piper/es_MX-claude-high.onnx')

OUTPUT_DIR     = str(Path(__file__).resolve().parent / "output")

# ══════════════════════════════════════════════════════════════════════════════
# UART: mic uplink still uses framed 12-bit packets (STM32 ProcessAndSend)
# ══════════════════════════════════════════════════════════════════════════════


def decode_samples(raw_bytes):
    """Decode 2-byte framed UART stream into 16-bit signed PCM (mic path)."""
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


def resample_pcm16_mono(samples, src_rate, dst_rate):
    """
    Resample int16 mono PCM to dst_rate. Prefers NumPy linear interp; falls back
    to pure Python for short clips if NumPy is missing.
    """
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

    # Pure-Python linear interpolation (fine for TTS-length audio)
    out = []
    for j in range(new_n):
        t = j * (n - 1) / (new_n - 1) if new_n > 1 else 0.0
        i0 = int(t)
        i1 = min(i0 + 1, n - 1)
        frac = t - i0
        s = samples[i0] * (1.0 - frac) + samples[i1] * frac
        v = int(round(s))
        v = max(-32768, min(32767, v))
        out.append(v)
    return out


# ══════════════════════════════════════════════════════════════════════════════
# STEP 1: RECORD
# ══════════════════════════════════════════════════════════════════════════════


def record(ser, wav_path):
    """Record audio from STM32 mic over UART, save as WAV."""
    total_samples = SAMPLE_RATE * RECORD_SECONDS
    bytes_needed = total_samples * 2 + 512

    print(f"\n[1/4] RECORDING {RECORD_SECONDS}s — speak now!")
    ser.reset_input_buffer()
    raw = ser.read(bytes_needed)
    print(f"  Got {len(raw)} bytes")

    samples = decode_samples(raw)
    samples = samples[:total_samples]

    if len(samples) == 0:
        print("  WARNING: No samples decoded")
        return False

    with wave.open(wav_path, 'w') as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(SAMPLE_WIDTH)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(struct.pack(f'<{len(samples)}h', *samples))

    print(f"  Saved {wav_path} ({len(samples)/SAMPLE_RATE:.1f}s)")
    return True


# ══════════════════════════════════════════════════════════════════════════════
# STEP 2: TRANSCRIBE (Whisper)
# ══════════════════════════════════════════════════════════════════════════════


def transcribe(wav_path):
    """Run whisper.cpp on WAV file, return English text."""
    print("\n[2/4] TRANSCRIBING with Whisper...")
    wav_path = os.path.abspath(wav_path)

    out_prefix = os.path.splitext(wav_path)[0] + '_transcription'

    result = subprocess.run([
        WHISPER_BIN,
        '-m', WHISPER_MODEL,
        '-f', wav_path,
        '--language', 'en',
        '--no-timestamps',
        '-otxt',
        '-of', out_prefix,
    ], capture_output=True, text=True)

    if result.returncode != 0:
        print("whisper.cpp stderr:")
        print(result.stderr[-500:])
        raise RuntimeError("Whisper failed")

    txt_file = out_prefix + '.txt'
    with open(txt_file) as f:
        text = f.read().strip()

    print(f"  English: {text}")
    return text


# ══════════════════════════════════════════════════════════════════════════════
# STEP 3: TRANSLATE (Llama)
# ══════════════════════════════════════════════════════════════════════════════


def load_llama():
    """Load Llama model (call once at startup)."""
    from huggingface_hub import hf_hub_download
    from llama_cpp import Llama

    print("Loading Llama model...")
    path = hf_hub_download(
        repo_id="bartowski/Llama-3.2-3B-Instruct-GGUF",
        filename="Llama-3.2-3B-Instruct-Q4_K_M.gguf",
    )
    llm = Llama(model_path=path, n_ctx=2048, n_threads=4, verbose=False)
    print("  Llama ready.")
    return llm


def translate(llm, english_text):
    """Translate English text to Spanish using Llama."""
    print("\n[3/4] TRANSLATING to Spanish...")
    prompt = (
        "You are a medical interpreter. Translate the following English text "
        "to Spanish. Output only the Spanish translation directly, nothing else no extra notes, speeches, rambles, or explanations.\n\n"
        f"English: {english_text}\nSpanish:"
    )
    response = llm(prompt, max_tokens=512, temperature=0)
    spanish = response['choices'][0]['text'].strip()
    print(f"  Spanish: {spanish}")
    return spanish


# ══════════════════════════════════════════════════════════════════════════════
# STEP 4: SPEAK (Piper → resample → UART as raw PCM16 @ SAMPLE_RATE)
# ══════════════════════════════════════════════════════════════════════════════


def synthesize_and_play(ser, spanish_text, wav_path):
    """Synthesize with Piper, resample to 16 kHz, stream raw PCM to STM32."""
    print("\n[4/4] SPEAKING Spanish...")

    result = subprocess.run([
        PIPER_BIN, '--model', PIPER_MODEL, '--output_file', wav_path],
        input=spanish_text.encode('utf-8'),
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Piper failed:\n{result.stderr.decode(errors='ignore')}")

    with wave.open(wav_path) as wf:
        channels = wf.getnchannels()
        rate = wf.getframerate()
        sampwidth = wf.getsampwidth()
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    if sampwidth != 2:
        raise RuntimeError(
            f"Piper WAV is {sampwidth * 8}-bit; expected 16-bit PCM. "
            "Re-export or transcode Piper output."
        )

    print(f"  Piper: {n_frames/rate:.1f}s, {rate} Hz, {channels} ch")

    if channels == 2:
        pairs = struct.unpack(f'<{n_frames * 2}h', raw)
        samples = [(pairs[i] + pairs[i + 1]) // 2 for i in range(0, len(pairs), 2)]
    else:
        samples = list(struct.unpack(f'<{n_frames}h', raw))

    if rate != SAMPLE_RATE:
        print(f"  Resampling {rate} Hz → {SAMPLE_RATE} Hz ({len(samples)} samples)")
        samples = resample_pcm16_mono(samples, rate, SAMPLE_RATE)

    # Exact bytes sent on UART = raw s16le mono (no WAV header)
    pcm_blob = struct.pack(f'<{len(samples)}h', *samples)

    base = os.path.splitext(wav_path)[0] + f'_uart_{SAMPLE_RATE}hz'
    debug_wav = base + '.wav'
    debug_raw = base + '.raw'
    with wave.open(debug_wav, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(pcm_blob)
    with open(debug_raw, 'wb') as f:
        f.write(pcm_blob)
    print(f"  Debug UART WAV: {debug_wav}")
    print(f"  Debug UART RAW: {debug_raw} (same bytes as serial stream)")

    print(f"  Sending {len(samples)} samples @ {SAMPLE_RATE} Hz (raw PCM16 LE)...")
    CHUNK_SAMPLES = 256
    CHUNK_BYTES = CHUNK_SAMPLES * 2
    t0 = time.time()
    sent = 0

    for off in range(0, len(pcm_blob), CHUNK_BYTES):
        ser.write(pcm_blob[off:off + CHUNK_BYTES])
        sent += min(CHUNK_BYTES, len(pcm_blob) - off) // 2

        due = sent / SAMPLE_RATE
        elapsed = time.time() - t0
        if elapsed < due:
            time.sleep(due - elapsed)

    print(f"  Done. Sent {sent} samples in {time.time() - t0:.1f}s.")


# ══════════════════════════════════════════════════════════════════════════════
# MAIN
# ══════════════════════════════════════════════════════════════════════════════


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print("=" * 50)
    print("  Edge-MD Pipeline (audio-patch)")
    print("  Record → Transcribe → Translate → Speak")
    print(f"  Playback: {SAMPLE_RATE} Hz raw PCM16 LE → UART")
    print("=" * 50)
    if np is None:
        print("  Note: install numpy for faster resampling: pip install numpy")

    llm = load_llama()

    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUD,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=RECORD_SECONDS + 3,
        )
    except serial.SerialException as e:
        print(f"ERROR: {e}")
        print(f"Try: sudo chmod 666 {PORT}")
        return

    cycle = 0

    try:
        while True:
            cycle += 1
            timestamp = time.strftime('%Y%m%d_%H%M%S')
            print(f"\n{'─' * 50}")
            print(f"  Cycle {cycle} — {timestamp}")
            print(f"{'─' * 50}")

            wav_path = os.path.join(OUTPUT_DIR, f'recording_{timestamp}.wav')
            piper_wav = os.path.join(OUTPUT_DIR, f'spanish_{timestamp}.wav')
            translation_file = os.path.join(OUTPUT_DIR, f'translation_{timestamp}.txt')

            ok = record(ser, wav_path)
            if not ok:
                print("  Skipping cycle — no audio captured.")
                continue

            english_text = transcribe(wav_path)
            if not english_text:
                print("  Skipping cycle — no speech detected.")
                continue

            spanish_text = translate(llm, english_text)

            with open(translation_file, 'w', encoding='utf-8') as f:
                f.write(f"English: {english_text}\nSpanish: {spanish_text}\n")
            print(f"  Saved translation to {translation_file}")

            synthesize_and_play(ser, spanish_text, piper_wav)

            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\n\nStopped by user.")
    finally:
        ser.close()
        print("Port closed.")


if __name__ == '__main__':
    main()
