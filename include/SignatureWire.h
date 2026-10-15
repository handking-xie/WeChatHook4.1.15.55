#pragma once
#include "NicknameWire.h"
namespace signature_wire {
inline bool Value(std::string_view info, std::string& signature) {
    signature.clear();
    std::vector<protobuf_wire::Field> fields;
    if (!protobuf_wire::Parse(info, fields)) return false;
    bool found = false;
    for (const auto& field : fields) if (field.number == 13) {
        if (found || field.type != 2) return false;
        signature.assign(field.bytes); found = true;
    }
    // Optional proto2 string has an empty default when absent.
    return true;
}
inline bool Request(std::string_view info, std::string_view signature, std::string& request) {
    request.clear();
    std::vector<protobuf_wire::Field> fields;
    if (!protobuf_wire::Parse(info, fields)) return false;
    bool found = false; std::string updated;
    for (const auto& field : fields) {
        if (field.number == 13) {
            if (found || field.type != 2) return false;
            updated += protobuf_wire::Bytes(13, signature); found = true;
        } else updated.append(field.raw);
    }
    if (!found) updated += protobuf_wire::Bytes(13, signature);
    const auto buffer = protobuf_wire::Integer(1, updated.size()) + protobuf_wire::Bytes(2, updated);
    const auto command = protobuf_wire::Integer(1, 1) + protobuf_wire::Bytes(2, buffer);
    request = protobuf_wire::Bytes(1, protobuf_wire::Integer(1, 1) + protobuf_wire::Bytes(2, command));
    return true;
}
}
