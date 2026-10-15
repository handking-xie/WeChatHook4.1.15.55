#pragma once
#include "json.hpp"
#include <string>

namespace timeline_request {
struct Parameters {
    int count = 20;
    std::string user;
    std::string cursor;
};

inline bool Parse(const std::string& body, bool requireUser, Parameters& out, std::string& error)
{
    out = Parameters{};
    error.clear();
    const auto fail = [&](const char* message) {
        out = Parameters{};
        error = message;
        return false;
    };
    auto input = nlohmann::json::object();
    try { if (!body.empty()) input = nlohmann::json::parse(body); }
    catch (...) { return fail("invalid JSON body"); }
    if (!input.is_object()) return fail("JSON body must be an object");
    for (const auto* key : {"user_name", "username", "wxid", "id"}) {
        if (!input.contains(key)) continue;
        if (!input[key].is_string()) return fail("user and id fields must be strings");
        const auto value = input[key].get<std::string>();
        if (value.find('\0') != std::string::npos || value.size() > 4096)
            return fail("user or id contains NUL or exceeds 4096 bytes");
        if (std::string(key) == "id") out.cursor = value;
        else if (out.user.empty()) out.user = value;
    }
    if (requireUser && out.user.empty()) return fail("user_name, username or wxid is required");
    if (input.contains("count")) {
        const auto& count = input["count"];
        if (!count.is_number_integer()) return fail("count must be an integer");
        if (count.is_number_unsigned()) {
            const auto value = count.get<unsigned long long>();
            out.count = value == 0 ? 20 : value > 100 ? 100 : static_cast<int>(value);
        } else {
            const auto value = count.get<long long>();
            out.count = value <= 0 ? 20 : value > 100 ? 100 : static_cast<int>(value);
        }
    }
    return true;
}

// SQL string literal content; the database wrapper does not expose bind parameters.
inline std::string Escape(const std::string& value)
{
    std::string result;
    for (char c : value) { result += c; if (c == '\'') result += c; }
    return result;
}

inline std::string EscapeLike(const std::string& value)
{
    std::string result;
    for (char c : value) {
        if (c == '\\' || c == '%' || c == '_') result += '\\';
        result += c;
    }
    return Escape(result);
}
}
