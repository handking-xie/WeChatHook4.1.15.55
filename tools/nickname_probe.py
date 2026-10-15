"""Read-only nickname implementation diagnostics. Prints addresses, never profile values."""
import ctypes as c
import ctypes.wintypes as w
import struct
import sys

k = c.WinDLL('kernel32', use_last_error=True)
k.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]
k.CreateToolhelp32Snapshot.restype = w.HANDLE
k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
k.OpenProcess.restype = w.HANDLE
k.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
k.CloseHandle.argtypes = [w.HANDLE]

class Module(c.Structure):
    _fields_ = [('size', w.DWORD), ('id', w.DWORD), ('pid', w.DWORD),
               ('globalUses', w.DWORD), ('processUses', w.DWORD),
               ('base', c.c_void_p), ('baseSize', w.DWORD), ('module', w.HMODULE),
               ('name', w.WCHAR * 256), ('path', w.WCHAR * 260)]
k.Module32FirstW.argtypes = [w.HANDLE, c.POINTER(Module)]
k.Module32NextW.argtypes = [w.HANDLE, c.POINTER(Module)]

for pid in map(int, sys.argv[1:]):
    snapshot = k.CreateToolhelp32Snapshot(0x18, pid)
    module = Module(); module.size = c.sizeof(module)
    ok = k.Module32FirstW(snapshot, c.byref(module))
    base = 0
    while ok:
        if module.name.lower() == 'weixin.dll':
            base = module.base
            break
        ok = k.Module32NextW(snapshot, c.byref(module))
    k.CloseHandle(snapshot)
    if not base:
        print(pid, 'no Weixin.dll snapshot', c.get_last_error()); continue
    process = k.OpenProcess(0x410, False, pid)
    def read(address, size):
        buf = c.create_string_buffer(size); copied = c.c_size_t()
        if not k.ReadProcessMemory(process, address, buf, size, c.byref(copied)) or copied.value != size:
            raise OSError(c.get_last_error(), 'ReadProcessMemory')
        return buf.raw
    def qword(address): return struct.unpack('<Q', read(address, 8))[0]
    try:
        root = qword(base + 0xB912690)
        print(pid, 'base', hex(base), 'root', hex(root))
        if root:
            table = qword(root)
            print('manager vtable RVA', hex(table-base))
            print('manager virtual RVAs', [hex(qword(table+i)-base) for i in range(0, 0x40, 8)])
            service = qword(root + 0x2c8)
            if service:
                table = qword(service)
                print('CGI service vtable RVA', hex(table-base))
                print('CGI service virtual RVAs', [hex(qword(table+i)-base) for i in range(0, 0x40, 8)])
    except OSError as error: print(pid, error)
    finally: k.CloseHandle(process)
