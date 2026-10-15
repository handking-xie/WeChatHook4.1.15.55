#pragma once
#include "SnsLikeNative.h"
#include "json.hpp"
#include "httplib.h"

namespace sns_like_route {
template<class Operation> inline nlohmann::json Handle(const std::string& body, Operation operation) {
    nlohmann::json reply = {{"ret", -1}, {"retmsg", "invalid JSON body"}, {"submitted", false}, {"verified", false}};
    nlohmann::json input;
    try { input = nlohmann::json::parse(body); } catch (...) { return reply; }
    if (!input.is_object() || !input.contains("sns_id") || !input["sns_id"].is_string() ||
        !input.contains("to_wxid") || !input["to_wxid"].is_string()) {
        reply["retmsg"] = "sns_id and to_wxid are required strings"; return reply;
    }
    const auto id = input["sns_id"].get<std::string>(), target = input["to_wxid"].get<std::string>();
    uint64_t parsed = 0;
    if (id.empty() || id.size() > 20 || !friend_timeline_wire::Cursor(id, parsed) || !parsed || !sns_like_native::ValidTarget(target)) {
        reply["retmsg"] = "invalid sns_id or to_wxid"; return reply;
    }
    const auto result = operation(id, target);
    reply["submitted"] = result.submitted; reply["verified"] = result.verified;
    reply["business_ret"] = result.businessRet;
    reply["ret"] = result.verified ? 0 : -1;
    reply["retmsg"] = result.verified ? "success" : result.error;
    return reply;
}
inline void Register(httplib::Server& server) {
    server.Post("/sns_like", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(Handle(req.body, sns_like_native::Like).dump(), "application/json");
    });
}
}
