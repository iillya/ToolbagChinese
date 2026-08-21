#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Machine-translate new dictionary candidates (English -> Chinese) and merge them
into dist/dictionary.txt.

Candidate sources (in priority order):
  1. reports/captured_to_translate.txt   (produced by merge_captured.py)
  2. reports/dictionary_static_ui_candidates.txt   (legacy scan output)
  3. a path passed as the first CLI argument

Usage:
    python scripts/translate_merge.py [candidates_file]

Notes:
  * Requires the 'argostranslate' package and an en->zh offline model.
  * Makes a timestamped backup of the dictionary before merging.
"""
import shutil
import sys
import time
from pathlib import Path

try:
    from argostranslate import package, translate
except ImportError as e:  # pragma: no cover - user machine may not have it
    raise SystemExit(
        "argostranslate is not installed. Install it and an en->zh model first:\n"
        "  pip install argostranslate\n"
        "  (then run translate_merge.py once to auto-download the model)"
    ) from e

ROOT = Path(__file__).resolve().parent.parent.parent.parent
DICT = ROOT / "plugin" / "data" / "dictionary.txt"
REPORTS = ROOT / "reports"
DEFAULT_CAND = REPORTS / "captured_to_translate.txt"
LEGACY_CAND = REPORTS / "dictionary_static_ui_candidates.txt"


def parse_candidates(path):
    """Yield unique candidate English strings from a `English;` (or `English;...`) file."""
    if not path.exists():
        return []
    out = []
    seen = set()
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        # strip trailing comment ("English;    # source ...")
        s = s.split("#", 1)[0].strip()
        # take the part before ';'
        s = s.split(";", 1)[0].strip()
        if s and s not in seen:
            seen.add(s)
            out.append(s)
    return out


def ensure_engine():
    package.update_package_index()
    pkg = next((p for p in package.get_available_packages()
                if p.from_code == "en" and p.to_code.startswith("zh")), None)
    if not pkg:
        raise SystemExit("No English-Chinese translation model available")
    installed = translate.get_installed_languages()
    en = next((x for x in installed if x.code == "en"), None)
    zh = next((x for x in installed if x.code.startswith("zh")), None)
    if not en or not zh:
        package.install_from_path(pkg.download())
        installed = translate.get_installed_languages()
        en = next(x for x in installed if x.code == "en")
        zh = next(x for x in installed if x.code.startswith("zh"))
    return en.get_translation(zh)


def main():
    cand_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_CAND
    if not cand_path.exists():
        cand_path = LEGACY_CAND
    if not cand_path.exists():
        raise SystemExit(f"No candidate file found: {DEFAULT_CAND}")

    current = DICT.read_text(encoding="utf-8", errors="replace").splitlines()
    known = {line.split(";", 1)[0].strip()
             for line in current if ";" in line and not line.lstrip().startswith("#")}

    candidates = [k for k in parse_candidates(cand_path) if k not in known]
    if not candidates:
        print("No new candidates to translate. Dictionary is up to date.")
        return

    print(f"candidates_file={cand_path}")
    print(f"new_candidates={len(candidates)}")

    engine = ensure_engine()

    backup = ROOT / "backups" / f"dictionary.backup-{time.strftime('%Y%m%d-%H%M%S')}.txt"
    shutil.copy2(DICT, backup)

    result = []
    for i, key in enumerate(candidates, 1):
        value = engine.translate(key).strip().replace("；", "，")
        if not value:
            value = key
        result.append(f"{key};{value}")
        if i % 50 == 0:
            print(f"translated {i}/{len(candidates)}", flush=True)

    with DICT.open("a", encoding="utf-8", newline="\n") as f:
        f.write("\n# --- Captured text: machine draft ---\n")
        f.write("\n".join(result) + "\n")

    print(f"merged={len(result)} backup={backup} dictionary={DICT}")


if __name__ == "__main__":
    main()
