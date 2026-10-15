#pragma once
#include "httplib.h"
#include "EditSelfNickname.h"
#include "json.hpp"

template<class Editor>
inline void RegisterEditSelfNickname(httplib::Server& svr, Editor editor) {
    using json = nlohmann::json;
    svr.Post("/EditSelfNickname", [editor](const httplib::Request& req, httplib::Response& res) {
        json resp = {{"submitted", false}, {"verified", false}};
        auto fail = [&](const char* message) {
            resp["ret"] = -1;
            resp["retmsg"] = message;
            res.set_content(resp.dump(), "application/json");
        };
        json reqJson = json::parse(req.body, nullptr, false);
        if (reqJson.is_discarded()) { fail("invalid json"); return; }
        if (!reqJson.is_object()) { fail("request must be a JSON object"); return; }
        const auto field = reqJson.contains("nickname") ? reqJson.find("nickname") : reqJson.find("nick_name");
        if (field == reqJson.end() || !field->is_string()) {
            fail("nickname must be a string"); return;
        }
        const std::string nickname = field->get<std::string>();
        if (nickname.empty())
        {
            resp["ret"] = -1;
            resp["retmsg"] = "nickname is required";
            res.set_content(resp.dump(), "application/json");
            return;
        }

        if (nickname.size() > 4096 || nickname.find('\0') != std::string::npos ||
            !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, nickname.data(),
                static_cast<int>(nickname.size()), nullptr, 0)) {
            fail("nickname must be valid UTF-8 without NUL, at most 4096 bytes"); return;
        }
        const auto result = editor(nickname);
        resp["ret"] = result.ret;
        resp["retmsg"] = result.message;
        resp["submitted"] = result.submitted;
        resp["verified"] = result.verified;
        res.set_content(resp.dump(), "application/json");
    });

}

inline void RegisterEditSelfNickname(httplib::Server& svr) {
    RegisterEditSelfNickname(svr, [](const std::string& value) { return self_nickname::Edit(value); });
}
