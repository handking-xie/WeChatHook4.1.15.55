#pragma once
#include "NativeCgi.h"
#include "SendTextWire.h"
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

namespace send_text_native {
struct Result {
    int ret = -1;
    bool submitted = false, verified = false;
    uint32_t clientId = 0;
    uint64_t messageId = 0;
    std::string message;
};
inline bool ValidText(const std::string& value, size_t limit) {
    return !value.empty() && value.size() <= limit && value.find('\0') == std::string::npos &&
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0) > 0;
}
inline Result Send(const std::string& target, const std::string& content) {
    Result result;
    if (!ValidText(target, 4096) || !ValidText(content, 1024 * 1024)) {
        result.message = "target and message must be nonempty valid UTF-8 without NUL"; return result;
    }
    profile_memory::Profile account; std::string error;
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) {
        result.message = error; return result;
    }
    if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&result.clientId), sizeof(result.clientId),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
        result.message = "cannot generate client message ID"; return result;
    }
    if (!result.clientId) result.clientId = 1;
    FILETIME now{}; GetSystemTimeAsFileTime(&now);
    const uint64_t ticks = (uint64_t(now.dwHighDateTime) << 32) | now.dwLowDateTime;
    const auto timestamp = static_cast<uint32_t>(ticks / 10000000ULL - 11644473600ULL);
    try {
        const auto request = send_text_wire::Request(target, content, timestamp, result.clientId);
        std::string response;
        native_cgi::Native native(base);
        if (!native.Cgi(0x2E82760, 48, 0x39F28C0, request, response, error, &result.submitted)) {
            result.message = error; return result;
        }
        int64_t businessRet = -1;
        if (!send_text_wire::Accepted(response, target, result.clientId, result.messageId, businessRet)) {
            result.message = "server send response rejected or identity mismatch; business ret=" + std::to_string(businessRet);
            return result;
        }
        result.ret = 0; result.verified = true;
        result.message = "message accepted by server";
    } catch (const std::exception& exception) { result.message = exception.what(); }
    return result;
}
}
