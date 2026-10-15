#pragma once
#include "LoginStatusMemory.h"
#include "httplib.h"
#include "json.hpp"
namespace online_info {
template<class Reader>
inline void Handle(httplib::Response& res, Reader reader) {
    login_status::Status status{};
    std::string error;
    nlohmann::json response;
    if (!reader(status, error)) {
        response = {{"ret", -1}, {"retmsg", error},
            {"online_status", -1}, {"is_online", nullptr}};
    } else {
        response = {{"ret", 0}, {"retmsg", "success"},
            {"online_status", status.logged_in ? 1 : 0},
            {"is_online", status.logged_in}, {"device", "PC"},
            {"scope", "local_client_login"}};
    }
    res.set_content(response.dump(), "application/json");
}
template<class Reader>
inline void Register(httplib::Server& server, Reader reader) {
    server.Post("/getonlineinfo", [reader](const httplib::Request&, httplib::Response& res) {
    Handle(res, reader);
    });
}
inline void Register(httplib::Server& server) {
    Register(server, [](login_status::Status& status, std::string& error) {
    return login_status::Read(GetCurrentProcess(),
        reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")), status, error);
    });
}
}


