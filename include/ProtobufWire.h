#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace protobuf_wire {
struct Field { uint32_t number, type; uint64_t value; std::string_view bytes, raw; };
inline bool Varint(std::string_view data, size_t& pos, uint64_t& out) {
    out = 0;
    for (unsigned i = 0; i < 10 && pos < data.size(); ++i) {
        const auto byte = static_cast<unsigned char>(data[pos++]);
        if (i == 9 && byte > 1) return false;
        out |= uint64_t(byte & 127) << (i * 7);
        if (!(byte & 128)) return true;
    }
    return false;
}
inline bool Parse(std::string_view data, std::vector<Field>& out) {
    out.clear(); size_t pos = 0;
    while (pos < data.size()) {
        size_t start = pos; uint64_t tag = 0, value = 0;
        if (!Varint(data, pos, tag) || !(tag >> 3) || (tag >> 3) > 0x1fffffff) return false;
        Field field{static_cast<uint32_t>(tag >> 3), static_cast<uint32_t>(tag & 7), 0, {}, {}};
        if (field.type == 0) { if (!Varint(data, pos, field.value)) return false; }
        else {
            if (field.type == 2) { if (!Varint(data, pos, value)) return false; }
            else if (field.type == 1) value = 8;
            else if (field.type == 5) value = 4;
            else return false;
            if (value > data.size() - pos) return false;
            field.bytes = data.substr(pos, static_cast<size_t>(value)); pos += static_cast<size_t>(value);
        }
        field.raw = data.substr(start, pos - start); out.push_back(field);
    }
    return true;
}
inline void PutVarint(std::string& out, uint64_t value) {
    while (value >= 128) { out.push_back(static_cast<char>((value & 127) | 128)); value >>= 7; }
    out.push_back(static_cast<char>(value));
}
inline std::string Integer(uint32_t number, uint64_t value) {
    std::string out; PutVarint(out, uint64_t(number) << 3); PutVarint(out, value); return out;
}
inline std::string Bytes(uint32_t number, std::string_view value) {
    std::string out; PutVarint(out, (uint64_t(number) << 3) | 2); PutVarint(out, value.size());
    out.append(value); return out;
}
inline bool Unique(std::string_view data, uint32_t number, uint32_t type, Field& result) {
    std::vector<Field> fields; if (!Parse(data, fields)) return false;
    bool found = false;
    for (const auto& field : fields) if (field.number == number) {
        if (found || field.type != type) return false;
        result = field; found = true;
    }
    return found;
}
}
