#!/usr/bin/env python3
import serial, wave, struct, sys

PORT = "/dev/ttyAMA0"
BAUD = 921600
FILE = sys.argv[1] if len(sys.argv) > 1 else "recording.wav"
GAIN = 8    # ← increase this if still too quiet (try 8, 12, 16)

def encode(val16, gain=GAIN):
    # Amplify
    s = int(val16 * gain)
    # Saturate
    if s >  32767: s =  32767
    if s < -32768: s = -32768
    # Encode to 2-byte frame
    val12 = ((s >> 4) + 2048) & 0xFFF
    return bytes([0x80 | (val12 >> 6), val12 & 0x3F])

ser = serial.Serial(PORT, BAUD, timeout=2)
wf  = wave.open(FILE, 'r')
ch  = wf.getnchannels()

print(f"Playing {FILE} with gain x{GAIN}...")

CHUNK = 256
sent  = 0
while True:
    frames = wf.readframes(CHUNK)
    if not frames: break
    samples = struct.unpack(f'<{len(frames)//2}h', frames)
    if ch == 2:
        samples = [(samples[i]+samples[i+1])//2
                   for i in range(0, len(samples), 2)]
    ser.write(b''.join(encode(s) for s in samples))
    sent += len(samples)

wf.close()
ser.close()
print(f"Done. Sent {sent} samples.")
