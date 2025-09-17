//
// Created by Administrator on 2025/5/7.
//

#ifndef CPERSISTENTDATASTORAGE_H
#define CPERSISTENTDATASTORAGE_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../models/CServer.h"
#include "../models/CBot.h"
#include "../models/CLLMProvider.h"
#include "sqlite3.h"
#include "spdlog/spdlog.h"

// SQLite result row wrapper for easier column access
class SQLiteRow {
public:
    SQLiteRow(int argc, char** argv, char** colNames);
    
    std::string getString(const std::string& columnName, const std::string& defaultValue = "") const;
    int getInt(const std::string& columnName, int defaultValue = 0) const;
    bool getBool(const std::string& columnName, bool defaultValue = false) const;
    bool hasColumn(const std::string& columnName) const;
    bool isNull(const std::string& columnName) const;

private:
    std::unordered_map<std::string, int> columnIndexMap;
    char** values;
    int columnCount;
};

class CPersistentDataStorage {
public:
    CPersistentDataStorage();
    bool loadDatabase();
    void unloadDatabase();

    bool loadObjects();
    sqlite3* getDb();

    static std::string getCurrentTimeString();
    
    // Session management
    struct LLMSessionData {
        std::string session_id;
        std::string bot_uuid;
        int provider_id;
        bool is_active;
        std::string created_at;
        std::string last_activity;
    };
    
    // SQLite query execution wrapper
    template<typename CallbackType>
    bool executeQuery(const std::string& query, CallbackType callback, const std::string& errorContext = "") {
        char* error = nullptr;
        
        auto wrapperCallback = [](void* data, int argc, char** argv, char** colNames) -> int {
            auto* callbackPtr = static_cast<CallbackType*>(data);
            SQLiteRow row(argc, argv, colNames);
            return (*callbackPtr)(row) ? 0 : 1;  // Return 0 to continue, 1 to stop
        };
        
        auto rc = sqlite3_exec(db, query.c_str(), wrapperCallback, &callback, &error);
        
        if (rc != SQLITE_OK) {
            std::string context = errorContext.empty() ? "executing query" : errorContext;
            spdlog::error("Error {}: {}", context, error);
            sqlite3_free(error);
            return false;
        }
        
        return true;
    }

    std::vector<std::shared_ptr<CBot>> vBots; // share it with LLMBotSession
    std::vector<std::shared_ptr<CLLMProvider>> vLLMProvider; // share it with LLMBotSession
    
    // Hash maps for O(1) lookup
    std::unordered_map<std::string, std::shared_ptr<CBot>> botsByUuid;
    std::unordered_map<int, std::shared_ptr<CLLMProvider>> llmProvidersById;

private:
    sqlite3 *db;
};



#endif //CPERSISTENTDATASTORAGE_H
