// CPersistentDataStorage.cpp
#include "CPersistentDataStorage.h"

#include <iomanip>
#include <iostream>

#include "../database/querybuilder.h"
#include "../database/DBSchema.h"
#include "spdlog/spdlog.h"

// SQLiteRow implementation
SQLiteRow::SQLiteRow(int argc, char** argv, char** colNames) 
    : values(argv), columnCount(argc) {
    for (int i = 0; i < argc; ++i) {
        columnIndexMap[colNames[i]] = i;
    }
}

std::string SQLiteRow::getString(const std::string& columnName, const std::string& defaultValue) const {
    auto it = columnIndexMap.find(columnName);
    if (it == columnIndexMap.end() || it->second >= columnCount || !values[it->second]) {
        return defaultValue;
    }
    return std::string(values[it->second]);
}

int SQLiteRow::getInt(const std::string& columnName, int defaultValue) const {
    auto it = columnIndexMap.find(columnName);
    if (it == columnIndexMap.end() || it->second >= columnCount || !values[it->second]) {
        return defaultValue;
    }
    try {
        return std::stoi(values[it->second]);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

bool SQLiteRow::getBool(const std::string& columnName, bool defaultValue) const {
    auto it = columnIndexMap.find(columnName);
    if (it == columnIndexMap.end() || it->second >= columnCount || !values[it->second]) {
        return defaultValue;
    }
    try {
        return std::stoi(values[it->second]) != 0;
    } catch (const std::exception&) {
        return defaultValue;
    }
}

bool SQLiteRow::hasColumn(const std::string& columnName) const {
    return columnIndexMap.find(columnName) != columnIndexMap.end();
}

bool SQLiteRow::isNull(const std::string& columnName) const {
    auto it = columnIndexMap.find(columnName);
    return it == columnIndexMap.end() || it->second >= columnCount || !values[it->second];
}

CPersistentDataStorage::CPersistentDataStorage() : db(nullptr) {}
bool CPersistentDataStorage::loadDatabase() {
    if (sqlite3_open("data/bmXL.db", &db) != SQLITE_OK) {
        spdlog::error("Failed to open database: {}", sqlite3_errmsg(db));
        return false;
    }
    
    // Enable foreign key constraints
    char* errMsg = nullptr;
    if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        spdlog::error("Failed to enable foreign keys: {}", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    
    // Begin transaction for table creation
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        spdlog::error("Failed to begin transaction: {}", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    
    std::vector<std::pair<std::string, std::string>> queries = {
        {"servers", R"(
            CREATE TABLE IF NOT EXISTS servers (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                host VARCHAR(128) NOT NULL,
                port INTEGER NOT NULL,
                name VARCHAR(64) NOT NULL,
                gamemode VARCHAR(64) NOT NULL,
                rule VARCHAR(64) NOT NULL,
                language VARCHAR(64) NOT NULL,
                players INTEGER DEFAULT 0,
                max_players INTEGER DEFAULT 0,
                ping INTEGER DEFAULT 0,
                last_update TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(host, port)
            ))"},

        {"bots", R"(
            CREATE TABLE IF NOT EXISTS bots (
                uuid TEXT PRIMARY KEY,
                name VARCHAR(30) NOT NULL,
                server_id INTEGER NOT NULL,
                invulnerable BOOLEAN NOT NULL DEFAULT 0,
                system_prompt TEXT DEFAULT '',
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (server_id) REFERENCES servers(id) ON DELETE CASCADE
            ))"},

        {"llm_providers", R"(
            CREATE TABLE IF NOT EXISTS llm_providers (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name VARCHAR(128) NOT NULL UNIQUE,
                api_key TEXT NOT NULL,
                base_url VARCHAR(255) NOT NULL,
                model VARCHAR(128) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            ))"},

        {"llm_sessions", R"(
            CREATE TABLE IF NOT EXISTS llm_sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                session_id VARCHAR(128) NOT NULL UNIQUE,
                bot_uuid TEXT NOT NULL,
                provider_id INTEGER NOT NULL,
                is_active BOOLEAN NOT NULL DEFAULT 1,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (bot_uuid) REFERENCES bots(uuid) ON DELETE CASCADE,
                FOREIGN KEY (provider_id) REFERENCES llm_providers(id) ON DELETE RESTRICT
            ))"}
    };
    for (const auto& [table_name, query] : queries) {
        char* errMsg = nullptr;
        if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            spdlog::error("Failed to create table '{}': {}", table_name, errMsg);
            sqlite3_free(errMsg);
            
            // Rollback transaction on error
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
        spdlog::debug("Successfully created/verified table: {}", table_name);
    }
    
    // Commit transaction
    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        spdlog::error("Failed to commit transaction: {}", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    
    spdlog::debug("Database schema initialization completed successfully");
    return loadObjects();
}

void CPersistentDataStorage::unloadDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool CPersistentDataStorage::loadObjects() {
    using namespace sql;

    // Load servers - only cache server info for bots
    SelectModel sm;
    sm.select("*").from(DB::Tables::SERVERS);

    // 缓存服务器的地址
    std::unordered_map<int, std::pair<std::string, int>> server_cache;

    auto serverLoader = [this, &server_cache](const SQLiteRow& row) -> bool {
        try {
            int id = row.getInt(DB::Servers::ID);
            std::string host = row.getString(DB::Servers::HOST);
            int port = row.getInt(DB::Servers::PORT);

            if (host.empty() || port <= 0) {
                spdlog::warn("Skipping invalid server record: host='{}', port={}", host, port);
                return true; // Continue processing
            }

            server_cache[id] = std::make_pair(host, port);
            return true; // Continue processing
        } catch (const std::exception& e) {
            spdlog::error("Error parsing server record: {}", e.what());
            return true; // Continue processing other records
        }
    };

    if (!executeQuery(sm.str(), serverLoader, "loading servers")) {
        return false;
    }

    // Load bots
    SelectModel bm;
    bm.select("*").from(DB::Tables::BOTS);
    
    auto botLoader = [this, &server_cache](const SQLiteRow& row) -> bool {
        try {
            std::string uuid = row.getString(DB::Bots::UUID);
            std::string name = row.getString(DB::Bots::NAME);
            int server_id = row.getInt(DB::Bots::SERVER_ID);
            bool invulnerable = row.getBool(DB::Bots::INVULNERABLE);
            std::string systemPrompt = row.getString(DB::Bots::SYSTEM_PROMPT);
            
            if (uuid.empty()) {
                spdlog::warn("Skipping bot record with empty UUID");
                return true; // Continue processing
            }
            
            auto bot = std::make_shared<CBot>(uuid);
            bot->setName(name);
            bot->setSystemPrompt(systemPrompt);
            
            // Set host and port from server cache
            if (server_cache.find(server_id) != server_cache.end()) {
                auto& server_info = server_cache.at(server_id);
                bot->setHost(server_info.first);
                bot->setPort(server_info.second);
            }
            // Note: invulnerable property could be added to CBot if needed
            
            vBots.push_back(bot);
            
            // Add to hash map for O(1) lookup by UUID
            botsByUuid[uuid] = bot;
            
            return true; // Continue processing
        } catch (const std::exception& e) {
            spdlog::error("Error parsing bot record: {}", e.what());
            return true; // Continue processing other records
        }
    };
    if (!executeQuery(bm.str(), botLoader, "loading bots")) {
        return false;
    }

    // Load LLM providers
    SelectModel lm;
    lm.select("*").from(DB::Tables::LLM_PROVIDERS);
    
    auto llmLoader = [this](const SQLiteRow& row) -> bool {
        try {
            int id = row.getInt(DB::LLMProviders::ID);
            std::string name = row.getString(DB::LLMProviders::NAME);
            std::string api_key = row.getString(DB::LLMProviders::API_KEY);
            std::string base_url = row.getString(DB::LLMProviders::BASE_URL);
            std::string model = row.getString(DB::LLMProviders::MODEL);
            
            if (name.empty() || api_key.empty() || base_url.empty() || model.empty()) {
                spdlog::warn("Skipping invalid LLM provider record: name='{}', base_url='{}'", name, base_url);
                return true; // Continue processing
            }
            
            auto llmProvider = std::make_shared<CLLMProvider>(id, name, api_key, base_url, model);
            llmProvider->setDbId(id);
            
            vLLMProvider.push_back(llmProvider);
            
            // Add to hash map for O(1) lookup by ID
            llmProvidersById[id] = llmProvider;
            
            return true; // Continue processing
        } catch (const std::exception& e) {
            spdlog::error("Error parsing LLM provider record: {}", e.what());
            return true; // Continue processing other records
        }
    };
    
    if (!executeQuery(lm.str(), llmLoader, "loading LLM providers")) {
        return false;
    }

    spdlog::debug("Successfully loaded {} bots from database", vBots.size());
    spdlog::debug("Successfully loaded {} LLM providers from database", vLLMProvider.size());
    return true;
}

std::vector<CPersistentDataStorage::LLMSessionData> CPersistentDataStorage::loadActiveLLMSessions() {
    using namespace sql;
    
    std::vector<LLMSessionData> sessions;
    
    // Load active LLM sessions
    SelectModel sm;
    sm.select("*").from(DB::Tables::LLM_SESSIONS)
      .where(DB::LLMSessions::IS_ACTIVE + " = 1");
    
    auto sessionLoader = [&sessions](const SQLiteRow& row) -> bool {
        try {
            LLMSessionData sessionData;
            sessionData.session_id = row.getString(DB::LLMSessions::SESSION_ID);
            sessionData.bot_uuid = row.getString(DB::LLMSessions::BOT_UUID);
            sessionData.provider_id = row.getInt(DB::LLMSessions::PROVIDER_ID);
            sessionData.is_active = row.getBool(DB::LLMSessions::IS_ACTIVE);
            sessionData.created_at = row.getString(DB::LLMSessions::CREATED_AT);
            sessionData.last_activity = row.getString(DB::LLMSessions::LAST_ACTIVITY);
            
            if (sessionData.session_id.empty() || sessionData.bot_uuid.empty()) {
                spdlog::warn("Skipping invalid LLM session record: session_id='{}', bot_uuid='{}'", 
                           sessionData.session_id, sessionData.bot_uuid);
                return true; // Continue processing
            }
            
            sessions.push_back(sessionData);
            return true; // Continue processing
        } catch (const std::exception& e) {
            spdlog::error("Error parsing LLM session record: {}", e.what());
            return true; // Continue processing other records
        }
    };
    
    if (!executeQuery(sm.str(), sessionLoader, "loading active LLM sessions")) {
        spdlog::error("Failed to load active LLM sessions from database");
        return {};
    }
    
    spdlog::debug("Successfully loaded {} active LLM sessions from database", sessions.size());
    return sessions;
}

sqlite3* CPersistentDataStorage::getDb() {
    return db;
}

std::string CPersistentDataStorage::getCurrentTimeString() {
    std::time_t t = std::time(nullptr);
    std::tm* gmt = std::localtime(&t);  // 使用 gmtime 生成 UTC 时间（或用 localtime 获取本地时间）
    std::ostringstream oss;
    oss << std::put_time(gmt, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
