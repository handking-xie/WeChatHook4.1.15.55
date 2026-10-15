#pragma once
#include "NativeCgi.h"
#include "NicknameWire.h"
#include <mutex>
#include <stdexcept>
#include <memory>

namespace self_nickname {
struct Result {
    int ret = -1;
    bool submitted = false, verified = false;
    std::string message;
};
using NativeString = native_cgi::NativeString;
class Native : public native_cgi::Native {
public:
    using native_cgi::Native::Native;
    bool Profile(std::string_view wxid, std::string& info, std::string& nickname, std::string& error) const {
        std::string response;
        if (!Cgi(0xFE5770, 32, 0x485240, nickname_wire::Bytes(2, wxid), response, error)) return false;
        if (!nickname_wire::UserInfo(response, wxid, info, nickname)) {
            error = "server profile missing fields or account identity mismatch"; return false;
        }
        return true;
    }
};

inline Result Edit(const std::string& nickname) {
    static std::mutex mutation;
    std::unique_lock<std::mutex> lock(mutation, std::try_to_lock);
    Result result;
    if (!lock.owns_lock()) { result.message = "another nickname request is running"; return result; }
    profile_memory::Profile account; std::string error;
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) { result.message = error; return result; }
    try {
        Native native(base); std::string original, oldNickname;
        if (!native.Profile(account.wxid, original, oldNickname, error)) { result.message = error; return result; }
        if (oldNickname == nickname) {
            result.ret = 0; result.verified = true; result.message = "nickname already matches server profile"; return result;
        }
        profile_memory::Profile current;
        if (!profile_memory::Read(GetCurrentProcess(), base, current, error) || current.wxid != account.wxid) {
            result.message = "account changed before nickname submission"; return result;
        }
        std::string wire, response;
        if (!nickname_wire::Request(original, nickname, wire)) { result.message = "cannot build nickname request"; return result; }
        if (!native.Cgi(0x3A93040, 24, 0x19A1010, wire, response, error, &result.submitted)) { result.message = error; return result; }
        if (!nickname_wire::Accepted(response)) { result.message = "server did not confirm nickname operation"; return result; }
        std::string confirmed, actual;
        if (!native.Profile(account.wxid, confirmed, actual, error)) {
            result.message = "operation accepted; server readback failed: " + error; return result;
        }
        if (actual != nickname) { result.message = "operation accepted; server nickname has not matched"; return result; }
        result.ret = 0; result.verified = true; result.message = "nickname updated and verified from server profile";
    } catch (const std::exception& exception) { result.message = exception.what(); }
    return result;
}
}
