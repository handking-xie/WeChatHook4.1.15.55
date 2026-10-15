#pragma once
#include "httplib.h"
#include "TimelineRequest.h"
#include "FriendTimelineNative.h"
#include <algorithm>

namespace friend_timeline_route {
inline std::string XmlTag(const std::string& xml, const std::string& tag) {
    const auto open = "<" + tag + ">", close = "</" + tag + ">";
    auto start = xml.find(open);
    if (start == std::string::npos) return {};
    start += open.size();
    const auto end = xml.find(close, start);
    if (end == std::string::npos) return {};
    auto value = xml.substr(start, end - start);
    if (value.size() >= 12 && value.compare(0, 9, "<![CDATA[") == 0 &&
        value.compare(value.size() - 3, 3, "]]>") == 0)
        value = value.substr(9, value.size() - 12);
    return value;
}
template<class Fetch>
inline nlohmann::json Handle(const std::string& body, Fetch fetch) {
    nlohmann::json response = {{"ret", -1}, {"submitted", false}, {"verified", false},
        {"source", "network"}, {"data", nlohmann::json::array()}};
    timeline_request::Parameters parameters;
    std::string error;
    if (!timeline_request::Parse(body, true, parameters, error)) {
        response["retmsg"] = error; return response;
    }
    auto result = fetch(parameters.user, parameters.cursor);
    response["submitted"] = result.submitted;
    response["verified"] = result.verified;
    if (result.submitted) response["business_ret"] = result.businessRet;
    if (!result.serverMessage.empty()) response["server_message"] = result.serverMessage;
    if (!result.verified) {
        response["retmsg"] = result.error;
        return response;
    }
    // MaxId pagination is based on ID, even when presentation order uses time.
    // Sort before applying count so a truncated page cannot skip newer objects.
    std::stable_sort(result.objects.begin(), result.objects.end(), [](const auto& a, const auto& b) {
        return a.id > b.id;
    });
    const auto returned = std::min(result.objects.size(), static_cast<size_t>(parameters.count));
    auto items = nlohmann::json::array();
    uint64_t nextId = 0;
    for (size_t i = 0; i < returned; ++i) {
        const auto& object = result.objects[i];
        if (!nextId || object.id < nextId) nextId = object.id;
        nlohmann::json item = {{"sns_id", std::to_string(object.id)}, {"user_name", object.user},
            {"create_time", std::to_string(object.createTime)}, {"nickname", object.nickname},
            {"content", object.xml}};
        const auto description = XmlTag(object.xml, "contentDesc");
        if (!description.empty()) item["content_desc"] = description;
        items.push_back(std::move(item));
    }
    std::stable_sort(items.begin(), items.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
        return std::stoull(a["create_time"].get<std::string>()) > std::stoull(b["create_time"].get<std::string>());
    });
    response["ret"] = 0;
    response["visibility_limited"] = result.visibilityLimited;
    response["retmsg"] = result.visibilityLimited
        ? (result.serverMessage.empty() ? "timeline visibility is limited by server" : result.serverMessage)
        : "success";
    response["user_name"] = parameters.user;
    response["count"] = items.size(); response["server_count"] = result.objects.size();
    response["data"] = std::move(items);
    if (nextId) response["next_id"] = std::to_string(nextId);
    return response;
}
inline void Register(httplib::Server& server) {
    const auto handler = [](const httplib::Request& req, httplib::Response& res) {
        try { res.set_content(Handle(req.body, friend_timeline_native::Fetch).dump(), "application/json"); }
        catch (const std::exception& e) {
            res.set_content(nlohmann::json({{"ret", -1}, {"retmsg", e.what()},
                {"submitted", false}, {"verified", false}, {"data", nlohmann::json::array()}}).dump(), "application/json");
        }
    };
    server.Post("/GetFriendTimeline", handler);
    server.Post("/mmsnsuserpage", handler);
}
}
