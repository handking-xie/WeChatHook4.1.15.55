#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

// 前向声明，不需要完整类型
class HttpServer;



extern int g_receive_type;
extern int g_StartPort;
extern int g_MsgSendPort;
extern HMODULE g_hWeixinDll;
extern HMODULE g_hWeixinExe;


extern HANDLE g_hLoginMonitor;
extern HANDLE g_hAfterLoginInit;

extern DWORD g_MainThreadId;
extern DWORD 进程PID;
extern DWORD 父进程PID;
extern HWND  g_WeixinMainHwnd;
extern volatile uint64_t g_IsLogin;   // 0=未登录 1=已登录
extern volatile uint64_t g_getprofile;

extern volatile bool g_LoginMonitorRunning;
extern std::string g_CallBack_Url;
extern CRITICAL_SECTION g_dbMgrCriticalSection;

extern std::wstring g_AppDataDir;
extern std::wstring g_DocumentDir;
extern std::wstring g_UsersDir;
extern HMODULE g_hModule;

extern uint64_t g_MyModuleBase;
extern uint64_t g_MyModuleSize;
extern uint64_t g_MyModuleEnd;


struct SelfInfo_t
{
    std::string wxid;
    std::string alias;
    std::string nickname;
    std::string phone;
    std::string email;

    uint64_t qq;
    std::string proiv;
    std::string area;
    std::string signinfo;


};
// 全局唯一实例
extern SelfInfo_t SelfInfo;

#define WX_ADDR(offset) ((void*)((uintptr_t)g_hWeixinDll + (offset)))
#define XWECHAT_MAIN_CLAZZ_OFFSET            ((void*)((uintptr_t)g_hWeixinDll + 0xA83AB20))

inline std::wstring g_MyDir;

inline HttpServer* g_httpServer = nullptr;

// 4.1.15.13: 0x229E9FF (4.1.10.27: 0x22D09E7)
inline constexpr uint64_t g_Patch_Revoke = 0x229E9FF;


constexpr size_t XWECHAT_SQLITE3_VFS_OFFSET = 0xB75C5B0;
constexpr size_t XWECHAT_SQLITE3_API_ROUTINES_OFFSET = 0x9891EF0;
constexpr size_t XWECHAT_SQLCIPHER_API_ROUTINES_OFFSET = 0x9892730;
constexpr size_t XWECHAT_SQLITE3_CODEC_GET_KEY_FUNC = 0x4EE64D0;	//可以废弃不用

namespace offset
{
    inline constexpr uint64_t dec_pic_call = 0x493E70;
    inline constexpr uint64_t create_param2 = 0xCE00; // 4.1.15.13 参数构造函数
    inline constexpr uint64_t send_message = 0x39EDFD0; // 4.1.15.13 真实 newsendmsg NetScene 入口
    inline constexpr uint64_t param1_vtable = 0x8D47218; // 4.1.15.13 参数1真实虚表

    inline constexpr uint64_t param2 = 0xBD12690; // 4.1.15.13 全局 ServiceManager 指针
    inline constexpr uint64_t param2_1 = 0x8D470D8; // 4.1.15.13 参数2子虚表1
    inline constexpr uint64_t param2_2 = 0x8D47188; // 4.1.15.13 参数2子虚表2
    inline constexpr uint64_t param2_3 = 0x8D47378; // 4.1.15.13 参数2子虚表3
    inline constexpr uintptr_t txt_message_ctr = 0x419E6A0; // 4.1.15.13 消息处理入口
    inline constexpr uintptr_t txt_message_vtbl = 0x8D47338; // 4.1.15.13 消息结构体虚表
    inline constexpr uint64_t img_msg_vtbl = 0x8D47408;
    inline constexpr uint64_t img_msg_vtb2 = 0x8D475B8;
}


//4.1.5.30  xml
namespace Offsets
{
    inline constexpr uintptr_t IMAGE_FIELD_VTABLE = 0x80D1098;          //ok  41930 ? 41923
    inline constexpr uintptr_t IMAGE_FIELD_VTABLE2 = 0x80D1128;         //ok
    inline constexpr uintptr_t IMAGE_DATA_VTABLE = 0x7415D28;
    inline constexpr uintptr_t IMAGE_DATA_VTABLE2 = 0x7415DB8;
    inline constexpr uintptr_t VIDEO_FIELD_VTABLE = 0x750C7F8;
    inline constexpr uintptr_t VIDEO_FIELD_VTABLE2 = 0x750C888;
    inline constexpr uintptr_t ANIMATION_FIELD_VTABLE = 0x750CC18;
    inline constexpr uintptr_t ANIMATION_FIELD_VTABLE2 = 0x750CCA8;
    inline constexpr uintptr_t MESSAGE_STRUCT_VTABLE = 0x76BD388;
    inline constexpr uintptr_t MESSAGE_STRUCT_VTABLE2 = 0x76BD338;
    inline constexpr uintptr_t MESSAGE_PARAM_VTABLE = 0x76BCF38;
    inline constexpr uintptr_t FORWARD_XML_CALL = 0x1CF3D20;
}



