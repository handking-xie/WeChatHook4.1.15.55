#define WIN32_LEAN_AND_MEAN
#include "../include/SelfQrRoute.h"
#include <thread>
#include "../include/ProtobufWire.h"
#include "../include/json.hpp"
#include <fstream>
using json = nlohmann::json;
static void Report(const json& data) {
    std::ofstream out("D:\\WeChatHook4.1.15.55\\.review\\self-qr-native-result.json");
    out << data.dump(2);
}
extern "C" __declspec(dllexport) DWORD WINAPI RunCodec(void*) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    self_profile_memory::Profile account; std::string error;
    if (!self_profile_memory::Read(GetCurrentProcess(), base, account, error)) {
        Report({{"error", error}}); return 1;
    }
    self_qr_cgi::Native native(base);
    auto wire = self_qr_wire::Request("wxid_codec_diagnostic");
    void* object = native.Function<void*(*)(size_t)>(0x754C41C)(48);
    native.Function<void*(*)(void*)>(0x382BA90)(object);
    self_qr_cgi::NativeString input;
    input.size = wire.size(); input.capacity = wire.size() | 15;
    auto data = wire.data(); std::memcpy(input.storage, &data, sizeof(data));
    bool parsed = native.Function<bool(*)(void*, self_qr_cgi::NativeString*)>(0x697160)(object, &input);
    bool matches = parsed && native.Serialize(object) == wire;
    native.Destroy(object);
    Report({{"parsed", parsed}, {"roundtrip", matches}});
    return matches ? 0 : 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunProfile(void*) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
    self_profile_memory::Profile account; std::string error;
    if (!self_profile_memory::Read(GetCurrentProcess(), base, account, error)) {
        Report({{"error", error}}); return 1;
    }
    try {
        self_qr_cgi::Native native(base); std::string response; bool submitted = false;
        bool ok = native.Cgi(0x382BA90, 48, 0x1726630, self_qr_wire::Request(account.wxid), response, error, &submitted);
        json fields = json::array();
        std::vector<protobuf_wire::Field> parsed;
        if (ok && protobuf_wire::Parse(response, parsed)) for (auto& f : parsed) {
            json item={{"field",f.number},{"type",f.type},{"length",f.bytes.size()}};
            if (f.type==0) item["value"]=f.value;
            if (f.type==2) {
                item["wechat_url"] = f.bytes.find("https://u.wechat.com/")==0 ||
                    f.bytes.find("http://weixin.qq.com/r/")==0 || f.bytes.find("https://weixin.qq.com/r/")==0;
                std::vector<protobuf_wire::Field> nested;
                if (f.number<=2 && protobuf_wire::Parse(f.bytes,nested)) {
                    json ns=json::array(); for(auto& n:nested) {
                        json v={{"field",n.number},{"type",n.type},{"length",n.bytes.size()}};
                        if(n.type==0) v["value"]=n.value;
                        ns.push_back(v);
                    } item["nested"]=ns;
                }
            } fields.push_back(item);
        }
        Report({{"ok",ok},{"submitted",submitted},{"error",error},{"fields",fields}});
        return ok ? 0 : 1;
    } catch(const std::exception& ex) { Report({{"error",ex.what()}}); return 1; }
}
extern "C" __declspec(dllexport) DWORD WINAPI RunHttp(void*) {
    httplib::Server server; self_qr::Register(server);
    const int port = server.bind_to_any_port("127.0.0.1");
    if (port <= 0) return 1;
    std::thread listener([&] { server.listen_after_bind(); });
    server.wait_until_ready();
    httplib::Client client("127.0.0.1", port); client.set_read_timeout(40);
    auto valid = client.Post("/GetQRCode", "{}", "application/json");
    auto invalid = client.Post("/GetQRCode", "{\"QRCode_URL\":\"https://u.wechat.com/fake\"}", "application/json");
    server.stop(); listener.join();
    if (!valid || !invalid) { Report({{"http", false}}); return 1; }
    auto good = json::parse(valid->body), bad = json::parse(invalid->body);
    bool ok = good["ret"] == 0 && good["verified"] == true &&
        good.contains("QRCode_URL") && self_qr_wire::ValidUrl(good["QRCode_URL"].get<std::string>()) &&
        bad["ret"] == -1 && bad["submitted"] == false && !bad.contains("QRCode_URL");
    Report({{"http",ok},{"business_ret",good["ret"]},{"verified",good["verified"]},
        {"url_length",good.contains("QRCode_URL")?good["QRCode_URL"].get<std::string>().size():0},
        {"manual_url_rejected",bad["ret"] == -1}});
    return ok ? 0 : 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID) { return TRUE; }
