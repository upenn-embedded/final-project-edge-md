from huggingface_hub import hf_hub_download
from llama_cpp import Llama

# Llama 3.2 3B Q4_K_M — Bartowski's quants are the community standard
path = hf_hub_download(
    repo_id="bartowski/Llama-3.2-3B-Instruct-GGUF",
    filename="Llama-3.2-3B-Instruct-Q4_K_M.gguf",
)

llm = Llama(model_path=path, n_ctx=2048, n_threads=4, verbose=False)  

def translate(english_text: str) -> str: 
    prompt = (
        "You are a medical interpreter. Translate the following English text "
        "to Spanish. Output only the Spanish translation directly, nothing else no extra notes, speeches, rambles, or explanations.\n\n"
        f"English: {english_text}\nSpanish:"
    )
    response = llm(prompt, max_tokens=512, temperature=0)
    return response['choices'][0]['text'].strip()

if __name__ == '__main__':
    import sys
    text = sys.argv[1] if len(sys.argv) > 1 else "The patient has a fever and headache."
    print(f"English: {text}")
    spanish = translate(text)
    print(f"Spanish: {spanish}")
