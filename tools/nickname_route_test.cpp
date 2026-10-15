#define WIN32_LEAN_AND_MEAN
#include "../include/EditSelfNicknameRoute.h"
#include <iostream>
#include <thread>
int main(){
 httplib::Server server;std::string received;int calls=0;
 self_nickname::Result backend{0,true,true,"verified"};
 RegisterEditSelfNickname(server,[&](const std::string& value){++calls;received=value;return backend;});
 int port=server.bind_to_any_port("127.0.0.1");if(port<=0)return 2;
 std::thread listener([&]{server.listen_after_bind();});server.wait_until_ready();
 httplib::Client client("127.0.0.1",port);client.set_read_timeout(5);
 int failed=0;auto check=[&](bool ok){if(!ok)++failed;};
 auto post=[&](const std::string& body){auto r=client.Post("/EditSelfNickname",body,"application/json");if(!r)throw std::runtime_error("HTTP failed");return nlohmann::json::parse(r->body);};
 for(auto key:{"nickname","nick_name"}){
   auto j=post(nlohmann::json({{key,"\xe6\xb5\x8b\xe8\xaf\x95"}}).dump());
   check(j["ret"]==0&&j["submitted"]==true&&j["verified"]==true&&received=="\xe6\xb5\x8b\xe8\xaf\x95");
 }
 post("{\"nickname\":\"first\",\"nick_name\":\"second\"}");check(received=="first");
 int before=calls;
 for(auto bad:{"", "[", "[]", "null", "{}", "{\"nickname\":1}","{\"nickname\":\"\"}","{\"nickname\":\"a\\u0000b\"}","{\"nickname\":1,\"nick_name\":\"valid\"}"}){
   auto j=post(bad);check(j["ret"]==-1&&j["submitted"]==false&&j["verified"]==false);
 }
 check(post(nlohmann::json({{"nickname",std::string(4097,'a')}}).dump())["ret"]==-1);
 std::string invalid="{\"nickname\":\"";invalid+='\xff';invalid+="\"}";
 check(post(invalid)["ret"]==-1);check(calls==before);
 backend={-1,true,false,"native completion timed out; outcome unknown"};
 auto j=post("{\"nickname\":\"test\"}");check(j["ret"]==-1&&j["submitted"]==true&&j["verified"]==false);
 backend={0,false,true,"already matches"};j=post("{\"nickname\":\"test\"}");check(j["ret"]==0&&j["submitted"]==false&&j["verified"]==true);
 server.stop();listener.join();std::cout<<"Nickname production route tests: "<<(failed?"FAILED":"PASS")<<'\n';return failed?1:0;
}
