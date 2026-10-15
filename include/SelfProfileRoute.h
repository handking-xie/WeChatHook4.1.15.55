#pragma once
#include "SelfProfileMemory.h"
#include "httplib.h"
#include "json.hpp"

namespace self_profile_memory {
template<class Reader>
inline void Handle(httplib::Response& res, Reader reader) {
    Profile value{};
    std::string error;
    nlohmann::json response;
    if (!reader(value, error)) {
        response = {{"ret", -1}, {"retmsg", error}};
    } else {
        response = {{"ret", 0}, {"retmsg", "success"},
            {"wxid", value.wxid}, {"alias", value.alias},
            {"nickname", value.nickname}, {"phone", value.phone},
            {"email", ""}, {"qq", 0}, {"proiv", ""}, {"area", ""}, {"signinfo", ""},
            {"unavailable_fields", {"email", "qq", "proiv", "area", "signinfo"}}};
    }
    res.set_content(response.dump(), "application/json");
}
template<class Reader>
inline void Register(httplib::Server& server, Reader reader) {
    server.Post("/GetSelfProfile", [reader](const httplib::Request&, httplib::Response& res) {
        Handle(res, reader);
    });
}
inline void Register(httplib::Server& server) {
    Register(server, [](Profile& value, std::string& error) {
        return Read(GetCurrentProcess(), reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")), value, error);
    });
}
}
