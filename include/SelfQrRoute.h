#pragma once
#include "SelfQrNative.h"
#include "httplib.h"
#include "json.hpp"

namespace self_qr {
template<class Fetcher>
inline void Register(httplib::Server& server, Fetcher fetcher) {
    server.Post("/GetQRCode", [fetcher](const httplib::Request& req, httplib::Response& res) {
        Result result;
        bool valid = true;
        if (!req.body.empty()) {
            auto body = nlohmann::json::parse(req.body, nullptr, false);
            valid = body.is_object() && body.empty();
        }
        if (!valid) result.message = "expected an empty JSON object; QR URL, target and reset parameters are not supported";
        else result = fetcher();
        // Never publish URL data on failure, even if a backend populated it.
        bool success = result.ret == 0 && result.verified && self_qr_wire::ValidUrl(result.url);
        nlohmann::json response = {{"ret", success ? 0 : -1},
            {"retmsg", success ? "success" : (result.message.empty() || result.ret == 0 ? "unverified personal QR response" : result.message)},
            {"submitted", result.submitted}, {"verified", success}};
        if (success) { response["QRCode_URL"] = result.url; response["qr_url"] = result.url; }
        res.set_content(response.dump(), "application/json");
    });
}
inline void Register(httplib::Server& server) { Register(server, [] { return Fetch(); }); }
}
