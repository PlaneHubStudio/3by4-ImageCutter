"""Inspect PE architecture, GUI subsystem, system-only imports and embedded resources."""
from pathlib import Path
import struct
p = Path(__file__).resolve().parent.parent / 'dist' / '3比4图片快切.exe'
b = p.read_bytes()
u16=lambda off:struct.unpack_from('<H',b,off)[0]
u32=lambda off:struct.unpack_from('<I',b,off)[0]
assert b[:2] == b'MZ'
pe=u32(0x3c);assert b[pe:pe+4]==b'PE\0\0'
assert u16(pe+4)==0x8664, 'Expected x64'
opt=pe+24;assert u16(opt)==0x20b and u16(opt+68)==2, 'Expected Windows GUI PE32+'
sections=[]
start=opt+u16(pe+20)
for i in range(u16(pe+6)):
 off=start+40*i
 sections.append((u32(off+12),max(u32(off+8),u32(off+16)),u32(off+20)))
def offset(rva):
 for addr,size,raw in sections:
  if addr<=rva<addr+size:return raw+rva-addr
 raise ValueError(f'Unknown RVA {rva:x}')
imports=[]
imp=u32(opt+112+8)
if imp:
 off=offset(imp)
 while any(b[off:off+20]):
  name=offset(u32(off+12)); imports.append(b[name:b.index(0,name)].decode().lower());off+=20
allowed={'kernel32.dll','user32.dll','gdi32.dll','gdiplus.dll','comdlg32.dll','comctl32.dll','shell32.dll','ole32.dll','msvcrt.dll','advapi32.dll','shlwapi.dll','ntdll.dll','imm32.dll','oleaut32.dll','uuid.dll'}
assert all(name in allowed or (name.startswith('api-ms-win-crt-') and name.endswith('.dll')) for name in imports), imports
resource=offset(u32(opt+112+16)); count=u16(resource+12)+u16(resource+14)
types={u32(resource+16+8*i) for i in range(count)}
assert {3,14,16,24}<=types, f'Missing icon/manifest/version: {types}'
print(f'PASS: x64 GUI, embedded icon + manifest + version, {len(b):,} bytes')
print('Only Windows system DLLs: '+', '.join(imports))
