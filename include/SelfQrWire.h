#pragma once
#include "ProtobufWire.h"
#include <string>

namespace self_qr_wire {
inline std::string Request(std::string_view wxid) {
    // Native GetQRCodeRequest: username is SKBuiltinString_t, opcode/style are zero.
    return protobuf_wire::Bytes(2, protobuf_wire::Bytes(1, wxid)) +
        protobuf_wire::Integer(3, 0) + protobuf_wire::Integer(4, 0);
}
inline bool ValidUrl(std::string_view url) {
    constexpr std::string_view prefixes[] = {"https://u.wechat.com/",
        "http://weixin.qq.com/r/", "https://weixin.qq.com/r/"};
    if (url.size() > 4096) return false;
    bool prefix = false;
    for (auto p : prefixes) if (url.size() > p.size() && url.substr(0, p.size()) == p) prefix = true;
    if (!prefix) return false;
    for (unsigned char c : url) if (c <= 32 || c >= 127 || c == '"' || c == '\'' || c == '<' || c == '>' || c == '\\') return false;
    return true;
}
inline bool Response(std::string_view wire, std::string& url, std::string& error) {
    using namespace protobuf_wire;
    url.clear(); error.clear(); Field base{}, ret{}, link{};
    if (!Unique(wire, 1, 2, base) || !Unique(base.bytes, 1, 0, ret)) {
        error = "native QR response has invalid BaseResponse"; return false;
    }
    if (ret.value != 0) {
        error = "server rejected QR request; business code=" + std::to_string(ret.value); return false;
    }
    if (!Unique(wire, 9, 2, link) || !ValidUrl(link.bytes)) {
        error = "native QR response is missing a valid personal QR URL"; return false;
    }
    url.assign(link.bytes); return true;
}
}
