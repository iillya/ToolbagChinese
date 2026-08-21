#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Machine-translate new dictionary candidates (English -> Chinese) and merge them
into plugin/data/dictionary.txt.

Backend selection:
  1. If DEEPSEEK_API_KEY env var is set -> DeepSeek chat API (batched, fast).
  2. Otherwise fall back to offline argostranslate (en->zh model).

Candidate file (first existing wins):
  - CLI arg
  - reports/missing_ui_confident.txt
  - reports/captured_to_translate.txt

Usage:
    python plugin/scripts/dictionary/translate_merge.py [candidates_file]
    # with DeepSeek:
    set DEEPSEEK_API_KEY=sk-...   (Windows)
    python plugin/scripts/dictionary/translate_merge.py
"""
import json
import os
import shutil
import sys
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
DICT = ROOT / "plugin" / "data" / "dictionary.txt"
REPORTS = ROOT / "reports"
CANDIDATES = [
    REPORTS / "missing_ui_confident.txt",
    REPORTS / "captured_to_translate.txt",
    REPORTS / "dictionary_ui_candidates.txt",
]

DEEPSEEK_URL = "https://api.deepseek.com/chat/completions"
DEEPSEEK_MODEL = os.environ.get("DEEPSEEK_MODEL", "deepseek-chat")
DEEPSEEK_KEY = os.environ.get("DEEPSEEK_API_KEY", "").strip()
BATCH = 50


def parse_candidates(path):
    """Yield unique candidate English strings from a `English;`-style file."""
    if not path.exists():
        return []
    out, seen = [], set()
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        s = s.split("#", 1)[0].strip()
        s = s.split(";", 1)[0].strip()
        if s and s not in seen:
            seen.add(s)
            out.append(s)
    return out


# --------------------------- DeepSeek backend ---------------------------
def deepseek_translate_batch(keys):
    prompt = (
        "You are a professional translator for a 3D software (Marmoset Toolbag 5). "
        "Translate each English UI string to Simplified Chinese. "
        "Keep technical terms accurate to the 3D/rendering context (e.g. Normal = 法线 in "
        "material context, but 正常 in a blend-mode list). Keep placeholders like %s, {0}, "
        "\\n and punctuation. Return ONLY a JSON array of strings, same order and same count.\n"
        "English strings:\n" + json.dumps(keys, ensure_ascii=False)
    )
    body = {
        "model": DEEPSEEK_MODEL,
        "messages": [{"role": "user", "content": prompt}],
        "temperature": 0.2,
        "max_tokens": 4096,
    }
    req = urllib.request.Request(
        DEEPSEEK_URL,
        data=json.dumps(body).encode("utf-8"),
        headers={
            "Content-Type": "application/json",
            "Authorization": "Bearer " + DEEPSEEK_KEY,
        },
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=120) as resp:
        data = json.loads(resp.read().decode("utf-8"))
    content = data["choices"][0]["message"]["content"].strip()
    # tolerate markdown fences
    if content.startswith("```"):
        content = content.strip("`")
        if content.startswith("json"):
            content = content[4:]
    arr = json.loads(content)
    if not isinstance(arr, list):
        raise ValueError("DeepSeek did not return a list")
    return arr


def deepseek_translate_all(keys):
    results = {}
    n = len(keys)
    for i in range(0, n, BATCH):
        batch = keys[i:i + BATCH]
        for attempt in range(3):
            try:
                vals = deepseek_translate_batch(batch)
                if len(vals) != len(batch):
                    raise ValueError("count mismatch")
                for k, v in zip(batch, vals):
                    v = (v or "").strip().replace("；", "，")
                    results[k] = v if v else k
                break
            except Exception as e:
                print(f"  batch {i // BATCH + 1} attempt {attempt + 1} failed: {e}", flush=True)
                time.sleep(2 * (attempt + 1))
        else:
            for k in batch:
                results[k] = k  # keep English on persistent failure
        print(f"translated {min(i + BATCH, n)}/{n}", flush=True)
    return results


# --------------------------- argostranslate backend ---------------------------
def argos_engine():
    try:
        from argostranslate import package, translate
    except ImportError as e:
        raise SystemExit(
            "argostranslate not installed and DEEPSEEK_API_KEY not set. "
            "Either set DEEPSEEK_API_KEY or: pip install argostranslate"
        ) from e
    package.update_package_index()
    pkg = next((p for p in package.get_available_packages()
                if p.from_code == "en" and p.to_code.startswith("zh")), None)
    if not pkg:
        raise SystemExit("No en->zh argos model available")
    installed = translate.get_installed_languages()
    en = next((x for x in installed if x.code == "en"), None)
    zh = next((x for x in installed if x.code.startswith("zh")), None)
    if not en or not zh:
        package.install_from_path(pkg.download())
        installed = translate.get_installed_languages()
        en = next(x for x in installed if x.code == "en")
        zh = next(x for x in installed if x.code.startswith("zh"))
    return en.get_translation(zh)


def argos_translate_all(keys, engine):
    results = {}
    for i, k in enumerate(keys, 1):
        v = engine.translate(k).strip().replace("；", "，")
        results[k] = v if v else k
        if i % 50 == 0:
            print(f"translated {i}/{len(keys)}", flush=True)
    return results


def main():
    cand_path = None
    if len(sys.argv) > 1:
        cand_path = Path(sys.argv[1])
    if not cand_path or not cand_path.exists():
        for p in CANDIDATES:
            if p.exists():
                cand_path = p
                break
    if not cand_path or not cand_path.exists():
        raise SystemExit("No candidate file found. Run scan_all_text.py first.")

    current = DICT.read_text(encoding="utf-8", errors="replace").splitlines()
    known = {line.split(";", 1)[0].strip()
             for line in current if ";" in line and not line.lstrip().startswith("#")}
    candidates = [k for k in parse_candidates(cand_path) if k not in known]
    print(f"candidates_file={cand_path}")
    print(f"new_candidates={len(candidates)}")
    if not candidates:
        print("No new candidates to translate. Dictionary is up to date.")
        return

    use_deepseek = bool(DEEPSEEK_KEY)
    print(f"backend={'deepseek(' + DEEPSEEK_MODEL + ')' if use_deepseek else 'argos'}")

    if use_deepseek:
        results = deepseek_translate_all(candidates)
    else:
        engine = argos_engine()
        results = argos_translate_all(candidates, engine)

    backup = ROOT / "backups" / f"dictionary.backup-{time.strftime('%Y%m%d-%H%M%S')}.txt"
    ROOT.joinpath("backups").mkdir(exist_ok=True)
    shutil.copy2(DICT, backup)

    with DICT.open("a", encoding="utf-8", newline="\n") as f:
        f.write("\n# --- Captured text: machine draft ---\n")
        for k in candidates:
            f.write(f"{k};{results[k]}\n")

    print(f"merged={len(results)} backup={backup} dictionary={DICT}")


if __name__ == "__main__":
    main()