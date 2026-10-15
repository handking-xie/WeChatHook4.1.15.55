#define WIN32_LEAN_AND_MEAN
#include "../include/EditSelfSignatureRoute.h"
#include <iostream>
#include <thread>
int main(){
 using namespace protobuf_wire;
 int failures=0;auto check=[&](bool ok,const char* name){if(!ok){++failures;std::cerr<<name<<'\n';}};
 auto info=Integer(1,0xffffffff)+Bytes(2,Bytes(1,"wxid_test"))+Bytes(3,Bytes(1,"nickname"))+Bytes(13,"original")+Bytes(99,"unknown");
 std::string value,request;check(signature_wire::Value(info,value)&&value=="original","read direct string");
 for(auto target:{std::string("test"),std::string(""),std::string("\xe6\xb5\x8b\xe8\xaf\x95")}){
  check(signature_wire::Request(info,target,request),"encode update");
  Field list{},command{},buffer{},body{},length{},signature{};
  check(Unique(request,1,2,list)&&Unique(list.bytes,2,2,command)&&Unique(command.bytes,2,2,buffer)&&
    Unique(buffer.bytes,2,2,body)&&Unique(buffer.bytes,1,0,length)&&length.value==body.bytes.size()&&
    Unique(body.bytes,13,2,signature)&&signature.bytes==target,"oplog envelope and signature");
  check(body.bytes==Integer(1,0xffffffff)+Bytes(2,Bytes(1,"wxid_test"))+Bytes(3,Bytes(1,"nickname"))+Bytes(13,target)+Bytes(99,"unknown"),"preserve other bytes");
 }
 auto absent=Integer(1,1)+Bytes(2,Bytes(1,"wxid_test"))+Bytes(3,Bytes(1,"nickname"));
 check(signature_wire::Value(absent,value)&&value.empty(),"absent optional string defaults empty");
 check(signature_wire::Request(absent,"test",request),"insert absent signature");
 check(!signature_wire::Value(info+Bytes(13,"duplicate"),value),"reject duplicate");
 check(!signature_wire::Request(info+Bytes(13,"duplicate"),"x",request)&&request.empty(),"duplicate clears request");
 check(!signature_wire::Value(Integer(13,1),value),"reject wrong signature type");
 check(!signature_wire::Request(std::string("\x6a\x80",2),"x",request),"reject truncated PB");
 httplib::Server server;int calls=0;std::string received;self_signature::Result backend{0,true,true,"verified"};
 self_signature::Register(server,[&](const std::string& v){++calls;received=v;return backend;});
 int port=server.bind_to_any_port("127.0.0.1");if(port<=0)return 2;
 std::thread listener([&]{server.listen_after_bind();});server.wait_until_ready();
 httplib::Client client("127.0.0.1",port);client.set_read_timeout(5);
 auto post=[&](const std::string& body){auto r=client.Post("/api/mod_self_nick_signature",body,"application/json");if(!r)throw std::runtime_error("HTTP failed");return nlohmann::json::parse(r->body);};
 auto good=post("{\"newSignature\":\"test\"}");check(good["ret"]==0&&good["errCode"]==1&&good["verified"]==true&&received=="test","production success");
 check(post("{\"newSignature\":\"\"}")["ret"]==0&&received.empty(),"empty clear request");
 int before=calls;
 for(auto bad:{"","[","[]","null","{}","{\"newSignature\":1}","{\"newSignature\":null}","{\"newSignature\":\"a\\u0000b\"}"}){
  auto j=post(bad);check(j["ret"]==-1&&j["errCode"]==-1&&j["submitted"]==false,"bad request");
 }
 check(post(nlohmann::json({{"newSignature",std::string(4097,'x')}}).dump())["ret"]==-1,"length limit");
 std::string bad="{\"newSignature\":\"";bad+='\xff';bad+="\"}";check(post(bad)["ret"]==-1,"invalid UTF8");check(calls==before,"invalid input never submits");
 backend={-1,true,false,"timeout; outcome unknown"};auto j=post("{\"newSignature\":\"test\"}");check(j["ret"]==-1&&j["submitted"]==true&&!j["verified"].get<bool>(),"timeout flags");
 backend={0,false,true,"already matches"};j=post("{\"newSignature\":\"test\"}");check(j["errCode"]==1&&j["submitted"]==false,"same signature");
 backend={0,true,false,"unverified"};check(post("{\"newSignature\":\"test\"}")["ret"]==-1,"no false success");
 server.stop();listener.join();
 std::cout<<"Signature wire and production HTTP tests: "<<(failures?"FAILED":"PASS")<<'\n';return failures?1:0;
}
