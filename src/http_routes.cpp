#include "httplib.h"
#include "http_routes.h"
#include "SendTextMsg.h"
#include "GetSelfProfile.h"
#include "ForwardXMLMsg.h"
#include "SendImageMsg.h"
#include "QueryDB.h"
#include "Timeline.h"
#include "ContactsMgr.h"
#include "MsgExtended.h"
#include "FriendsMgr.h"
#include "ChatRoomExtended.h"
#include "SnsAndMisc.h"
#include "PublicAccountMgr.h"
#include "MiniAppMgr.h"
#include "FinderSearchMgr.h"

void RegisterRoutes(httplib::Server& svr)
{
    Route_SendTextMsg(svr);
    RegisterGetSelfProfile(svr);
    Route_ForwardXMLMsg(svr);
    Route_SendImageMsg(svr);
    Route_QueryDB(svr);
    Route_Timeline(svr);
    Route_ContactsMgr(svr);
    Route_MsgExtended(svr);
    Route_FriendsMgr(svr);
    Route_ChatRoomExtended(svr);
    Route_SnsAndMisc(svr);
    Route_PublicAccountMgr(svr);
    Route_MiniAppMgr(svr);
    Route_FinderSearchMgr(svr);
}
