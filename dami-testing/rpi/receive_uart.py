#!/usr/bin/env python3
"""
Receive a line of text from the STM32 over UART (GPIO14=RX, GPIO15=TX).

Wiring:
    STM32 PA11 (USART6 TX) -> RPi GPIO15 / header pin 10 (RXD)
    STM32 PA12 (USART6 RX) <- RPi GPIO14 / header pin 8  (TXD)
    GND <-> GND

Prereqs:
    sudo raspi-config -> Interface Options -> Serial Port
      * login shell over serial: No
      * serial hardware: Yes
    sudo apt install python3-serial
"""

import sys

try:
    import serial
except ImportError:
    print("pyserial not installed. Run: sudo apt install python3-serial", file=sys.stderr)
    sys.exit(1)

PORT = "/dev/serial0"     # symlinks to the primary UART on GPIO14/15
BAUD = 115200
EXPECTED = "hola como estas"
TRANSLATION = "hello how are you"


def main() -> None:
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"Failed to open {PORT}: {e}", file=sys.stderr)
        print("Is UART enabled? sudo raspi-config -> Interface Options -> Serial Port", file=sys.stderr)
        sys.exit(1)

    print(f"Listening on {PORT} @ {BAUD} baud...")
    print(f"Expecting: \"{EXPECTED}\"")
    print("Press Ctrl-C to stop.\n")

    try:
        while True:
            raw = ser.readline()           # blocks until '\n' or 1s timeout
            if not raw:
                continue                    # timeout, loop and keep listening

            line = raw.decode("ascii", errors="replace").strip()
            if not line:
                continue

            print(f"RECEIVED: {line!r}")

            if line == EXPECTED:
                print(f"  [OK] Match! Translation: \"{TRANSLATION}\"\n")
            else:
                print(f"  [--] No match (expected \"{EXPECTED}\")\n")

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
