#pragma once
#include "SelfQrCgi.h"
#include "NicknameWire.h"
#include "SelfProfileMutation.h"
#include <mutex>
#include <stdexcept>
#include <memory>

namespace self_nickname {
struct Result {
    int ret = -1;
    bool submitted = false, verified = false;
    std::string message;
};
using NativeString = self_qr_cgi::NativeString;
class Native : public self_qr_cgi::Native {
public:
    using self_qr_cgi::Native::Native;
    bool Profile(std::string_view wxid, std::string& info, std::string& nickname, std::string& error) const {
        std::string response;
        if (!Cgi(0x1009600, 32, 0x49A1C0, nickname_wire::Bytes(2, wxid), response, error, nullptr, wxid)) return false;
        if (!nickname_wire::UserInfo(response, wxid, info, nickname)) {
            error = "server profile missing fields or account identity mismatch"; return false;
        }
        return true;
    }
};

inline Result Edit(const std::string& nickname) {
    Result invalid;
    if (nickname.empty() || nickname.size() > 4096 || nickname.find('\0') != std::string::npos ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, nickname.data(), static_cast<int>(nickname.size()), nullptr, 0)) {
        invalid.message = "nickname must be nonempty valid UTF-8 without NUL, at most 4096 bytes"; return invalid;
    }

    std::unique_lock<std::mutex> lock(SelfProfileMutationMutex(), std::try_to_lock);
    Result result;
    if (!lock.owns_lock()) { result.message = "another profile update request is running"; return result; }
    self_profile_memory::Profile account; std::string error;
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    if (!self_profile_memory::Read(GetCurrentProcess(), base, account, error)) { result.message = error; return result; }
    try {
        Native native(base); std::string original, oldNickname;
        if (!native.Profile(account.wxid, original, oldNickname, error)) { result.message = error; return result; }
        self_profile_memory::Profile checked;
        if (!self_profile_memory::Read(GetCurrentProcess(), base, checked, error) || checked.wxid != account.wxid) {
            result.message = "account changed during server profile read"; return result;
        }
        if (oldNickname == nickname) {
            result.ret = 0; result.verified = true; result.message = "nickname already matches server profile"; return result;
        }
        self_profile_memory::Profile current;
        if (!self_profile_memory::Read(GetCurrentProcess(), base, current, error) || current.wxid != account.wxid) {
            result.message = "account changed before nickname submission"; return result;
        }
        std::string wire, response;
        if (!nickname_wire::Request(original, nickname, wire)) { result.message = "cannot build nickname request"; return result; }
        if (!native.Cgi(0x3B1B040, 24, 0x19B7D40, wire, response, error, &result.submitted, account.wxid)) { result.message = error; return result; }
        if (!nickname_wire::Accepted(response)) { result.message = "server did not confirm nickname operation"; return result; }
        std::string confirmed, actual;
        if (!native.Profile(account.wxid, confirmed, actual, error)) {
            result.message = "operation accepted; server readback failed: " + error; return result;
        }
        if (actual != nickname) { result.message = "operation accepted; server nickname has not matched"; return result; }
        if (!self_profile_memory::Read(GetCurrentProcess(), base, current, error) || current.wxid != account.wxid) {
            result.message = "operation accepted; account changed during verification"; return result;
        }
        result.ret = 0; result.verified = true; result.message = "nickname updated and verified from server profile";
    } catch (const std::exception& exception) { result.message = exception.what(); }
    return result;
}
}
