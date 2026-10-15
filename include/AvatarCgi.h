#pragma once
#include "SelfQrCgi.h"
#include "ProtobufWire.h"
#include <map>
#include <mutex>
#include <memory>
namespace avatar_cgi {
// Per-instance adapter for the verified 4.1.15.55 scene layout.
// The installed module's global vtables and executable code are never modified.
struct State {
    uintptr_t base;
    HANDLE event = CreateEventW(nullptr,TRUE,FALSE,nullptr);
    std::mutex mutex;
    std::string tail,response,error;
    int type=-1,code=-1;
    uintptr_t original=0, table[6]{};
    explicit State(uintptr_t b):base(b){if(!event)throw std::runtime_error("cannot create avatar completion event");}
    ~State(){CloseHandle(event);}
};
inline std::mutex& MapMutex(){static std::mutex value;return value;}
inline std::map<void*,std::shared_ptr<State>>& States(){static std::map<void*,std::shared_ptr<State>> value;return value;}
inline std::shared_ptr<State> Find(void* scene){std::lock_guard<std::mutex> lock(MapMutex());auto i=States().find(scene);return i==States().end()?nullptr:i->second;}
inline void* Destroy(void* scene,unsigned flags){
    auto s=Find(scene);if(!s)return nullptr;
    {std::lock_guard<std::mutex> lock(MapMutex());States().erase(scene);}
    *reinterpret_cast<uintptr_t*>(scene)=s->original;
    return reinterpret_cast<void*(*)(void*,unsigned)>(*reinterpret_cast<uintptr_t*>(s->original))(scene,flags);
}
inline bool Pack(void* scene,void* buffer,void* context,bool* result,void* mode) noexcept {
    auto s=Find(scene);if(!s)return false;
    try {
        self_qr_cgi::Native native(s->base);
        // Same base-request preparation and argument order as 0x49A6D0.
        bool prepared=native.Function<bool(*)(void*,void*,void*)>(0x49A810)(scene,mode,context);
        if(result)*result=prepared;
        if(!prepared)return false;
        void* baseRequest=*reinterpret_cast<void**>(static_cast<unsigned char*>(scene)+0xB0);
        if(!baseRequest)return false;
        auto wire=protobuf_wire::Bytes(1,native.Serialize(baseRequest))+s->tail;
        return native.Function<int(*)(void*,const char*,int)>(0x6979C0)(buffer,wire.data(),static_cast<int>(wire.size()))==static_cast<int>(wire.size());
    }catch(...){std::lock_guard<std::mutex> lock(s->mutex);s->error="avatar request serialization failed";return false;}
}
inline bool Parse(void* scene,void* buffer) noexcept {
    auto s=Find(scene);if(!s)return false;
    try {
        self_qr_cgi::Native native(s->base);
        int size=native.Function<int(*)(void*)>(0x697A30)(buffer);
        if(size<=0||size>4*1024*1024)return false;
        const char* bytes=native.Function<const char*(*)(void*,int)>(0x697A00)(buffer,0);
        if(!bytes)return false;
        std::lock_guard<std::mutex> lock(s->mutex);s->response.assign(bytes,static_cast<size_t>(size));return true;
    }catch(...){std::lock_guard<std::mutex> lock(s->mutex);s->error="avatar response copy failed";return false;}
}
struct Callback {
    const uintptr_t* table;
    std::shared_ptr<State> state;
    explicit Callback(std::shared_ptr<State> s):table(Table()),state(std::move(s)){}
    static Callback* Copy(Callback* self,void*){return new Callback(self->state);}
    static bool Invoke(Callback* self,void**,int* type,int* code)noexcept{
        auto s=self->state;std::lock_guard<std::mutex> lock(s->mutex);
        if(type&&code){s->type=*type;s->code=*code;}else s->error="missing avatar completion status";
        SetEvent(s->event);return true;
    }
    static const void* Type(Callback*){return nullptr;}
    static void Delete(Callback* self,bool free){if(free)delete self;else self->~Callback();}
    static void* Target(Callback*,const void*){return nullptr;}
    static const uintptr_t* Table(){static const uintptr_t value[]={reinterpret_cast<uintptr_t>(&Copy),reinterpret_cast<uintptr_t>(&Copy),reinterpret_cast<uintptr_t>(&Invoke),reinterpret_cast<uintptr_t>(&Type),reinterpret_cast<uintptr_t>(&Delete),reinterpret_cast<uintptr_t>(&Target)};return value;}
};
inline bool Send(uintptr_t base,uint32_t cgi,const std::string& uri,const std::string& tail,
                 std::string_view wxid,std::string& response,std::string& error,bool& submitted){
    submitted=false;response.clear();error.clear();
    self_profile_memory::Profile account;
    if(!self_profile_memory::Read(GetCurrentProcess(),base,account,error)||account.wxid!=wxid){if(error.empty())error="account changed before avatar request";return false;}
    // Only the investigated upload operation and the read-only transport diagnostic are admitted.
    if(!((cgi==157&&uri=="/cgi-bin/micromsg-bin/uploadhdheadimg")||
         (cgi==302&&uri=="/cgi-bin/micromsg-bin/getprofile"))){error="unsupported avatar scene operation";return false;}
    uintptr_t manager=0,service=0,serviceTable=0;
    if(!profile_memory::ReadValue(GetCurrentProcess(),base+0xBB92FC0,manager)||!manager||
       !profile_memory::ReadValue(GetCurrentProcess(),manager+0x2C8,service)||!service||
       !profile_memory::ReadValue(GetCurrentProcess(),service,serviceTable)||serviceTable!=base+0x8FCD388){error="native avatar service unavailable";return false;}
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&Pack),&pinned)){error="cannot retain avatar callback module";return false;}
    auto state=std::make_shared<State>(base);state->tail=tail;
    self_qr_cgi::Native native(base);
    alignas(16) unsigned char config[64]{};
    std::memcpy(config,reinterpret_cast<const void*>(base+0x8FD8670),16);
    std::memcpy(config+16,&cgi,sizeof(cgi));
    self_qr_cgi::NativeString text;text.size=uri.size();
    if(uri.size()<=15)std::memcpy(text.storage,uri.data(),uri.size());
    else{auto data=uri.data();text.capacity=uri.size()|15;std::memcpy(text.storage,&data,sizeof(data));}
    std::memcpy(config+24,&text,sizeof(text));
    void* scene=native.Function<void*(*)(size_t)>(0x754C41C)(0xF0);
    if(!scene){error="native avatar scene allocation failed";return false;}
    native.Function<void*(*)(void*,void*)>(0x49A500)(scene,config);
    state->original=*reinterpret_cast<uintptr_t*>(scene);
    if(state->original!=base+0x8FDC488){native.Destroy(scene);error="avatar scene layout mismatch";return false;}
    std::memcpy(state->table,reinterpret_cast<const void*>(state->original-8),sizeof(state->table));
    state->table[1]=reinterpret_cast<uintptr_t>(&Destroy);state->table[2]=reinterpret_cast<uintptr_t>(&Pack);state->table[3]=reinterpret_cast<uintptr_t>(&Parse);
    {std::lock_guard<std::mutex> lock(MapMutex());States()[scene]=state;}
    *reinterpret_cast<uintptr_t*>(scene)=reinterpret_cast<uintptr_t>(state->table+1);
    try{
        *reinterpret_cast<Callback**>(static_cast<unsigned char*>(scene)+0x98)=new Callback(state);
        if(!self_profile_memory::Read(GetCurrentProcess(),base,account,error)||account.wxid!=wxid){Destroy(scene,1);error="account changed before avatar submission";return false;}
        submitted=true;
        reinterpret_cast<void(*)(void*,void*)>(*reinterpret_cast<uintptr_t*>(serviceTable+0x28))(reinterpret_cast<void*>(service),scene);
        auto scheduler=native.Function<void*(*)()>(0x691E60)();
        native.Function<void(*)(void*,void*)>(0x692200)(scheduler,static_cast<unsigned char*>(scene)+0x18);
    }catch(...){if(!submitted)Destroy(scene,1);error="native avatar scene submission failed";return false;}
    if(WaitForSingleObject(state->event,30000)!=WAIT_OBJECT_0){error="avatar native completion timed out; outcome unknown";return false;}
    std::lock_guard<std::mutex> lock(state->mutex);
    if(state->type||state->code||!state->error.empty()||state->response.empty()){
        error="avatar native CGI failed: type="+std::to_string(state->type)+", code="+std::to_string(state->code)+"; "+state->error;return false;
    }
    response=state->response;return true;
}
}
