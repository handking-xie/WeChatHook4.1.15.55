#pragma once
#include "ProtobufWire.h"
#include "../3rdparty/md5/md5.hpp"
namespace avatar_wire {
inline std::string Chunk(const std::string& image,size_t offset,size_t count,const std::string& hash){
    using namespace protobuf_wire;
    if(image.empty()||offset>=image.size()||!count||count>image.size()-offset||hash.size()!=32)return {};
    return Integer(2,image.size())+Integer(3,offset)+Integer(4,1)+
        Bytes(5,Integer(1,count)+Bytes(2,std::string_view(image).substr(offset,count)))+Bytes(6,hash);
}
inline bool Url(std::string_view value){
    if(value.empty()||value.size()>4096)return false;
    if(value.substr(0,7)!="http://"&&value.substr(0,8)!="https://")return false;
    for(unsigned char c:value)if(c<=32||c>=127||c=='"'||c=='\''||c=='<'||c=='>'||c=='\\')return false;
    return true;
}
struct Progress{uint32_t total=0,next=0;std::string md5,big,small;};
inline bool Response(std::string_view wire,Progress& out,std::string& error){
    using namespace protobuf_wire;out={};error.clear();Field base{},ret{},total{},next{};
    if(!Unique(wire,1,2,base)||!Unique(base.bytes,1,0,ret)){error="invalid avatar BaseResponse";return false;}
    if(ret.value){error="server rejected avatar upload";return false;}
    if(!Unique(wire,2,0,total)||total.value>UINT32_MAX||!Unique(wire,3,0,next)||next.value>total.value){error="invalid avatar upload progress";return false;}
    Progress value;value.total=static_cast<uint32_t>(total.value);value.next=static_cast<uint32_t>(next.value);
    std::vector<Field> fields;if(!Parse(wire,fields))return false;bool seen[3]{};
    for(auto& field:fields)if(field.number>=4&&field.number<=6){
        auto index=field.number-4;if(seen[index]||field.type!=2){error="duplicate or invalid avatar result field";return false;}seen[index]=true;
        (index==0?value.md5:(index==1?value.big:value.small)).assign(field.bytes);
    }
    out=std::move(value);return true;
}
}
