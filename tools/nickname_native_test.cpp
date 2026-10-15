#define WIN32_LEAN_AND_MEAN
#include "../include/EditSelfNicknameRoute.h"
#include "../include/json.hpp"
#include <fstream>
#include <thread>
using json = nlohmann::json;
extern "C" __declspec(dllexport) DWORD WINAPI RunHttp(void*) {
    httplib::Server server;
    RegisterEditSelfNickname(server);
    if (!server.bind_to_port("127.0.0.1", 30089)) return 1;
    std::thread stop([&] { Sleep(45000); server.stop(); });
    server.listen_after_bind(); stop.join(); return 0;
}
static void report(const json& data) {
    std::ofstream file("D:\\WeChatHook\\.review\\nickname-native-result.json", std::ios::trunc);
    file << data.dump(2);
}
extern "C" __declspec(dllexport) DWORD WINAPI RunProfile(void*) {
    report({{"status", "reading"}});
    std::string error; profile_memory::Profile account;
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) {
        report({{"status", "failed"}, {"error", error}}); return 1;
    }
    try {
        self_nickname::Native native(base); std::string info, nickname;
        bool ok = native.Profile(account.wxid, info, nickname, error);
        report({{"status", ok ? "passed" : "failed"}, {"error", error},
            {"server_profile_matches_local", ok && nickname == account.nickname}});
        return ok ? 0 : 1;
    } catch (const std::exception& ex) { report({{"status", "failed"}, {"error", ex.what()}}); return 1; }
}
extern "C" __declspec(dllexport) DWORD WINAPI RunEditTest(void*) {
    report({{"status", "editing"}});
    auto result = self_nickname::Edit("test");
    report({{"ret", result.ret}, {"submitted", result.submitted}, {"verified", result.verified}, {"retmsg", result.message}});
    return result.ret == 0 ? 0 : 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunCodec(void*) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    self_nickname::Native native(base);
    std::string wire = nickname_wire::Bytes(2, "wxid_codec_test");
    void* message = native.Function<void*(*)(size_t)>(0x74AB89C)(32);
    native.Function<void*(*)(void*)>(0xFE5770)(message);
    report({{"status", "codec_constructed"}});
    self_nickname::NativeString input; input.size = wire.size(); input.capacity = wire.size() | 15;
    auto data = wire.data(); std::memcpy(input.storage, &data, sizeof(data));
    bool ok = native.Function<bool(*)(void*, self_nickname::NativeString*)>(0x681480)(message, &input);
    report({{"status", "codec_parsed"}, {"ok", ok}});
    if (ok) { auto output = native.Serialize(message); report({{"status", "codec_serialized"}, {"matches", output == wire}}); }
    native.Destroy(message);
    report({{"status", "codec_passed"}, {"ok", ok}});
    return ok ? 0 : 1;
}
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
