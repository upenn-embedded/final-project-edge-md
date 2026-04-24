import spidev
import time
import math

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 500000
spi.mode = 0

SAMPLE_RATE = 16000
freq = 1000
amp  = 16000

print('Sending 1kHz test tone...')
t = 0
while True:
    chunk = []
    for i in range(256):
        sample = int(amp * math.sin(2 * math.pi * freq * t / SAMPLE_RATE))
        chunk.append((sample >> 8) & 0xFF)
        chunk.append(sample & 0xFF)
        t += 1
    spi.writebytes2(bytes(chunk))
    # no sleep - let buffer fill naturally
