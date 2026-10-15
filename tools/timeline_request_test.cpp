#include "../include/TimelineRequest.h"
#include <iostream>
#include <stdexcept>

static void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
int main() {
    try {
        timeline_request::Parameters p;
        std::string error;
        const auto parse = [&](const std::string& body, bool required = true) {
            return timeline_request::Parse(body, required, p, error);
        };
        Check(parse(R"({"wxid":"friend","id":"18446744073709551615","count":5})") &&
            p.user == "friend" && p.cursor == "18446744073709551615" && p.count == 5, "friend page");
        Check(!parse("{}") && p.user.empty() && p.cursor.empty(), "required friend / cleared output");
        Check(parse("", false) && p.count == 20, "optional home request");
        for (const auto* body : {"{", "[]", "null", R"({"wxid":1})", R"({"wxid":"x","id":1})",
            R"({"wxid":"x","count":1.5})", R"({"wxid":"x\u0000y"})"})
            Check(!parse(body) && !error.empty(), "invalid request");
        Check(parse(R"({"user_name":"","username":"friend","wxid":"other"})") && p.user == "friend", "first nonempty alias");
        Check(parse(R"({"user_name":"first","username":"second"})") && p.user == "first", "alias precedence");
        Check(parse(R"({"wxid":"x","count":18446744073709551615})") && p.count == 100, "unsigned clamp");
        Check(parse(R"({"wxid":"x","count":-9223372036854775808})") && p.count == 20, "negative default");
        Check(timeline_request::Escape("x' OR 1=1 --") == "x'' OR 1=1 --", "SQL quote");
        Check(timeline_request::EscapeLike("%_\\'") == "\\%\\_\\\\''", "literal LIKE cursor");
        std::cout << "Timeline request tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
