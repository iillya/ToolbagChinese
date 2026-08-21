#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Phase 0 — dictionary hygiene.

Removes duplicate keys (keeps first occurrence) and reports malformed lines.
Writes a backup before modifying dist/dictionary.txt.
"""
import shutil
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
DICT = ROOT / "plugin" / "data" / "dictionary.txt"


def main():
    raw = DICT.read_text(encoding="utf-8-sig", errors="replace")
    lines = raw.splitlines()
    seen = set()
    kept = []
    removed = []
    malformed = []

    for i, ln in enumerate(lines, 1):
        s = ln.strip()
        if not s or s.startswith("#"):
            kept.append(ln)
            continue
        if ";" in s:
            key = s.split(";", 1)[0].strip()
        elif "\t" in s:
            key = s.split("\t", 1)[0].strip()
        else:
            malformed.append((i, ln))
            kept.append(ln)
            continue
        if key in seen:
            removed.append((i, key, ln))
            continue
        seen.add(key)
        kept.append(ln)

    if removed or malformed:
        stamp = time.strftime("%Y%m%d-%H%M%S")
        backup = ROOT / "backups" / f"dictionary.cleanup-{stamp}.txt"
        shutil.copy2(DICT, backup)
        DICT.write_text("\n".join(kept) + "\n", encoding="utf-8")
        print(f"备份: {backup}")
    else:
        print("无需清理：没有重复或坏行。")

    print(f"保留行数: {len(kept)}")
    print(f"移除重复: {len(removed)}")
    for i, k, ln in removed:
        print(f"  移除 行{i}  {k!r}")
    print(f"坏行(保留): {len(malformed)}")
    for i, ln in malformed:
        print(f"  行{i}: {ln!r}")


if __name__ == "__main__":
    main()
