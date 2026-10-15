#define WIN32_LEAN_AND_MEAN
#include "../include/SendTextNoSrcRoute.h"
#include <fstream>
#include <thread>
static void report(const nlohmann::json& data) {
    std::ofstream file("D:\\WeChatHook\\.review\\sendtext-native-result.json", std::ios::trunc);
    file << data.dump(2);
}
extern "C" __declspec(dllexport) DWORD WINAPI RunCodec(void*) {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    std::string error; profile_memory::Profile account;
    if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) { report({{"error", error}}); return 1; }
    native_cgi::Native native(base);
    auto wire = send_text_wire::Request("filehelper", "codec \xe5\x8a\xa0\xe6\xb2\xb9", 1700000000, 123);
    void* object = native.Function<void*(*)(size_t)>(0x74AB89C)(48);
    native.Function<void*(*)(void*)>(0x2E82760)(object);
    native_cgi::NativeString input; input.size = wire.size(); input.capacity = wire.size() | 15;
    auto data = wire.data(); std::memcpy(input.storage, &data, sizeof(data));
    const bool parsed = native.Function<bool(*)(void*, native_cgi::NativeString*)>(0x681480)(object, &input);
    const bool initialized = parsed && reinterpret_cast<bool(*)(void*)>((*reinterpret_cast<uintptr_t**>(object))[4])(object);
    const bool matches = parsed && native.Serialize(object) == wire;
    native.Destroy(object);
    report({{"parsed", parsed}, {"initialized", initialized}, {"roundtrip_matches", matches}});
    return parsed && initialized && matches ? 0 : 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunHttp(void*) {
    httplib::Server server; RegisterSendTextNoSrc(server);
    if (!server.bind_to_port("127.0.0.1", 30089)) return 1;
    std::thread stop([&] { Sleep(45000); server.stop(); });
    server.listen_after_bind(); stop.join(); return 0;
}
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
