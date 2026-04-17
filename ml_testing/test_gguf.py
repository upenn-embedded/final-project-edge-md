"""Quick sanity check that the GGUF model loads and translates."""

import glob
from llama_cpp import Llama

# Find whatever .gguf file exists in current dir (Unsloth naming varies)
gguf_candidates = glob.glob("./*.gguf") + glob.glob("./**/*.gguf", recursive=True)
if not gguf_candidates:
    raise FileNotFoundError("No .gguf file found. Run download_model.py first.")

GGUF_PATH = gguf_candidates[0]
print(f"Using GGUF file: {GGUF_PATH}")

TEST_SENTENCE = (
    "Se recomienda monitorizar el recuento de plaquetas con regularidad "
    "durante las ocho primeras semanas de tratamiento."
)

print("\nLoading GGUF model...")
llm = Llama(
    model_path=GGUF_PATH,
    n_ctx=512,
    verbose=False,
)
print("Loaded.\n")

prompt = f"Translate this Spanish medical text to English:\n{TEST_SENTENCE}"
output = llm(
    prompt,
    max_tokens=128,
    temperature=0.0,
    stop=["\n\n", "Translate this"],
    echo=False,
)

print("=== Translation ===")
print(output["choices"][0]["text"].strip())
