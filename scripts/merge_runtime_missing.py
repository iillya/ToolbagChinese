from pathlib import Path
import re, shutil, time
from argostranslate import translate

ROOT=Path(__file__).resolve().parent.parent
LOG=Path.home()/"AppData/Local/Marmoset Toolbag 5/ChineseLocalizer_missing.tsv"
DICT=ROOT/"dist/dictionary.txt"

lines=DICT.read_text(encoding="utf-8",errors="replace").splitlines()
known={x.split(";",1)[0].strip() for x in lines if ";" in x and not x.lstrip().startswith("#")}

def ui_text(s):
    s=s.strip()
    if not 2<=len(s)<=200 or s in known or ";" in s: return False
    if not re.search(r"[A-Za-z]",s): return False
    if any(c in s for c in "\\/{}[]<>`|"): return False
    if re.match(r"^(https?://|\.|data:)",s,re.I): return False
    if re.search(r"\.(tga|png|jpg|jpeg|json|frag|vert|comp|dll|exe|obj|fbx|dae|glb|usdc|mview|tbscene)$",s,re.I): return False
    if re.fullmatch(r"[A-Z][A-Z0-9_ .-]*",s) and ("_" in s or len(s)>12): return False
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*",s) and "_" in s: return False
    if re.search(r"\b(toolbag5|marmoset\.co|MSET[A-Z]+)\b",s): return False
    return True

refs={}
for raw in LOG.read_text(encoding="utf-8",errors="replace").splitlines():
    parts=raw.split("\t",2)
    if len(parts)!=3: continue
    kind,rva,text=parts
    text=text.strip()
    if ui_text(text): refs.setdefault(text,[]).append((kind,rva))

installed=translate.get_installed_languages()
en=next(x for x in installed if x.code=="en")
zh=next(x for x in installed if x.code.startswith("zh"))
engine=en.get_translation(zh)

backup=ROOT/"backups"/f"dictionary.backup-runtime-{time.strftime('%Y%m%d-%H%M%S')}.txt"
shutil.copy2(DICT,backup)
out=[]
for i,key in enumerate(refs,1):
    value=engine.translate(key).strip().replace("；","，") or key
    out.append(f"{key};{value}")
    if i%50==0: print(f"translated {i}/{len(refs)}",flush=True)
with DICT.open("a",encoding="utf-8",newline="\n") as f:
    f.write("\n# --- Runtime-observed UI text (machine draft) ---\n")
    f.write("\n".join(out)+"\n")

report=ROOT/"reports/runtime_merge_report.tsv"
report.write_text("\n".join(f"{k}\t{','.join(a+':'+b for a,b in v)}" for k,v in refs.items())+"\n",encoding="utf-8")
print(f"captured={len(LOG.read_text(encoding='utf-8',errors='replace').splitlines())} merged={len(out)} backup={backup} report={report}")
