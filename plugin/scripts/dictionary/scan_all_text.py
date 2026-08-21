#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Full-text collector for Marmoset Toolbag 5 Chinese localization.

Goal: capture *all* displayable UI text without dropping candidates early.
Strategy:
  * scan toolbag.exe (.rdata/.data) for ASCII + UTF-16 strings (wide length window)
  * scan the whole data/ tree for text-bearing files (.txt/.loc/.json/.xml/.cfg/...)
  * parse JSON string values (workspaces, spline profiles, presets, ...)
  * read data/text/english.txt as the authoritative official UI strings
  * dedupe against the existing dictionary
  * output tiered reports (all / ui / other) instead of discarding anything

Outputs (under reports/):
  all_captured.tsv                 machine readable:  TEXT<TAB>SOURCE<TAB>KIND
  dictionary_all_candidates.txt    every captured string (incl. likely-internal)
  dictionary_ui_candidates.txt     high-confidence UI strings only
  dictionary_other_candidates.txt  everything not marked as UI
"""
import json
import re
import struct
from pathlib import Path

PROJECT = Path(__file__).resolve().parent.parent.parent.parent
ROOT = Path(r"C:\Program Files\Marmoset\Toolbag 5")
EXE = ROOT / "toolbag.exe"
DATA = ROOT / "data"
DICT = PROJECT / "plugin" / "data" / "dictionary.txt"
TSV = PROJECT / "reports" / "all_captured.tsv"
FULL = PROJECT / "reports" / "dictionary_all_candidates.txt"
UI = PROJECT / "reports" / "dictionary_ui_candidates.txt"
OTHER = PROJECT / "reports" / "dictionary_other_candidates.txt"
ENGLISH = DATA / "text" / "english.txt"

# text-bearing extensions we scan recursively under data/
# (code files are excluded on purpose: their identifiers are not UI text)
TEXT_EXTS = {
    ".txt", ".loc", ".json", ".xml", ".cfg", ".ini", ".ui", ".sdef",
    ".toml", ".yaml", ".yml", ".csv", ".preset",
}

# strings that are almost certainly not user-visible UI text
BAD_UI = re.compile(
    r"(::|^(https?://|data:|file:|[A-Za-z]:\\|[/\\]))|"
    r"\.(png|tga|jpg|jpeg|jpe|gif|bmp|dds|exr|hdr|dll|exe|obj|fbx|dae|"
    r"glb|gltf|usdc|usda|usdz|mview|tbscene|log|json|xml|frag|vert|comp|geom|"
    r"hlsl|glsl|py|pyw|lua|ini|cfg|loc|sdef|toml|yaml|yml|csv)$",
    re.I,
)
FILE_PATH = re.compile(r"[A-Za-z]:[\\/]|\.\.[\\/]|^[/\\]|(^|[/\\])[^/\\]+\.\w{1,5}$")
HEXISH = re.compile(r"^[0-9A-Fa-f\-]{16,}$")
IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
HAS_LETTER = re.compile(r"[A-Za-z]")


def existing_keys():
    keys = set()
    for raw in DICT.read_text(encoding="utf-8", errors="replace").splitlines():
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        if ";" in s:
            keys.add(s.split(";", 1)[0].strip())
        elif "\t" in s:
            keys.add(s.split("\t", 1)[0].strip())
    return keys


def sections(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    pos = pe + 24 + opt
    for i in range(count):
        off = pos + i * 40
        name = data[off:off + 8].split(b"\0", 1)[0].decode("ascii", "ignore")
        vsize, rva, rawsize, raw = struct.unpack_from("<IIII", data, off + 8)
        if name in (".rdata", ".data"):
            yield name, rva, raw, rawsize


def is_ui(s):
    """High-confidence UI heuristic (kept loose; used only for tiering)."""
    s = s.strip()
    if not (1 <= len(s) <= 200):
        return False
    if not HAS_LETTER.search(s):
        return False
    if BAD_UI.search(s):
        return False
    if FILE_PATH.search(s):
        return False
    if HEXISH.fullmatch(s):
        return False
    if "\x00" in s or re.search(r"[\x00-\x1f\x7f]", s):
        return False
    # short strings starting with punctuation are almost always binary noise
    if len(s) < 4 and not s[0].isalnum():
        return False
    # pure long identifiers / camelCase with no space are usually internal
    if " " not in s and len(s) > 32:
        return False
    # pure lowercase underscore identifiers are internal symbols
    if IDENT.fullmatch(s) and "_" in s:
        return False
    return True


def is_ui_confident(s):
    """Stricter tier used for the gap/coverage report and the UI candidate list.

    Keeps strings that look like real user-visible labels and drops the binary
    noise (C++ internals, format specifiers, identifiers, punctuation garbage).
    """
    if not is_ui(s):
        return False
    # must start with a letter/digit (not punctuation like ! " # ...)
    if not s[0].isalnum():
        return False
    # drop strings carrying code/C++ operators
    if re.search(r"::|->|&&|\|\||==|!=|<=|>=|[{}[\]()]", s):
        return False
    # drop format-specifier strings ("%s", "%d", ...)
    if re.search(r"%[sdflupeEfgGoxXc]", s):
        return False
    # camelCase with no space (uppercase after first char) => internal identifier
    if " " not in s and any(c.isupper() for c in s[1:]):
        return False
    # require a space OR a short plain word (OK/Yes/No/Open/Close/...)
    if " " not in s and not (len(s) <= 6 and s.isalpha()):
        return False
    return True


# Heuristics that make a string *less* likely to be a friendly UI label, but do
# NOT remove it from the UI tier (format strings like "Load %s failed" are real
# UI text). Used only to rank output so the most plausible labels come first.
NOISE = re.compile(r"(::|->|=>|\b0x[0-9a-fA-F]+\b|[{}]|&&|\|\||!=|==|<=|>=|"
                   r"%[sdflup%]|\\n|\\t|\bNULL\b|\btrue\b|\bfalse\b)")
def ui_score(s):
    sc = 0
    if " " in s:
        sc += 4
    if s[:1].isupper():
        sc += 2
    if len(s) <= 60:
        sc += 1
    if not NOISE.search(s):
        sc += 2
    if re.fullmatch(r"[A-Za-z0-9 ()'&,+.:;!?%#_\-]+", s):
        sc += 1
    return (-sc, s.lower())


def extract_ascii(blob, base):
    for m in re.finditer(rb"[\x20-\x7e]{1,512}\x00", blob):
        s = m.group()[:-1].decode("ascii", "replace")
        if s.strip():
            yield s, base + m.start(), "ascii"


def extract_utf16(blob, base):
    pat = re.compile(rb"(?:[\x20-\x7e]\x00){1,512}\x00\x00")
    for m in pat.finditer(blob):
        s = m.group()[:-2].decode("utf-16-le", "replace")
        if s.strip():
            yield s, base + m.start(), "utf16"


def json_strings(obj, out):
    if isinstance(obj, str):
        if obj.strip():
            out.append(obj)
    elif isinstance(obj, dict):
        for v in obj.values():
            json_strings(v, out)
    elif isinstance(obj, (list, tuple)):
        for v in obj:
            json_strings(v, out)


def extract_text_file(path):
    """Yield (text, kind) from a text-bearing data file."""
    ext = path.suffix.lower()
    try:
        raw = path.read_bytes()
    except OSError:
        return
    # Try JSON first: captures structured display names.
    if ext == ".json":
        try:
            obj = json.loads(raw.decode("utf-8", errors="replace"))
            for s in json_strings(obj, []):
                if s.strip():
                    yield s, "json"
            return
        except Exception:
            pass
    text = raw.decode("utf-8", errors="replace")
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith(("#", "//", "/*", "*", ";")):
            continue
        # key;value  /  key=value / key:value -> keep both sides if plausible
        for part in re.split(r"[;=:]", line, maxsplit=1):
            part = part.strip().strip('"').strip("'")
            if 1 <= len(part) <= 200 and HAS_LETTER.search(part):
                yield part, "datafile"


def main():
    known = existing_keys()
    captured = {}          # text -> set of (source, kind)
    sources = {"exe": EXE.name}

    # 1) executable binary strings
    data = EXE.read_bytes()
    for name, rva, raw, size in sections(data):
        blob = data[raw:raw + size]
        for s, addr, enc in (*extract_ascii(blob, rva), *extract_utf16(blob, rva)):
            s = s.strip()
            if s and s not in known:
                captured.setdefault(s, set()).add((f"{EXE.name}:{name}", enc))

    # 2) official language file
    if ENGLISH.exists():
        for raw in ENGLISH.read_text(encoding="utf-8", errors="replace").splitlines():
            if ";" not in raw:
                continue
            value = raw.split(";", 1)[1].strip()
            if value and value not in known:
                captured.setdefault(value, set()).add(("english.txt", "official"))

    # 3) whole data tree
    # Skip bundled runtimes/library payloads that are not Toolbag UI text.
    SKIP_REL_PREFIXES = ("data/python/",)  # embedded CPython stdlib
    for path in sorted(DATA.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in TEXT_EXTS:
            continue
        rel = str(path.relative_to(ROOT)).replace("\\", "/")
        if any(rel.startswith(p) for p in SKIP_REL_PREFIXES):
            continue
        if "ChineseLocalizer" in path.parts:
            continue  # skip our own plugin copy
        sources[rel] = rel
        for s, kind in extract_text_file(path):
            s = s.strip()
            if s and s not in known:
                captured.setdefault(s, set()).add((rel, kind))

    # ---- write machine readable TSV ----
    with TSV.open("w", encoding="utf-8", newline="") as f:
        f.write("text\tsource\tkind\tis_ui\n")
        for s in sorted(captured):
            for src, kind in sorted(captured[s]):
                f.write(f"{s}\t{src}\t{kind}\t{'yes' if is_ui(s) else 'no'}\n")

    # ---- tiered human-readable reports ----
    ui_items = sorted((s for s in captured if is_ui_confident(s)), key=ui_score)
    other_items = sorted(s for s in captured if not is_ui_confident(s))

    with FULL.open("w", encoding="utf-8", newline="") as f:
        f.write("# Toolbag ALL captured strings (full dump, incl. likely-internal)\n")
        f.write("# Format: English;\n")
        f.write("\n")
        for s in sorted(captured):
            srcs = ", ".join(sorted({k for k, _ in captured[s]}))
            f.write(f"{s};\t# {srcs}\n")

    with UI.open("w", encoding="utf-8", newline="") as f:
        f.write("# High-confidence Toolbag UI strings\n")
        f.write("# Format: English;\n")
        f.write("\n")
        for s in ui_items:
            srcs = ", ".join(sorted({k for k, _ in captured[s]}))
            f.write(f"{s};\t# {srcs}\n")

    with OTHER.open("w", encoding="utf-8", newline="") as f:
        f.write("# Captured strings that are likely NOT UI text (for manual review)\n")
        f.write("# Format: English;\n")
        f.write("\n")
        for s in other_items:
            srcs = ", ".join(sorted({k for k, _ in captured[s]}))
            f.write(f"{s};\t# {srcs}\n")

    print(f"existing_in_dictionary={len(known)}")
    print(f"unique_captured={len(captured)}  ui={len(ui_items)}  other={len(other_items)}")
    print(f"sources_scanned={len(sources)}")
    print(f"tsv={TSV}")
    print(f"all={FULL}\nui={UI}\nother={OTHER}")


if __name__ == "__main__":
    main()
