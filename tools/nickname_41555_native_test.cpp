#define WIN32_LEAN_AND_MEAN
#include "../include/EditSelfNicknameRoute.h"
#include <wincrypt.h>
#include <fstream>
#include <thread>
using json=nlohmann::json;
constexpr const char* output="D:\\WeChatHook4.1.15.55\\.review\\nickname-41555-result.json";
constexpr const char* recovery="D:\\WeChatHook4.1.15.55\\.review\\nickname-recovery.dpapi";
static void Report(const json& value){std::ofstream out(output);out<<value.dump(2);}
static bool Account(self_profile_memory::Profile& p,std::string& error){
 return self_profile_memory::Read(GetCurrentProcess(),reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")),p,error);
}
static bool Backup(const std::string& wxid,const std::string& nickname){
 auto text=json({{"wxid",wxid},{"nickname",nickname}}).dump();
 DATA_BLOB input{static_cast<DWORD>(text.size()),reinterpret_cast<BYTE*>(text.data())},encrypted{};
 if(!CryptProtectData(&input,L"nickname restoration",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&encrypted))return false;
 std::ofstream out(recovery,std::ios::binary|std::ios::trunc);
 out.write(reinterpret_cast<char*>(encrypted.pbData),encrypted.cbData);out.close();LocalFree(encrypted.pbData);
 return bool(out);
}
extern "C" __declspec(dllexport) DWORD WINAPI RunCodec(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error)){Report({{"error",error}});return 1;}
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 const auto info=nickname_wire::Integer(1,1)+nickname_wire::Bytes(2,nickname_wire::Bytes(1,"wxid_codec"))+nickname_wire::Bytes(3,nickname_wire::Bytes(1,"original"));
 std::string oplog; if(!nickname_wire::Request(info,"test",oplog))return 1;
 bool success=true;
 for(int i=0;i<2;++i){
   auto wire=i?oplog:nickname_wire::Bytes(2,"wxid_codec");
   void* object=native.Function<void*(*)(size_t)>(0x754C41C)(i?24:32);
   native.Function<void*(*)(void*)>(i?0x3B1B040:0x1009600)(object);
   self_nickname::NativeString input; input.size=wire.size();
   if(wire.size()<=15)std::memcpy(input.storage,wire.data(),wire.size());
   else{input.capacity=wire.size()|15;auto bytes=wire.data();std::memcpy(input.storage,&bytes,sizeof(bytes));}
   bool parsed=native.Function<bool(*)(void*,self_nickname::NativeString*)>(0x697160)(object,&input);
   success=success&&parsed&&native.Serialize(object)==wire;native.Destroy(object);
 }
 Report({{"codec_roundtrips",success}});return success?0:1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunProfile(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error)){Report({{"error",error}});return 1;}
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 std::string info,nickname;bool ok=native.Profile(p.wxid,info,nickname,error);
 Report({{"profile",ok},{"error",error},{"nickname_length",nickname.size()},{"local_matches_server",ok&&nickname==p.nickname}});return ok?0:1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunHttp(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error))return 1;
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 std::string info,nickname;if(!native.Profile(p.wxid,info,nickname,error))return 1;
 httplib::Server server;RegisterEditSelfNickname(server);
 int port=server.bind_to_any_port("127.0.0.1");if(port<=0)return 1;
 std::thread listener([&]{server.listen_after_bind();});server.wait_until_ready();
 httplib::Client client("127.0.0.1",port);client.set_read_timeout(100);
 bool ok=true;
 for(auto key:{"nickname","nick_name"}){
   auto r=client.Post("/EditSelfNickname",json({{key,nickname}}).dump(),"application/json");
   if(!r){ok=false;continue;}auto j=json::parse(r->body);
   ok=ok&&j["ret"]==0&&j["verified"]==true&&j["submitted"]==false;
 }
 for(auto bad:{"[","[]","{}","{\"nickname\":1}","{\"nickname\":\"\"}","{\"nickname\":\"a\\u0000b\"}"}){
   auto r=client.Post("/EditSelfNickname",bad,"application/json");if(!r){ok=false;continue;}
   auto j=json::parse(r->body);ok=ok&&j["ret"]==-1&&j["submitted"]==false;
 }
 server.stop();listener.join();Report({{"production_http",ok},{"same_nickname_no_mutation",ok}});return ok?0:1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunEditRestore(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error)){Report({{"error",error}});return 1;}
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 std::string info,original;if(!native.Profile(p.wxid,info,original,error)){Report({{"error",error}});return 1;}
 if(GetFileAttributesA(recovery)!=INVALID_FILE_ATTRIBUTES){Report({{"error","existing recovery file must be resolved before another edit"}});return 1;}
 if(!Backup(p.wxid,original)){Report({{"error","cannot save encrypted restoration data"}});return 1;}
 Report({{"phase","editing"},{"encrypted_recovery_saved",true}});
 auto edited=self_nickname::Edit("test");
 Report({{"phase","restoring"},{"edit_ret",edited.ret},{"edit_verified",edited.verified},{"edit_submitted",edited.submitted}});
 self_profile_memory::Profile current;
 if(!Account(current,error)||current.wxid!=p.wxid){Report({{"error","account changed; restoration skipped, encrypted recovery retained"}});return 1;}
 auto restored=self_nickname::Edit(original);
 std::string confirmed,actual;bool checked=native.Profile(p.wxid,confirmed,actual,error)&&actual==original;
 bool success=edited.ret==0&&edited.verified&&restored.ret==0&&restored.verified&&checked;
 if(restored.ret==0&&restored.verified&&checked)DeleteFileA(recovery);
 Report({{"edit_ret",edited.ret},{"edit_submitted",edited.submitted},{"edit_verified",edited.verified},
  {"restore_ret",restored.ret},{"restore_verified",restored.verified},{"restore_submitted",restored.submitted},
  {"independent_restore_readback",checked},{"recovery_retained",GetFileAttributesA(recovery)!=INVALID_FILE_ATTRIBUTES}});
 return success?0:1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
