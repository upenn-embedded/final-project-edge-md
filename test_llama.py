#!/usr/bin/env python3
"""
Quick test: send English text to LLaMA via Ollama and print Spanish translation.
Ollama must be running: ollama serve
Usage: python3 test_llama.py "The patient has a fever."
"""
import json
import sys
import urllib.request

OLLAMA_URL = "http://localhost:11434/api/generate"
MODEL      = "llama3.1:8b"

def translate(english_text: str) -> str:
    prompt = (
        "You are a medical interpreter. Translate the following English text "
        "to Spanish. Output only the Spanish translation, nothing else.\n\n"
        f"English: {english_text}\nSpanish:"
    )
    body = json.dumps({
        "model":  MODEL,
        "prompt": prompt,
        "stream": False,
        "options": {"temperature": 0},
    }).encode()

    req = urllib.request.Request(
        OLLAMA_URL,
        data=body,
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=120) as r:
        return json.loads(r.read())["response"].strip()


if __name__ == '__main__':
    text = ' '.join(sys.argv[1:]) if len(sys.argv) > 1 else "The patient has a fever and sore throat."
    print(f"English: {text}")
    print("Translating...")
    spanish = translate(text)
    print(f"Spanish: {spanish}")
