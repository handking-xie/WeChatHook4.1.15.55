"""Run the bounded native nickname 4.1.15.55 probe in an explicitly selected Weixin process."""
import ctypes as c
import ctypes.wintypes as w
import pathlib
import sys
import argparse

root = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / '.review/analysis_packages'))
import pefile
parser = argparse.ArgumentParser()
parser.add_argument('--pid', type=int, required=True)

parser.add_argument('--dll', default='nickname_41555_native.dll')
parser.add_argument('--edit-restore', action='store_true', help='change to test, then restore original server nickname')
parser.add_argument('--http', action='store_true', help='bounded loopback production route test')
parser.add_argument('--codec', action='store_true', help='native protobuf only; no network request')

parser.add_argument('--result-file', default='nickname-41555-result.json', help='diagnostic result filename inside .review')
args = parser.parse_args()
k = c.WinDLL('kernel32', use_last_error=True)
def bind(name, result, *arguments):
    function = getattr(k, name); function.restype = result; function.argtypes = list(arguments); return function
snapshot = bind('CreateToolhelp32Snapshot', w.HANDLE, w.DWORD, w.DWORD)
close = bind('CloseHandle', w.BOOL, w.HANDLE)
class Module(c.Structure):
    _fields_ = [('size', w.DWORD), ('id', w.DWORD), ('pid', w.DWORD), ('globalUses', w.DWORD),
        ('processUses', w.DWORD), ('base', c.c_void_p), ('baseSize', w.DWORD), ('module', w.HMODULE),
        ('name', w.WCHAR * 256), ('path', w.WCHAR * 260)]
first = bind('Module32FirstW', w.BOOL, w.HANDLE, c.POINTER(Module))
next_module = bind('Module32NextW', w.BOOL, w.HANDLE, c.POINTER(Module))
def modules(pid):
    handle = snapshot(0x18, pid)
    if handle == c.c_void_p(-1).value: raise c.WinError(c.get_last_error())
    item = Module(); item.size = c.sizeof(item); result = {}
    try:
        ok = first(handle, c.byref(item))
        while ok:
            result[item.name.lower()] = (item.base, item.baseSize)
            ok = next_module(handle, c.byref(item))
    finally: close(handle)
    return result
remote = modules(args.pid)
if 'weixin.dll' not in remote: raise RuntimeError('selected process has no Weixin.dll')
open_process = bind('OpenProcess', w.HANDLE, w.DWORD, w.BOOL, w.DWORD)
allocate = bind('VirtualAllocEx', c.c_void_p, w.HANDLE, c.c_void_p, c.c_size_t, w.DWORD, w.DWORD)
free = bind('VirtualFreeEx', w.BOOL, w.HANDLE, c.c_void_p, c.c_size_t, w.DWORD)
write = bind('WriteProcessMemory', w.BOOL, w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t))
create = bind('CreateRemoteThread', w.HANDLE, w.HANDLE, c.c_void_p, c.c_size_t, c.c_void_p, c.c_void_p, w.DWORD, c.POINTER(w.DWORD))
wait = bind('WaitForSingleObject', w.DWORD, w.HANDLE, w.DWORD)
exit_code = bind('GetExitCodeThread', w.BOOL, w.HANDLE, c.POINTER(w.DWORD))
get_module = bind('GetModuleHandleW', w.HMODULE, w.LPCWSTR)
get_proc = bind('GetProcAddress', c.c_void_p, w.HMODULE, w.LPCSTR)
process = open_process(0x43A, False, args.pid)
if not process: raise c.WinError(c.get_last_error())
def run(address, argument, timeout):
    handle = create(process, None, 0, address, argument, 0, None)
    if not handle: raise c.WinError(c.get_last_error())
    try:
        if wait(handle, timeout) != 0: raise TimeoutError('native worker still running; probe DLL retained')
        code = w.DWORD(); exit_code(handle, c.byref(code)); return code.value
    finally: close(handle)
try:
    if pathlib.Path(args.dll).name != args.dll: raise ValueError('DLL must be a filename inside .review')
    dll = root / '.review' / args.dll
    with dll.open('rb') as source:
        if source.read(2) != b'MZ': raise ValueError('diagnostic is not a PE DLL')
    if dll.name.lower() not in remote:
        address = get_proc(get_module('kernel32.dll'), b'LoadLibraryW')
        local = modules(bind('GetCurrentProcessId', w.DWORD)())
        name, (local_base, _) = next((name, value) for name, value in local.items() if value[0] <= address < value[0] + value[1])
        loader = remote[name][0] + address - local_base
        data = str(dll).encode('utf-16-le') + b'\0\0'
        memory = allocate(process, None, len(data), 0x3000, 4)
        if not memory: raise c.WinError(c.get_last_error())
        copied = c.c_size_t(); buffer = c.create_string_buffer(data)
        if not write(process, memory, buffer, len(data), c.byref(copied)) or copied.value != len(data):
            raise c.WinError(c.get_last_error())
        run(loader, memory, 30000); free(process, memory, 0, 0x8000)
        remote = modules(args.pid)
    if dll.name.lower() not in remote: raise RuntimeError('probe DLL did not load')
    image = pefile.PE(str(dll))
    export = b'RunEditRestore' if args.edit_restore else (b'RunHttp' if args.http else (b'RunCodec' if args.codec else b'RunProfile'))
    offset = next(symbol.address for symbol in image.DIRECTORY_ENTRY_EXPORT.symbols if symbol.name == export)
    print('native worker exit', run(remote[dll.name.lower()][0] + offset, None, 300000 if args.edit_restore else 120000))
    if pathlib.Path(args.result_file).name != args.result_file: raise ValueError('result filename must not contain a path')
    output = root / '.review' / args.result_file
    if output.exists(): print(output.read_text(encoding='utf-8'))
finally: close(process)


