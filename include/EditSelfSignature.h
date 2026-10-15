#pragma once
#include "EditSelfNickname.h"
#include "SignatureWire.h"
#include "SelfProfileMutation.h"
namespace self_signature {
using Result = self_nickname::Result;
inline bool Valid(const std::string& value) {
    return value.size() <= 4096 && value.find('\0') == std::string::npos &&
        (value.empty() || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            value.data(), static_cast<int>(value.size()), nullptr, 0));
}
inline Result Edit(const std::string& signature) {
    Result result;
    if (!Valid(signature)) { result.message = "newSignature must be valid UTF-8 without NUL, at most 4096 bytes"; return result; }
    std::unique_lock<std::mutex> lock(SelfProfileMutationMutex(), std::try_to_lock);
    if (!lock.owns_lock()) { result.message = "another profile update request is running"; return result; }
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    self_profile_memory::Profile account, current; std::string error;
    if (!self_profile_memory::Read(GetCurrentProcess(), base, account, error)) { result.message = error; return result; }
    try {
        self_nickname::Native native(base); std::string original, oldNickname, previous;
        if (!native.Profile(account.wxid, original, oldNickname, error) ||
            !signature_wire::Value(original, previous) || !Valid(previous)) {
            result.message = "cannot read verified server signature"; return result;
        }
        if (!self_profile_memory::Read(GetCurrentProcess(), base, current, error) || current.wxid != account.wxid) {
            result.message = "account changed during signature read"; return result;
        }
        if (previous == signature) {
            result.ret = 0; result.verified = true; result.message = "signature already matches server profile"; return result;
        }
        std::string request, response;
        if (!signature_wire::Request(original, signature, request)) { result.message = "cannot build signature update"; return result; }
        if (!native.Cgi(0x3B1B040, 24, 0x19B7D40, request, response, error, &result.submitted, account.wxid)) {
            result.message = error; return result;
        }
        if (!nickname_wire::Accepted(response)) { result.message = "server did not confirm signature operation"; return result; }
        std::string confirmed, currentNickname, actual;
        if (!native.Profile(account.wxid, confirmed, currentNickname, error) ||
            !signature_wire::Value(confirmed, actual) || !Valid(actual)) {
            result.message = "operation accepted; signature server readback failed"; return result;
        }
        if (actual != signature || currentNickname != oldNickname) {
            result.message = "operation accepted; signature or preserved nickname verification failed"; return result;
        }
        if (!self_profile_memory::Read(GetCurrentProcess(), base, current, error) || current.wxid != account.wxid) {
            result.message = "operation accepted; account changed during verification"; return result;
        }
        result.ret = 0; result.verified = true; result.message = "signature updated and verified from server profile";
    } catch (const std::exception&) { result.message = "native signature request failed"; }
    return result;
}
}
