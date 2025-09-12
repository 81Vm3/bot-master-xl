//
// Database Schema Constants
// Centralized location for all table and column names
//

#ifndef DBSCHEMA_H
#define DBSCHEMA_H

#include <string>

namespace DB {
    // Table names
    namespace Tables {
        const std::string SERVERS = "servers";
        const std::string BOTS = "bots";
        const std::string LLM_PROVIDERS = "llm_providers";
        const std::string LLM_SESSIONS = "llm_sessions";
        const std::string SERVER_MEMORY = "server_memory";
        const std::string SERVER_MEMORY_TAG = "server_memory_tag";
        const std::string BOT_MEMORY = "bot_memory";
        const std::string BOT_MEMORY_TAG = "bot_memory_tag";
    }
    
    // Servers table columns
    namespace Servers {
        const std::string ID = "id";
        const std::string HOST = "host";
        const std::string PORT = "port";
        const std::string NAME = "name";
        const std::string GAMEMODE = "gamemode";
        const std::string RULE = "rule";
        const std::string LANGUAGE = "language";
        const std::string PLAYERS = "players";
        const std::string MAX_PLAYERS = "max_players";
        const std::string PING = "ping";
        const std::string LAST_UPDATE = "last_update";
        const std::string CREATED_AT = "created_at";
    }
    
    // Bots table columns
    namespace Bots {
        const std::string UUID = "uuid";
        const std::string NAME = "name";
        const std::string SERVER_ID = "server_id";
        const std::string INVULNERABLE = "invulnerable";
        const std::string SYSTEM_PROMPT = "system_prompt";
        const std::string CREATED_AT = "created_at";
    }
    
    // LLM Providers table columns
    namespace LLMProviders {
        const std::string ID = "id";
        const std::string NAME = "name";
        const std::string API_KEY = "api_key";
        const std::string BASE_URL = "base_url";
        const std::string MODEL = "model";
        const std::string CREATED_AT = "created_at";
    }
    
    // Server Memory table columns
    namespace ServerMemory {
        const std::string ID = "id";
        const std::string SERVER_ID = "server_id";
        const std::string CONTENT = "content";
        const std::string CREATED_AT = "created_at";
    }
    
    // Server Memory Tag table columns
    namespace ServerMemoryTag {
        const std::string ID = "id";
        const std::string MEMORY_ID = "memory_id";
        const std::string TAG = "tag";
        const std::string CREATED_AT = "created_at";
    }
    
    // Bot Memory table columns
    namespace BotMemory {
        const std::string ID = "id";
        const std::string UUID = "uuid";
        const std::string SERVER_ID = "server_id";
        const std::string CONTENT = "content";
        const std::string CREATED_AT = "created_at";
    }
    
    // Bot Memory Tag table columns
    namespace BotMemoryTag {
        const std::string ID = "id";
        const std::string MEMORY_ID = "memory_id";
        const std::string TAG = "tag";
        const std::string CREATED_AT = "created_at";
    }
    
    // LLM Sessions table columns
    namespace LLMSessions {
        const std::string ID = "id";
        const std::string SESSION_ID = "session_id";
        const std::string BOT_UUID = "bot_uuid";
        const std::string PROVIDER_ID = "provider_id";
        const std::string IS_ACTIVE = "is_active";
        const std::string CREATED_AT = "created_at";
        const std::string LAST_ACTIVITY = "last_activity";
    }
}

#endif //DBSCHEMA_H