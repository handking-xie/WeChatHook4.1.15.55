#pragma once
#include "ProtobufWire.h"
#include <limits>
#include <set>

namespace friend_timeline_wire {
using namespace protobuf_wire;
struct Object {
    uint64_t id = 0;
    uint32_t createTime = 0;
    std::string user, nickname, xml;
};
inline bool Cursor(std::string_view text, uint64_t& value) {
    value = 0;
    if (text.empty()) return true;
    for (char c : text) {
        if (c < '0' || c > '9' || value > (std::numeric_limits<uint64_t>::max() - (c - '0')) / 10)
            return false;
        value = value * 10 + (c - '0');
    }
    return true;
}
inline std::string Request(std::string_view user, uint64_t maxId) {
    // Native album caller 0x2FE4950 sets +0x10 (field 2) to empty MD5,
    // copies the username into +0x18 (field 3), and sets presence mask 0x17E.
    // Match its initial zero-valued fields; never send a cache digest.
    // BaseRequest is filled by the native scene before serialization.
    return Bytes(2, "") + Bytes(3, user) + Integer(4, maxId) + Integer(5, 0) +
        Integer(6, 0) + Integer(7, 0) + Integer(9, 0);
}
inline bool Decode(std::string_view wire, std::string_view user, uint64_t maxId,
                   std::vector<Object>& out, int64_t& businessRet, std::string& error,
                   std::string* serverMessage = nullptr) {
    out.clear(); businessRet = -1; error.clear();
    if (serverMessage) serverMessage->clear();
    const auto fail = [&](const char* message) { error = message; return false; };
    Field base{}, ret{}, count{};
    std::vector<Field> fields;
    if (!Parse(wire, fields) || !Unique(wire, 1, 2, base) || !Unique(base.bytes, 1, 0, ret))
        return fail("invalid SnsUserPage BaseResponse");
    if (ret.value > INT32_MAX && ret.value < UINT64_MAX - INT32_MAX)
        return fail("invalid SnsUserPage business code");
    businessRet = static_cast<int32_t>(static_cast<uint32_t>(ret.value));
    // Native album caller 0x2FE6A41 classifies 207 separately and consumes
    // response field 12 (+0x60) as its display message. A live response confirms
    // a visibility window with zero objects, not an ordinary RPC rejection.
    for (const auto& field : fields) if (field.number == 12) {
        Field message{};
        if (!Unique(wire, 12, 2, message)) return fail("invalid SnsUserPage server message");
        if (serverMessage) serverMessage->assign(message.bytes);
    }
    if (businessRet != 0 && businessRet != 207) return fail("SnsUserPage rejected by server");
    if (!Unique(wire, 3, 0, count) || count.value > 10000)
        return fail("invalid SnsUserPage object count");
    std::vector<Object> objects;
    std::set<uint64_t> ids;
    for (const auto& field : fields) if (field.number == 4) {
        Field id{}, name{}, time{}, desc{}, length{}, buffer{}, nick{};
        if (field.type != 2 || !Unique(field.bytes, 1, 0, id) || !id.value ||
            !Unique(field.bytes, 2, 2, name) || name.bytes != user ||
            !Unique(field.bytes, 4, 0, time) || time.value > UINT32_MAX ||
            !Unique(field.bytes, 5, 2, desc) || !Unique(desc.bytes, 1, 0, length) ||
            !Unique(desc.bytes, 2, 2, buffer) || length.value != buffer.bytes.size() ||
            (maxId && id.value >= maxId) || !ids.insert(id.value).second)
            return fail("invalid SnsObject or user/cursor mismatch");
        Object object;
        object.id = id.value; object.createTime = static_cast<uint32_t>(time.value);
        object.user.assign(name.bytes); object.xml.assign(buffer.bytes);
        // Nickname is optional, but a present field must be unique and a string.
        std::vector<Field> objectFields;
        if (!Parse(field.bytes, objectFields)) return fail("invalid SnsObject");
        for (const auto& f : objectFields) if (f.number == 3) {
            if (!Unique(field.bytes, 3, 2, nick)) return fail("invalid SnsObject nickname");
            object.nickname.assign(nick.bytes);
        }
        objects.push_back(std::move(object));
    }
    if (objects.size() != count.value) return fail("SnsUserPage count mismatch");
    out = std::move(objects);
    return true;
}
}
