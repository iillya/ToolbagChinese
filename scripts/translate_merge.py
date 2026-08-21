from pathlib import Path
import shutil, time
from argostranslate import package, translate

ROOT=Path(__file__).resolve().parent.parent
DICT=ROOT/"dist/dictionary.txt"
CAND=ROOT/"reports/dictionary_static_ui_candidates.txt"

package.update_package_index()
pkg=next((p for p in package.get_available_packages() if p.from_code=="en" and p.to_code.startswith("zh")),None)
if not pkg: raise SystemExit("No English-Chinese translation model available")
installed=translate.get_installed_languages()
en=next((x for x in installed if x.code=="en"),None)
zh=next((x for x in installed if x.code.startswith("zh")),None)
if not en or not zh:
    package.install_from_path(pkg.download())
    installed=translate.get_installed_languages()
    en=next(x for x in installed if x.code=="en")
    zh=next(x for x in installed if x.code.startswith("zh"))
engine=en.get_translation(zh)

current=DICT.read_text(encoding="utf-8",errors="replace").splitlines()
known={line.split(";",1)[0].strip() for line in current if ";" in line and not line.lstrip().startswith("#")}
candidates=[]; active=False
for line in CAND.read_text(encoding="utf-8",errors="replace").splitlines():
    if line.startswith("# --- Official"): active=True; continue
    if line.startswith("# --- Executable"): break
    if active and line.endswith(";"):
        key=line[:-1].strip()
        if key and key not in known: candidates.append(key)

backup=ROOT/"backups"/f"dictionary.backup-{time.strftime('%Y%m%d-%H%M%S')}.txt"
shutil.copy2(DICT,backup)
result=[]
for i,key in enumerate(candidates,1):
    value=engine.translate(key).strip().replace("；","，")
    if not value: value=key
    result.append(f"{key};{value}")
    if i%50==0: print(f"translated {i}/{len(candidates)}",flush=True)

with DICT.open("a",encoding="utf-8",newline="\n") as f:
    f.write("\n# --- Static scan: official Toolbag help text (machine draft) ---\n")
    f.write("\n".join(result)+"\n")
print(f"merged={len(result)} backup={backup} dictionary={DICT}")
