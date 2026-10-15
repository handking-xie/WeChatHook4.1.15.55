// Use powershell or cpp to scan memory for "https://u.wechat.com/"
const { execSync } = require('child_process');

const cppCode = `
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    DWORD pid = atoi(argv[1]);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
        return 1;
    }

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* p = (unsigned char*)si.lpMinimumApplicationAddress;

    std::string needle = "https://u.wechat.com/";
    std::string needle_wide = "h\\0t\\0t\\0p\\0s\\0:\\0/\\0/\\0u\\0.\\0w\\0e\\0c\\0h\\0a\\0t\\0.\\0c\\0o\\0m\\0/";

    int foundCount = 0;

    while (p < (unsigned char*)si.lpMaximumApplicationAddress) {
        if (VirtualQueryEx(hProcess, p, &mbi, sizeof(mbi)) == sizeof(mbi)) {
            if ((mbi.State == MEM_COMMIT) && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) && !(mbi.Protect & PAGE_GUARD)) {
                std::vector<char> buffer(mbi.RegionSize);
                SIZE_T bytesRead = 0;
                if (ReadProcessMemory(hProcess, p, buffer.data(), mbi.RegionSize, &bytesRead) && bytesRead > 0) {
                    // search ascii
                    for (size_t i = 0; i + needle.size() < bytesRead; ++i) {
                        if (memcmp(buffer.data() + i, needle.c_str(), needle.size()) == 0) {
                            std::string hit(buffer.data() + i, min((size_t)80, bytesRead - i));
                            // null terminate
                            size_t nullPos = hit.find('\\0');
                            if (nullPos != std::string::npos) hit = hit.substr(0, nullPos);
                            // ensure printable
                            bool printable = true;
                            for (char c : hit) if (c < 32 || c > 126) printable = false;
                            if (printable && hit.length() > needle.size()) {
                                std::cout << "FOUND ASCII [0x" << std::hex << (uintptr_t)(p + i) << "]: " << hit << std::endl;
                                foundCount++;
                            }
                        }
                    }
                    // search unicode
                    for (size_t i = 0; i + needle.size() * 2 < bytesRead; i += 2) {
                        bool match = true;
                        for (size_t j = 0; j < needle.size(); ++j) {
                            if (buffer[i + j * 2] != needle[j] || buffer[i + j * 2 + 1] != 0) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            std::wstring hitW((wchar_t*)(buffer.data() + i), min((size_t)60, (bytesRead - i) / 2));
                            size_t nullPos = hitW.find(L'\\0');
                            if (nullPos != std::wstring::npos) hitW = hitW.substr(0, nullPos);
                            std::string hitA(hitW.begin(), hitW.end());
                            std::cout << "FOUND UNICODE [0x" << std::hex << (uintptr_t)(p + i) << "]: " << hitA << std::endl;
                            foundCount++;
                        }
                    }
                }
            }
            p += mbi.RegionSize;
        } else {
            p += si.dwPageSize;
        }
    }

    CloseHandle(hProcess);
    std::cout << "Total hits: " << std::dec << foundCount << std::endl;
    return 0;
}
`;

const fs = require('fs');
fs.writeFileSync('d:\\WeChat-Hook\\tools\\scan_qr.cpp', cppCode);
console.log('Written scan_qr.cpp');
