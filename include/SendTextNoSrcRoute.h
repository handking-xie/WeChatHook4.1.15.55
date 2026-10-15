#pragma once
#include "httplib.h"
#include "json.hpp"
#include "SendTextNative.h"

inline void RegisterSendTextNoSrc(httplib::Server& server) {
    server.Post("/SendTextMsg_NoSrc", [](const httplib::Request& request, httplib::Response& response) {
        using json = nlohmann::json;
        json result = {{"ret", -1}, {"submitted", false}, {"verified", false}};
        const auto body = json::parse(request.body, nullptr, false);
        std::string target, content, error;
        auto read = [&](std::initializer_list<const char*> names, std::string& value) {
            for (const auto name : names) {
                const auto field = body.find(name);
                if (field == body.end()) continue;
                if (!field->is_string()) { error = std::string(name) + " must be a string"; return false; }
                value = field->get<std::string>();
                if (!value.empty()) return true;
            }
            return true;
        };
        if (body.is_discarded()) error = "invalid json";
        else if (!body.is_object()) error = "request must be a JSON object";
        else if (read({"toid", "wxid", "wxidorgid", "gid"}, target) && read({"msg", "content"}, content)) {
            if (!send_text_native::ValidText(target, 4096)) error = "target must be nonempty valid UTF-8 without NUL, at most 4096 bytes";
            else if (!send_text_native::ValidText(content, 1024 * 1024)) error = "message must be nonempty valid UTF-8 without NUL, at most 1048576 bytes";
            else {
                const auto sent = send_text_native::Send(target, content);
                result["ret"] = sent.ret; result["retmsg"] = sent.message;
                result["submitted"] = sent.submitted; result["verified"] = sent.verified;
                if (sent.submitted) result["client_msg_id"] = sent.clientId;
                if (sent.verified) result["msg_id"] = std::to_string(sent.messageId);
            }
        }
        if (!error.empty()) result["retmsg"] = error;
        response.set_content(result.dump(), "application/json");
    });
}
