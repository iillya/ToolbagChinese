#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Merge every capture source into one deduplicated, translation-ready candidate
list. This closes the "no-omission" loop:

  static scan        reports/all_captured.tsv             (is_ui == yes)
  runtime UI dump    reports/ui_runtime_dump.txt          (dump_ui_text.ps1)
  runtime sniffer    %TOOLBAG_DIR%\\data\\ChineseLocalizer\\ChineseLocalizer_sniffer.json
                     (written by the Font-only F12 capture)

Output:
  reports/captured_to_translate.txt   (English;)  -> feed to translate_merge.py
"""
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from scan_all_text import is_ui_confident  # noqa: E402

PROJECT = Path(__file__).resolve().parent.parent.parent.parent
REPORTS = PROJECT / "reports"
DICT = PROJECT / "plugin" / "data" / "dictionary_zh.json"
TSV = REPORTS / "all_captured.tsv"
RUNTIME = REPORTS / "ui_runtime_dump.txt"
TOOLBAG = Path(os.environ.get("TOOLBAG_DIR", r"C:\Program Files\Marmoset\Toolbag 5"))
MISSING = TOOLBAG / "data" / "ChineseLocalizer" / "ChineseLocalizer_sniffer.json"
OUT = REPORTS / "captured_to_translate.txt"


def existing_keys():
    keys = set()
    if not DICT.exists():
        return keys
    return set(json.loads(DICT.read_text(encoding="utf-8-sig")).get("translations", {}))


def main():
    known = existing_keys()
    captured = {}  # text -> set of sources

    # 1) static scanner TSV (ui tier)
    if TSV.exists():
        for raw in TSV.read_text(encoding="utf-8", errors="replace").splitlines():
            if not raw or raw.startswith("text\t"):
                continue
            parts = raw.split("\t")
            if len(parts) >= 4 and parts[3] == "yes":
                captured.setdefault(parts[0], set()).add(parts[1])

    # 2) runtime UI dump
    if RUNTIME.exists():
        for raw in RUNTIME.read_text(encoding="utf-8", errors="replace").splitlines():
            s = raw.strip()
            if s:
                captured.setdefault(s, set()).add("ui_runtime_dump")

    # 3) runtime missing log
    if MISSING.exists():
        data = json.loads(MISSING.read_text(encoding="utf-8-sig"))
        for entry in data.get("entries", []):
            text = str(entry.get("text", "")).strip()
            if text:
                captured.setdefault(text, set()).add(f"sniffer:{entry.get('source', 'FONT')}")

    items = [(s, sources) for s, sources in captured.items() if is_ui_confident(s) and s not in known]
    items.sort(key=lambda x: (x[0].lower(), x[0]))

    with OUT.open("w", encoding="utf-8", newline="") as f:
        f.write("# Unified capture list (static + runtime + Font sniffer)\n")
        f.write("# Format: English;    # source1, source2 ...\n")
        f.write("\n")
        for s, sources in items:
            src = ", ".join(sorted(sources))
            f.write(f"{s};\t# {src}\n")

    print(f"existing_in_dictionary={len(known)}")
    print(f"new_candidates_to_translate={len(items)}")
    print(f"out={OUT}")


if __name__ == "__main__":
    main()
