#include "../include/SendTextWire.h"
#include <iostream>
#include <stdexcept>
using namespace protobuf_wire;
static void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main() {
    try {
        const auto request = send_text_wire::Request("filehelper", "hello \xe5\x8a\xa0\xe6\xb2\xb9", 1700000000, 123);
        Field count{}, item{}, target{}, content{}, source{};
        require(Unique(request, 1, 0, count) && count.value == 1 && Unique(request, 2, 2, item), "request count");
        require(Unique(item.bytes, 1, 2, target) && Unique(target.bytes, 1, 2, content) && content.bytes == "filehelper", "target");
        require(Unique(item.bytes, 2, 2, content) && content.bytes == "hello \xe5\x8a\xa0\xe6\xb2\xb9", "UTF-8 content");
        require(Unique(item.bytes, 6, 2, source) && source.bytes.empty(), "NoSrc source must be empty");
        const auto reply = Integer(1, 0) + Bytes(2, Bytes(1, "filehelper")) + Integer(4, 123) + Integer(8, 123456789012345ULL);
        const auto response = Bytes(1, Integer(1, 0)) + Integer(2, 1) + Bytes(3, reply);
        uint64_t messageId; int64_t ret;
        require(send_text_wire::Accepted(response, "filehelper", 123, messageId, ret) && messageId == 123456789012345ULL, "server ID");
        require(!send_text_wire::Accepted(response, "other", 123, messageId, ret), "wrong recipient");
        require(!send_text_wire::Accepted(response, "filehelper", 124, messageId, ret), "wrong client ID");
        require(!send_text_wire::Accepted(response + Bytes(3, reply), "filehelper", 123, messageId, ret), "duplicate result");
        require(!send_text_wire::Accepted(Bytes(1, Integer(1, uint64_t(-3))), "filehelper", 123, messageId, ret) && ret == -3, "base rejection");
        require(!send_text_wire::Accepted(Bytes(1, Integer(1, 0)) + Integer(2, 1) + Bytes(3, Integer(1, uint64_t(-4))), "filehelper", 123, messageId, ret) && ret == -4, "item rejection");
        require(!send_text_wire::Accepted(response.substr(0, response.size()-1), "filehelper", 123, messageId, ret), "truncated response");
        std::cout << "SendText protocol tests passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
