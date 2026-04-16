import serial
import wave
import struct
import time

# 2 bytes per sample, each UART byte = 10 bits (8 data + start + stop)
# Max sample rate = 115200 / (2 * 10) = 5760 Hz
SAMPLE_RATE = 5760
RECORD_SECONDS = 10
PORT = '/dev/ttyAMA0'
BAUD = 115200

# SPW2430 output is biased at VCC/2 (~2048 ADC counts).
# Voice swings only ~±50 counts at normal distance.
# Gain amplifies that small swing to fill the 16-bit range.
BIAS = 2048
GAIN = 20

def record(filename):
    s = serial.Serial(PORT, BAUD, timeout=2)
    s.flushInput()
    samples = []
    total = SAMPLE_RATE * RECORD_SECONDS
    print(f"Recording {RECORD_SECONDS}s ({total} samples) -> {filename}...")
    t0 = time.time()
    while len(samples) < total:
        # Sync: scan for a byte with bit7=1 (header byte)
        b = s.read(1)
        if not b or not (b[0] & 0x80):
            continue
        # Read the data byte — must have bit7=0
        d = s.read(1)
        if not d or (d[0] & 0x80):
            continue
        # Reconstruct 12-bit ADC value
        val = ((b[0] & 0x3F) << 6) | (d[0] & 0x3F)
        # Center on bias, amplify, clamp to 16-bit signed range
        signed = max(-32768, min(32767, (val - BIAS) * GAIN))
        samples.append(signed)
    elapsed = time.time() - t0
    actual_rate = len(samples) / elapsed
    print(f"Collected {len(samples)} samples in {elapsed:.2f}s (actual rate: {actual_rate:.0f} Hz)")
    with wave.open(filename, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(struct.pack(f'<{len(samples)}h', *samples))
    s.close()
    print(f"Saved {filename}")

count = 0
while True:
    try:
        filename = f"recording_{count:03d}.wav"
        record(filename)
        count += 1
    except KeyboardInterrupt:
        print("Stopped.")
        break
    except Exception as e:
        print(f"Error: {e}")
        count += 1
