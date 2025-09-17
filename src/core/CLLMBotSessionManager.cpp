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
    botSessionMap[bot->getUuid()] = session_id;
    
    CLogger::getInstance()->llm->info("Created LLM bot session {} for bot {} with provider {}",
                 session_id.c_str(), 
                 bot ? bot->getName().c_str() : "unknown",
                 llm_provider ? llm_provider->getName().c_str() : "unknown");
    return session_id;
}

void CLLMBotSessionManager::restoreSession(const std::string &session_id, std::unique_ptr<CLLMBotSession> session) {
    if (sessions.find(session_id) == sessions.end() && session && session->bot) {
        std::string bot_uuid = session->bot->getUuid();
        botSessionMap[bot_uuid] = session_id;
        // Save session info before moving
        auto msg = fmt::format("Restored LLM bot session {} for bot {}",
                 session_id.c_str(),
                 session->bot->getName(),
                 session->llm_provider->getName());
        sessions[session_id] = std::move(session);
        CLogger::getInstance()->llm->info(msg);
    } else {
        spdlog::warn("Failed to restore session {}: invalid session or bot", session_id);
    }
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
        if (it->second->bot) {
            std::string bot_uuid = it->second->bot->getUuid();
            botSessionMap.erase(bot_uuid);
        }
        it->second->deactivate();
        sessions.erase(it);
        spdlog::info("Ended LLM bot session: {}", session_id.c_str());
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
    if (!session || !session->bot || !session->is_active) {
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
    auto it = botSessionMap.find(bot_uuid);
    if (it != botSessionMap.end()) {
        auto session_it = sessions.find(it->second);
        if (session_it != sessions.end() && session_it->second->is_active) {
            return session_it->second.get();
        }
    }
    return nullptr;
}

CLLMBotSession* CLLMBotSessionManager::getLLMSessionFromBot(std::shared_ptr<CBot> bot) const {
    if (!bot) return nullptr;
    return getLLMSessionFromBot(bot->getUuid());
}
