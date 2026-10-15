#pragma once
#include "SelfQrCgi.h"
#include "SelfQrWire.h"
#include <mutex>

namespace self_qr {
struct Result {
    int ret = -1;
    bool submitted = false, verified = false;
    std::string message, url;
};
inline Result Fetch() {
    // Serialize only this DLL's reads. No reset opcode is exposed.
    static std::mutex requests;
    std::unique_lock<std::mutex> lock(requests, std::try_to_lock);
    Result result;
    if (!lock.owns_lock()) { result.message = "another personal QR request is running"; return result; }
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    self_profile_memory::Profile account; std::string error;
    if (!self_profile_memory::Read(GetCurrentProcess(), base, account, error)) {
        result.message = error; return result;
    }
    try {
        self_qr_cgi::Native native(base); std::string response, url;
        if (!native.Cgi(0x382BA90, 48, 0x1726630, self_qr_wire::Request(account.wxid),
                        response, error, &result.submitted, account.wxid)) {
            result.message = error; return result;
        }
        if (!self_qr_wire::Response(response, url, error)) {
            result.message = error; return result;
        }
        self_profile_memory::Profile current;
        if (!self_profile_memory::Read(GetCurrentProcess(), base, current, error) || current.wxid != account.wxid) {
            result.message = "account changed or became unavailable during personal QR request"; return result;
        }
        result.ret = 0; result.verified = true;
        result.message = "success"; result.url = std::move(url);
    } catch (const std::exception&) { result.message = "native personal QR request failed"; }
    return result;
}
}
