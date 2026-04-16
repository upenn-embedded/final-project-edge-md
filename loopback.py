import serial
import time

ser = serial.Serial(
    port='/dev/serial0',
    baudrate=115200,
    timeout=1
)

print("Starting loopback test...")

try:
    while True:
        msg = "Hello loopback\n"

        # Send
        ser.write(msg.encode())
        print("Sent:", msg.strip())

        time.sleep(0.1)

        # Receive
        data = ser.readline().decode('utf-8', errors='ignore').strip()

        if data:
            print("Received:", data)

        time.sleep(1)

except KeyboardInterrupt:
    ser.close()
    print("Serial closed")
