
import spidev
import wave
import sys
import time
import struct

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 500000
spi.mode = 0

SAMPLE_RATE = 16000

def stream_wav(filename):
    wf = wave.open(filename, 'rb')
    n_frames  = wf.getnframes()
    framerate = wf.getframerate()
    print(f'Playing: {filename} {n_frames/framerate:.2f}s')

    start = time.time()
    samples_sent = 0

    while True:
        raw = wf.readframes(256)
        if not raw:
            break

        # build raw I2S frames: each mono sample → stereo frame
        # [LEFT_HI, LEFT_LO, RIGHT_HI, RIGHT_LO]
        i2s_frames = []
        for i in range(0, len(raw), 2):
            lo = raw[i]
            hi = raw[i+1]
            # left channel MSB first
            i2s_frames.append(hi)
            i2s_frames.append(lo)
            # right channel same sample
            i2s_frames.append(hi)
            i2s_frames.append(lo)

        spi.writebytes2(bytes(i2s_frames))

        samples_sent += len(raw) // 2
        elapsed = time.time() - start
        drift = (samples_sent / SAMPLE_RATE) - elapsed
        if drift > 0:
            time.sleep(drift)

    wf.close()
    print('Done.')

try:
    filename = sys.argv[1] if len(sys.argv) > 1 else 'spanish_16k.wav'
    while True:
        stream_wav(filename)
except KeyboardInterrupt:
    print('\nStopped.')
finally:
    spi.close()
