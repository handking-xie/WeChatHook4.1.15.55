# 个人信息九接口 — 4.1.15.55 状态

日期：2026-10-15。仅指当前项目，未完成全部九项。

| 顺序 | 路由 | 当前状态 |
|---|---|---|
| 1 | /GetSelfProfile | 复用现有新版读取并重新验证；四字段可读，五字段未定位，phone 语义有核对限制 |
| 2 | /getonlineinfo | 新增原生本机登录态实现；模拟、只读实时读取及临时 HTTP 测试通过 |
| 3 | /Busif2fgetqrcodev2 | 未实现；已移除固定通用收款链接，改为明确失败 |
| 4 | /GetSelfAddress | 未实现；已移除用个人资料拼接收货地址的路径 |
| 5 | /GetQRCode | 原生 CGI 168 已实现，native codec、真实网络及短时生产 HTTP 验证通过；完整 DLL 未部署 |
| 6 | /EditSelfNickname | 已适配新版原生 GetProfile/Oplog，真实改为 test 并恢复原昵称，服务端验证通过；完整 DLL 未部署 |
| 7 | /api/mod_self_nick_signature | 新版原生 field 13 + Oplog 更新，真实改为 test 并恢复原空签名，昵称未变；完整 DLL 未部署 |
| 8 | /api/upload_head_img | 原生 CGI157 分块上传；指定 C:\1.jpeg 真实上传及本人资料回读通过，昵称/签名未变；完整 DLL 未部署 |
| 9 | /api/get_config_path | 未定位真实配置目录 getter；新增明确失败路由 |

第 3、4、9 项的失败保护由 include/PersonalInfoAvailability.h 提供。
保护响应 ret=-1、implemented=false、submitted=false、verified=false，不调用原生修改操作。
兼容 /SetQRCode 同样明确失败，避免人工填入的链接被误认成当前账号二维码。
这些保护不是完成实现，不得计入实现数量。

第 2 项返回 scope=local_client_login，仅说明本机账号对象登录位；无法证明服务器连接或其它设备在线。
生产调用链为 RegisterGetSelfProfile → online_info::Register/Handle → login_status::Read。
具体构建、指针链和一致性限制沿用 is-login-status-4.1.15.55.md，未复用旧版操作函数偏移。

本轮 GetSelfProfile 与 getonlineinfo 均重新编译测试程序，模拟检查和短时本机 HTTP 通过。
重新枚举微信进程后以只读权限验证同一生产 Read/Register，登录位为 1，仅打印资料长度。
没有注入诊断 DLL，没有修改昵称、签名或头像，也没有发送消息或发起支付。
GetSelfProfile.cpp、MsgExtended.cpp 本轮新对象编译通过；Apifox JSON 与差异检查通过。
完整 Hook DLL 未构建、未替换部署。

## 尚缺的原生证据

REA MCP 已连接，但 Ghidra 未配置（GHIDRA_INSTALL_DIR 未设置，Java/反编译器不可用）；
本轮使用现有 .review/login_analysis.py 的 pefile/Capstone 做普通本机静态观察，不称为 REA Evidence。
已安装目标为 C:\Program Files\Tencent\Weixin\4.1.15.55\Weixin.dll。
字符串搜索定位 /cgi-bin/micromsg-bin/getqrcode 到 RVA 0x9299080，
micromsg.GetQRCodeRequest/Response 到 0x96D0398/0x96D04A8。
现有 batchrefs 的有限 RIP-relative MOV/LEA 搜索未找到引用；这不证明接口不存在。
字符串只能提供线索，尚不能证明请求构造、异步 scene、线程、回调 ABI、内存所有权和响应字段。
收款/地址的有限精确字符串搜索没有取得可用入口，同样不能据此认定客户端不支持。
昵称和签名需要核验新版 GetProfile/Oplog、保留未知字段并服务端回读；头像需要上传协议及完成结果；
配置目录应定位实际 getter，不能用 Hook DLL 目录、可执行文件目录或猜测用户目录替代。


收款二维码专项调查及当前缺口见 [Busif2fgetqrcodev2 定位记录](collection-qrcode-status-4.1.15.55.md)。
已核对响应字段 qrcode_url/name，并定位新版 TenPay 异步场景候选；目标业务仍未实现。

本人二维码已接入新版原生路径，详见 [GetQRCode 实现与验证](self-qrcode-status-4.1.15.55.md)。此前该项未定位的描述是历史调查边界，现已由专项实现记录更新。

昵称修改新版实现及真实修改/恢复验证见 [EditSelfNickname 4.1.15.55](edit-self-nickname-4.1.15.55.md)。前述旧版未适配描述为历史调查范围，现由本记录更新。

签名更新已适配新版并通过真实修改/清空恢复验证，详见 [签名实现记录](edit-self-signature-4.1.15.55.md)。历史缺口描述不代表当前签名状态。

头像接口已完成当前构建原生实现和指定图片真实上传验证，详见 [头像实现记录](edit-self-avatar-4.1.15.55.md)。前述无头像操作描述为初始调查记录，不是本轮头像验证结果。

