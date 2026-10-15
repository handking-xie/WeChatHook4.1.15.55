#include "../include/NicknameWire.h"
#include <iostream>
#include <stdexcept>
using namespace nickname_wire;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        const std::string unknown = Bytes(200, std::string("\0\xff", 2));
        const auto info = Integer(1, 0x1234) + Bytes(2, Bytes(1, "wxid_test")) +
            Bytes(3, Bytes(1, "old")) + Integer(8, 2) + unknown;
        const auto profile = Bytes(1, Integer(1, 0)) + Bytes(2, info);
        std::string saved, nickname, request;
        require(UserInfo(profile, "wxid_test", saved, nickname) && nickname == "old", "profile parse");
        require(!UserInfo(profile, "other", saved, nickname), "reject wrong account");
        require(Request(info, "\xe6\xb5\x8b\xe8\xaf\x95", request), "build UTF-8 request");
        Field list{}, command{}, buffer{}, body{}, nick{}, text{};
        require(Unique(request, 1, 2, list) && Unique(list.bytes, 2, 2, command) &&
            Unique(command.bytes, 2, 2, buffer) && Unique(buffer.bytes, 2, 2, body) &&
            Unique(body.bytes, 3, 2, nick) && Unique(nick.bytes, 1, 2, text), "request envelope");
        require(text.bytes == "\xe6\xb5\x8b\xe8\xaf\x95", "UTF-8 bytes preserved");
        require(body.bytes.find(unknown) != std::string_view::npos, "unknown fields preserved");
        require(Unique(body.bytes, 1, 0, text) && text.value == 0x1234, "account flags preserved");
        require(Accepted(Integer(1, 0) + Bytes(2, Integer(1, 1) + Bytes(2, std::string(1, '\0')))), "packed success");
        require(Accepted(Integer(1, 0) + Bytes(2, Integer(1, 1) + Integer(2, 0))), "unpacked success");
        require(!Accepted(Integer(1, 0)), "missing command result rejected");
        require(!Accepted(Integer(1, 0) + Bytes(2, Integer(1, 1) + Integer(2, 5))), "server rejection");
        require(!Accepted(Integer(1, 0) + Bytes(2, Integer(1, 2) + Bytes(2, std::string(2, '\0')))), "wrong result count");
        std::vector<Field> fields;
        require(!Parse(std::string("\x12\x80", 2), fields), "truncated varint");
        require(!Parse(std::string("\x12\x05x", 3), fields), "truncated length");
        require(!Request(info + Bytes(3, Bytes(1, "duplicate")), "new", request), "duplicate nickname rejected");
        std::cout << "Nickname protocol tests passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
