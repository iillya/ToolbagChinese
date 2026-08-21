#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Merge a human/AI reviewed translations file into dist/dictionary.txt.

Translations file format:  英文;中文   (lines starting with # are ignored)

Checks before merging:
  * non-empty key and value
  * no duplicate keys inside the file
  * key not already in the dictionary
Writes a timestamped backup, then appends a clearly-marked section.
"""
import shutil
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
DICT = ROOT / "plugin" / "data" / "dictionary.txt"
ASSETS = ROOT / "plugin" / "data" / "dictionary_assets.txt"


def read_translations(path):
    entries = []
    seen = set()
    problems = []
    for i, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        if ";" not in s:
            problems.append((i, "缺少分号", s[:80]))
            continue
        k, v = s.split(";", 1)
        k = k.strip()
        v = v.strip()
        if not k or not v:
            problems.append((i, "key 或 value 为空", s[:80]))
            continue
        if k in seen:
            problems.append((i, "文件内重复", k))
            continue
        seen.add(k)
        entries.append((k, v))
    return entries, problems


def main():
    # target: --main (default) 或 --assets
    args = [a for a in sys.argv[1:]]
    target = ASSETS if "--assets" in args else DICT
    args = [a for a in args if a != "--assets"]

    if args:
        tpath = Path(args[0])
    else:
        tpath = ROOT / "work" / "phase1_translations.txt"
    if not tpath.exists():
        raise SystemExit(f"找不到翻译文件: {tpath}")

    entries, problems = read_translations(tpath)
    print(f"翻译文件: {tpath}")
    print(f"有效条目: {len(entries)}  问题: {len(problems)}")
    for i, kind, detail in problems[:20]:
        print(f"  [行{i}] {kind}: {detail}")
    if problems:
        print("存在格式问题，已停止合并，请先修正。")
        sys.exit(1)

    # 现有字典 key（同时排除主/素材字典，避免两字典重复）
    known = set()
    for p in (DICT, ASSETS):
        if not p.exists():
            continue
        for raw in p.read_text(encoding="utf-8-sig", errors="replace").splitlines():
            s = raw.strip()
            if not s or s.startswith("#"):
                continue
            if ";" in s:
                known.add(s.split(";", 1)[0].strip())
            elif "\t" in s:
                known.add(s.split("\t", 1)[0].strip())

    new_entries = [(k, v) for k, v in entries if k not in known]
    dup_existing = len(entries) - len(new_entries)
    print(f"已存在(跳过): {dup_existing}  新增: {len(new_entries)}")

    if not new_entries:
        print("没有新条目，无需合并。")
        return

    stamp = time.strftime("%Y%m%d-%H%M%S")
    backup = ROOT / "backups" / f"dictionary.merge-{stamp}.txt"
    shutil.copy2(target, backup)
    print(f"备份: {backup}")

    with target.open("a", encoding="utf-8", newline="\n") as f:
        f.write(f"\n# --- {tpath.stem} (DeepSeek 审核) ---\n")
        for k, v in new_entries:
            f.write(f"{k};{v}\n")
    print(f"已合并 {len(new_entries)} 条到 {target}")


if __name__ == "__main__":
    main()
