#pragma once

#include "../models/CLLMBotSession.h"
#include "../models/CLLMProvider.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <hv/json.hpp>

using json = nlohmann::json;

class CLLMBotSessionManager {
private:
    std::unordered_map<std::string, std::unique_ptr<CLLMBotSession>> sessions;

    // session_id, bot_uuid
    std::unordered_map<std::string, std::string> botSessionMap;

    mutable std::mutex sessions_mutex;
    std::chrono::minutes session_timeout{30};
    std::chrono::seconds action_cooldown{2};
    
    // 异步更新控制
    std::thread update_thread;
    std::condition_variable update_cv;
    std::mutex update_mutex;
    std::atomic<bool> should_stop{false};
    std::chrono::milliseconds update_interval{5000}; // 5 seconds default

public:
    CLLMBotSessionManager();
    ~CLLMBotSessionManager();

    // LLM会话管理
    std::string createSession(std::shared_ptr<CBot> bot, std::shared_ptr<CLLMProvider> llm_provider);
    void restoreSession(const std::string& uuid, std::unique_ptr<CLLMBotSession> session);

    CLLMBotSession* getSession(const std::string& session_id);
    bool endSession(const std::string& session_id);
    void cleanupExpiredSessions();

    // 会话操作
    void updateSessionActivity(const std::string& session_id);
    bool checkActionCooldown(const std::string& session_id, const std::string& action);
    void setActionCooldown(const std::string& session_id, const std::string& action);
    
    // Async update control
    void startAsyncUpdates();
    void stopAsyncUpdates();
    void setUpdateInterval(std::chrono::milliseconds interval);
    void triggerUpdate(const std::string& session_id);
    
    // Configuration
    void setSessionTimeout(std::chrono::minutes timeout);
    void setActionCooldown(std::chrono::seconds cooldown);
    
    // Query methods
    std::vector<json> getAllSessionsInfo() const;
    size_t getActiveSessionCount() const;
    bool hasSession(const std::string& session_id) const;

    CBot* getBotFromLLMSession(const std::string& session_id) const;
    CLLMBotSession* getLLMSessionFromBot(const std::string& bot_uuid) const;
    CLLMBotSession* getLLMSessionFromBot(std::shared_ptr<CBot> bot) const;
    
private:
    std::string generateSessionId();
    void asyncUpdateLoop();
    void processSessionUpdate(CLLMBotSession* session);
};