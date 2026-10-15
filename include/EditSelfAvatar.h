#pragma once
#include "AvatarCgi.h"
#include "AvatarWire.h"
#include "AvatarFile.h"
#include "EditSelfNickname.h"
#include "SignatureWire.h"
#include "SelfProfileMutation.h"
#ifdef small
#undef small
#endif
namespace self_avatar {
struct Result {int ret=-1;bool submitted=false,uploaded=false,verified=false;size_t total=0,acknowledged=0;std::string message;};
// Compare the full resource, allowing only the observed HTTP/HTTPS scheme upgrade.
inline bool SameUrl(std::string_view a,std::string_view b){
 if(!avatar_wire::Url(a)||!avatar_wire::Url(b))return false;
 auto resource=[](std::string_view s){return s.substr(s.find("://")+3);};return resource(a)==resource(b);
}
inline bool Profile(self_nickname::Native& native,std::string_view wxid,std::string& info,std::string& nickname,std::string& big,std::string& smallUrl,std::string& error){
 std::string response;protobuf_wire::Field ext{},b{},s{};
 if(!native.Cgi(0x1009600,32,0x49A1C0,protobuf_wire::Bytes(2,wxid),response,error,nullptr,wxid)||
    !nickname_wire::UserInfo(response,wxid,info,nickname)||!protobuf_wire::Unique(response,3,2,ext)||
    !protobuf_wire::Unique(ext.bytes,9,2,b)||!protobuf_wire::Unique(ext.bytes,10,2,s)){
  if(error.empty())error="cannot read verified server avatar profile";return false;
 }
 big.assign(b.bytes);smallUrl.assign(s.bytes);return true;
}
inline Result Upload(const std::string& path){
 Result result;
 std::unique_lock<std::mutex> lock(SelfProfileMutationMutex(),std::try_to_lock);
 if(!lock.owns_lock()){result.message="another profile update request is running";return result;}
 try{
  std::string image,error;if(!avatar_file::Read(path,image,error)){result.message=error;return result;}
  const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Weixin.dll"));self_profile_memory::Profile account,current;
  if(!self_profile_memory::Read(GetCurrentProcess(),base,account,error)){result.message=error;return result;}
  self_nickname::Native native(base);std::string original,nickname,oldBig,oldSmall,signature;
  if(!Profile(native,account.wxid,original,nickname,oldBig,oldSmall,error)||!signature_wire::Value(original,signature)){result.message=error.empty()?"cannot read original signature":error;return result;}
  result.total=image.size();const auto hash=md5::md5_hash_hex(image);avatar_wire::Progress progress;
  const auto deadline=GetTickCount64()+120000;
  for(size_t offset=0;offset<image.size();){
   if(GetTickCount64()>deadline){result.message="avatar upload time budget exceeded; no automatic retry";return result;}
   const size_t count=(std::min)(size_t(30000),image.size()-offset);std::string response;bool submitted=false;
   bool completed=avatar_cgi::Send(base,157,"/cgi-bin/micromsg-bin/uploadhdheadimg",avatar_wire::Chunk(image,offset,count,hash),account.wxid,response,error,submitted);
   result.submitted=result.submitted||submitted;
   if(!completed||!avatar_wire::Response(response,progress,error)){result.message=error;return result;}
   if(progress.total!=image.size()||progress.next!=offset+count){result.message="server avatar progress does not match requested chunk";return result;}
   if(!progress.md5.empty()&&progress.md5!=hash){result.message="server avatar digest differs from uploaded file";return result;}
   offset=progress.next;result.acknowledged=offset;
  }
  result.uploaded=true;
  if(!avatar_wire::Url(progress.big)||!avatar_wire::Url(progress.smallUrl)){result.message="upload completed; final avatar URLs missing or invalid";return result;}
  for(int attempt=0;attempt<5;++attempt){
   if(attempt)Sleep(1000);
   std::string info,name,big,smallUrl,actualSignature;
   if(!Profile(native,account.wxid,info,name,big,smallUrl,error)||!signature_wire::Value(info,actualSignature))continue;
   if(name!=nickname||actualSignature!=signature){result.message="upload completed; preserved nickname or signature changed";return result;}
   if(!SameUrl(big,progress.big)||!SameUrl(smallUrl,progress.smallUrl))continue;
   if(!self_profile_memory::Read(GetCurrentProcess(),base,current,error)||current.wxid!=account.wxid){result.message="upload completed; account changed during verification";return result;}
   result.ret=0;result.verified=true;result.message="avatar uploaded and verified from server profile";return result;
  }
  result.message="upload completed; server profile avatar verification pending; do not automatically retry";
 }catch(const std::exception&){result.message="native avatar operation failed";}
 return result;
}
}
