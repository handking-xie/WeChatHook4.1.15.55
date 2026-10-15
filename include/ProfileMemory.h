#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

// Verified against the loaded 4.1.15.13 image and its account vtable getters.
namespace profile_memory {
inline constexpr uintptr_t manager_rva = 0xB912690;
inline constexpr uintptr_t account_member = 0x68;
inline constexpr uintptr_t account_vtable = 0x8DFE2F8;
inline constexpr DWORD image_timestamp = 0x6AB0A378;
inline constexpr DWORD image_size = 0xC0AE000;

struct Profile {
    std::string wxid, alias, nickname, phone;
};

inline bool ReadBytes(HANDLE process, uintptr_t address, void* out, size_t size) {
    SIZE_T copied = 0;
    return address && ReadProcessMemory(process, reinterpret_cast<const void*>(address),
        out, size, &copied) && copied == size;
}
template<class T> inline bool ReadValue(HANDLE process, uintptr_t address, T& out) {
    return ReadBytes(process, address, &out, sizeof(out));
}

inline bool ReadString(HANDLE process, uintptr_t address, std::string& out) {
    struct StringHeader { unsigned char storage[16]; uint64_t length, capacity; } header{}, after{};
    if (!ReadValue(process, address, header) || header.length > 4096 ||
        header.capacity < header.length || header.capacity > 1024 * 1024 ||
        header.capacity < 15) return false;
    std::string value(static_cast<size_t>(header.length), '\0');
    if (header.capacity == 15) {
        if (header.length > 15) return false;
        std::memcpy(value.data(), header.storage, value.size());
    } else {
        uintptr_t ptr = 0;
        std::memcpy(&ptr, header.storage, sizeof(ptr));
        if (!value.empty() && !ReadBytes(process, ptr, value.data(), value.size())) return false;
    }
    if (!ReadValue(process, address, after) || std::memcmp(&header, &after, sizeof(header))) return false;
    if (value.find('\0') != std::string::npos || (!value.empty() &&
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0))) return false;
    out = std::move(value);
    return true;
}

inline bool Read(HANDLE process, uintptr_t base, Profile& out, std::string& error) {
    out = {};
    error.clear();
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS64 nt{};
    if (!ReadValue(process, base, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000 ||
        !ReadValue(process, base + dos.e_lfanew, nt) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt.FileHeader.TimeDateStamp != image_timestamp || nt.OptionalHeader.SizeOfImage != image_size) {
        error = "unsupported Weixin.dll build; expected verified 4.1.15.13 image";
        return false;
    }
    uintptr_t manager = 0, account = 0, vtable = 0;
    unsigned char loggedIn = 0;
    if (!ReadValue(process, base + manager_rva, manager) || !manager ||
        !ReadValue(process, manager + account_member, account) || !account ||
        !ReadValue(process, account, vtable) || vtable != base + account_vtable) {
        error = "account object unavailable or layout mismatch";
        return false;
    }
    // The account vtable's first getter returns byte [this+0x38] & 1.
    if (!ReadValue(process, account + 0x38, loggedIn) || !(loggedIn & 1)) {
        error = "account is not logged in";
        return false;
    }
    Profile value{}, confirm{};
    if (!ReadString(process, account + 0x48, value.wxid) || value.wxid.empty() ||
        !ReadString(process, account + 0x68, value.alias) ||
        !ReadString(process, account + 0x88, value.nickname) ||
        !ReadString(process, account + 0xA8, value.phone)) {
        error = "account fields unreadable or invalid UTF-8";
        return false;
    }
    uintptr_t managerAfter = 0, accountAfter = 0;
    if (!ReadValue(process, base + manager_rva, managerAfter) || managerAfter != manager ||
        !ReadValue(process, manager + account_member, accountAfter) || accountAfter != account ||
        !ReadValue(process, account + 0x38, loggedIn) || !(loggedIn & 1) ||
        !ReadString(process, account + 0x48, confirm.wxid) || confirm.wxid != value.wxid ||
        !ReadString(process, account + 0x68, confirm.alias) || confirm.alias != value.alias ||
        !ReadString(process, account + 0x88, confirm.nickname) || confirm.nickname != value.nickname ||
        !ReadString(process, account + 0xA8, confirm.phone) || confirm.phone != value.phone) {
        error = "account changed while reading; retry request";
        return false;
    }
    out = std::move(value);
    return true;
}
}
