"""Quick sanity check that the fp16 model loads and translates."""

import torch
from transformers import AutoModelForCausalLM, AutoTokenizer

MODEL_PATH = "./merged_16bit"

TEST_SENTENCE = (
    "Se recomienda monitorizar el recuento de plaquetas con regularidad "
    "durante las ocho primeras semanas de tratamiento."
)
# Expected: something like "It is recommended that the platelet count is
# monitored regularly during the first 8 weeks of treatment."

print("Loading fp16 model...")
tokenizer = AutoTokenizer.from_pretrained(MODEL_PATH)
model = AutoModelForCausalLM.from_pretrained(
    MODEL_PATH,
    torch_dtype=torch.float16,
    device_map="auto",
)
model.eval()
print(f"Loaded on device: {next(model.parameters()).device}\n")

prompt = f"Translate this Spanish medical text to English:\n{TEST_SENTENCE}"
inputs = tokenizer(prompt, return_tensors="pt").to(model.device)

with torch.no_grad():
    outputs = model.generate(
        **inputs,
        max_new_tokens=128,
        do_sample=False,
        pad_token_id=tokenizer.eos_token_id,
    )

full = tokenizer.decode(outputs[0], skip_special_tokens=True)
print("=== Full output ===")
print(full)
print("\n=== Just the translation ===")
print(full[len(prompt):].strip())
