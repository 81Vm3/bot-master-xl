//
// Created by rain on 8/1/25.
//

#include "CBotService.h"

#include <chrono>
#include <sqlite3.h>

#include "../utils/JsonResponse.h"
#include "../database/DBSchema.h"
#include "../CApp.h"
#include "../core/CPersistentDataStorage.h"
#include "../models/CBot.h"
#include <hv/json.hpp>
#include "../database/querybuilder.h"
#include "core/CLLMBotSessionManager.h"
#include "spdlog/spdlog.h"
#include "core/CLogger.h"

using json = nlohmann::json;

CBotService::CBotService(const std::string& uri) : CBaseService(uri) {

}

void CBotService::on_install(HttpService *router) {
    router->GET(getRelativePath("list").c_str(), CBotService::list_bots);
    router->POST(getRelativePath("create").c_str(), CBotService::create_bot);
    router->POST(getRelativePath("delete").c_str(), CBotService::delete_bot);
    router->POST(getRelativePath("set_password").c_str(), CBotService::set_password);
    router->POST(getRelativePath("reconnect").c_str(), CBotService::reconnect_bot);
    router->POST(getRelativePath("enable_llm").c_str(), CBotService::enable_llm_session);
    router->POST(getRelativePath("disable_llm").c_str(), CBotService::disable_llm_session);
    router->POST(getRelativePath("update_prompt").c_str(), CBotService::update_system_prompt);
}

int CBotService::list_bots(HttpRequest* req, HttpResponse* resp) {
    try {
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        sql::SelectModel sm;
        sm.select("*").from(DB::Tables::BOTS);
        
        std::string query = sm.str();
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(database->getDb(), query.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Failed to prepare query: {}", sqlite3_errmsg(database->getDb()));
            return resp->Json(JsonResponse::internal_error());
        }

        auto llmSessionManager = CApp::getInstance()->getLLMSessionManager();

        json bots = json::array();
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string uuid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            
            // Check if this bot has an active LLM session
            bool has_llm_session = false;
            std::string session_id = "";
            if (llmSessionManager) {
                CLLMBotSession* session = llmSessionManager->getLLMSessionFromBot(uuid);
                if (session) {
                    has_llm_session = true;
                    session_id = session->session_id;
                }
            }
            
            json bot = {
                {"uuid", uuid},
                {"name", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))},
                {"server_id", sqlite3_column_int(stmt, 2)},
                {"invulnerable", sqlite3_column_int(stmt, 3) == 1},
                {"system_prompt", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))},
                {"created_at", reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))},
                {"has_llm_session", has_llm_session}
            };
            // Add session_id if the bot has an active session
            if (has_llm_session && !session_id.empty()) {
                bot["llm_session_id"] = session_id;
            } else {
                bot["llm_session_id"] = -1;
            }
            
            bots.push_back(bot);
        }
        
        sqlite3_finalize(stmt);
        return resp->Json(JsonResponse::with_success(bots, "Bots retrieved successfully"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in list_bots: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CBotService::create_bot(HttpRequest* req, HttpResponse* resp) {
    try {
        json body;
        if (req->Body().empty()) {
            return resp->Json(JsonResponse::with_error("Request body is required"));
        }
        
        body = json::parse(req->Body());
        
        // Validate required fields
        if (!body.contains("name") || body["name"].get<std::string>().empty()) {
            return resp->Json(JsonResponse::with_error("Bot name is required"));
        }
        
        if (!body.contains("server_id")) {
            return resp->Json(JsonResponse::with_error("Server ID is required"));
        }

        std::string name = body["name"];
        int server_id = body["server_id"];
        bool invulnerable = body.value("invulnerable", false);
        std::string system_prompt = body.value("system_prompt", "");
        std::string password = body.value("password", "");
        int llm_provider_id = body.value("llm_provider_id", -1);

        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }

        // Validate the server exists
        sql::SelectModel select_model;
        select_model.select(DB::Servers::HOST)
            .select(DB::Servers::PORT)
            .from(DB::Tables::SERVERS)
            .where(DB::Servers::ID + " = " + std::to_string(server_id));
        
        // Create CBot instance (UUID will be auto-generated in CRakBot constructor)
        auto bot = std::make_shared<CBot>(name);
        bot->setSystemPrompt(system_prompt);

        // set password
        bot->setPassword(password);
        
        // Get the auto-generated UUID
        std::string uuid = bot->getUuid();

        // Check if server exists
        std::string host;
        int port = 0;
        bool server_exists = false;
        
        auto serverChecker = [&](const SQLiteRow& row) -> bool {
            host = row.getString(DB::Servers::HOST);
            port = row.getInt(DB::Servers::PORT);
            server_exists = true;
            return true; // Stop after first match
        };
        
        if (!database->executeQuery(select_model.str(), serverChecker, "checking server")) {
            return resp->Json(JsonResponse::internal_error());
        }
        if (!server_exists) {
            return resp->Json(JsonResponse::with_error("Server not found"));
        }
        
        // Set server connection details
        bot->setHost(host);
        bot->setPort(port);
        
        // Insert into database using query builder (handles escaping automatically)
        sql::InsertModel im;
        im.insert(DB::Bots::UUID, uuid)
          .insert(DB::Bots::NAME, name)
          .insert(DB::Bots::SERVER_ID, server_id)
          .insert(DB::Bots::INVULNERABLE, invulnerable ? 1 : 0)
          .insert(DB::Bots::SYSTEM_PROMPT, system_prompt)
          .insert(DB::Bots::PASSWORD, password)
          .into(DB::Tables::BOTS);
        
        char* error_msg = nullptr;
        int rc = sqlite3_exec(database->getDb(), im.str().c_str(), nullptr, nullptr, &error_msg);
        
        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Failed to create bot: {}", error_msg ? error_msg : "Unknown error");
            if (error_msg) sqlite3_free(error_msg);
            return resp->Json(JsonResponse::internal_error());
        }

        // Add bot to memory and hash map
        database->vBots.push_back(bot);
        database->botsByUuid[uuid] = bot;
        
        // Create LLM session if provider ID is provided
        std::string llm_session_id = "";
        if (llm_provider_id > 0) {
            llm_session_id = createLLMSessionForBot(uuid, llm_provider_id);
            if (llm_session_id.empty()) {
                return resp->Json(JsonResponse::with_error("Failed to create LLM session"));
            }
        }
        
        json bot_data = {
            {"uuid", uuid},
            {"name", name},
            {"server_id", server_id},
            {"invulnerable", invulnerable},
            {"system_prompt", system_prompt},
            {"status", "created"}
        };
        
        // Add LLM session info if created
        if (!llm_session_id.empty()) {
            bot_data["llm_session_id"] = llm_session_id;
            bot_data["llm_provider_id"] = llm_provider_id;
            bot_data["has_llm_session"] = true;
        } else {
            bot_data["has_llm_session"] = false;
        }
        
        return resp->Json(JsonResponse::with_success(bot_data, "Bot created successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in create_bot: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CBotService::delete_bot(HttpRequest* req, HttpResponse* resp) {
    try {
        json body;
        if (req->Body().empty()) {
            return resp->Json(JsonResponse::with_error("Request body is required"));
        }
        
        body = json::parse(req->Body());
        
        if (!body.contains("uuid") || body["uuid"].get<std::string>().empty()) {
            return resp->Json(JsonResponse::with_error("Bot UUID is required"));
        }
        
        std::string uuid = body["uuid"];
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        // Check if bot exists using query builder with column operators
        sql::SelectModel sm;
        sm.select("COUNT(*)").from(DB::Tables::BOTS)
          .where(sql::column(DB::Bots::UUID) == uuid);
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(database->getDb(), sm.str().c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Failed to prepare check query: {}", sqlite3_errmsg(database->getDb()));
            return resp->Json(JsonResponse::internal_error());
        }
        
        rc = sqlite3_step(stmt);
        int count = (rc == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : 0;
        sqlite3_finalize(stmt);
        
        if (count == 0) {
            return resp->Json(JsonResponse::not_found());
        }
        
        // Find bot using O(1) hash map lookup
        auto bot_it = database->botsByUuid.find(uuid);
        if (bot_it == database->botsByUuid.end()) {
            return resp->Json(JsonResponse::not_found());
        }
        
        auto& bot = bot_it->second;

        // Clean up any LLM sessions associated with this bot
        deleteLLMSessionForBot(uuid);
        
        // Remove from memory and hash map
        auto& bots = database->vBots;
        bots.erase(std::remove_if(bots.begin(), bots.end(),
            [&uuid](const std::shared_ptr<CBot>& bot) {
                return bot->getUuid() == uuid;
            }), bots.end());
        database->botsByUuid.erase(uuid);
        
        // Delete from database using query builder with column operators
        sql::DeleteModel dm;
        dm.from(DB::Tables::BOTS).where(sql::column(DB::Bots::UUID) == uuid);
        
        char* error_msg = nullptr;
        rc = sqlite3_exec(database->getDb(), dm.str().c_str(), nullptr, nullptr, &error_msg);
        
        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Failed to delete bot: {}", error_msg ? error_msg : "Unknown error");
            if (error_msg) sqlite3_free(error_msg);
            return resp->Json(JsonResponse::internal_error());
        }
        
        return resp->Json(JsonResponse::with_success(json::object(), "Bot deleted successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in delete_bot: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CBotService::set_password(HttpRequest* req, HttpResponse* resp) {
    try {
        json body;
        if (req->Body().empty()) {
            return resp->Json(JsonResponse::with_error("Request body is required"));
        }
        
        body = json::parse(req->Body());
        
        if (!body.contains("uuid") || body["uuid"].get<std::string>().empty()) {
            return resp->Json(JsonResponse::with_error("Bot UUID is required"));
        }
        
        if (!body.contains("password")) {
            return resp->Json(JsonResponse::with_error("Password is required"));
        }
        
        std::string uuid = body["uuid"];
        std::string password = body["password"];
        
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        // Find bot using O(1) hash map lookup
        auto bot_it = database->botsByUuid.find(uuid);
        if (bot_it == database->botsByUuid.end()) {
            return resp->Json(JsonResponse::not_found());
        }
        
        auto bot = bot_it->second;
        bot->setPassword(password);
        
        return resp->Json(JsonResponse::with_success(json::object(), "Bot password updated successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in set_password: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CBotService::reconnect_bot(HttpRequest* req, HttpResponse* resp) {
    try {
        json body;
        if (req->Body().empty()) {
            return resp->Json(JsonResponse::with_error("Request body is required"));
        }
        
        body = json::parse(req->Body());
        
        if (!body.contains("uuid") || body["uuid"].get<std::string>().empty()) {
            return resp->Json(JsonResponse::with_error("Bot UUID is required"));
        }
        
        std::string uuid = body["uuid"];
        
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        

        // Find bot using O(1) hash map lookup
        auto bot_it = database->botsByUuid.find(uuid);
        if (bot_it == database->botsByUuid.end()) {
            return resp->Json(JsonResponse::not_found());
        }
        auto bot = bot_it->second;
        
        // Disconnect if connected, then reconnect
        if (bot->isConnected()) {
            bot->disconnect();
        }
        
        // Attempt to connect
        bot->connect();
        bool success = bot->isConnected();
        
        json result = {
            {"uuid", uuid},
            {"status", success ? "connected" : "failed"},
            {"connected", success}
        };
        
        return resp->Json(JsonResponse::with_success(result, 
            success ? "Bot reconnected successfully" : "Failed to reconnect bot"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in reconnect_bot: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CBotService::enable_llm_session(HttpRequest* req, HttpResponse* resp) {
    try {
        json body;
        if (req->Body().empty()) {
            return resp->Json(JsonResponse::with_error("Request body is required"));
        }
        
        body = json::parse(req->Body());
        
        if (!body.contains("uuid") || body["uuid"].get<std::string>().empty()) {
            return resp->Json(JsonResponse::with_error("Bot UUID is required"));
        }
        
        if (!body.contains("provider_id")) {
            return resp->Json(JsonResponse::with_error("LLM Provider ID is required"));
        }
        
        std::string uuid = body["uuid"];
        int provider_id = body["provider_id"];
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }

        // Find bot in memory
        auto bot_it = database->botsByUuid.find(uuid);
        if (bot_it == database->botsByUuid.end()) {
            return resp->Json(JsonResponse::not_found());
        }
        
        // Create LLM session using helper method
        std::string sessionId = createLLMSessionForBot(uuid, provider_id);
        if (sessionId.empty()) {
            return resp->Json(JsonResponse::with_error("Failed to create LLM session"));
        }
        
        json result = {
            {"uuid", uuid},
            {"session_id", sessionId},
            {"provider_id", provider_id},
            {"status", "LLM session enabled"}
        };
        
        return resp->Json(JsonResponse::with_success(result, "LLM session enabled successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in enable_llm_session: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CBotService::disable_llm_session(HttpRequest* req, HttpResponse* resp) {
    try {
        json body;
        if (req->Body().empty()) {
            return resp->Json(JsonResponse::with_error("Request body is required"));
        }
        
        body = json::parse(req->Body());
        
        if (!body.contains("uuid") || body["uuid"].get<std::string>().empty()) {
            return resp->Json(JsonResponse::with_error("Bot UUID is required"));
        }
        
        std::string uuid = body["uuid"];
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        // Find bot in memory
        auto bot_it = database->botsByUuid.find(uuid);
        if (bot_it == database->botsByUuid.end()) {
            return resp->Json(JsonResponse::not_found());
        }
        
        // Delete LLM session using helper method
        bool sessionDeleted = deleteLLMSessionForBot(uuid);
        if (!sessionDeleted) {
            return resp->Json(JsonResponse::with_error("Failed to disable LLM session"));
        }
        
        json result = {
            {"uuid", uuid},
            {"status", "LLM session disabled"}
        };
        
        return resp->Json(JsonResponse::with_success(result, "LLM session disabled successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in disable_llm_session: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CBotService::update_system_prompt(HttpRequest* req, HttpResponse* resp) {
    try {
        json body;
        if (req->Body().empty()) {
            return resp->Json(JsonResponse::with_error("Request body is required"));
        }
        
        body = json::parse(req->Body());
        
        if (!body.contains("uuid") || body["uuid"].get<std::string>().empty()) {
            return resp->Json(JsonResponse::with_error("Bot UUID is required"));
        }
        
        if (!body.contains("system_prompt")) {
            return resp->Json(JsonResponse::with_error("System prompt is required"));
        }
        
        std::string uuid = body["uuid"];
        std::string system_prompt = body["system_prompt"];
        
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        // Find bot in memory
        auto bot_it = database->botsByUuid.find(uuid);
        if (bot_it == database->botsByUuid.end()) {
            return resp->Json(JsonResponse::not_found());
        }
        auto bot = bot_it->second;
        
        // Update bot's system prompt in memory
        bot->setSystemPrompt(system_prompt);
        
        // Update database
        sql::UpdateModel um;
        um.update(DB::Tables::BOTS)
          .set(DB::Bots::SYSTEM_PROMPT, system_prompt)
          .where(sql::column(DB::Bots::UUID) == uuid);
        
        char* error_msg = nullptr;
        int rc = sqlite3_exec(database->getDb(), um.str().c_str(), nullptr, nullptr, &error_msg);
        
        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Failed to update bot system prompt: {}", error_msg ? error_msg : "Unknown error");
            if (error_msg) sqlite3_free(error_msg);
            // Revert the memory change since database update failed
            bot->setSystemPrompt(""); // This should revert to original, but we don't have it stored
            return resp->Json(JsonResponse::internal_error());
        }
        
        json result = {
            {"uuid", uuid},
            {"system_prompt", system_prompt},
            {"status", "System prompt updated"}
        };
        
        return resp->Json(JsonResponse::with_success(result, "System prompt updated successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in update_system_prompt: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

std::string CBotService::createLLMSessionForBot(const std::string& botUuid, int providerId) {
    auto database = CApp::getInstance()->getDatabase();
    if (!database) {
        CLogger::getInstance()->api->error("Database not available for LLM session creation");
        return "";
    }
    
    // Validate that the LLM provider exists
    auto llm_provider_it = database->llmProvidersById.find(providerId);
    if (llm_provider_it == database->llmProvidersById.end()) {
        CLogger::getInstance()->api->error("LLM provider {} not found", providerId);
        return "";
    }
    
    // Find bot in memory
    auto bot_it = database->botsByUuid.find(botUuid);
    if (bot_it == database->botsByUuid.end()) {
        CLogger::getInstance()->api->error("Bot {} not found in memory", botUuid);
        return "";
    }
    auto bot = bot_it->second;
    
    // Get LLM session manager
    auto llmSessionManager = CApp::getInstance()->getLLMSessionManager();
    if (!llmSessionManager) {
        CLogger::getInstance()->api->error("LLM session manager not available");
        return "";
    }
    
    // Check if bot already has an active LLM session using session manager
    CLLMBotSession* existingSession = llmSessionManager->getLLMSessionFromBot(botUuid);
    if (existingSession != nullptr) {
        CLogger::getInstance()->api->error("Bot {} already has an active LLM session", botUuid);
        return "";
    }
    
    // Create LLM session
    std::string sessionId = llmSessionManager->createSession(bot, llm_provider_it->second);
    if (sessionId.empty()) {
        CLogger::getInstance()->api->error("Failed to create LLM session for bot {}", botUuid);
        return "";
    }
    
    // Insert into llm_sessions table
    sql::InsertModel im;
    im.insert(DB::LLMSessions::SESSION_ID, sessionId)
      .insert(DB::LLMSessions::BOT_UUID, botUuid)
      .insert(DB::LLMSessions::PROVIDER_ID, providerId)
      .insert(DB::LLMSessions::IS_ACTIVE, 1)
      .into(DB::Tables::LLM_SESSIONS);
    
    char* error_msg = nullptr;
    auto rc = sqlite3_exec(database->getDb(), im.str().c_str(), nullptr, nullptr, &error_msg);
    
    if (rc != SQLITE_OK) {
        CLogger::getInstance()->api->error("Failed to create LLM session record: {}", error_msg ? error_msg : "Unknown error");
        if (error_msg) sqlite3_free(error_msg);
        // Clean up the session since database update failed
        llmSessionManager->endSession(sessionId);
        return "";
    }
    
    return sessionId;
}

bool CBotService::deleteLLMSessionForBot(const std::string& botUuid) {
    auto database = CApp::getInstance()->getDatabase();
    if (!database) {
        CLogger::getInstance()->api->error("Database not available for LLM session deletion");
        return false;
    }
    
    // Get LLM session manager
    auto llmSessionManager = CApp::getInstance()->getLLMSessionManager();
    if (!llmSessionManager) {
        CLogger::getInstance()->api->error("LLM session manager not available");
        return false;
    }
    
    // Check if bot has an active LLM session using session manager
    CLLMBotSession* existingSession = llmSessionManager->getLLMSessionFromBot(botUuid);
    if (existingSession == nullptr) {
        //CLogger::getInstance()->api->warn("Bot {} does not have an active LLM session", botUuid);
        return true; // Not an error - bot simply doesn't have a session
    }
    
    std::string sessionId = existingSession->session_id;
    
    // End the LLM session in session manager
    bool sessionEnded = llmSessionManager->endSession(sessionId);
    if (!sessionEnded) {
        CLogger::getInstance()->api->error("Failed to end LLM session {} for bot {}", sessionId, botUuid);
        return false;
    }
    
    // Update database to set is_active = false
    sql::UpdateModel um;
    um.update(DB::Tables::LLM_SESSIONS)
      .set(DB::LLMSessions::IS_ACTIVE, 0)
      .where(DB::LLMSessions::BOT_UUID + " = '" + botUuid + "' AND " + DB::LLMSessions::SESSION_ID + " = '" + sessionId + "'");
    
    char* error_msg = nullptr;
    int rc = sqlite3_exec(database->getDb(), um.str().c_str(), nullptr, nullptr, &error_msg);
    
    if (rc != SQLITE_OK) {
        CLogger::getInstance()->api->error("Failed to update LLM session status in database: {}", error_msg ? error_msg : "Unknown error");
        if (error_msg) sqlite3_free(error_msg);
        return false;
    }
    
    CLogger::getInstance()->api->info("Successfully deleted LLM session {} for bot {}", sessionId, botUuid);
    return true;
}
