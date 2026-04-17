import serial

ser = serial.Serial('/dev/serial0', 115200, timeout=1)

print("Listening on Pi UART...")

try:
    while True:
        data = ser.read(128)
        if data:
            print(data.decode('utf-8', errors='replace'), end='')
except KeyboardInterrupt:
    ser.close()
    print("\nClosed")
