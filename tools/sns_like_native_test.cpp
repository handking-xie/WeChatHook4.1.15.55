#define WIN32_LEAN_AND_MEAN
#include "../include/SnsLikeRoute.h"
#include <fstream>
static void Report(const nlohmann::json& result) { std::ofstream("D:\\WeChatHook\\.review\\sns-like-native-result.json") << result.dump(2); }
extern "C" __declspec(dllexport) DWORD WINAPI RunCodec(void*) {
 auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));
 profile_memory::Profile account; std::string error;
 if (!profile_memory::Read(GetCurrentProcess(), base, account, error)) { Report({{"error",error}}); return 1; }
 native_cgi::Native native(base);
 auto wire = sns_like_wire::Request(15027695064333161080ULL, account.wxid, "wxid_0xexx8mmtng912", 1);
 void* object = native.Function<void*(*)(size_t)>(0x74AB89C)(sns_like_native::requestSize);
 native.Function<void*(*)(void*)>(sns_like_native::requestCtor)(object);
 native_cgi::NativeString input; input.size = wire.size(); input.capacity = wire.size() | 15;
 auto data = wire.data(); std::memcpy(input.storage, &data, sizeof(data));
 bool parsed = native.Function<bool(*)(void*,native_cgi::NativeString*)>(0x681480)(object,&input);
 bool matches = parsed && native.Serialize(object) == wire;
 native.Destroy(object); Report({{"parsed",parsed},{"roundtrip_matches",matches}}); return parsed && matches ? 0 : 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunLike(void*) {
 auto result = sns_like_route::Handle("{\"sns_id\":\"15027695064333161080\",\"to_wxid\":\"wxid_0xexx8mmtng912\"}",sns_like_native::Like);
 Report(result); return result["verified"].get<bool>() ? 0 : 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID) { return TRUE; }

