#define WIN32_LEAN_AND_MEAN
#include "../include/SnsLikeRoute.h"
#include <iostream>
#include <stdexcept>
void Check(bool ok) { if (!ok) throw std::runtime_error("sns_like test failed"); }
int main() {
    using namespace protobuf_wire;
    const uint64_t id = 15027695064333161080ULL;
    Field group{}, object{}, action{}, type{};
    auto request = sns_like_wire::Request(id, "self", "target", 123);
    Check(Unique(request, 2, 2, group) && Unique(group.bytes, 1, 0, object) && object.value == id);
    Check(Unique(group.bytes, 3, 2, action) && Unique(action.bytes, 5, 0, type) && type.value == 1);
    int64_t ret; std::string error;
    auto response = Bytes(1, Integer(1, 0)) + Bytes(2, Integer(1, id));
    Check(sns_like_wire::Decode(response, id, ret, error));
    Check(!sns_like_wire::Decode(response, id - 1, ret, error));
    Check(!sns_like_wire::Decode(Bytes(1, Integer(1, 0)), id, ret, error));
    Check(!sns_like_wire::Decode(response + Bytes(2, Integer(1, id)), id, ret, error));
    Check(!sns_like_wire::Decode(Bytes(1, Integer(1, UINT64_MAX)), id, ret, error) && ret == -1);
    Check(!sns_like_wire::Decode(response.substr(0, response.size()-1), id, ret, error));
    int calls = 0;
    auto operation = [&](const std::string&, const std::string&) { ++calls; sns_like_native::Result r; r.submitted = true; r.verified = true; r.businessRet = 0; return r; };
    for (const auto* body : {"{", "[]", "{}", "{\"sns_id\":1,\"to_wxid\":\"a\"}",
        "{\"sns_id\":\"0\",\"to_wxid\":\"a\"}", "{\"sns_id\":\"18446744073709551616\",\"to_wxid\":\"a\"}",
        "{\"sns_id\":\"1\",\"to_wxid\":\"a\\u0000\"}"})
        Check(sns_like_route::Handle(body, operation)["ret"] == -1);
    Check(calls == 0);
    Check(sns_like_route::Handle("{\"sns_id\":\"15027695064333161080\",\"to_wxid\":\"target\"}", operation)["verified"] == true && calls == 1);
    auto timeout = [](const std::string&, const std::string&) {
        sns_like_native::Result r; r.submitted = true; r.error = "outcome unknown"; return r;
    };
    auto unknown = sns_like_route::Handle("{\"sns_id\":\"1\",\"to_wxid\":\"target\"}", timeout);
    Check(unknown["ret"] == -1 && unknown["submitted"] == true && unknown["verified"] == false);
    auto longTarget = nlohmann::json({{"sns_id", "1"}, {"to_wxid", std::string(4097, 'a')}}).dump();
    Check(sns_like_route::Handle(longTarget, operation)["ret"] == -1 && calls == 1);
    Check(!sns_like_native::ValidTarget(std::string("\xc0\xaf", 2)));
    std::cout << "sns_like protocol and route tests passed\n";
}
