import re, struct
from pathlib import Path

PROJECT = Path(__file__).resolve().parent.parent
EXE = Path(r"C:\Program Files\Marmoset\Toolbag 5\toolbag.exe")
DICT = PROJECT/"dist/dictionary.txt"
OUT = PROJECT/"reports/dictionary_static_candidates.txt"
UI_OUT = PROJECT/"reports/dictionary_static_ui_candidates.txt"
ENGLISH = Path(r"C:\Program Files\Marmoset\Toolbag 5\data\text\english.txt")

def existing_keys():
    keys=set()
    for raw in DICT.read_text(encoding="utf-8",errors="replace").splitlines():
        s=raw.strip()
        if not s or s.startswith("#"): continue
        if ";" in s: keys.add(s.split(";",1)[0].strip())
        elif "\t" in s: keys.add(s.split("\t",1)[0].strip())
    return keys

def sections(data):
    pe=struct.unpack_from("<I",data,0x3C)[0]
    count=struct.unpack_from("<H",data,pe+6)[0]
    opt=struct.unpack_from("<H",data,pe+20)[0]
    pos=pe+24+opt
    for i in range(count):
        off=pos+i*40
        name=data[off:off+8].split(b"\0",1)[0].decode("ascii","ignore")
        vsize,rva,rawsize,raw=struct.unpack_from("<IIII",data,off+8)
        if name in (".rdata",".data"): yield name,rva,raw,rawsize

def plausible(s):
    s=s.strip()
    if not 2<=len(s)<=160 or not re.search(r"[A-Za-z]",s): return False
    if any(c in s for c in ("\\","/","{","}","<",">")): return False
    if re.search(r"\.(png|tga|jpg|jpeg|frag|vert|comp|dll|exe|json|py|obj|fbx|log)$",s,re.I): return False
    if re.match(r"^(https?://|data:|[A-Z]:|[_?.$@])",s,re.I): return False
    if re.fullmatch(r"[A-Fa-f0-9-]{16,}",s): return False
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_:<>@?$]{24,}",s) and " " not in s: return False
    printable=sum(c.isprintable() for c in s)/len(s)
    return printable==1 and not re.search(r"[^\x20-\x7e]",s)

def extract_ascii(blob,base):
    for m in re.finditer(rb"[\x20-\x7e]{2,160}\x00",blob):
        s=m.group()[:-1].decode("ascii")
        if plausible(s): yield s,base+m.start(),"ascii"

def extract_utf16(blob,base):
    pat=re.compile(rb"(?:[\x20-\x7e]\x00){2,160}\x00\x00")
    for m in pat.finditer(blob):
        s=m.group()[:-2].decode("utf-16-le")
        if plausible(s): yield s,base+m.start(),"utf16"

data=EXE.read_bytes(); known=existing_keys(); found={}
for name,rva,raw,size in sections(data):
    blob=data[raw:raw+size]
    for s,addr,enc in (*extract_ascii(blob,rva),*extract_utf16(blob,rva)):
        s=s.strip()
        if s in known: continue
        found.setdefault(s,[]).append((addr,enc,name))

def score(item):
    s,refs=item
    value=0
    if " " in s: value+=4
    if s[:1].isupper(): value+=2
    if re.fullmatch(r"[A-Za-z0-9 ()'&,+.:;!?%#_\-]+",s): value+=2
    if len(s)<=60: value+=2
    value+=min(len(refs),3)
    return (-value,s.lower())

lines=["# Toolbag static UI dictionary candidates", "# Format: English;Chinese    # RVA / encoding", ""]
for s,refs in sorted(found.items(),key=score):
    ref=" ".join(f"0x{a:X}/{e}" for a,e,_ in refs[:4])
    lines.append(f"{s};    # {ref}")
OUT.write_text("\n".join(lines)+"\n",encoding="utf-8")

official=[]
for raw in ENGLISH.read_text(encoding="utf-8",errors="replace").splitlines():
    if ";" not in raw: continue
    value=raw.split(";",1)[1].strip()
    if value and value not in known and plausible(value): official.append(value)
official=list(dict.fromkeys(official))

bad=re.compile(r"(::|\b(assert|shader|heap|vertex|index|block|library version|callable|parameter can|address of|detected)\b)",re.I)
labels=[]
for s,refs in found.items():
    if s in official or len(s)>90 or bad.search(s): continue
    if re.search(r"[=\[\]`~^|]",s): continue
    if len(re.findall(r"[^A-Za-z0-9 ()'&,+.:;!?%#_\-]",s))>0: continue
    if " " not in s and not s[:1].isupper(): continue
    labels.append((s,refs))
labels.sort(key=score)

ui=["# High-confidence static UI candidates", "# Existing dictionary entries are excluded.", "", "# --- Official tooltip/help text (data/text/english.txt) ---"]
ui.extend(f"{s};" for s in official)
ui.extend(("", "# --- Executable UI labels/messages ---"))
for s,refs in labels:
    ref=" ".join(f"0x{a:X}/{e}" for a,e,_ in refs[:3])
    ui.append(f"{s};    # {ref}")
UI_OUT.write_text("\n".join(ui)+"\n",encoding="utf-8")
print(f"existing={len(known)} raw={len(found)} official={len(official)} ui_labels={len(labels)}")
print(f"raw_output={OUT}\nui_output={UI_OUT}")
