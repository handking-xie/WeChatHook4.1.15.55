#pragma once
#include "LoginStatusMemory.h"
#include "httplib.h"
#include "json.hpp"

namespace login_status {
template<class Reader>
inline void Handle(httplib::Response& res, Reader reader) {
    Status status{};
    std::string error;
    nlohmann::json response;
    if (!reader(status, error)) {
        response = {{"ret", -1}, {"retmsg", error}, {"status", -1}, {"is_login", nullptr}};
    } else {
        response = {{"ret", 0}, {"retmsg", "success"},
            {"status", status.logged_in ? 1 : 0}, {"is_login", status.logged_in}};
        if (status.logged_in) {
            response["wxid"] = status.wxid;
            response["alias"] = status.alias;
            response["nickname"] = status.nickname;
        }
    }
    res.set_content(response.dump(), "application/json");
}
template<class Reader>
inline void Register(httplib::Server& server, Reader reader) {
    server.Post("/IsLoginStatus", [reader](const httplib::Request&, httplib::Response& res) {
        Handle(res, reader);
    });
}
inline void Register(httplib::Server& server) {
    Register(server, [](Status& status, std::string& error) {
        return Read(GetCurrentProcess(), reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")), status, error);
    });
}
}
