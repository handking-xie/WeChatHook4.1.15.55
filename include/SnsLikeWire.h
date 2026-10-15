#pragma once
#include "FriendTimelineWire.h"

namespace sns_like_wire {
using namespace protobuf_wire;
inline std::string Request(uint64_t id, std::string_view self, std::string_view target, uint32_t time) {
    // SnsActionGroup: Id=1, CurrentAction=3. Type=1 adds a like (5 removes it).
    const auto action = Bytes(1, self) + Bytes(2, target) + Integer(5, 1) +
        Integer(6, 0) + Integer(7, time);
    return Bytes(2, Integer(1, id) + Bytes(3, action));
}
inline bool Decode(std::string_view wire, uint64_t id, int64_t& businessRet, std::string& error) {
    businessRet = -1; error.clear();
    Field base{}, ret{}, object{}, objectId{};
    if (!Unique(wire, 1, 2, base) || !Unique(base.bytes, 1, 0, ret) ||
        (ret.value > INT32_MAX && ret.value < UINT64_MAX - INT32_MAX)) {
        error = "invalid SnsComment BaseResponse"; return false;
    }
    businessRet = static_cast<int32_t>(static_cast<uint32_t>(ret.value));
    if (businessRet) { error = "SnsComment rejected by server: " + std::to_string(businessRet); return false; }
    if (!Unique(wire, 2, 2, object) || !Unique(object.bytes, 1, 0, objectId) || objectId.value != id) {
        error = "SnsComment object ID mismatch or missing"; return false;
    }
    return true;
}
}
