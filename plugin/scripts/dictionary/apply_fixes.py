#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Apply key;new_value corrections to a dictionary file."""
import shutil, sys, time
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent.parent.parent

def main():
    fix_path = Path(sys.argv[1])
    target = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT/'plugin/data/dictionary.txt'
    fixes = {}
    for l in fix_path.read_text(encoding='utf-8', errors='replace').splitlines():
        s = l.strip()
        if not s or s.startswith('#') or ';' not in s: continue
        k, v = s.split(';', 1)
        fixes[k.strip()] = v.strip()
    text = target.read_text(encoding='utf-8-sig', errors='replace')
    lines = text.splitlines()
    applied = 0
    notfound = 0
    out = []
    for ln in lines:
        s = ln.strip()
        if s and not s.startswith('#') and ';' in s:
            k, v = s.split(';', 1)
            kk = k.strip()
            if kk in fixes:
                out.append(f"{kk};{fixes[kk]}")
                applied += 1
                continue
        out.append(ln)
    notfound = len(fixes) - applied
    stamp = time.strftime('%Y%m%d-%H%M%S')
    shutil.copy2(target, ROOT/'backups'/f"dict.fix-{stamp}.txt")
    target.write_text('\n'.join(out) + '\n', encoding='utf-8')
    print(f"已修正: {applied}  未找到: {notfound}")
    if notfound:
        print("未找到的 key:")
        for k in fixes:
            if k not in [l.split(';')[0].strip() for l in lines if ';' in l and not l.strip().startswith('#')]:
                print('  ', k)

if __name__ == '__main__':
    main()
