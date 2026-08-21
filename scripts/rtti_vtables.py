from pathlib import Path
import struct,sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
from capstone.x86 import X86_OP_MEM,X86_REG_RIP

p=Path(r"C:\Program Files\Marmoset\Toolbag 5\toolbag.exe");d=p.read_bytes();pe=struct.unpack_from('<I',d,0x3c)[0]
n=struct.unpack_from('<H',d,pe+6)[0];os=struct.unpack_from('<H',d,pe+20)[0];base=struct.unpack_from('<Q',d,pe+48)[0];st=pe+24+os
secs=[]
for i in range(n):
 o=st+i*40;name=d[o:o+8].split(b'\0')[0].decode();vs,va,rs,ro=struct.unpack_from('<IIII',d,o+8);secs.append((name,va,vs,ro,rs))
def r2rva(raw):
 for _,va,vs,ro,rs in secs:
  if ro<=raw<ro+rs:return va+raw-ro
 raise ValueError
def rva2r(rva):
 for _,va,vs,ro,rs in secs:
  if va<=rva<va+max(vs,rs):return ro+rva-va
 raise ValueError
def refs32(v):
 q=struct.pack('<I',v);out=[];s=0
 while True:
  x=d.find(q,s)
  if x<0:return out
  try:out.append(r2rva(x))
  except:pass
  s=x+1
text=next(x for x in secs if x[0]=='.text');_,trva,tvs,tro,trs=text;code=d[tro:tro+trs]
md=Cs(CS_ARCH_X86,CS_MODE_64);md.detail=True
names=sys.argv[1:] or ['.?AVText@mset@@','.?AVFontResource@mset@@','.?AUCompiledString@Font@mset@@']
for name in names:
 raw=d.index(name.encode());td=r2rva(raw)-16;print(f'{name} type=0x{td:X}')
 for ref in refs32(td):
  col=ref-12
  try:sig,off,cd,ptd,ch,selfr=struct.unpack_from('<IIIIII',d,rva2r(col))
  except:continue
  if sig>1 or ptd!=td:continue
  pat=struct.pack('<Q',base+col);s=0
  while True:
   hit=d.find(pat,s)
   if hit<0:break
   try:vt=r2rva(hit)+8
   except:s=hit+1;continue
   funcs=[];vr=rva2r(vt)
   for i in range(64):
    va=struct.unpack_from('<Q',d,vr+i*8)[0];rva=va-base
    if not trva<=rva<trva+trs:break
    funcs.append(rva)
   print(f'  COL=0x{col:X} vtable=0x{vt:X} offset={off} funcs='+','.join(f'{i}:0x{x:X}' for i,x in enumerate(funcs)))
   for ins in md.disasm(code,base+trva):
    for op in ins.operands:
     if op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP and ins.address+ins.size+op.mem.disp-base==vt:
      print(f'    vtable-xref=0x{ins.address-base:X} {ins.mnemonic} {ins.op_str}')
   s=hit+1
