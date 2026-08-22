#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Coverage / gap report: how much of the captured Toolbag UI text is already in
the dictionary, and what is still untranslated.

Reads:
  plugin/data/dictionary_zh.json             (translated keys)
  reports/all_captured.tsv                   (static scanner, ui tier)
  reports/ui_runtime_dump.txt                (UI Automation dump, if present)
  %TOOLBAG_DIR%\\data\\ChineseLocalizer\\ChineseLocalizer_sniffer.json

Writes:
  reports/coverage_report.txt                (summary + gap list)
  reports/dictionary_gap_candidates.txt      (English; list of missing UI strings)

This is the final quality gate: when the gap list stops growing / reaches the
strings you care about, the localization is effectively "no omission" at runtime.
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
OUT = REPORTS / "coverage_report.txt"
GAP = REPORTS / "dictionary_gap_candidates.txt"

def dict_keys():
    keys = set()
    if not DICT.exists():
        return keys
    return set(json.loads(DICT.read_text(encoding="utf-8-sig")).get("translations", {}))


def main():
    known = dict_keys()
    captured = {}  # text -> set of sources

    if TSV.exists():
        for raw in TSV.read_text(encoding="utf-8", errors="replace").splitlines():
            if not raw or raw.startswith("text\t"):
                continue
            p = raw.split("\t")
            if len(p) >= 4 and p[3] == "yes":
                captured.setdefault(p[0], set()).add(p[1])

    if RUNTIME.exists():
        for raw in RUNTIME.read_text(encoding="utf-8", errors="replace").splitlines():
            s = raw.strip()
            if s:
                captured.setdefault(s, set()).add("ui_runtime_dump")

    if MISSING.exists():
        data = json.loads(MISSING.read_text(encoding="utf-8-sig"))
        for entry in data.get("entries", []):
            text = str(entry.get("text", "")).strip()
            if text:
                captured.setdefault(text, set()).add(f"sniffer:{entry.get('source', 'FONT')}")

    # only count strings that pass the confident-UI filter
    all_ui = {s for s in captured if is_ui_confident(s)}
    covered = {s for s in all_ui if s in known}
    gaps = {s for s in all_ui if s not in known}
    rate = 100.0 * len(covered) / len(all_ui) if all_ui else 0.0

    with OUT.open("w", encoding="utf-8", newline="") as f:
        f.write("Toolbag UI text coverage report\n")
        f.write("=" * 40 + "\n\n")
        f.write(f"dictionary_entries      : {len(known)}\n")
        f.write(f"captured_ui_strings     : {len(all_ui)}\n")
        f.write(f"covered                 : {len(covered)}\n")
        f.write(f"gaps (untranslated)     : {len(gaps)}\n")
        f.write(f"coverage_rate           : {rate:.2f}%\n\n")
        f.write("Top gap sources (most missing strings by source):\n")
        src_count = {}
        for s in gaps:
            for src in captured[s]:
                src_count[src] = src_count.get(src, 0) + 1
        for src, n in sorted(src_count.items(), key=lambda x: -x[1]):
            f.write(f"  {n:6d}  {src}\n")
        f.write("\nGap list (first 1000):\n")
        for i, s in enumerate(sorted(gaps, key=lambda x: (x.lower(), x))):
            if i >= 1000:
                f.write(f"  ... and {len(gaps) - 1000} more (see dictionary_gap_candidates.txt)\n")
                break
            f.write(f"  {s}\n")

    with GAP.open("w", encoding="utf-8", newline="") as f:
        f.write("# Untranslated captured UI strings (English;)\n")
        for s in sorted(gaps, key=lambda x: (x.lower(), x)):
            src = ", ".join(sorted(captured[s]))
            f.write(f"{s};\t# {src}\n")

    print(f"dictionary={len(known)} captured_ui={len(all_ui)} covered={len(covered)} "
          f"gaps={len(gaps)} coverage={rate:.2f}%")
    print(f"report={OUT}\ngap_list={GAP}")


if __name__ == "__main__":
    main()
