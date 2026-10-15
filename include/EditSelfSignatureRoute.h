#pragma once
#include "EditSelfSignature.h"
#include "httplib.h"
#include "json.hpp"
namespace self_signature {
template<class Editor>
inline void Register(httplib::Server& server, Editor editor) {
    server.Post("/api/mod_self_nick_signature", [editor](const httplib::Request& req, httplib::Response& res) {
        Result result;
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (!body.is_object() || !body.contains("newSignature") || !body["newSignature"].is_string()) {
            result.message = "newSignature must be a string in a JSON object";
        } else {
            auto value = body["newSignature"].template get<std::string>();
            if (!Valid(value)) result.message = "newSignature must be valid UTF-8 without NUL, at most 4096 bytes";
            else result = editor(value);
        }
        const bool success = result.ret == 0 && result.verified;
        nlohmann::json response = {{"ret",success ? 0 : -1},{"retmsg",result.message},
            {"submitted",result.submitted},{"verified",success},
            {"errCode",success ? 1 : -1},{"errMsg",result.message}};
        res.set_content(response.dump(), "application/json");
    });
}
inline void Register(httplib::Server& server) { Register(server, [](const std::string& value) { return Edit(value); }); }
}
