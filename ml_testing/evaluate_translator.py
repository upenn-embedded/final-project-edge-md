"""
Evaluate fine-tuned Spanish->English medical translator on EMEA corpus.
Compares fp16 merged model (transformers) against Q4_K_M GGUF (llama.cpp).

Usage:
    python evaluate_translator.py \
        --fp16-path ./merged_16bit \
        --gguf-path ./model.Q4_K_M.gguf \
        --es-file EMEA_en-es.es \
        --en-file EMEA_en-es.en \
        --n-samples 100 \
        --output results.csv

Requires:
    pip install torch transformers llama-cpp-python sacrebleu pandas tqdm
"""

import argparse
import csv
import random
import time
from pathlib import Path

import pandas as pd
import sacrebleu
import torch
from llama_cpp import Llama
from tqdm import tqdm
from transformers import AutoModelForCausalLM, AutoTokenizer


# Must match the prompt format used during training
PROMPT_TEMPLATE = "Translate this Spanish medical text to English:\n{src}"


def load_parallel_corpus(
    es_path, en_path, n_samples,
    start_line=None, end_line=None,
    min_chars=15, max_chars=300, seed=42,
):
    """Load parallel sentences from a specific line range, filter by length, sample randomly.

    Line numbers are 1-indexed and inclusive. If start/end are None, uses the whole file.
    """
    with open(es_path, encoding="utf-8") as f_es, open(en_path, encoding="utf-8") as f_en:
        pairs = []
        for i, (es_line, en_line) in enumerate(zip(f_es, f_en), start=1):
            if start_line is not None and i < start_line:
                continue
            if end_line is not None and i > end_line:
                break
            es = es_line.strip()
            en = en_line.strip()
            if not es or not en:
                continue
            # Filter: skip codes/headers (too short) and very long sentences
            if not (min_chars <= len(es) <= max_chars):
                continue
            if not (min_chars <= len(en) <= max_chars):
                continue
            # Skip lines that are mostly numbers/codes
            if sum(c.isalpha() for c in es) < len(es) * 0.5:
                continue
            pairs.append((i, es, en))

    range_desc = f"lines {start_line or 1}–{end_line or 'EOF'}"
    print(f"Loaded {len(pairs):,} candidate pairs from {range_desc} after filtering")
    random.seed(seed)
    random.shuffle(pairs)
    return pairs[:n_samples]


def translate_fp16(model, tokenizer, spanish_text, max_new_tokens=128):
    """Run one translation through the fp16 transformers model."""
    prompt = PROMPT_TEMPLATE.format(src=spanish_text)
    inputs = tokenizer(prompt, return_tensors="pt").to(model.device)
    with torch.no_grad():
        outputs = model.generate(
            **inputs,
            max_new_tokens=max_new_tokens,
            do_sample=False,
            pad_token_id=tokenizer.eos_token_id,
        )
    full = tokenizer.decode(outputs[0], skip_special_tokens=True)
    # Strip the prompt to get just the translation
    return full[len(prompt):].strip().split("\n")[0].strip()


def translate_gguf(llm, spanish_text, max_new_tokens=128):
    """Run one translation through the GGUF model via llama.cpp."""
    prompt = PROMPT_TEMPLATE.format(src=spanish_text)
    output = llm(
        prompt,
        max_tokens=max_new_tokens,
        temperature=0.0,
        stop=["\n\n", "Translate this"],
        echo=False,
    )
    return output["choices"][0]["text"].strip().split("\n")[0].strip()


def compute_metrics(predictions, references):
    """BLEU + chrF corpus-level, plus per-sentence scores for the CSV."""
    bleu = sacrebleu.corpus_bleu(predictions, [references])
    chrf = sacrebleu.corpus_chrf(predictions, [references])

    per_sent_bleu = [
        sacrebleu.sentence_bleu(p, [r]).score for p, r in zip(predictions, references)
    ]
    per_sent_chrf = [
        sacrebleu.sentence_chrf(p, [r]).score for p, r in zip(predictions, references)
    ]
    exact_matches = sum(1 for p, r in zip(predictions, references) if p.lower().strip() == r.lower().strip())

    return {
        "corpus_bleu": bleu.score,
        "corpus_chrf": chrf.score,
        "exact_match_pct": 100 * exact_matches / len(predictions),
        "per_sent_bleu": per_sent_bleu,
        "per_sent_chrf": per_sent_chrf,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fp16-path", required=True, help="Path to merged fp16 model directory")
    parser.add_argument("--gguf-path", required=True, help="Path to .gguf file")
    parser.add_argument("--es-file", required=True, help="Spanish source corpus")
    parser.add_argument("--en-file", required=True, help="English reference corpus")
    parser.add_argument("--n-samples", type=int, default=50)
    parser.add_argument(
        "--start-line", type=int, default=200_000,
        help="1-indexed start line of the corpus range to sample from (default: 200000, skips header/early BS)"
    )
    parser.add_argument(
        "--end-line", type=int, default=500_000,
        help="1-indexed end line of the corpus range (default: 500000, middle of the corpus where prose is densest)"
    )
    parser.add_argument("--output", default="results.csv")
    parser.add_argument("--skip-fp16", action="store_true", help="Only evaluate GGUF")
    parser.add_argument("--skip-gguf", action="store_true", help="Only evaluate fp16")
    parser.add_argument("--n-ctx", type=int, default=512, help="GGUF context window")
    parser.add_argument("--n-threads", type=int, default=None, help="GGUF CPU threads (default: auto)")
    args = parser.parse_args()

    # Load test set
    print(f"\n=== Loading corpus ===")
    test_pairs = load_parallel_corpus(
        args.es_file, args.en_file, args.n_samples,
        start_line=args.start_line, end_line=args.end_line,
    )
    print(f"Sampled {len(test_pairs)} pairs for evaluation\n")

    line_nums = [i for i, _, _ in test_pairs]
    sources = [es for _, es, _ in test_pairs]
    references = [en for _, _, en in test_pairs]

    results = {"line": line_nums, "source": sources, "reference": references}
    timings = {}

    # ---- fp16 evaluation ----
    if not args.skip_fp16:
        print("=== Loading fp16 model ===")
        tokenizer = AutoTokenizer.from_pretrained(args.fp16_path)
        model = AutoModelForCausalLM.from_pretrained(
            args.fp16_path,
            torch_dtype=torch.float16,
            device_map="auto",
        )
        model.eval()
        device = next(model.parameters()).device
        print(f"fp16 model loaded on: {device}\n")

        print("=== Translating with fp16 ===")
        fp16_preds = []
        t0 = time.time()
        for src in tqdm(sources, desc="fp16"):
            try:
                fp16_preds.append(translate_fp16(model, tokenizer, src))
            except Exception as e:
                print(f"\nfp16 error on: {src[:60]}... -> {e}")
                fp16_preds.append("")
        timings["fp16_total_sec"] = time.time() - t0
        timings["fp16_sec_per_sentence"] = timings["fp16_total_sec"] / len(sources)
        results["fp16_prediction"] = fp16_preds

        # Free GPU memory before loading GGUF
        del model, tokenizer
        if torch.cuda.is_available():
            torch.cuda.empty_cache()

    # ---- GGUF evaluation ----
    if not args.skip_gguf:
        print("\n=== Loading GGUF model ===")
        llm_kwargs = {
            "model_path": args.gguf_path,
            "n_ctx": args.n_ctx,
            "verbose": False,
        }
        if args.n_threads:
            llm_kwargs["n_threads"] = args.n_threads
        llm = Llama(**llm_kwargs)
        print("GGUF model loaded\n")

        print("=== Translating with GGUF ===")
        gguf_preds = []
        t0 = time.time()
        for src in tqdm(sources, desc="gguf"):
            try:
                gguf_preds.append(translate_gguf(llm, src))
            except Exception as e:
                print(f"\ngguf error on: {src[:60]}... -> {e}")
                gguf_preds.append("")
        timings["gguf_total_sec"] = time.time() - t0
        timings["gguf_sec_per_sentence"] = timings["gguf_total_sec"] / len(sources)
        results["gguf_prediction"] = gguf_preds

    # ---- Metrics ----
    print("\n=== Computing metrics ===")
    summary = {}
    if "fp16_prediction" in results:
        m = compute_metrics(results["fp16_prediction"], references)
        summary["fp16"] = {
            "BLEU": m["corpus_bleu"],
            "chrF": m["corpus_chrf"],
            "exact_match_%": m["exact_match_pct"],
            "avg_sec/sent": timings.get("fp16_sec_per_sentence", 0),
        }
        results["fp16_bleu"] = m["per_sent_bleu"]
        results["fp16_chrf"] = m["per_sent_chrf"]

    if "gguf_prediction" in results:
        m = compute_metrics(results["gguf_prediction"], references)
        summary["gguf"] = {
            "BLEU": m["corpus_bleu"],
            "chrF": m["corpus_chrf"],
            "exact_match_%": m["exact_match_pct"],
            "avg_sec/sent": timings.get("gguf_sec_per_sentence", 0),
        }
        results["gguf_bleu"] = m["per_sent_bleu"]
        results["gguf_chrf"] = m["per_sent_chrf"]

    # ---- Save per-sentence CSV ----
    df = pd.DataFrame(results)
    df.to_csv(args.output, index=False, quoting=csv.QUOTE_ALL)
    print(f"\nPer-sentence results saved to {args.output}")

    # ---- Print summary ----
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    summary_df = pd.DataFrame(summary).T
    print(summary_df.to_string(float_format=lambda x: f"{x:.2f}"))

    # ---- Head-to-head: where does GGUF diverge most from fp16? ----
    if "fp16_prediction" in results and "gguf_prediction" in results:
        print("\n=== Largest BLEU drops from fp16 -> GGUF ===")
        df["bleu_delta"] = df["fp16_bleu"] - df["gguf_bleu"]
        worst = df.nlargest(5, "bleu_delta")[
            ["source", "reference", "fp16_prediction", "gguf_prediction", "bleu_delta"]
        ]
        for _, row in worst.iterrows():
            print(f"\nΔBLEU: {row['bleu_delta']:.1f}")
            print(f"  ES:   {row['source']}")
            print(f"  REF:  {row['reference']}")
            print(f"  fp16: {row['fp16_prediction']}")
            print(f"  gguf: {row['gguf_prediction']}")

    print("\nDone.")


if __name__ == "__main__":
    main()
