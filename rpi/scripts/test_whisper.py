#!/usr/bin/env python3
"""
Quick test: run whisper.cpp on a WAV file and save transcription to .txt
Usage: python3 test_whisper.py samples/recording_000.wav
"""
import subprocess
import sys
import os
from pathlib import Path

WHISPER_BIN   = os.path.expanduser('~/whisper.cpp/build/bin/whisper-cli')
WHISPER_MODEL = os.path.expanduser('~/whisper.cpp/models/ggml-small.en.bin')

def transcribe(wav_path: str) -> str:
    wav_path = os.path.abspath(wav_path)
    if not os.path.exists(wav_path):
        raise FileNotFoundError(f"WAV file not found: {wav_path}")

    # Output file prefix: same name as WAV, no extension
    out_prefix = os.path.splitext(wav_path)[0] + '_transcription'

    result = subprocess.run([
        # Using the whisper.cpp CLI because whisper.cpp is native C++.
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

    txt_path = out_prefix + '.txt'
    with open(txt_path) as f:
        text = f.read().strip()
    return text, txt_path


if __name__ == '__main__':
    default_wav = Path(__file__).resolve().parents[2] / "samples" / "recording_000.wav"
    wav = sys.argv[1] if len(sys.argv) > 1 else str(default_wav)
    print(f"Transcribing: {wav}")
    text, txt_path = transcribe(wav)
    print(f"\nTranscription:\n{text}")
    print(f"\nSaved to: {txt_path}")
