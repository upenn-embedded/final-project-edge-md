#!/usr/bin/env python3
"""
Edge-MD Full Pipeline (RPi side)
  1. Record audio from STM32 over UART
  2. Whisper.cpp → English transcription
  3. LLaMA 3.2 via llama.cpp → Spanish translation
  4. Piper TTS → Spanish WAV
  5. Stream WAV back to STM32 over UART for speaker playback

STM32 firmware must be in RECORD mode during step 1,
then switched to PLAYBACK mode before step 5.
See EdgMD/Core/Src/main.c for the state-machine TODO.
"""

import os
import sys
import struct
import subprocess
import tempfile
import time
import wave
import serial

# ── Paths — update these to match your RPi filesystem ─────────────────────────
WHISPER_BIN   = os.path.expanduser('~/whisper.cpp/main')
WHISPER_MODEL = os.path.expanduser('~/whisper.cpp/models/ggml-small.bin')
LLAMA_BIN     = os.path.expanduser('~/llama.cpp/llama-cli')
LLAMA_MODEL   = os.path.expanduser('~/models/llama-3.2-3b.Q4_K_M.gguf')
PIPER_BIN     = os.path.expanduser('~/piper/piper')
PIPER_MODEL   = os.path.expanduser('~/piper/es_ES-davefx-medium.onnx')

# ── UART config ────────────────────────────────────────────────────────────────
PORT           = '/dev/ttyAMA0'
RECORD_BAUD    = 115200   # STM32 → RPi (mic, ADC stream)
PLAYBACK_BAUD  = 460800   # RPi → STM32 (audio out)

# ── Recording config (must match record.py / STM32 framing) ───────────────────
SAMPLE_RATE    = 5760   # Hz — 115200 / (2 bytes × 10 UART bits)
RECORD_SECONDS = 6
BIAS           = 2048   # SPW2430 DC bias midpoint
GAIN           = 20     # amplify small mic signal

# ── Playback framing (must match passback/rpi_send.py) ────────────────────────
MAGIC = b'\xDE\xAD\xBE\xEF'


# ── Step 1: Record ─────────────────────────────────────────────────────────────
def record_audio(filename: str) -> None:
    s = serial.Serial(PORT, RECORD_BAUD, timeout=2)
    s.flushInput()
    total = SAMPLE_RATE * RECORD_SECONDS
    samples = []
    print(f"[1/5] Recording {RECORD_SECONDS}s...")
    t0 = time.time()
    while len(samples) < total:
        b = s.read(1)
        if not b or not (b[0] & 0x80):
            continue
        d = s.read(1)
        if not d or (d[0] & 0x80):
            continue
        val    = ((b[0] & 0x3F) << 6) | (d[0] & 0x3F)
        signed = max(-32768, min(32767, (val - BIAS) * GAIN))
        samples.append(signed)
    s.close()
    with wave.open(filename, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(struct.pack(f'<{len(samples)}h', *samples))
    print(f"    Saved {len(samples)} samples in {time.time()-t0:.1f}s → {filename}")


# ── Step 2: Whisper STT ────────────────────────────────────────────────────────
def transcribe(wav_path: str) -> str:
    print("[2/5] Transcribing with Whisper...")
    # whisper.cpp outputs <wav_path>.txt when -otxt is set
    txt_path = wav_path + '.txt'
    subprocess.run([
        WHISPER_BIN,
        '-m', WHISPER_MODEL,
        '-f', wav_path,
        '--language', 'en',
        '--no-timestamps',
        '-otxt',
    ], check=True, capture_output=True)
    with open(txt_path) as f:
        text = f.read().strip()
    os.remove(txt_path)
    print(f"    English: {text!r}")
    return text


# ── Step 3: LLaMA translation ──────────────────────────────────────────────────
def translate(english_text: str) -> str:
    print("[3/5] Translating with LLaMA...")
    prompt = (
        "You are a medical interpreter. Translate the following English text "
        "to Spanish. Output only the Spanish translation, nothing else.\n\n"
        f"English: {english_text}\nSpanish:"
    )
    result = subprocess.run([
        LLAMA_BIN,
        '-m',  LLAMA_MODEL,
        '-p',  prompt,
        '--temp', '0',
        '-n',  '256',
        '--log-disable',
    ], capture_output=True, text=True, check=True)
    # llama-cli prints the prompt then the completion — grab text after "Spanish:"
    output = result.stdout
    if 'Spanish:' in output:
        spanish = output.split('Spanish:')[-1].strip().splitlines()[0].strip()
    else:
        spanish = output.strip().splitlines()[-1].strip()
    print(f"    Spanish: {spanish!r}")
    return spanish


# ── Step 4: Piper TTS ──────────────────────────────────────────────────────────
def synthesize(spanish_text: str, out_wav: str) -> None:
    print("[4/5] Generating Spanish speech with Piper...")
    subprocess.run([
        PIPER_BIN,
        '--model',       PIPER_MODEL,
        '--output_file', out_wav,
    ], input=spanish_text.encode(), check=True, capture_output=True)
    with wave.open(out_wav) as wf:
        rate = wf.getframerate()
        n    = wf.getnframes()
    print(f"    Generated {n/rate:.1f}s of audio at {rate} Hz → {out_wav}")


# ── Step 5: Send WAV to STM32 ──────────────────────────────────────────────────
def send_audio(wav_path: str) -> None:
    with wave.open(wav_path) as wf:
        assert wf.getnchannels() == 1, "Piper should output mono"
        assert wf.getsampwidth() == 2, "Expected 16-bit output from Piper"
        rate   = wf.getframerate()
        n      = wf.getnframes()
        raw    = wf.readframes(n)

    print(f"[5/5] Sending {len(raw)} bytes to STM32 at {PLAYBACK_BAUD} baud...")
    s = serial.Serial(PORT, PLAYBACK_BAUD, timeout=5)
    s.flushInput()
    time.sleep(0.1)

    # Header: magic(4) + sample_rate(4 LE) + num_samples(4 LE)
    s.write(MAGIC + struct.pack('<II', rate, n))

    # Stream PCM paced to the playback sample rate so STM32 buffer doesn't overflow
    chunk = 512
    t0 = time.time()
    for i in range(0, len(raw), chunk):
        s.write(raw[i:i + chunk])
        sent = i + len(raw[i:i + chunk])
        due  = sent / (rate * 2)
        lag  = due - (time.time() - t0)
        if lag > 0:
            time.sleep(lag)
    s.close()
    print("    Done.")


# ── Main loop ──────────────────────────────────────────────────────────────────
def run_once(index: int) -> None:
    with tempfile.TemporaryDirectory() as tmp:
        mic_wav  = os.path.join(tmp, f'mic_{index:03d}.wav')
        spk_wav  = os.path.join(tmp, f'spk_{index:03d}.wav')

        record_audio(mic_wav)
        english  = transcribe(mic_wav)

        if not english:
            print("    No speech detected, skipping.")
            return

        spanish  = translate(english)
        synthesize(spanish, spk_wav)
        send_audio(spk_wav)


if __name__ == '__main__':
    print("Edge-MD Pipeline ready. Press Ctrl-C to stop.\n")
    count = 0
    while True:
        try:
            run_once(count)
            count += 1
            print()
        except KeyboardInterrupt:
            print("\nStopped.")
            break
        except subprocess.CalledProcessError as e:
            print(f"Subprocess error: {e.stderr.decode(errors='ignore')[-300:]}")
            count += 1
        except Exception as e:
            print(f"Error: {e}")
            count += 1
