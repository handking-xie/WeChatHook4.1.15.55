#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

struct SectionInfo {
    std::string name;
    DWORD virtualAddress;
    DWORD virtualSize;
    DWORD rawAddress;
    DWORD rawSize;
};

int main(int argc, char* argv[]) {
    const char* dllPath = "C:\\Program Files\\Tencent\\Weixin\\4.1.15.13\\Weixin.dll";
    HANDLE hFile = CreateFileA(dllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open Weixin.dll\n";
        return 1;
    }

    HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    BYTE* pBase = (BYTE*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)pBase;
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*)(pBase + dosHeader->e_lfanew);
    IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(ntHeaders);

    SectionInfo textSec{}, rdataSec{}, dataSec{};
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        char secName[9] = {0};
        memcpy(secName, sec[i].Name, 8);
        SectionInfo s{ secName, sec[i].VirtualAddress, sec[i].Misc.VirtualSize, sec[i].PointerToRawData, sec[i].SizeOfRawData };
        if (strcmp(secName, ".text") == 0) textSec = s;
        if (strcmp(secName, ".rdata") == 0) rdataSec = s;
        if (strcmp(secName, ".data") == 0) dataSec = s;
    }

    auto FindStringRvas = [&](const std::string& str) -> std::vector<DWORD> {
        std::vector<DWORD> rvas;
        BYTE* rdataPtr = pBase + rdataSec.rawAddress;
        size_t len = str.length();
        for (size_t i = 0; i + len < rdataSec.rawSize; i++) {
            if (memcmp(rdataPtr + i, str.c_str(), len) == 0 && rdataPtr[i + len] == 0) {
                rvas.push_back(rdataSec.virtualAddress + (DWORD)i);
            }
        }
        return rvas;
    };

    auto FindXrefs = [&](DWORD targetRva) -> std::vector<DWORD> {
        std::vector<DWORD> xrefs;
        BYTE* textPtr = pBase + textSec.rawAddress;
        for (DWORD i = 0; i + 7 < textSec.rawSize; i++) {
            int32_t disp = *(int32_t*)(textPtr + i);
            DWORD curRva = textSec.virtualAddress + i;
            if ((DWORD)(curRva + 4 + disp) == targetRva) {
                xrefs.push_back(curRva - 3);
            }
        }
        return xrefs;
    };

    std::vector<std::string> searchList = {
        "/cgi-bin/micromsg-bin/newsendmsg",
        "newsendmsg",
        "revokemsg",
        "filehelper",
        "msg_send",
        "SendMsg",
        "SendMessage"
    };

    std::cout << "=== IDA Pro Target Strings & XREFs in Weixin.dll ===\n\n";
    for (auto& s : searchList) {
        auto rvas = FindStringRvas(s);
        std::cout << "[String] \"" << s << "\" -> Found " << rvas.size() << " occurrences:\n";
        for (auto rva : rvas) {
            std::cout << "  RVA: 0x" << std::hex << rva << std::dec << "\n";
            auto xrefs = FindXrefs(rva);
            for (auto x : xrefs) {
                std::cout << "    Referenced in .text at RVA: 0x" << std::hex << x << std::dec << "\n";
            }
        }
        std::cout << "\n";
    }

    UnmapViewOfFile(pBase);
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return 0;
}
