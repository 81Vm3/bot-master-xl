#include "CLLMBotSessionManager.h"
#include "../CApp.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <spdlog/spdlog.h>

#include "CLogger.h"
#include "CPersistentDataStorage.h"

CLLMBotSessionManager::CLLMBotSessionManager() {
    startAsyncUpdates();
}

CLLMBotSessionManager::~CLLMBotSessionManager() {
    stopAsyncUpdates();
}

std::string CLLMBotSessionManager::createSession(std::shared_ptr<CBot> bot, std::shared_ptr<CLLMProvider> llm_provider) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    cleanupExpiredSessions();
    
    std::string session_id = generateSessionId();
    auto session = std::make_unique<CLLMBotSession>(session_id, bot, llm_provider);
    
    sessions[session_id] = std::move(session);
    botSessionMap[session_id] = bot->getUuid();
    
    CLogger::getInstance()->llm->info("Created LLM bot session {} for bot {} with provider {}",
                 session_id.c_str(), 
                 bot ? bot->getName().c_str() : "unknown",
                 llm_provider ? llm_provider->getName().c_str() : "unknown");
    return session_id;
}

void CLLMBotSessionManager::loadSessionsFromDatabase() {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto database = CApp::getInstance()->getDatabase();
    if (!database) {
        spdlog::error("Database not available for loading sessions");
        return;
    }
    
    // Load active sessions from database
    auto sessionDataList = database->loadActiveLLMSessions();
    
    for (const auto& sessionData : sessionDataList) {
        try {
            // Find the bot by UUID
            auto bot = database->botsByUuid.find(sessionData.bot_uuid);
            if (bot == database->botsByUuid.end()) {
                spdlog::warn("Bot with UUID {} not found for session {}, skipping", 
                           sessionData.bot_uuid, sessionData.session_id);
                continue;
            }
            
            // Find the LLM provider by ID
            auto provider = database->llmProvidersById.find(sessionData.provider_id);
            if (provider == database->llmProvidersById.end()) {
                spdlog::warn("LLM provider with ID {} not found for session {}, skipping", 
                           sessionData.provider_id, sessionData.session_id);
                continue;
            }
            
            // Create the session
            auto session = std::make_unique<CLLMBotSession>(sessionData.session_id, bot->second, provider->second);
            
            // Restore the session state
            session->is_active = sessionData.is_active;
            
            // Store the session
            sessions[sessionData.session_id] = std::move(session);
            botSessionMap[sessionData.session_id] = sessionData.bot_uuid;
            
            spdlog::info("Restored LLM session {} for bot {} with provider {}", 
                        sessionData.session_id, 
                        sessionData.bot_uuid, 
                        provider->second->getName());
            
        } catch (const std::exception& e) {
            spdlog::error("Error restoring session {}: {}", sessionData.session_id, e.what());
        }
    }
    
    spdlog::info("Loaded {} LLM sessions from database", sessionDataList.size());
}

CLLMBotSession* CLLMBotSessionManager::getSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session_id);
    if (it != sessions.end() && it->second->is_active) {
        return it->second.get();
    }
    return nullptr;
}

bool CLLMBotSessionManager::endSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session_id);
    if (it != sessions.end()) {
        it->second->deactivate();
        sessions.erase(it);
        botSessionMap.erase(session_id);
        spdlog::info("Ended LLM bot session: %s", session_id.c_str());
        return true;
    }
    return false;
}

void CLLMBotSessionManager::cleanupExpiredSessions() {
    for (auto it = sessions.begin(); it != sessions.end();) {
        if (it->second->isExpired(session_timeout)) {
            spdlog::info("Cleaning up expired LLM bot session: %s", it->first.c_str());
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}

void CLLMBotSessionManager::updateSessionActivity(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session_id);
    if (it != sessions.end()) {
        it->second->updateActivity();
    }
}

bool CLLMBotSessionManager::checkActionCooldown(const std::string& session_id, const std::string& action) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session_id);
    if (it != sessions.end()) {
        return it->second->checkActionCooldown(action, action_cooldown);
    }
    return true; // Allow action if session not found
}

void CLLMBotSessionManager::setActionCooldown(const std::string& session_id, const std::string& action) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session_id);
    if (it != sessions.end()) {
        it->second->setActionCooldown(action);
    }
}

void CLLMBotSessionManager::setSessionTimeout(std::chrono::minutes timeout) {
    session_timeout = timeout;
}

void CLLMBotSessionManager::setActionCooldown(std::chrono::seconds cooldown) {
    action_cooldown = cooldown;
}

std::vector<json> CLLMBotSessionManager::getAllSessionsInfo() const {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    std::vector<json> sessions_info;
    for (const auto& pair : sessions) {
        sessions_info.push_back(pair.second->toJson());
    }
    return sessions_info;
}

size_t CLLMBotSessionManager::getActiveSessionCount() const {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    return std::count_if(sessions.begin(), sessions.end(),
        [](const auto& pair) { return pair.second->is_active; });
}

bool CLLMBotSessionManager::hasSession(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    auto it = sessions.find(session_id);
    return it != sessions.end() && it->second->is_active;
}

std::string CLLMBotSessionManager::generateSessionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

void CLLMBotSessionManager::startAsyncUpdates() {
    should_stop = false;
    update_thread = std::thread(&CLLMBotSessionManager::asyncUpdateLoop, this);
}

void CLLMBotSessionManager::stopAsyncUpdates() {
    should_stop = true;
    update_cv.notify_all();
    if (update_thread.joinable()) {
        update_thread.join();
    }
}

void CLLMBotSessionManager::setUpdateInterval(std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(update_mutex);
    update_interval = interval;
}

void CLLMBotSessionManager::triggerUpdate(const std::string& session_id) {
    std::lock_guard<std::mutex> sessions_lock(sessions_mutex);
    auto it = sessions.find(session_id);
    if (it != sessions.end() && it->second->is_active) {
        processSessionUpdate(it->second.get());
    }
}

void CLLMBotSessionManager::asyncUpdateLoop() {
    while (!should_stop) {
        std::unique_lock<std::mutex> lock(update_mutex);
        
        // Wait for the specified interval or until signaled to stop
        if (update_cv.wait_for(lock, update_interval, [this] { return should_stop.load(); })) {
            break; // should_stop was set to true
        }
        
        // Process all active sessions
        std::lock_guard<std::mutex> sessions_lock(sessions_mutex);
        for (const auto& pair : sessions) {
            if (pair.second->is_active && !should_stop) {
                processSessionUpdate(pair.second.get());
            }
        }
        
        // Cleanup expired sessions
        cleanupExpiredSessions();
    }
}

void CLLMBotSessionManager::processSessionUpdate(CLLMBotSession* session) {
    if (!session || !session->bot) {
        return;
    }
    
    // // Skip autonomous updates if session is idle waiting for LLM response
    // if (session->is_idle_waiting_llm) {
    //     return;
    // }
    //
    // Check if the bot should perform an LLM API call
    if (!session->is_idle_waiting_llm) {
        if (session->checkActionCooldown("llm_update", std::chrono::seconds(10))) {
            // Delegate autonomous update logic to the session itself
            session->performAutonomousUpdate();

            // Set cooldown and update activity
            session->setActionCooldown("llm_update");
            session->updateActivity();
        }
    }
}


CBot* CLLMBotSessionManager::getBotFromLLMSession(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    auto it = sessions.find(session_id);
    if (it != sessions.end() && it->second->is_active) {
        return it->second->bot.get();
    }
    return nullptr;
}

CLLMBotSession* CLLMBotSessionManager::getLLMSessionFromBot(const std::string& bot_uuid) const {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    for (const auto& [session_id, uuid] : botSessionMap) {
        if (uuid == bot_uuid) {
            auto it = sessions.find(session_id);
            if (it != sessions.end() && it->second->is_active) {
                return it->second.get();
            }
        }
    }
    return nullptr;
}

CLLMBotSession* CLLMBotSessionManager::getLLMSessionFromBot(std::shared_ptr<CBot> bot) const {
    if (!bot) return nullptr;
    return getLLMSessionFromBot(bot->getUuid());
}
