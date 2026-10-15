#pragma once
#include "ProtobufWire.h"
namespace nickname_wire {
using namespace protobuf_wire;
inline bool UserInfo(std::string_view profile, std::string_view wxid, std::string& info, std::string& nickname) {
    Field user{}, name{}, nick{}, text{}, flag{}, base{}, ret{};
    if (!Unique(profile, 1, 2, base) || !Unique(base.bytes, 1, 0, ret) || ret.value ||
        !Unique(profile, 2, 2, user) || !Unique(user.bytes, 1, 0, flag) ||
        !Unique(user.bytes, 2, 2, name) || !Unique(name.bytes, 1, 2, text) || text.bytes != wxid ||
        !Unique(user.bytes, 3, 2, nick) || !Unique(nick.bytes, 1, 2, text)) return false;
    info.assign(user.bytes); nickname.assign(text.bytes); return true;
}
inline bool Request(std::string_view original, std::string_view nickname, std::string& request) {
    std::vector<Field> fields; if (!Parse(original, fields)) return false;
    // Keep every other field byte-for-byte, including unknown fields and account flags.
    std::string updated; bool replaced = false;
    for (const auto& field : fields) {
        if (field.number == 3) {
            if (replaced || field.type != 2) return false;
            updated += Bytes(3, Bytes(1, nickname)); replaced = true;
        } else updated.append(field.raw);
    }
    if (!replaced) return false;
    const auto buffer = Integer(1, updated.size()) + Bytes(2, updated);
    const auto command = Integer(1, 1) + Bytes(2, buffer);
    request = Bytes(1, Integer(1, 1) + Bytes(2, command)); return true;
}
inline bool Accepted(std::string_view response) {
    Field ret{}, list{}, count{}; std::vector<Field> fields;
    if (!Unique(response, 1, 0, ret) || ret.value || !Unique(response, 2, 2, list) ||
        !Unique(list.bytes, 1, 0, count) || count.value != 1 || !Parse(list.bytes, fields)) return false;
    unsigned results = 0;
    for (const auto& field : fields) if (field.number == 2) {
        if (field.type == 0) { if (field.value) return false; ++results; }
        else if (field.type == 2) {
            size_t pos = 0; uint64_t value = 0;
            while (pos < field.bytes.size()) {
                if (!Varint(field.bytes, pos, value) || value) return false;
                ++results;
            }
        } else return false;
    }
    return results == 1;
}
}
