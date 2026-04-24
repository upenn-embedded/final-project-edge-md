import serial
import time

ser = serial.Serial('/dev/serial0', 9600, timeout=0.5)

print("Starting loopback test...")

try:
    while True:
        msg = b'ABC123\r\n'
        ser.write(msg)
        print("Sent:", msg)

        time.sleep(3)

        data = ser.read(64)
        if data:
            print("Received:", data)

        time.sleep(1)

except KeyboardInterrupt:
    ser.close()
    print("Serial closed")
