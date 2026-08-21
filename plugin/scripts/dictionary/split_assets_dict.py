#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Move asset/library-name sections out of the main dictionary into a separate
dictionary_assets.txt.

Asset sections are marked by a header comment containing "assets" (e.g.
"# --- phase2_assets_batch1 (DeepSeek 审核) ---").

Keeps the main dictionary.txt for genuine UI text only, so the two can be
managed/updated independently.
"""
import shutil
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
MAIN = ROOT / "plugin" / "data" / "dictionary.txt"
ASSETS = ROOT / "plugin" / "data" / "dictionary_assets.txt"


def main():
    lines = MAIN.read_text(encoding="utf-8-sig", errors="replace").splitlines()
    kept = []
    asset_lines = []
    in_asset_section = False

    for ln in lines:
        s = ln.strip()
        if s.startswith("#") and "assets" in s.lower() and "---" in s:
            in_asset_section = True
            asset_lines.append(ln)
            continue
        if in_asset_section:
            if s and not s.startswith("#"):
                asset_lines.append(ln)   # entry line of the asset section
            elif s.startswith("#"):
                # another header: exit asset mode unless it's another asset header
                in_asset_section = "assets" in s.lower() and "---" in s
                if in_asset_section:
                    asset_lines.append(ln)
                else:
                    kept.append(ln)
            else:
                asset_lines.append(ln)   # blank inside section
            continue
        kept.append(ln)

    # filter to only real asset entry lines
    asset_entries = [ln for ln in asset_lines if ln.strip() and not ln.strip().startswith("#")]
    if not asset_entries:
        print("没有发现素材段落，无需拆分。")
        return

    stamp = time.strftime("%Y%m%d-%H%M%S")
    backup_main = ROOT / "backups" / f"dictionary.pre-split-{stamp}.txt"
    shutil.copy2(MAIN, backup_main)
    print(f"主字典备份: {backup_main}")

    # write main without asset sections
    MAIN.write_text("\n".join(kept).rstrip("\n") + "\n", encoding="utf-8")

    # append asset entries to dictionary_assets.txt
    if ASSETS.exists():
        assets_text = ASSETS.read_text(encoding="utf-8-sig", errors="replace").rstrip("\n") + "\n"
    else:
        assets_text = "# 素材/库名称字典（与主 UI 字典分开维护）\n"
    with ASSETS.open("a", encoding="utf-8", newline="\n") as f:
        f.write("\n")
        f.write(f"# --- 自 dictionary.txt 拆分 ({stamp}) ---\n")
        for ln in asset_entries:
            f.write(ln + "\n")

    main_count = sum(1 for l in kept if l.strip() and not l.strip().startswith("#") and (";" in l or "\t" in l))
    print(f"移入素材字典: {len(asset_entries)} 条")
    print(f"主字典剩余条目: {main_count}")
    print(f"素材字典: {ASSETS}")


if __name__ == "__main__":
    main()
