#pragma once
#include "httplib.h"
#include "json.hpp"

namespace personal_info {
// A negative response is a guard, never an implementation of the native operation.
inline void Unavailable(httplib::Server& server, const char* route, const char* reason) {
    server.Post(route, [reason](const httplib::Request&, httplib::Response& res) {
        nlohmann::json response = {{"ret", -1}, {"retmsg", reason},
            {"implemented", false}, {"submitted", false}, {"verified", false}};
        res.set_content(response.dump(), "application/json");
    });
}
}
