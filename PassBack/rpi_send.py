import serial
import wave
import struct
import sys
import time

# Must match STM32 UART config. 460800 gives 46080 bytes/sec,
# enough headroom for 16kHz 16-bit PCM (32000 bytes/sec).
PORT = '/dev/ttyAMA0'
BAUD = 460800

# 8-byte frame header: magic + sample_rate + num_samples
MAGIC = b'\xDE\xAD\xBE\xEF'

def send_wav(filename):
    with wave.open(filename, 'r') as wf:
        channels   = wf.getnchannels()
        sampwidth  = wf.getsampwidth()
        framerate  = wf.getframerate()
        n_frames   = wf.getnframes()
        raw        = wf.readframes(n_frames)

    if channels != 1:
        raise ValueError(f"Expected mono, got {channels} channels")
    if sampwidth != 2:
        raise ValueError(f"Expected 16-bit samples, got {sampwidth*8}-bit")

    print(f"{filename}: {framerate} Hz, {n_frames} samples, {len(raw)} bytes")

    s = serial.Serial(PORT, BAUD, timeout=5)
    s.flushInput()
    time.sleep(0.1)  # let STM32 settle after serial open

    # Header: magic (4B) + sample_rate (4B LE) + num_samples (4B LE)
    header = MAGIC + struct.pack('<II', framerate, n_frames)
    s.write(header)

    # Stream PCM in 512-byte chunks (256 samples) — matches STM32 buffer
    chunk_size = 512
    sent = 0
    t0 = time.time()
    for i in range(0, len(raw), chunk_size):
        chunk = raw[i:i + chunk_size]
        s.write(chunk)
        sent += len(chunk)
        # Pace output to avoid overrunning STM32's UART buffer.
        # Send no faster than the audio playback rate.
        expected_time = sent / (framerate * 2)
        elapsed = time.time() - t0
        if elapsed < expected_time:
            time.sleep(expected_time - elapsed)

    s.close()
    print(f"Sent {sent} bytes in {time.time()-t0:.2f}s")

if __name__ == '__main__':
    fname = sys.argv[1] if len(sys.argv) > 1 else 'recording_000.wav'
    send_wav(fname)
