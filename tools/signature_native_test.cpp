#define WIN32_LEAN_AND_MEAN
#include "../include/EditSelfSignatureRoute.h"
#include "../include/EditSelfNicknameRoute.h"
#include <wincrypt.h>
#include <fstream>
#include <thread>
using json=nlohmann::json;
constexpr const char* recovery="D:\\WeChatHook4.1.15.55\\.review\\signature-recovery.dpapi";
static void Report(const json& data){std::ofstream out("D:\\WeChatHook4.1.15.55\\.review\\signature-result.json");out<<data.dump(2);}
static bool Account(self_profile_memory::Profile& p,std::string& error){return self_profile_memory::Read(GetCurrentProcess(),reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")),p,error);}
static bool Read(self_nickname::Native& native,const std::string& wxid,std::string& info,std::string& nickname,std::string& signature,std::string& error){
 return native.Profile(wxid,info,nickname,error)&&signature_wire::Value(info,signature)&&self_signature::Valid(signature);
}
static bool Backup(const std::string& wxid,const std::string& signature){
 auto text=json({{"wxid",wxid},{"signature",signature}}).dump();DATA_BLOB input{static_cast<DWORD>(text.size()),reinterpret_cast<BYTE*>(text.data())},encrypted{};
 if(!CryptProtectData(&input,L"signature restoration",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&encrypted))return false;
 std::ofstream out(recovery,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<char*>(encrypted.pbData),encrypted.cbData);out.close();LocalFree(encrypted.pbData);return bool(out);
}
extern "C" __declspec(dllexport) DWORD WINAPI RunCodec(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error)){Report({{"error",error}});return 1;}
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 auto info=nickname_wire::Integer(1,1)+nickname_wire::Bytes(2,nickname_wire::Bytes(1,"wxid_codec"))+nickname_wire::Bytes(3,nickname_wire::Bytes(1,"nickname"))+nickname_wire::Bytes(13,"original");
 std::string wire;if(!signature_wire::Request(info,"test",wire))return 1;
 void* object=native.Function<void*(*)(size_t)>(0x754C41C)(24);native.Function<void*(*)(void*)>(0x3B1B040)(object);
 self_nickname::NativeString input;input.size=wire.size();input.capacity=wire.size()|15;auto data=wire.data();std::memcpy(input.storage,&data,sizeof(data));
 bool parsed=native.Function<bool(*)(void*,self_nickname::NativeString*)>(0x697160)(object,&input);
 bool ok=parsed&&native.Serialize(object)==wire;native.Destroy(object);Report({{"signature_oplog_codec",ok}});return ok?0:1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunProfile(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error)){Report({{"error",error}});return 1;}
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 std::string info,nickname,signature;bool ok=Read(native,p.wxid,info,nickname,signature,error);
 Report({{"server_profile",ok},{"signature_length",signature.size()},{"error",error}});return ok?0:1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunHttp(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error))return 1;
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 std::string info,nickname,signature;if(!Read(native,p.wxid,info,nickname,signature,error))return 1;
 httplib::Server server;self_signature::Register(server);RegisterEditSelfNickname(server);
 int port=server.bind_to_any_port("127.0.0.1");if(port<=0)return 1;
 std::thread listener([&]{server.listen_after_bind();});server.wait_until_ready();httplib::Client client("127.0.0.1",port);client.set_read_timeout(100);
 bool ok=true;auto response=client.Post("/api/mod_self_nick_signature",json({{"newSignature",signature}}).dump(),"application/json");
 if(!response)ok=false;else{auto j=json::parse(response->body);ok=j["ret"]==0&&j["errCode"]==1&&j["verified"]==true&&j["submitted"]==false;}
 for(auto bad:{"[","[]","{}","{\"newSignature\":1}","{\"newSignature\":\"a\\u0000b\"}"}){
  auto r=client.Post("/api/mod_self_nick_signature",bad,"application/json");if(!r){ok=false;continue;}auto j=json::parse(r->body);ok=ok&&j["ret"]==-1&&j["submitted"]==false;
 }
 bool shared=true;
 {
  std::lock_guard<std::mutex> lock(SelfProfileMutationMutex());
  auto sr=client.Post("/api/mod_self_nick_signature",json({{"newSignature",signature}}).dump(),"application/json");
  auto nr=client.Post("/EditSelfNickname",json({{"nickname",nickname}}).dump(),"application/json");
  shared=sr&&nr;
  if(shared)for(auto body:{sr->body,nr->body}){auto j=json::parse(body);shared=shared&&j["ret"]==-1&&j["submitted"]==false;}
 }
 server.stop();listener.join();Report({{"production_http",ok},{"same_signature_no_mutation",ok},{"nickname_signature_shared_lock",shared}});return ok&&shared?0:1;
}
extern "C" __declspec(dllexport) DWORD WINAPI RunEditRestore(void*){
 self_profile_memory::Profile p;std::string error;if(!Account(p,error))return 1;
 self_nickname::Native native(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll")));
 std::string info,nickname,original;if(!Read(native,p.wxid,info,nickname,original,error))return 1;
 if(GetFileAttributesA(recovery)!=INVALID_FILE_ATTRIBUTES||!Backup(p.wxid,original)){Report({{"error","cannot create fresh encrypted restoration backup"}});return 1;}
 Report({{"phase","editing"}});auto edited=self_signature::Edit("test");
 Report({{"phase","restoring"},{"edit_verified",edited.verified},{"edit_submitted",edited.submitted}});
 self_profile_memory::Profile current;if(!Account(current,error)||current.wxid!=p.wxid){Report({{"error","account changed; recovery retained"}});return 1;}
 auto restored=self_signature::Edit(original);
 std::string confirmed,afterNickname,actual;bool readback=Read(native,p.wxid,confirmed,afterNickname,actual,error);
 bool originalConfirmed=readback&&actual==original;bool nicknamePreserved=readback&&afterNickname==nickname;
 bool success=edited.ret==0&&edited.verified&&restored.ret==0&&restored.verified&&originalConfirmed&&nicknamePreserved;
 if(success)DeleteFileA(recovery);
 Report({{"edit_ret",edited.ret},{"edit_submitted",edited.submitted},{"edit_verified",edited.verified},
 {"restore_ret",restored.ret},{"restore_submitted",restored.submitted},{"restore_verified",restored.verified},
 {"independent_restore_readback",originalConfirmed},{"nickname_preserved",nicknamePreserved},
 {"recovery_retained",GetFileAttributesA(recovery)!=INVALID_FILE_ATTRIBUTES}});return success?0:1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
