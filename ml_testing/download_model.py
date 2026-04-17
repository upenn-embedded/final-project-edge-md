"""
Download fine-tuned Spanish->English medical translator from Hugging Face.
Pulls both the fp16 merged model (for laptop testing) and the Q4_K_M GGUF
(for Pi deployment / fast laptop inference).

Usage:
    # First, log in once:  huggingface-cli login
    python download_model.py
"""

from pathlib import Path
from huggingface_hub import snapshot_download, list_repo_files

# ---- EDIT THIS ----
REPO = "dammi19/medical-translator-es-en"   # <-- replace with your actual HF repo
# -------------------


def download_fp16(repo_id: str, local_dir: str = "./merged_16bit") -> str:
    """Download the fp16 merged model folder (everything except GGUF files)."""
    print(f"\n=== Downloading fp16 merged model ===")
    path = snapshot_download(
        repo_id=repo_id,
        local_dir=local_dir,
        ignore_patterns=["*.gguf"],   # GGUFs live in the same repo but we want them separately
    )
    print(f"fp16 model downloaded to: {path}")
    return path


def download_gguf(repo_id: str, local_dir: str = "./") -> str:
    """Download the GGUF file. Finds whatever Unsloth named it."""
    print(f"\n=== Downloading GGUF model ===")

    # Unsloth's GGUF filenames vary by version — find whatever's in the repo
    files = list_repo_files(repo_id)
    gguf_files = [f for f in files if f.endswith(".gguf")]

    if not gguf_files:
        raise FileNotFoundError(f"No .gguf file found in {repo_id}")

    # Prefer Q4_K_M if multiple quantizations are present
    target = next((f for f in gguf_files if "Q4_K_M" in f or "q4_k_m" in f), gguf_files[0])
    print(f"Found GGUF file in repo: {target}")

    path = snapshot_download(
        repo_id=repo_id,
        local_dir=local_dir,
        allow_patterns=[target],
    )
    gguf_full_path = str(Path(path) / target)
    print(f"GGUF downloaded to: {gguf_full_path}")
    return gguf_full_path


if __name__ == "__main__":
    if REPO == "your-username/medical-translator-es-en":
        print("ERROR: Edit REPO at the top of this file to your actual HF repo path")
        print('Example: REPO = "damodarpai/medical-translator-es-en"')
        exit(1)

    fp16_dir = download_fp16(REPO)
    gguf_file = download_gguf(REPO)

    print("\n" + "=" * 60)
    print("Downloads complete.")
    print(f"fp16 directory:  {fp16_dir}")
    print(f"GGUF file:       {gguf_file}")
    print("=" * 60)
    print("\nNext: run test_fp16.py and test_gguf.py to sanity-check each model.")
