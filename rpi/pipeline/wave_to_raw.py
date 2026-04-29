import subprocess
import os

# Filenames since they are in the same directory
INPUT_WAV = "spanish_20260417_015738.wav"
OUTPUT_RAW = "out.raw"

def convert():
    if not os.path.exists(INPUT_WAV):
        print(f"❌ Error: {INPUT_WAV} not found in this folder.")
        return

    print(f"Converting {INPUT_WAV} to {OUTPUT_RAW}...")

    # ffmpeg command for STM32 compatibility:
    # 16kHz, Mono, Signed 16-bit Little Endian
    cmd = [
        "ffmpeg", "-y",
        "-i", INPUT_WAV,
        "-f", "s16le",
        "-ar", "16000",
        "-ac", "1",
        OUTPUT_RAW
    ]

    try:
        subprocess.run(cmd, check=True)
        print(f"✅ Success! {OUTPUT_RAW} created.")
    except Exception as e:
        print(f"❌ Failed: {e}")

if __name__ == "__main__":
    convert()