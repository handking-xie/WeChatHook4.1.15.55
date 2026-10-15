#define WIN32_LEAN_AND_MEAN
#include "EditSelfAvatarRoute.h"
#include <fstream>
#include <thread>
using json=nlohmann::json;
static void Report(const json& value){std::ofstream out("D:\\WeChatHook4.1.15.55\\.review\\avatar-production-result.json");out<<value.dump(2);}
extern "C" __declspec(dllexport) DWORD WINAPI RunProfile(void*){
 auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));self_profile_memory::Profile p;std::string error,response,info,name;bool submitted=false;
 bool ok=self_profile_memory::Read(GetCurrentProcess(),base,p,error)&&avatar_cgi::Send(base,302,"/cgi-bin/micromsg-bin/getprofile",protobuf_wire::Bytes(2,p.wxid),p.wxid,response,error,submitted)&&nickname_wire::UserInfo(response,p.wxid,info,name);
 Report({{"raw_scene_profile_verified",ok},{"submitted",submitted},{"error",error}});return ok?0:1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunUpload(void*){
 httplib::Server server;self_avatar::Register(server);int port=server.bind_to_any_port("127.0.0.1");if(port<1)return 1;
 std::thread listener([&]{server.listen_after_bind();});server.wait_until_ready();httplib::Client client("127.0.0.1",port);client.set_read_timeout(240);
 bool invalid=true;for(auto body:{"[","{}","{\"filepath\":1}","{\"filepath\":\"relative.jpg\"}","{\"filepath\":\"C:\\\\missing-avatar-12345.jpeg\"}"}){
  auto r=client.Post("/api/upload_head_img",body,"application/json");invalid=invalid&&r&&json::parse(r->body)["submitted"]==false;
 }
 bool locked=false;{std::lock_guard<std::mutex> lock(SelfProfileMutationMutex());auto r=client.Post("/api/upload_head_img",json({{"filepath","C:\\1.jpeg"}}).dump(),"application/json");locked=r&&json::parse(r->body)["submitted"]==false;}
 auto response=client.Post("/api/upload_head_img",json({{"filepath","C:\\1.jpeg"}}).dump(),"application/json");
 json result=response?json::parse(response->body):json({{"error","HTTP transport failed"}});result["invalid_inputs_no_submission"]=invalid;result["shared_lock"]=locked;
 server.stop();listener.join();Report(result);return response&&result.value("verified",false)&&invalid&&locked?0:1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
