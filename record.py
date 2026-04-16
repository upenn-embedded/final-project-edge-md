import serial
import wave
import struct
import time

# At 115200 baud, each 3-byte packet takes 30 bit-times = ~260 µs
# Max throughput: ~3840 packets/sec — use this as actual sample rate.
# To get true 8000 Hz, the STM32 needs a timer gating ADC reads (see note below).
SAMPLE_RATE = 3840
RECORD_SECONDS = 10
PORT = '/dev/ttyAMA0'
BAUD = 115200

def record(filename):
    s = serial.Serial(PORT, BAUD, timeout=2)
    s.flushInput()
    samples = []
    total = SAMPLE_RATE * RECORD_SECONDS
    print(f"Recording {RECORD_SECONDS}s ({total} samples) -> {filename}...")
    t0 = time.time()
    while len(samples) < total:
        b = s.read(1)
        if len(b) == 0:
            continue
        if b[0] == 0xFF:
            data = s.read(2)
            if len(data) < 2:
                continue
            val = data[0] | (data[1] << 8)
            if val <= 4095:
                # Map 0-4095 to signed 16-bit (-32768 to 32767)
                signed = int((val / 4095.0) * 65535) - 32768
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