import subprocess
import sys
import os

WHISPER_BIN   = os.path.expanduser('~/whisper.cpp/build/bin/whisper-cli')
WHISPER_MODEL = os.path.expanduser('~/whisper.cpp/models/ggml-small.en.bin') 

def transcribe(wav_path: str) -> str: 
    wav_path = os.path.abspath(wav_path)
    #absolute path needed for whisper.cpp to find the file
    if not os.path.exists(wav_path):
        raise FileNotFoundError(f"WAV file not found: {wav_path}")

    # Output file prefix: same name as WAV, no extension
    out_prefix = os.path.splitext(wav_path)[0] + '_transcription' 

    result = subprocess.run([# Here we are swithcing to use the whisper command line. Note this is because Whisper.cpp is a C++ file
        WHISPER_BIN, 
        '-m', WHISPER_MODEL, 
        '-f', wav_path, 
        '--language', 'en',  # NOTING THIS IS WHERE WE SPECIFY ENGLISH AS THE MAIN LANGUAGE NOT SPANISH
        '--no-timestamps',
        '-otxt', 
        '-of', out_prefix, 
        ], capture_output=True, text=True)  

    if result.returncode != 0:
        print("whisper.cpp stderr:")
        print(result.stderr[-500:])
        raise RuntimeError("Whisper failed") 
    
    txt_path = out_prefix + '.txt' # Convert to .txt path so that we can read with llama
    with open(txt_path) as f:
        text = f.read().strip()
    return text, txt_path







if __name__ == '__main__':
    wav = sys.argv[1] if len(sys.argv) > 1 else 'recording_000.wav'
    print(f"Transcribing: {wav}")
    text, txt_path = transcribe(wav)
    print(f"\nTranscription:\n{text}")
    print(f"\nSaved to: {txt_path}")
