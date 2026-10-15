#define WIN32_LEAN_AND_MEAN
#include "../include/FriendTimelineRoute.h"
#include <fstream>
#include <thread>
static void Report(const nlohmann::json& data) {
    std::ofstream file("D:\\WeChatHook\\.review\\friend-timeline-native-result.json", std::ios::trunc);
    file << data.dump(2);
}
extern "C" __declspec(dllexport) DWORD WINAPI RunCodec(void*) {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    profile_memory::Profile account; std::string error;
    if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) { Report({{"error", error}}); return 1; }
    native_cgi::Native native(base);
    const auto wire = friend_timeline_wire::Request(account.wxid, 0);
    void* object = native.Function<void*(*)(size_t)>(0x74AB89C)(friend_timeline_native::requestSize);
    native.Function<void*(*)(void*)>(friend_timeline_native::requestCtor)(object);
    native_cgi::NativeString input; input.size = wire.size();
    if (wire.size() <= 15) std::memcpy(input.storage, wire.data(), wire.size());
    else { input.capacity = wire.size() | 15; auto data = wire.data(); std::memcpy(input.storage, &data, sizeof(data)); }
    const bool parsed = native.Function<bool(*)(void*, native_cgi::NativeString*)>(0x681480)(object, &input);
    const bool matches = parsed && native.Serialize(object) == wire;
    const bool initialized = parsed && reinterpret_cast<bool(*)(void*)>((*reinterpret_cast<uintptr_t**>(object))[4])(object);
    native.Destroy(object);
    // Partial request lacks BaseRequest; scene preparation fills it for the network.
    Report({{"parsed", parsed}, {"roundtrip_matches", matches}, {"initialized_before_scene", initialized}});
    return parsed && matches && !initialized ? 0 : 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunProfile(void*) {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    profile_memory::Profile account; std::string error;
    if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) { Report({{"error", error}}); return 1; }
    std::string target = account.wxid;
    // Optional explicit read-only target, kept out of source and result logs.
    std::ifstream input("D:\\WeChatHook\\.review\\friend-timeline-probe-input.json");
    if (input) { nlohmann::json options; input >> options; target = options.at("wxid").get<std::string>(); }
    const auto result = friend_timeline_native::Fetch(target, "");
    Report({{"submitted", result.submitted}, {"verified", result.verified},
        {"business_ret", result.businessRet}, {"count", result.objects.size()}, {"error", result.error},
        {"visibility_limited", result.visibilityLimited}, {"server_message", result.serverMessage}});
    return result.verified ? 0 : 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunInspect(void*) {
    try {
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
        profile_memory::Profile account; std::string error;
        if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) { Report({{"error", error}}); return 1; }
        std::ifstream input("D:\\WeChatHook\\.review\\friend-timeline-probe-input.json");
        nlohmann::json options; input >> options;
        const auto target = options.at("wxid").get<std::string>();
        if (!friend_timeline_native::ValidUtf8(target) || target.size() > 4096) return 1;
        native_cgi::Native native(base); std::string response; bool submitted = false;
        if (!native.Cgi(friend_timeline_native::requestCtor, friend_timeline_native::requestSize,
            friend_timeline_native::submit, friend_timeline_wire::Request(target, 0), response, error, &submitted)) {
            Report({{"error", error}, {"submitted", submitted}}); return 1;
        }
        using namespace protobuf_wire;
        Field baseResponse{}, ret{}, count{}, text{}, message{}, buffer{};
        nlohmann::json report = {{"submitted", submitted}};
        if (Unique(response, 1, 2, baseResponse) && Unique(baseResponse.bytes, 1, 0, ret))
            report["business_ret"] = static_cast<int32_t>(static_cast<uint32_t>(ret.value));
        if (Unique(response, 3, 0, count)) report["object_count"] = count.value;
        if (Unique(response, 12, 2, text)) report["server_message"] = std::string(text.bytes);
        if (Unique(baseResponse.bytes, 2, 2, message) && Unique(message.bytes, 1, 2, buffer))
            report["base_message"] = std::string(buffer.bytes);
        std::vector<Field> fields; Parse(response, fields);
        size_t objects = 0; for (const auto& field : fields) if (field.number == 4) ++objects;
        report["objects_in_wire"] = objects;
        Report(report); return 0;
    } catch (const std::exception& e) { Report({{"error", e.what()}}); return 1; }
}
extern "C" __declspec(dllexport) DWORD WINAPI RunHttp(void*) {
    httplib::Server server; friend_timeline_route::Register(server);
    if (!server.bind_to_port("127.0.0.1", 30089)) return 1;
    std::thread stop([&] { Sleep(45000); server.stop(); });
    server.listen_after_bind(); stop.join(); return 0;
}
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
