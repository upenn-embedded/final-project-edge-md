import serial
import wave
import struct

SAMPLE_RATE = 8000
RECORD_SECONDS = 10
PORT = '/dev/ttyAMA10'
BAUD = 9600

def record(filename):
    s = serial.Serial(PORT, BAUD, timeout=2)
    s.flushInput()
    samples = []
    total = SAMPLE_RATE * RECORD_SECONDS
    print(f"Recording {RECORD_SECONDS}s -> {filename}...")
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
                signed = int((val / 4095.0) * 65535) - 32768
                samples.append(signed)
    with wave.open(filename, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        for sample in samples:
            wf.writeframes(struct.pack('<h', sample))
    s.close()
    print(f"Saved {filename}")

count = 0
while True:
    try:
        filename = f"recording_{count:03d}.wav"
        record(filename)
        count += 1
    except Exception as e:
        print(f"Error: {e}")
        count += 1