#define WIN32_LEAN_AND_MEAN
#include "../include/SelfProfileRoute.h"
#include <thread>
#include <iostream>
#include <cstdlib>


template<class Reader>
static nlohmann::json HttpCheck(Reader reader) {
    httplib::Server server;
    self_profile_memory::Register(server, reader);
    int port = server.bind_to_any_port("127.0.0.1");
    if (port <= 0) throw std::runtime_error("cannot bind diagnostic server");
    std::thread listener([&] { server.listen_after_bind(); });
    server.wait_until_ready();
    httplib::Client client("127.0.0.1", port);
    client.set_read_timeout(5);
    auto response = client.Post("/GetSelfProfile", "{}", "application/json");
    server.stop(); listener.join();
    if (!response || response->status != 200) throw std::runtime_error("HTTP diagnostic failed");
    return nlohmann::json::parse(response->body);
}

int main(int argc, char** argv) {
    using namespace self_profile_memory;
    if (argc == 3) {
        HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE,
            static_cast<DWORD>(std::strtoul(argv[1], nullptr, 10)));
        if (!process) return 2;
        Profile profile;
        std::string error;
        bool ok = Read(process, std::strtoull(argv[2], nullptr, 0), profile, error);
        auto response = HttpCheck([&](Profile& value, std::string& err) {
            return Read(process, std::strtoull(argv[2], nullptr, 0), value, err);
        });
        CloseHandle(process);
        if (ok && (response["ret"] != 0 || response["wxid"] != profile.wxid ||
            response["alias"] != profile.alias || response["nickname"] != profile.nickname ||
            response["phone"] != profile.phone)) return 1;
        std::cout << "Production HTTP route: " << (ok ? "PASS" : "FAILED") << '\n';
        if (!ok) { std::cerr << error << '\n'; return 1; }
        std::cout << "Native reader success; byte lengths: wxid=" << profile.wxid.size()
            << " alias=" << profile.alias.size() << " nickname=" << profile.nickname.size()
            << " phone=" << profile.phone.size() << '\n';
        return 0;
    }
    // Sparse synthetic image exercises the exact production reader.
    auto image = static_cast<unsigned char*>(VirtualAlloc(nullptr, image_size, MEM_RESERVE, PAGE_READWRITE));
    if (!image) return 2;
    VirtualAlloc(image, 4096, MEM_COMMIT, PAGE_READWRITE);
    VirtualAlloc(image + (manager_rva & ~uintptr_t(4095)), 4096, MEM_COMMIT, PAGE_READWRITE);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 128;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image + 128);
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    nt->FileHeader.TimeDateStamp = image_timestamp;
    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    nt->OptionalHeader.SizeOfImage = image_size;
    alignas(8) unsigned char manager[128]{}, account[256]{};
    uintptr_t base = reinterpret_cast<uintptr_t>(image);
    *reinterpret_cast<uintptr_t*>(image + manager_rva) = reinterpret_cast<uintptr_t>(manager);
    *reinterpret_cast<uintptr_t*>(manager + account_member) = reinterpret_cast<uintptr_t>(account);
    *reinterpret_cast<uintptr_t*>(account) = base + account_vtable;
    account[0x38] = 1;
    auto set = [&](size_t off, const char* value) {
        size_t length = std::strlen(value);
        std::memset(account + off, 0, 32);
        if (length <= 15) std::memcpy(account + off, value, length);
        else *reinterpret_cast<const char**>(account + off) = value;
        *reinterpret_cast<uint64_t*>(account + off + 16) = length;
        *reinterpret_cast<uint64_t*>(account + off + 24) = length <= 15 ? 15 : 31;
    };
    set(0x48, "wxid_long_test_value"); set(0x68, "alias");
    set(0x88, "\xe6\xb5\x8b\xe8\xaf\x95"); set(0xA8, "12345678901");
    Profile out; std::string error; int failures = 0;
    auto check = [&](bool expected, const char* name) {
        bool actual = Read(GetCurrentProcess(), base, out, error);
        bool cleared = actual || (out.wxid.empty() && out.phone.empty());
        if (actual != expected || !cleared) { ++failures; std::cerr << "FAIL " << name << '\n'; }
    };
    check(true, "valid SSO and heap strings");
    if (out.nickname != "\xe6\xb5\x8b\xe8\xaf\x95" || out.wxid != "wxid_long_test_value") ++failures;
    account[0x38] = 0; check(false, "logged out"); account[0x38] = 1;
    ++nt->FileHeader.TimeDateStamp; check(false, "wrong build"); --nt->FileHeader.TimeDateStamp;
    *reinterpret_cast<uintptr_t*>(account) = 0; check(false, "wrong vtable");
    *reinterpret_cast<uintptr_t*>(account) = base + account_vtable;
    set(0x88, "\xff"); check(false, "invalid UTF-8"); set(0x88, "valid");
    *reinterpret_cast<uint64_t*>(account + 0x58) = 5000; check(false, "oversized string");
    set(0x48, "wxid_long_test_value");
    *reinterpret_cast<uintptr_t*>(account + 0x48) = 1; check(false, "unreadable heap string");
    set(0x48, ""); check(false, "empty account");
    *reinterpret_cast<uintptr_t*>(manager + account_member) = 0; check(false, "null account");
    *reinterpret_cast<uintptr_t*>(manager + account_member) = reinterpret_cast<uintptr_t>(account);
    set(0x48, "wxid_long_test_value");
    auto reader = [&](Profile& value, std::string& err) {
        return Read(GetCurrentProcess(), base, value, err);
    };
    auto response = HttpCheck(reader);
    if (response["ret"] != 0 || response["wxid"] != "wxid_long_test_value" ||
        response["phone"] != "12345678901" || response["unavailable_fields"].size() != 5) ++failures;
    account[0x38] = 0;
    response = HttpCheck(reader);
    if (response["ret"] != -1 || response.contains("wxid") || response.contains("phone")) ++failures;
    account[0x38] = 1;
    ++nt->FileHeader.TimeDateStamp;
    response = HttpCheck(reader);
    if (response["ret"] != -1 || response.contains("wxid")) ++failures;
    VirtualFree(image, 0, MEM_RELEASE);
    std::cout << "Native reader synthetic checks: " << (failures ? "FAILED" : "PASS") << '\n';
    return failures ? 1 : 0;
}

