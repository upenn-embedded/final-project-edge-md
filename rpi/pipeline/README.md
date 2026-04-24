Main Raspberry Pi pipeline entry point and stages.

- Start with `main_code.py` for record -> transcribe -> translate -> synthesize.
- Stage files (`record_main.py`, `whisper_main.py`, `Llama_main.py`, `piper_main.py`) can run independently for testing.
- Generated outputs are written under `rpi/pipeline/output/`.
