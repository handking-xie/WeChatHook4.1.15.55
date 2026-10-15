#pragma once
#include "NativeCgi.h"
#include "SnsLikeWire.h"
#include <ctime>

namespace sns_like_native {
inline constexpr uintptr_t requestCtor = 0x5523190, submit = 0x301E860;
inline constexpr size_t requestSize = 0x28;
struct Result {
    bool submitted = false, verified = false;
    int64_t businessRet = -1;
    std::string error;
};
inline bool ValidTarget(const std::string& target) {
    return !target.empty() && target.size() <= 4096 && target.find('\0') == std::string::npos &&
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, target.data(), static_cast<int>(target.size()), nullptr, 0) > 0;
}
inline Result Like(const std::string& snsId, const std::string& target) {
    Result result; uint64_t id = 0;
    if (snsId.empty() || snsId.size() > 20 || !friend_timeline_wire::Cursor(snsId, id) || !id || !ValidTarget(target)) {
        result.error = "sns_id must be a nonzero decimal uint64 string; to_wxid must be valid UTF-8"; return result;
    }
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    profile_memory::Profile account;
    if (!profile_memory::Read(GetCurrentProcess(), base, account, result.error)) return result;
    try {
        native_cgi::Native native(base); std::string response;
        if (!native.Cgi(requestCtor, requestSize, submit,
            sns_like_wire::Request(id, account.wxid, target, static_cast<uint32_t>(std::time(nullptr))),
            response, result.error, &result.submitted)) return result;
        if (!sns_like_wire::Decode(response, id, result.businessRet, result.error)) return result;
        profile_memory::Profile after;
        if (!profile_memory::Read(GetCurrentProcess(), base, after, result.error) || after.wxid != account.wxid) {
            result.error = "account changed during SnsComment request"; return result;
        }
        result.verified = true;
    } catch (const std::exception& e) { result.error = e.what(); }
    return result;
}
}
