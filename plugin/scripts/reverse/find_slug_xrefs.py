from pathlib import Path
import os
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_MEM, X86_REG_RIP

EXE = Path(os.environ.get("TOOLBAG_DIR", r"C:\Program Files\Marmoset\Toolbag 5")) / "toolbag.exe"
data = EXE.read_bytes()
pe = struct.unpack_from("<I", data, 0x3C)[0]
sections_count = struct.unpack_from("<H", data, pe + 6)[0]
optional_size = struct.unpack_from("<H", data, pe + 20)[0]
image_base = struct.unpack_from("<Q", data, pe + 24 + 24)[0]
section_table = pe + 24 + optional_size
sections = []
for i in range(sections_count):
    off = section_table + i * 40
    name = data[off:off+8].split(b"\0", 1)[0].decode("ascii", "replace")
    virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, off + 8)
    sections.append((name, virtual_address, virtual_size, raw_offset, raw_size))

def raw_to_rva(raw):
    for name, va, vs, ro, rs in sections:
        if ro <= raw < ro + rs:
            return va + raw - ro
    raise ValueError(raw)

def rva_to_raw(rva):
    for name, va, vs, ro, rs in sections:
        if va <= rva < va + max(vs,rs): return ro + rva - va
    raise ValueError(rva)

text = next(s for s in sections if s[0] == ".text")
_, text_rva, _, text_raw, text_size = text
code = data[text_raw:text_raw+text_size]
md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = True
targets = {}
for needle in (b"data/shader/ui/slug.vert", b"data/shader/ui/slug.frag"):
    raw = data.index(needle)
    targets[raw_to_rva(raw)] = needle.decode()

print(f"image_base=0x{image_base:X}")
for rva, label in targets.items():
    print(f"target {label} RVA=0x{rva:X}")
    packed = struct.pack("<Q", image_base + rva)
    start = 0
    while True:
        raw = data.find(packed, start)
        if raw < 0: break
        try: print(f"pointer {label} raw=0x{raw:X} RVA=0x{raw_to_rva(raw):X}")
        except ValueError: print(f"pointer {label} raw=0x{raw:X}")
        start = raw + 1

for ins in md.disasm(code, image_base + text_rva):
    for op in ins.operands:
        if op.type == X86_OP_MEM and op.mem.base == X86_REG_RIP:
            target_va = ins.address + ins.size + op.mem.disp
            target_rva = target_va - image_base
            if target_rva in targets:
                print(f"xref RVA=0x{ins.address-image_base:X} {ins.mnemonic} {ins.op_str} -> {targets[target_rva]}")

def show_region(start_rva, end_rva):
    print(f"\nregion 0x{start_rva:X}-0x{end_rva:X}")
    blob = code[start_rva-text_rva:end_rva-text_rva]
    for ins in md.disasm(blob, image_base + start_rva):
        print(f"{ins.address-image_base:08X}  {ins.mnemonic:8} {ins.op_str}")

show_region(0xC95F80, 0xC96520)
show_region(0xC99D00, 0xC9B100)
show_region(0xCF63E0, 0xCF68A0)
show_region(0xCA7000, 0xCA8600)
show_region(0x87E000, 0x87F180)
show_region(0x269800, 0x269B20)
show_region(0x277E00, 0x278100)
show_region(0xCC3A00, 0xCC3D00)
show_region(0xD15600, 0xD15900)

def refs32(rva):
    pat = struct.pack("<I", rva)
    out=[]; start=0
    while True:
        raw=data.find(pat,start)
        if raw<0:return out
        try: out.append(raw_to_rva(raw))
        except ValueError: pass
        start=raw+1

rtti_raw=data.index(b".?AUSlugCache@Font@mset@@")
rtti_rva=raw_to_rva(rtti_raw)-16
print(f"\nSlugCache TypeDescriptor RVA=0x{rtti_rva:X}")
level1=refs32(rtti_rva)
print("refs32 type:",", ".join(f"0x{x:X}" for x in level1))
for x in level1:
    r=refs32(x)
    if r: print(f"refs32 0x{x:X}:",", ".join(f"0x{y:X}" for y in r))
for col in level1:
    rr=refs32(col)
    if rr: print(f"refs32 possible COL 0x{col:X}:",", ".join(f"0x{x:X}" for x in rr))
    pat=struct.pack("<Q",image_base+col);start=0
    while True:
        raw=data.find(pat,start)
        if raw<0:break
        try: print(f"absolute ref to 0x{col:X}: RVA=0x{raw_to_rva(raw):X}")
        except ValueError: pass
        start=raw+1
for x in [rtti_rva,*level1,0x1442490]:
    raw=rva_to_raw(x);print(f"bytes 0x{x:X}: {data[raw:raw+48].hex(' ')}")
for type_ref in level1:
    col=type_ref-12
    raw=rva_to_raw(col); sig,off,cd,td,ch,selfr=struct.unpack_from("<IIIIII",data,raw)
    if sig<=1 and td==rtti_rva:
        print(f"COL RVA=0x{col:X} offset={off} hierarchy=0x{ch:X} self=0x{selfr:X}")
        pat=struct.pack("<Q",image_base+col);start=0
        while True:
            hit=data.find(pat,start)
            if hit<0:break
            try: print(f"vftable[-1] RVA=0x{raw_to_rva(hit):X}, vftable RVA=0x{raw_to_rva(hit)+8:X}")
            except ValueError: pass
            start=hit+1
            vt=raw_to_rva(hit)+8;vr=rva_to_raw(vt);funcs=[]
            for i in range(32):
                va=struct.unpack_from("<Q",data,vr+i*8)[0];rva=va-image_base
                if not(text_rva<=rva<text_rva+text_size):break
                funcs.append(rva)
            print("SlugCache vfuncs:",", ".join(f"[{i}]=0x{x:X}" for i,x in enumerate(funcs)))
            vt_target=vt
            for ins in md.disasm(code,image_base+text_rva):
                for op in ins.operands:
                    if op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP:
                        if ins.address+ins.size+op.mem.disp-image_base==vt_target:
                            print(f"vftable xref RVA=0x{ins.address-image_base:X} {ins.mnemonic} {ins.op_str}")
