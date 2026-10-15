#include "EditSelfAvatarRoute.h"
#include <iostream>
#include <thread>
#define CHECK(x) do{if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}}while(0)
int main(){
 using namespace protobuf_wire;avatar_wire::Progress p;std::string error;
 auto response=Bytes(1,Integer(1,0))+Integer(2,9)+Integer(3,9)+Bytes(5,"http://wx.qlogo.cn/a/0")+Bytes(6,"http://wx.qlogo.cn/a/132");
 CHECK(avatar_wire::Response(response,p,error)&&p.total==9&&p.next==9&&p.md5.empty());
 CHECK(!avatar_wire::Response(response+Integer(3,9),p,error));
 CHECK(!avatar_wire::Response(Bytes(1,Integer(1,1))+Integer(2,9)+Integer(3,9),p,error));
 CHECK(!avatar_wire::Response(Bytes(1,Integer(1,0))+Integer(2,9)+Integer(3,10),p,error));
 CHECK(!avatar_wire::Response(response+Integer(5,1),p,error));
 auto wire=avatar_wire::Chunk("123456789",3,4,md5::md5_hash_hex("123456789"));Field f{},buffer{};
 CHECK(Unique(wire,2,0,f)&&f.value==9);CHECK(Unique(wire,3,0,f)&&f.value==3);
 CHECK(Unique(wire,5,2,buffer)&&Unique(buffer.bytes,1,0,f)&&f.value==4&&Unique(buffer.bytes,2,2,f)&&f.bytes=="4567");
 CHECK(avatar_wire::Chunk("abc",2,2,std::string(32,'a')).empty());
 CHECK(self_avatar::SameUrl("http://wx.qlogo.cn/a/0","https://wx.qlogo.cn/a/0"));
 CHECK(!self_avatar::SameUrl("https://wx.qlogo.cn/a/0","https://wx.qlogo.cn/b/0"));
 CHECK(!self_avatar::SameUrl("https://wx.qlogo.cn/a/0","https://evil.cn/a/0"));
 CHECK(!self_avatar::SameUrl("ftp://wx.qlogo.cn/a/0","https://wx.qlogo.cn/a/0"));
 std::string image;CHECK(!avatar_file::Read("relative.jpg",image,error));CHECK(!avatar_file::Read("C:\\missing-avatar-12345.jpeg",image,error));
 CHECK(avatar_file::Read("C:\\1.jpeg",image,error)&&image.size()==2619); // Explicitly supplied validation fixture; no upload.
 httplib::Server server;int calls=0;
 self_avatar::Register(server,[&](const std::string&){++calls;self_avatar::Result r;r.uploaded=true;r.submitted=true;r.message="pending";return r;});
 int port=server.bind_to_any_port("127.0.0.1");CHECK(port>0);std::thread listener([&]{server.listen_after_bind();});server.wait_until_ready();httplib::Client client("127.0.0.1",port);bool ok=true;
 for(auto body:{"[","[]","{}","{\"filepath\":1}"}){auto r=client.Post("/api/upload_head_img",body,"application/json");ok=ok&&r&&nlohmann::json::parse(r->body)["submitted"]==false;}
 auto r=client.Post("/api/upload_head_img","{\"filepath\":\"C:\\\\1.jpeg\"}","application/json");
 if(!r)ok=false;else{auto j=nlohmann::json::parse(r->body);ok=ok&&j["ret"]==-1&&j["uploaded"]==true&&j["verified"]==false;}
 server.stop();listener.join();CHECK(ok&&calls==1);
 {std::lock_guard<std::mutex> lock(SelfProfileMutationMutex());auto result=self_avatar::Upload("C:\\1.jpeg");CHECK(!result.submitted&&result.ret==-1);}
 std::cout<<"avatar wire, file decode, URL identity, HTTP status and shared lock passed\n";
}
