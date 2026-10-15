#pragma once

namespace httplib { class Server; }

// 注册朋友圈首页、下一页及指定好友相册 /GetFriendTimeline /mmsnsuserpage。
// 首页/下一页读取 sns.db；好友相册通过原生网络 CGI，必须传用户，支持 id 游标。
void Route_Timeline(httplib::Server& svr);
