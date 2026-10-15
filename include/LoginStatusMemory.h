#pragma once
#include "ProfileMemory.h"

// Independently verified for IsLoginStatus; does not enable other native APIs.
namespace login_status {
inline constexpr uintptr_t manager_rva = 0xBB92FC0;
inline constexpr uintptr_t account_member = 0x68;
inline constexpr uintptr_t account_vtable = 0x8FD1498;
inline constexpr DWORD image_timestamp = 0x6ACCBF76;
inline constexpr DWORD image_size = 0xC34F000;

struct Status {
    bool logged_in = false;
    std::string wxid, alias, nickname;
};

inline bool Read(HANDLE process, uintptr_t base, Status& out, std::string& error) {
    using profile_memory::ReadValue;
    using profile_memory::ReadString;
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
        error = "unsupported Weixin.dll build; expected verified 4.1.15.55 image";
        return false;
    }
    uintptr_t manager = 0, account = 0, table = 0;
    unsigned char flag = 0;
    if (!ReadValue(process, base + manager_rva, manager) || !manager ||
        !ReadValue(process, manager + account_member, account) || !account ||
        !ReadValue(process, account, table) || table != base + account_vtable ||
        !ReadValue(process, account + 0x38, flag)) {
        error = "account state unavailable or layout mismatch";
        return false;
    }
    Status value{};
    value.logged_in = (flag & 1) != 0;
    if (value.logged_in &&
        (!ReadString(process, account + 0x48, value.wxid) || value.wxid.empty() ||
         !ReadString(process, account + 0x68, value.alias) ||
         !ReadString(process, account + 0x88, value.nickname))) {
        error = "logged-in account fields unreadable or invalid UTF-8";
        return false;
    }
    uintptr_t managerAfter = 0, accountAfter = 0, tableAfter = 0;
    unsigned char flagAfter = 0;
    Status confirm{};
    if (!ReadValue(process, base + manager_rva, managerAfter) || managerAfter != manager ||
        !ReadValue(process, manager + account_member, accountAfter) || accountAfter != account ||
        !ReadValue(process, account, tableAfter) || tableAfter != table ||
        !ReadValue(process, account + 0x38, flagAfter) || ((flagAfter ^ flag) & 1) ||
        (value.logged_in &&
         (!ReadString(process, account + 0x48, confirm.wxid) || confirm.wxid != value.wxid ||
          !ReadString(process, account + 0x68, confirm.alias) || confirm.alias != value.alias ||
          !ReadString(process, account + 0x88, confirm.nickname) || confirm.nickname != value.nickname))) {
        error = "account changed while reading; retry request";
        return false;
    }
    out = std::move(value);
    return true;
}
}
