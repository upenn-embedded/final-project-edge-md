import spidev
import wave
import sys
import time

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 500000
spi.mode = 0  # back to mode 0
spi.bits_per_word = 8  # 8-bit transfers, no swapping

SAMPLE_RATE = 16000

def stream_wav(filename):
    wf = wave.open(filename, 'rb')
    framerate = wf.getframerate()
    n_frames  = wf.getnframes()
    print(f'Playing: {filename} {framerate}Hz {n_frames/framerate:.2f}s')

    start = time.time()
    samples_sent = 0

    while True:
        # read raw PCM bytes directly, no processing
        raw = wf.readframes(256)
        if not raw:
            break

        # send raw bytes as-is, no byte swapping
        spi.writebytes2(raw)

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
