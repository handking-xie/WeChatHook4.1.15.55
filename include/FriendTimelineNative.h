#pragma once
#include "NativeCgi.h"
#include "FriendTimelineWire.h"

namespace friend_timeline_native {
inline constexpr uintptr_t requestCtor = 0x5522F50;
inline constexpr size_t requestSize = 0x58;
inline constexpr uintptr_t submit = 0x3018880;
struct Result {
    bool submitted = false, verified = false, visibilityLimited = false;
    int64_t businessRet = -1;
    std::string error, serverMessage;
    std::vector<friend_timeline_wire::Object> objects;
};
inline bool ValidUtf8(const std::string& text, bool allowEmpty = false) {
    return (allowEmpty || !text.empty()) && text.find('\0') == std::string::npos &&
        (text.empty() || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            text.data(), static_cast<int>(text.size()), nullptr, 0) > 0);
}
inline Result Fetch(const std::string& user, const std::string& cursor) {
    Result result;
    uint64_t maxId = 0;
    if (user.size() > 4096 || !ValidUtf8(user) || !friend_timeline_wire::Cursor(cursor, maxId)) {
        result.error = "user must be valid UTF-8; id must be a decimal uint64 string";
        return result;
    }
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    profile_memory::Profile account;
    if (!profile_memory::Read(GetCurrentProcess(), base, account, result.error)) return result;
    try {
        native_cgi::Native native(base);
        std::string response;
        if (!native.Cgi(requestCtor, requestSize, submit, friend_timeline_wire::Request(user, maxId),
            response, result.error, &result.submitted)) return result;
        std::vector<friend_timeline_wire::Object> objects;
        const bool decoded = friend_timeline_wire::Decode(response, user, maxId, objects,
            result.businessRet, result.error, &result.serverMessage);
        if (result.serverMessage.size() > 4096 || !ValidUtf8(result.serverMessage, true)) {
            result.serverMessage.clear();
            result.error = "invalid SnsUserPage server message"; return result;
        }
        if (!decoded) {
            if (!result.serverMessage.empty() && result.error == "SnsUserPage rejected by server")
                result.error += ": " + result.serverMessage;
            return result;
        }
        profile_memory::Profile after;
        if (!profile_memory::Read(GetCurrentProcess(), base, after, result.error) || after.wxid != account.wxid) {
            result.error = "account changed during SnsUserPage request";
            return result;
        }
        for (const auto& object : objects) if (!ValidUtf8(object.xml, true) || !ValidUtf8(object.nickname, true)) {
            result.error = "SnsObject contains invalid UTF-8 or NUL";
            return result;
        }
        result.objects = std::move(objects);
        result.visibilityLimited = result.businessRet == 207;
        result.verified = true;
    } catch (const std::exception& e) { result.error = e.what(); }
    return result;
}
}
