"""Read-only static analysis of the installed Weixin image. No account data."""
import sys, pathlib, re, struct, bisect
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / '.review/analysis_packages'))
import pefile, capstone
path = pathlib.Path(r'C:\Program Files\Tencent\Weixin\4.1.15.13\Weixin.dll')
pe = pefile.PE(str(path), fast_load=True)
b = path.read_bytes()
base = pe.OPTIONAL_HEADER.ImageBase
code = next(s for s in pe.sections if s.Name.startswith(b'.text'))
cb = code.get_data()
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
pdata = next(s for s in pe.sections if s.Name.startswith(b'.pdata')).get_data()
funcs = [struct.unpack_from('<III', pdata, i)[:2] for i in range(0, len(pdata)-11, 12)]
funcs = sorted((s,e) for s,e in funcs if s and e>s)
starts = [s for s,e in funcs]
def enclosing(rva):
    i = bisect.bisect_right(starts, rva)-1
    return funcs[i] if i>=0 and rva<funcs[i][1] else (rva,rva+160)
def dis(rva, count=0):
    s,e = enclosing(rva)
    if count: s,e=rva,rva+count
    raw=pe.get_data(s,min(e-s,16000))
    for a,n,m,o in cs.disasm_lite(raw,s):
        note=''
        match=re.search(r'\[rip ([+-]) (0x[0-9a-f]+)\]',o)
        if match:
            target=a+n+int(match[2],16)*(1 if match[1]=='+' else -1)
            data=pe.get_data(target,180).split(b'\0')[0]
            if len(data)>3 and all(32<=v<127 for v in data): note=' ; '+data.decode()
            else: note=' ; '+hex(target)
        print(f'{a:08x} {m:8} {o}{note}')
def refs(target):
    found=[]
    # RIP-relative LEA/MOV references, decoded to reject coincidental bytes.
    for m in re.finditer(rb'[\x48-\x4f][\x8d\x8b][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',cb):
        pos=m.start(); a=code.VirtualAddress+pos
        if a+7+struct.unpack_from('<i',cb,pos+3)[0]==target: found.append(a)
    return found
def calls(target):
    return [code.VirtualAddress+m.start() for m in re.finditer(b'\xe8',cb)
        if m.start()+5<=len(cb) and code.VirtualAddress+m.start()+5+struct.unpack_from('<i',cb,m.start()+1)[0]==target]
if sys.argv[1]=='faststrings':
    patterns=[x.encode().lower() for x in sys.argv[2:]]
    for m in re.finditer(rb'[ -~]{6,}',b):
        if any(p in m.group().lower() for p in patterns):
            print(hex(pe.get_rva_from_offset(m.start())),m.group().decode()[:500])
elif sys.argv[1]=='strings':
    patterns=[x.encode().lower() for x in sys.argv[2:]]
    for m in re.finditer(rb'[ -~]{6,}',b):
        if any(p in m.group().lower() for p in patterns):
            r=pe.get_rva_from_offset(m.start()); print(hex(r),m.group().decode()[:500], 'refs', [hex(x) for x in refs(r)])
elif sys.argv[1]=='dis': dis(int(sys.argv[2],0),int(sys.argv[3],0) if len(sys.argv)>3 else 0)
elif sys.argv[1]=='refs': print([(hex(r),tuple(hex(v) for v in enclosing(r))) for r in refs(int(sys.argv[2],0))])
elif sys.argv[1]=='calls': print([(hex(r),tuple(hex(v) for v in enclosing(r))) for r in calls(int(sys.argv[2],0))])
elif sys.argv[1]=='table':
    start=int(sys.argv[2],0)
    for i in range(int(sys.argv[3],0)):
        v=struct.unpack('<Q',pe.get_data(start+8*i,8))[0]-base
        print(hex(start+8*i),hex(v),tuple(hex(x) for x in enclosing(v)))
elif sys.argv[1]=='pointers':
    target=int(sys.argv[2],0)
    needle=struct.pack('<Q',base+target)
    for m in re.finditer(re.escape(needle),b): print(hex(pe.get_rva_from_offset(m.start())))
elif sys.argv[1]=='batchrefs':
    for value in sys.argv[2:]:
        r=int(value,0);print(hex(r),[(hex(a),tuple(hex(x) for x in enclosing(a))) for a in refs(r)])
elif sys.argv[1]=='outline':
    low,high=map(lambda x:int(x,0),sys.argv[2:4])
    for s,e in funcs:
        if low<=s<high:
            instructions=list(cs.disasm_lite(pe.get_data(s,e-s),s))
            print(hex(s),hex(e), 'calls:', ' '.join(o for a,n,m,o in instructions if m=='call'))
            for a,n,m,o in instructions[:18]: print(f'  {a:x} {m} {o}')
elif sys.argv[1]=='broadrefs':
    import numpy as np
    targets=[int(x,0) for x in sys.argv[2:]]
    for low in range(0,len(cb)-3,4*1024*1024):
        block=cb[low:low+4*1024*1024+3]
        for alignment in range(4):
            count=(len(block)-alignment)//4
            values=np.frombuffer(block,dtype='<i4',count=count,offset=alignment).astype(np.int64)
            positions=code.VirtualAddress+low+alignment+np.arange(count,dtype=np.int64)*4
            for target in targets:
                for index in np.flatnonzero(values+positions+4==target):
                    rva=int(positions[index]);start,end=enclosing(rva)
                    print(hex(target),'disp',hex(rva),'function',hex(start),hex(end))
