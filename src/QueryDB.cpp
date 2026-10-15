#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include "tools.h"
#include "global.h"
#include "db_mgr.h"

#include "QueryDB.h"

using json = nlohmann::json;

void Route_QueryDB(httplib::Server& svr)
{
    svr.Post("/QueryDB/execute", [](const httplib::Request& req, httplib::Response& res)
        {
            json reqJson;
            json resp;

            try
            {
                reqJson = json::parse(req.body);
            }
            catch (...)
            {
                resp["ret"] = -1;
                resp["msg"] = "invalid json";
                res.set_content(resp.dump(), "application/json");
                return;
            }

            xmgr::DatabaseMgr& g_dbMgr = xmgr::DatabaseMgr::getInstance();

            std::string optDbName = reqJson.value("optDbName", "");
            if (optDbName.empty()) optDbName = reqJson.value("db_name", "");
            if (optDbName.empty()) optDbName = reqJson.value("dbname", "");

            std::string sql = reqJson.value("SQL", "");
            if (sql.empty()) sql = reqJson.value("sql", "");

            if (optDbName.empty() || sql.empty())
            {
                resp["ret"] = -1;
                resp["msg"] = "optDbName and SQL are required";
                res.set_content(resp.dump(), "application/json");
                return;
            }

            auto result = g_dbMgr.execute(optDbName, sql);
            res.set_content(result.dump(4, ' ', false), "application/json");
        });

    svr.Post("/QueryDB/GetAllDBName", [](const httplib::Request& req, httplib::Response& res)
        {
            xmgr::DatabaseMgr& g_dbMgr = xmgr::DatabaseMgr::getInstance();
            auto dbInfo = g_dbMgr.getDatabaseInfo();
            res.set_content(dbInfo.dump(4, ' ', false), "application/json");
        });

    svr.Get("/QueryDB/status", [](const httplib::Request&, httplib::Response& res) {
        json resp;
        resp["IsLogin"] = g_IsLogin;
        resp["hWeixin"] = (uint64_t)g_hWeixinDll;
        res.set_content(resp.dump(4, ' ', false), "application/json");
    });
}
