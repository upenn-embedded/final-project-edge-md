import spidev
import wave
import sys
import time

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 3_900_000   # ~7.6x headroom over minimum — plenty of margin
spi.mode = 0

SAMPLE_RATE = 16000
CHUNK_FRAMES = 256             # keep your chunk size, it's fine

def stream_wav(filename):
    wf = wave.open(filename, 'rb')
    n_frames  = wf.getnframes()
    framerate = wf.getframerate()
    duration  = n_frames / framerate
    print(f'Playing: {filename} {duration:.2f}s')

    # pre-build entire i2s byte buffer in one shot — faster than per-chunk list append
    all_raw = wf.readframes(n_frames)
    wf.close()

    # mono 16-bit PCM → stereo I2S: each sample duplicated L+R, MSB first
    # use bytearray for speed — avoid Python list overhead in the hot path
    i2s_buf = bytearray(len(all_raw) * 2)   # 2x: mono → stereo
    j = 0
    for i in range(0, len(all_raw), 2):
        lo = all_raw[i]
        hi = all_raw[i + 1]
        i2s_buf[j]     = hi   # L high
        i2s_buf[j + 1] = lo   # L low
        i2s_buf[j + 2] = hi   # R high
        i2s_buf[j + 3] = lo   # R low
        j += 4

    # stream in chunks, paced to real time
    chunk_bytes  = CHUNK_FRAMES * 4   # 4 bytes per stereo I2S frame
    total_chunks = len(i2s_buf) // chunk_bytes
    start        = time.monotonic()   # monotonic is safer than time.time()
    samples_sent = 0

    for c in range(total_chunks):
        offset = c * chunk_bytes
        spi.writebytes2(i2s_buf[offset : offset + chunk_bytes])

        samples_sent += CHUNK_FRAMES
        deadline = samples_sent / SAMPLE_RATE
        elapsed  = time.monotonic() - start
        drift    = deadline - elapsed
        if drift > 0:
            time.sleep(drift)

    # handle remainder (if total frames not divisible by CHUNK_FRAMES)
    remainder = len(i2s_buf) % chunk_bytes
    if remainder:
        spi.writebytes2(i2s_buf[-remainder:])

    print('Done.')

try:
    filename = sys.argv[1] if len(sys.argv) > 1 else 'spanish_16k.wav'
    while True:
        stream_wav(filename)
except KeyboardInterrupt:
    print('\nStopped.')
finally:
    spi.close()
