#pragma once
#include "ProtobufWire.h"

namespace send_text_wire {
inline std::string Request(std::string_view target, std::string_view content,
                           uint32_t timestamp, uint32_t clientId) {
    using namespace protobuf_wire;
    const auto message = Bytes(1, Bytes(1, target)) + Bytes(2, content) +
        Integer(3, 1) + Integer(4, timestamp) + Integer(5, clientId) +
        Bytes(6, "") + Integer(7, 0);
    return Integer(1, 1) + Bytes(2, message);
}
inline bool Accepted(std::string_view response, std::string_view target,
                     uint32_t clientId, uint64_t& messageId, int64_t& businessRet) {
    using namespace protobuf_wire;
    messageId = 0; businessRet = -1;
    Field base{}, ret{}, count{}, item{}, name{}, text{}, client{}, id{};
    if (!Unique(response, 1, 2, base) || !Unique(base.bytes, 1, 0, ret)) return false;
    businessRet = static_cast<int64_t>(ret.value);
    if (businessRet || !Unique(response, 2, 0, count) || count.value != 1 ||
        !Unique(response, 3, 2, item) || !Unique(item.bytes, 1, 0, ret)) return false;
    businessRet = static_cast<int64_t>(ret.value);
    if (businessRet || !Unique(item.bytes, 2, 2, name) || !Unique(name.bytes, 1, 2, text) ||
        text.bytes != target || !Unique(item.bytes, 4, 0, client) || client.value != clientId ||
        !Unique(item.bytes, 8, 0, id) || !id.value) return false;
    messageId = id.value; return true;
}
}
