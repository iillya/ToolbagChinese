#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Append all required files to a raw EXE to produce a self-contained installer.

The payload is appended after the PE image:
    [ file data... ][ entries... ][ manifestSize u32 ][ count u32 ][ magic u64 ]
Each entry: [ nameLen u32 ][ name bytes ][ offset u64 ][ size u64 ][ sha256 32 bytes ]

The C++ installer (source/installer.cpp) reads this payload at runtime, extracts
the files to a temp folder and runs scripts\\install.ps1.
"""
import struct
import hashlib
from pathlib import Path

MAGIC = 0x314D484254  # "TBHM1"

# logical name (as extracted) -> source path relative to repo root
PAYLOAD_FILES = [
    ("scripts/install.ps1", "scripts/install.ps1"),
    ("dist/dictionary_zh.json", "translations/dictionary_zh.json"),
    ("dist/segoeui.slug", "fonts/segoeui.slug"),
    ("dist/tbscene.ico", "icon/tbscene.ico"),
    ("dist/ToolbagChineseHook.dll", "build/out/ToolbagChineseHook.dll"),
    ("dist/ToolbagChineseLauncher.exe", "build/out/ToolbagChineseLauncher.exe"),
]


def build_payload(root: Path, raw_len: int) -> bytes:
    entries = []
    blob = bytearray()
    for name, rel in PAYLOAD_FILES:
        data = (root / rel).read_bytes()
        offset = raw_len + len(blob)   # absolute offset in the final EXE
        blob += data
        entries.append((name.encode("utf-8"), offset, len(data), hashlib.sha256(data).digest()))

    manifest = b""
    for name_bytes, offset, size, digest in entries:
        manifest += struct.pack("<I", len(name_bytes)) + name_bytes
        manifest += struct.pack("<QQ", offset, size) + digest

    blob.extend(manifest)
    blob.extend(struct.pack("<IIQ", len(manifest), len(entries), MAGIC))
    return bytes(blob)


def main():
    root = Path(__file__).resolve().parent.parent
    raw = root / "build" / "out" / "ChineseInstaller.exe"
    out = root / "dist" / "ToolbagChineseInstaller.exe"

    if not raw.exists():
        raise SystemExit("缺少 build/out/ChineseInstaller.exe，请先运行 source\\build.bat")

    for _, rel in PAYLOAD_FILES:
        if not (root / rel).exists():
            raise SystemExit(f"缺少 {rel}，无法打包")

    raw_data = raw.read_bytes()
    raw_len = len(raw_data)
    payload = build_payload(root, raw_len)
    data = raw_data + payload
    out.write_bytes(data)

    size_mb = len(data) / (1024 * 1024)
    print(f"已生成自包含安装器: {out}")
    print(f"大小: {size_mb:.1f} MB  (原始EXE {raw.stat().st_size/1024:.0f} KB + 嵌入数据)")
    print("用户只需这一个 exe：安装 / 卸载 均可。")


if __name__ == "__main__":
    main()
