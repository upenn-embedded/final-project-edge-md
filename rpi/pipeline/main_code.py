#!/usr/bin/env python3
"""
main_code.py
Full pipeline: Record English → Transcribe → Translate to Spanish → Speak Spanish

1. Record 10s of audio from STM32 mic over UART
2. Transcribe English audio with whisper.cpp
3. Translate English → Spanish with Llama 3.2
4. Synthesize Spanish speech with Piper TTS
5. Play back through STM32 speaker over UART
"""

import os
import struct
import subprocess
import time
import wave
from pathlib import Path

import serial
import spidev

# ══════════════════════════════════════════════════════════════════════════════
# SETTINGS
# ══════════════════════════════════════════════════════════════════════════════

PORT           = "/dev/ttyAMA0"
BAUD           = 921600
RECORD_SECONDS = 5
SAMPLE_RATE    = 16000
CHANNELS       = 1
SAMPLE_WIDTH   = 2

WHISPER_BIN    = os.path.expanduser('~/whisper.cpp/build/bin/whisper-cli')
WHISPER_MODEL  = os.path.expanduser('~/whisper.cpp/models/ggml-small.en.bin')

PIPER_BIN      = os.path.expanduser('~/piper/piper/piper')
PIPER_MODEL    = os.path.expanduser('~/piper/es_MX-claude-high.onnx')

OUTPUT_DIR     = str(Path(__file__).resolve().parent / "output")

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 500000
spi.mode = 0

# ══════════════════════════════════════════════════════════════════════════════
# UART FRAMING (must match STM32 firmware)
# ══════════════════════════════════════════════════════════════════════════════

def decode_samples(raw_bytes):
    """Decode 2-byte framed UART stream into 16-bit signed PCM."""
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
    """Encode 16-bit signed PCM into 2-byte UART frame."""
    val12 = ((val16 >> 4) + 2048) & 0xFFF
    b0 = 0x80 | (val12 >> 6)
    b1 = val12 & 0x3F
    return bytes([b0, b1])


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


def translate(llm, original_text):
    """Translate English text to Spanish using Llama."""
    print("\n[3/4] TRANSLATING to Spanish...")
    prompt = (
        "You are a medical interpreter. "
        "First Interpret if this is English or Spanish."
        "If this is Spanish, translate it to English. If this is English, translate it"
        "to Spanish. Output only the translation directly, nothing else no extra notes, speeches, rambles, or explanations.\n\n"
        f"English: {original_text}\nSpanish:"
    )
    response = llm(prompt, max_tokens=512, temperature=0)
    spanish = response['choices'][0]['text'].strip()
    print(f"  Spanish: {spanish}")
    return spanish


import spidev

# ══════════════════════════════════════════════════════════════════════════════
# SPI SETUP (add near the top of your file, after SETTINGS)
# ══════════════════════════════════════════════════════════════════════════════

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 500000
spi.mode = 0


# ══════════════════════════════════════════════════════════════════════════════
# STEP 4: SPEAK (Piper TTS → SPI → STM32 I2S → MAX98357A)
# ══════════════════════════════════════════════════════════════════════════════

def synthesize_and_play(translated_text, wav_path):
    """Synthesize Spanish speech with Piper, then stream to STM32 via SPI."""
    print("\n[4/4] SPEAKING Spanish...")

    # Synthesize with Piper
    result = subprocess.run(
        [PIPER_BIN, '--model', PIPER_MODEL, '--output_file', wav_path],
        input=translated_text.encode('utf-8'),
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Piper failed:\n{result.stderr.decode(errors='ignore')}")

    # Read WAV
    with wave.open(wav_path) as wf:
        channels = wf.getnchannels()
        rate = wf.getframerate()
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    print(f"  Generated {n_frames/rate:.1f}s of audio at {rate} Hz")

    # If stereo, downmix to mono bytes (little-endian 16-bit samples)
    if channels == 2:
        pairs = struct.unpack(f'<{n_frames*2}h', raw)
        mono = [(pairs[i] + pairs[i+1]) // 2 for i in range(0, len(pairs), 2)]
        raw = struct.pack(f'<{len(mono)}h', *mono)

    # Stream to STM32 over SPI using raw I2S frames
    print(f"  Sending {n_frames} samples to STM32 via SPI...")
    CHUNK_SAMPLES = 256
    CHUNK_BYTES = CHUNK_SAMPLES * 2   # 2 bytes per 16-bit sample
    t0 = time.time()
    samples_sent = 0

    for offset in range(0, len(raw), CHUNK_BYTES):
        chunk = raw[offset:offset + CHUNK_BYTES]
        if len(chunk) < 2:
            break

        # Build raw I2S frames: each mono sample → stereo frame
        # [LEFT_HI, LEFT_LO, RIGHT_HI, RIGHT_LO]
        i2s_frames = []
        for i in range(0, len(chunk), 2):
            lo = chunk[i]
            hi = chunk[i + 1]
            # left channel MSB first
            i2s_frames.append(hi)
            i2s_frames.append(lo)
            # right channel same sample
            i2s_frames.append(hi)
            i2s_frames.append(lo)

        spi.writebytes2(bytes(i2s_frames))

        samples_sent += len(chunk) // 2
        elapsed = time.time() - t0
        drift = (samples_sent / rate) - elapsed
        if drift > 0:
            time.sleep(drift)

    print(f"  Done. Sent {samples_sent} samples in {time.time() - t0:.1f}s.")


# ══════════════════════════════════════════════════════════════════════════════
# MAIN
# ══════════════════════════════════════════════════════════════════════════════

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print("=" * 50)
    print("  Edge-MD Pipeline")
    print("  Record → Transcribe → Translate → Speak")
    print("=" * 50)

    # Load Llama once at startup
    llm = load_llama()

    # Open serial port once
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

            # Step 1: Record
            ok = record(ser, wav_path)
            if not ok:
                print("  Skipping cycle — no audio captured.")
                continue

            # Step 2: Transcribe
            original_text = transcribe(wav_path)
            if not original_text:
                print("  Skipping cycle — no speech detected.")
                continue

            # Step 3: Translate
            translated_text = translate(llm, original_text)

            # Save translation locally
            with open(translation_file, 'w', encoding='utf-8') as f:
                f.write(f"Original: {original_text}\nTranslated: {translated_text}\n")
            print(f"  Saved translation to {translation_file}")

            # Step 4: Speak
            synthesize_and_play(translated_text, piper_wav)

            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\n\nStopped by user.")
    finally:
        ser.close()
        print("Port closed.")


if __name__ == '__main__':
    main()
