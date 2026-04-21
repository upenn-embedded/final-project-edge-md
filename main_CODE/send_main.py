
import serial
import wave
import struct
import time
import os 

# ══════════════════════════════════════════════════════════════════════════════
# USER SETTINGS
# ══════════════════════════════════════════════════════════════════════════════

RECORD_SECONDS = 10
OUTPUT_FILE    = "recording.wav"
PORT           = "/dev/ttyAMA0"
BAUD           = 921600
SAMPLE_RATE    = 16000
LOOP_FOREVER   = True       # set False to run only once
# ══════════════════════════════════════════════════════════════════════════════

def encode_sample(val16):
    """Encode 16-bit signed PCM into 2-byte frame for STM32."""
    val12 = ((val16 >> 4) + 2048) & 0xFFF
    b0 = 0x80 | (val12 >> 6)
    b1 = val12 & 0x3F
    return bytes([b0, b1])

def playback(ser):
    """Play back WAV file through STM32 I2S speaker over UART."""
    if not os.path.exists(OUTPUT_FILE):
        print("  [PLAY] No file to play")
        return

    wf = wave.open(OUTPUT_FILE, 'r')
    channels    = wf.getnchannels()
    total       = wf.getnframes()
    rate        = wf.getframerate()

    print(f"  [PLAY] Playing {OUTPUT_FILE} ({total/rate:.1f}s)...")

    CHUNK = 256
    sent  = 0

    while True:
        frames = wf.readframes(CHUNK)
        if not frames:
            break

        num = len(frames) // 2
        samples = struct.unpack(f'<{num}h', frames)

        # Mix stereo to mono if needed
        if channels == 2:
            samples = [(samples[i] + samples[i+1]) // 2
                       for i in range(0, len(samples), 2)]

        packet = b''.join(encode_sample(s) for s in samples)
        ser.write(packet)
        sent += len(samples)

    wf.close()
    print(f"  [PLAY] Done. Sent {sent} samples.") 
