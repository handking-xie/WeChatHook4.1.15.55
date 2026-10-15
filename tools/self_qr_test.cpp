#define WIN32_LEAN_AND_MEAN
#include "../include/SelfQrRoute.h"
#include <iostream>
#include <thread>
int main() {
    using namespace protobuf_wire;
    const std::string url = "https://u.wechat.com/Mtest_valid";
    const std::string base = Bytes(1, Integer(1, 0));
    std::string value, error;
    int failed=0;
    auto check=[&](bool condition,const char* name){ if(!condition){++failed;std::cerr<<"FAIL "<<name<<'\n';} };
    const auto good=base+Bytes(9,url);
    check(self_qr_wire::Response(good,value,error)&&value==url,"valid response");
    check(self_qr_wire::Response(good+Integer(30,5),value,error),"unknown fields retained");
    for(auto bad : {Bytes(9,url),Bytes(1,"")+Bytes(9,url),Bytes(1,Integer(1,1))+Bytes(9,url),
        base+Bytes(9,url)+Bytes(9,url),base+Integer(9,0),base,base+Bytes(9,"https://evil.example/x"),
        base+Bytes(9,"https://u.wechat.com/"),base+Bytes(9,url+"\n"),base+Bytes(9,std::string("https://u.wechat.com/a\0b",24)),
        good.substr(0,good.size()-1),base+Bytes(9,std::string(5000,'a')),
        base+Bytes(1,Integer(1,0))+Bytes(9,url),Bytes(1,Integer(1,0)+Integer(1,0))+Bytes(9,url)}) {
        value="stale"; check(!self_qr_wire::Response(bad,value,error)&&value.empty(),"invalid response clears output");
    }
    Field username{},text{},opcode{};
    auto request=self_qr_wire::Request("wxid_test");
    check(Unique(request,2,2,username)&&Unique(username.bytes,1,2,text)&&text.bytes=="wxid_test","native username wrapper");
    check(Unique(request,3,0,opcode)&&opcode.value==0,"read opcode");
    httplib::Server server; int calls=0;
    self_qr::Result backend{0,true,true,"success",url};
    self_qr::Register(server,[&]{++calls; return backend;});
    int port=server.bind_to_any_port("127.0.0.1");
    if(port<=0)return 2;
    std::thread listener([&]{server.listen_after_bind();});server.wait_until_ready();
    httplib::Client client("127.0.0.1",port);client.set_read_timeout(5);
    auto post=[&](const std::string& body){
        auto response=client.Post("/GetQRCode",body,"application/json");
        if(!response)throw std::runtime_error("HTTP failed");
        return nlohmann::json::parse(response->body);
    };
    auto result=post("{}");check(result["ret"]==0&&result["QRCode_URL"]==url&&result["qr_url"]==url,"production success route");
    check(post("")["ret"]==0,"empty body");
    int before=calls;
    for(auto bad : {"[", "[]", "null", "true", "1", "{\"QRCode_URL\":\"https://u.wechat.com/fake\"}","{\"opcode\":1}","{\"wxid\":\"other\"}"}) {
        result=post(bad);check(result["ret"]==-1&&!result.contains("QRCode_URL")&&result["submitted"]==false,"invalid request rejected");
    }
    check(calls==before,"invalid parameters do not submit");
    backend={-1,true,false,"native CGI completion timed out; outcome is unknown",url};
    result=post("{}");check(result["ret"]==-1&&result["submitted"]==true&&!result.contains("QRCode_URL"),"timeout never exposes URL");
    backend={0,true,false,"",url};check(post("{}")["ret"]==-1,"unverified response rejected");
    backend={0,true,true,"success","https://evil.example/x"};check(post("{}")["ret"]==-1,"invalid backend URL rejected");
    server.stop();listener.join();
    std::cout<<"Self QR wire and production HTTP checks: "<<(failed?"FAILED":"PASS")<<'\n';
    return failed?1:0;
}
