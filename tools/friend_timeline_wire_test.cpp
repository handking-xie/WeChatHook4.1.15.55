#include "../include/FriendTimelineWire.h"
#include <iostream>
#include <stdexcept>
using namespace protobuf_wire;
static void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static std::string Object(uint64_t id, const std::string& user = "friend") {
    const std::string xml = "<TimelineObject><contentDesc><![CDATA[hello]]></contentDesc></TimelineObject>";
    return Integer(1, id) + Bytes(2, user) + Bytes(3, "nickname") + Integer(4, 1700000000) +
        Bytes(5, Integer(1, xml.size()) + Bytes(2, xml));
}
int main() {
    try {
        uint64_t cursor;
        Check(friend_timeline_wire::Cursor("18446744073709551615", cursor) && cursor == UINT64_MAX, "uint64 cursor");
        for (const auto* value : {"18446744073709551616", "-1", "1x", "1.0", " 1"})
            Check(!friend_timeline_wire::Cursor(value, cursor), "reject invalid cursor");
        Check(friend_timeline_wire::Cursor("", cursor) && cursor == 0, "home cursor");
        const auto request = friend_timeline_wire::Request("friend", UINT64_MAX);
        Field user{}, max{}, digest{};
        Check(Unique(request, 3, 2, user) && user.bytes == "friend" && Unique(request, 4, 0, max) &&
            max.value == UINT64_MAX && Unique(request, 2, 2, digest) && digest.bytes.empty(), "full server request");
        const auto okBase = Bytes(1, Integer(1, 0));
        const auto object = Object(12345678901234567890ULL);
        const auto reply = okBase + Integer(3, 1) + Bytes(4, object) + Bytes(99, "unknown");
        std::vector<friend_timeline_wire::Object> out;
        int64_t ret; std::string error;
        const auto decode = [&](const std::string& wire, uint64_t maxId = 0) {
            return friend_timeline_wire::Decode(wire, "friend", maxId, out, ret, error);
        };
        Check(decode(reply) && out.size() == 1 && out[0].id == 12345678901234567890ULL &&
            out[0].nickname == "nickname" && ret == 0, "valid response / precise ID / unknown fields");
        Check(decode(okBase + Integer(3, 0)) && out.empty(), "empty server album");
        const auto limitedBase = Bytes(1, Integer(1, 207));
        const auto notice = Bytes(12, "visible window is limited");
        std::string serverMessage;
        Check(friend_timeline_wire::Decode(limitedBase + Integer(3, 0) + notice, "friend", 0,
            out, ret, error, &serverMessage) && ret == 207 && out.empty() &&
            serverMessage == "visible window is limited", "207 visibility response");
        Check(decode(limitedBase + Integer(3, 1) + Bytes(4, object)) && out.size() == 1,
            "limited response retains validated visible objects");
        Check(!decode(limitedBase + Integer(3, 1)) && out.empty(), "207 does not bypass count validation");
        Check(!decode(limitedBase + Integer(3, 0) + notice + notice), "duplicate server message");
        Check(!decode(Bytes(1, Integer(1, 4294967296ULL)) + Integer(3, 0)), "business code must not truncate to success");
        Check(!decode(Bytes(1, Integer(1, 208)) + Integer(3, 0)), "unknown nonzero code still fails");
        for (const auto& wire : {reply.substr(0, reply.size() - 1), reply + Integer(3, 1),
            okBase + Integer(3, 2) + Bytes(4, object), okBase + Integer(3, 2) + Bytes(4, object) + Bytes(4, object),
            okBase + Integer(3, 1) + Bytes(4, Object(7, "other")),
            okBase + Integer(3, 1) + Bytes(4, object + Integer(1, 9)),
            Bytes(1, Integer(1, uint64_t(-3))) + Integer(3, 0), Integer(3, 0)})
            Check(!decode(wire) && out.empty() && !error.empty(), "invalid reply / clear output");
        Check(!decode(reply, 12345678901234567890ULL), "exclusive maxId");
        Check(decode(reply, UINT64_MAX), "older page identity");
        std::cout << "Friend timeline wire tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
