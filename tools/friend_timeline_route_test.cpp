#define WIN32_LEAN_AND_MEAN
#include "../include/FriendTimelineRoute.h"
#include <iostream>
#include <stdexcept>
static void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        bool called = false;
        const auto fetch = [&](const std::string& user, const std::string& cursor) {
            called = true;
            Check(user == "friend" && cursor == "100", "forward user and cursor");
            friend_timeline_native::Result result;
            result.submitted = result.verified = true; result.businessRet = 0;
            result.objects = {{90, 30, user, "a", "<contentDesc><![CDATA[hello]]></contentDesc>"},
                              {95, 20, user, "b", ""}, {85, 50, user, "c", ""}};
            return result;
        };
        auto reply = friend_timeline_route::Handle(R"({"wxid":"friend","id":"100","count":2})", fetch);
        Check(called && reply["ret"] == 0 && reply["source"] == "network" && reply["count"] == 2 &&
            reply["server_count"] == 3 && reply["next_id"] == "90", "count and safe cursor before time sort");
        Check(reply["data"][0]["sns_id"] == "90" && reply["data"][0]["content_desc"] == "hello" &&
            reply["data"][1]["sns_id"] == "95", "time order and CDATA");
        called = false;
        reply = friend_timeline_route::Handle("{}", fetch);
        Check(!called && reply["ret"] == -1 && reply["submitted"] == false, "validate before fetch");
        reply = friend_timeline_route::Handle(R"({"wxid":"friend"})", [](const auto&, const auto&) {
            friend_timeline_native::Result result; result.submitted = true; result.error = "timeout"; return result;
        });
        Check(reply["ret"] == -1 && reply["submitted"] == true && reply["verified"] == false &&
            reply["data"].empty() && reply["retmsg"] == "timeout", "submitted failure / no cache fallback");
        reply = friend_timeline_route::Handle(R"({"wxid":"friend"})", [](const auto&, const auto&) {
            friend_timeline_native::Result result;
            result.submitted = result.verified = result.visibilityLimited = true;
            result.businessRet = 207; result.serverMessage = "visible window is limited";
            return result;
        });
        Check(reply["ret"] == 0 && reply["business_ret"] == 207 && reply["verified"] == true &&
            reply["visibility_limited"] == true && reply["count"] == 0 && reply["data"].empty() &&
            !reply.contains("next_id") && reply["retmsg"] == "visible window is limited", "restricted empty page");
        std::cout << "Friend timeline route tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
