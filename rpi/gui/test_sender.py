import socket
import json
import time
import random

PAYLOADS = [
    {
        "direction": "Spanish to English",
        "original": "Hola, ¿cómo estás?",
        "translated": "Hello, how are you?"
    },
    {
        "direction": "English to Spanish",
        "original": "Where is the nearest train station?",
        "translated": "¿Dónde está la estación de tren más cercana?"
    },
    {
        "direction": "Spanish to English",
        "original": "Una mesa para dos, por favor.",
        "translated": "A table for two, please."
    }
]

HOST = '127.0.0.1'
PORT = 5001

def main():
    print(f"Test sender trying to reach Display GUI at {HOST}:{PORT}")
    
    # Continuously attempt to connect and send payloads
    while True:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                print("Established Connection. Pushing payloads...")
                
                # Start pushing JSON
                while True:
                    payload = random.choice(PAYLOADS)
                    # We terminate with a newline so readline() works on the display app side
                    data = json.dumps(payload) + "\n"
                    
                    s.sendall(data.encode('utf-8'))
                    print(f"Pushed to GUI -> {payload['original']}")
                    
                    # Push every 2 seconds simulating live speech 
                    time.sleep(2)
                    
        except ConnectionRefusedError:
            print(f"Connection refused (app not running yet?). Retrying in 2 seconds...")
            time.sleep(2)
        except Exception as e:
            print(f"Connection dropped: {e}. Attempting reconnect...")
            time.sleep(2)

if __name__ == "__main__":
    main()
