#include "db_mgr.h"
#include "xwechat_offsets.h"
#include "SundaySearch.h"
#include "global.h"
#include "tools.h"

static void codec_get_key(sqlite3CodecGetKey func, sqlite3* db, int index, void** pKey, int* pLen) {
	__try {
		func(db, index, pKey, pLen);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		//这里输出异常调试信息 
	}
}

static int SafePrepare(sqlite3_api_routines* routines, void* db, const char* sql, sqlite3_stmt** outStmt)
{
	__try {
		if (!routines || !routines->prepare) return -1;
		return routines->prepare(db, sql, -1, outStmt, 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -999;
	}
}

static int SafeStep(sqlite3_api_routines* routines, sqlite3_stmt* stmt)
{
	__try {
		if (!routines || !routines->step) return -1;
		return routines->step(stmt);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -999;
	}
}

static int SafeFinalize(sqlite3_api_routines* routines, sqlite3_stmt* stmt)
{
	__try {
		if (!routines || !routines->finalize) return -1;
		return routines->finalize(stmt);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -999;
	}
}

static int SafeErrcode(sqlite3_api_routines* routines, void* db)
{
	__try {
		if (!routines || !routines->errcode) return -1;
		return routines->errcode(db);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -999;
	}
}

namespace xmgr {

	static std::string toHexString(const std::string& str)
	{
		static const char hex[] = "0123456789ABCDEF";
		std::string out;
		out.reserve(str.size() * 2);

		for (size_t i = 0; i < str.size(); ++i)
		{
			out.push_back(hex[(unsigned char)str[i] >> 4]);
			out.push_back(hex[(unsigned char)str[i] & 0x0F]);
		}
		return out;
	}

	DatabaseMgr::DatabaseMgr() {
		size_t module_base = (size_t)g_hWeixinDll;
		m_sqlite3Rountines = (sqlite3_api_routines*)(module_base + XWECHAT_SQLITE3_API_ROUTINES_OFFSET);
		m_sqlcipherRountines = (sqlcipher_api_routines*)(module_base + XWECHAT_SQLCIPHER_API_ROUTINES_OFFSET);
		m_codecGetKey = (sqlite3CodecGetKey)(module_base + XWECHAT_SQLITE3_CODEC_GET_KEY_FUNC);
		if (m_sqlite3Rountines != nullptr && m_sqlite3Rountines->backup_init != nullptr) {
			LPVOID patchAddress = (LPVOID)((size_t)m_sqlite3Rountines->backup_init + 0xAD);
			std::vector<BYTE> nopData(14, 0x90);
			m_backupAsmCode.resize(nopData.size(), 0);
			ReadProcessMemory(GetCurrentProcess(), patchAddress, m_backupAsmCode.data(), m_backupAsmCode.size(), nullptr);
			WriteProcessMemory(GetCurrentProcess(), patchAddress, (LPCVOID)nopData.data(), nopData.size(), 0);
		}
	}

	DatabaseMgr::~DatabaseMgr() {
		if (m_sqlite3Rountines != nullptr && m_sqlite3Rountines->backup_init != nullptr && m_backupAsmCode.size() != 0) {
			LPVOID patchAddress = (LPVOID)((size_t)m_sqlite3Rountines->backup_init + 0xAD);
			WriteProcessMemory(GetCurrentProcess(), patchAddress, (LPCVOID)m_backupAsmCode.data(), m_backupAsmCode.size(), 0);
			m_backupAsmCode.resize(0);
		}
	}

	nlohmann::ordered_json DatabaseMgr::execute(sqlite3* db, const std::string& sql)
	{
		nlohmann::ordered_json rdata = { {"status",0},{"desc",""} };
		if (db == nullptr || m_sqlite3Rountines == nullptr) {
			rdata["status"] = -1;
			rdata["desc"] = "input db handle or sqlite3 routines is nullptr";
			return rdata;
		}
		sqlite3_stmt* stmt = nullptr;
		int rc = SafePrepare(m_sqlite3Rountines, (LPVOID)db, sql.c_str(), &stmt);
		if (rc != SQLITE_OK || stmt == nullptr) {
			rdata["status"] = rc;
			rdata["desc"] = format_string("execute %s failed (code %d)", "sqlite3_prepare", rc);
			return rdata;
		}
		nlohmann::ordered_json items = nlohmann::ordered_json::array();
		while (SafeStep(m_sqlite3Rountines, stmt) == SQLITE_ROW)
		{
			int col_count = m_sqlite3Rountines->column_count(stmt);
			nlohmann::ordered_json item;
			for (int i = 0; i < col_count; i++)
			{
				const char* ColName = m_sqlite3Rountines->column_name(stmt, i);
				int nType = m_sqlite3Rountines->column_type(stmt, i);
				const void* pReadBlobData = m_sqlite3Rountines->column_blob(stmt, i);
				int nLength = m_sqlite3Rountines->column_bytes(stmt, i);
				std::string key = ColName ? ColName : "";
				std::string value;
				if (pReadBlobData && nLength > 0)
				{
					if (nType == SQLITE_BLOB)
					{
						value = toHexString(std::string((char*)pReadBlobData, nLength));
					}
					else
					{
						value = std::string((char*)pReadBlobData, nLength);
					}
				}
				item[key] = value;
			}
			items.push_back(item);
		}
		SafeFinalize(m_sqlite3Rountines, stmt);
		rdata["data"] = items;
		return rdata;
	}

	nlohmann::ordered_json DatabaseMgr::execute(const std::string& dbname, const std::string& sql)
	{
		nlohmann::ordered_json rdata = { {"status",0},{"desc",""} };
		LPVOID dbHandle = getDatabaseHandle(dbname);
		if (dbHandle == nullptr) {
			rdata["status"] = -1;
			rdata["desc"] = format_string("get database handle which named %s failed", dbname);
			return rdata;
		}
		rdata = execute((sqlite3*)dbHandle, sql);
		return rdata;
	}

	std::string DatabaseMgr::codec_get_key(const std::string& dbname) {
		LPVOID dbHandle = getDatabaseHandle(dbname);
		if (dbHandle == nullptr)
			return "";
		return codec_get_key((sqlite3*)dbHandle);
	}

	std::string DatabaseMgr::codec_get_key(sqlite3* db) {
		char* pKey = nullptr;
		int iLen = 0;
		std::string szKey;
		return szKey;
		if (m_codecGetKey == nullptr)
			return szKey;
		nlohmann::ordered_json queryResult = execute(db, std::string("PRAGMA cipher_store_pass"));
		if (queryResult["data"][0]["cipher_store_pass"] == 0) {
			return szKey;
		}
		::codec_get_key(m_codecGetKey, db, 0, (void**)&pKey, &iLen);
		if (pKey == nullptr)
			return szKey;
		szKey = std::string(pKey, iLen);
		return szKey;
	}

	void DatabaseMgr::backup_xprogress(int remaining, int pagecount) {
		//LL_DEBUG("backup process: %d/%d\n", pagecount - remaining, pagecount);
		//OutputDebugStringA("[backup process]\n");
	}

	int DatabaseMgr::backup(sqlite3* db, const std::string& out_path)
	{
		int rc = SQLITE_OK;
		if (db == nullptr) {
			return -1;
		}
		sqlite3* pNewDbHandle = nullptr;
		sqlite3_backup* pBackupHandle = nullptr;
		rc = m_sqlite3Rountines->open(out_path.c_str(), &pNewDbHandle);
		if (rc == SQLITE_OK) {
			pBackupHandle = m_sqlite3Rountines->backup_init(pNewDbHandle, (const char*)"main", db, (const char*)"main");
			if (pBackupHandle) {
				do {
					rc = m_sqlite3Rountines->backup_step(pBackupHandle, 5);
					backup_xprogress(
						m_sqlite3Rountines->backup_remaining(pBackupHandle),
						m_sqlite3Rountines->backup_pagecount(pBackupHandle)
					);
					if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
						m_sqlite3Rountines->sleep(50);
					}
				} while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);
				(void)m_sqlite3Rountines->backup_finish(pBackupHandle);
			}
			rc = m_sqlite3Rountines->errcode(pNewDbHandle);
		}
		(void)m_sqlite3Rountines->close(pNewDbHandle);
		return rc;
	}

	int DatabaseMgr::backup(const std::string& dbname, const std::string& out_path)
	{
		int rc = SQLITE_OK;
		sqlite3* pExistDbHandle = (sqlite3*)getDatabaseHandle(dbname);
		if (pExistDbHandle == nullptr) {
			return -1;
		}
		return backup(pExistDbHandle, out_path);
	}

	const std::map<std::string, LPVOID>& DatabaseMgr::searchDatabases() {
		std::lock_guard<std::mutex> lg(m_searchDbMtx);
		for (auto& it : m_dbs) {
			if (IsBadReadPtr(it.second, sizeof(LPVOID)) || SafeErrcode(m_sqlite3Rountines, it.second) == SQLITE_MISUSE) {
				m_dbs.clear();
				m_dbPaths.clear();
				break;
			}
		}
		if (m_dbs.size() > 0) {
			return m_dbs;
		}
		std::vector<LPVOID> results;
		size_t vfs_addr = (size_t)g_hWeixinDll + XWECHAT_SQLITE3_VFS_OFFSET;
		ScanPattern(GetCurrentProcess(), (BYTE*)&vfs_addr, sizeof(LPVOID), results);
		for (size_t i = 0; i < results.size(); i++) {
			auto result = results[i];
			if (IsBadReadPtr(result, sizeof(LPVOID)) == 0) {
				nlohmann::ordered_json queryResult = execute((sqlite3*)result, "PRAGMA database_list");
				if (queryResult.value("status", -1) == 0 && queryResult.contains("data") && queryResult["data"].is_array() && !queryResult["data"].empty()) {
					if (queryResult["data"][0].contains("file")) {
						std::string dbpath = queryResult["data"][0]["file"].get<std::string>();
						if (!dbpath.empty()) {
							auto pos = dbpath.find_last_of("\\");
							std::string dbname = dbpath.substr(pos + 1);
							m_dbs[dbname] = result;
							m_dbPaths[dbname] = dbpath;
							if (dbname == "contact.db" || dbname == "MicroMsg.db") {
								g_IsLogin = 1;
							}
						}
					}
				}
			}
		}
		return m_dbs;
	}

	LPVOID DatabaseMgr::getDatabaseHandle(const std::string& dbname)
	{
		auto& dbs = searchDatabases();
		if (m_dbs.find(dbname) == m_dbs.end())
			return nullptr;
		return m_dbs[dbname];
	}

	std::string DatabaseMgr::getDatabasePath(const std::string& dbname)
	{
		searchDatabases();
		auto it = m_dbPaths.find(dbname);
		if (it != m_dbPaths.end())
			return it->second;
		return "";
	}

	nlohmann::ordered_json DatabaseMgr::getDatabaseInfo() {
		nlohmann::ordered_json jdbs = nlohmann::ordered_json::array();
		m_dbs.clear();
		m_dbPaths.clear();
		auto& dbs = searchDatabases();
		for (auto& db : dbs) {
			nlohmann::json jdb = { {"dbName",db.first},{"dbHandle",(size_t)db.second} };
			jdbs.push_back(jdb);
		}
		return jdbs;
	}
}