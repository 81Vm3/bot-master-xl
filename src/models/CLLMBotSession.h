#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <hv/json.hpp>
#include "CBot.h"
#include "CLLMProvider.h"

using json = nlohmann::json;

class CLLMBotSession {
public:
    std::string session_id;
    std::shared_ptr<CBot> bot;
    std::shared_ptr<CLLMProvider> llm_provider;
    std::deque<json> conversation_history;
    std::chrono::steady_clock::time_point last_activity;
    std::map<std::string, std::chrono::steady_clock::time_point> action_cooldowns;
    bool is_active;
    bool is_idle_waiting_llm;  // Non-blocking idle state for LLM responses
    
    CLLMBotSession(const std::string& sid, std::shared_ptr<CBot> bot_ptr, std::shared_ptr<CLLMProvider> provider_ptr);
    ~CLLMBotSession() = default;
    
    void updateActivity();
    void addToConversationHistory(const json& message);
    bool isExpired(std::chrono::minutes timeout) const;
    bool checkActionCooldown(const std::string& action, std::chrono::seconds cooldown) const;
    void setActionCooldown(const std::string& action);
    void deactivate();
    
    // Bot access
    std::shared_ptr<CBot> getBot() const { return bot; }
    
    // State management for LLM context
    json getCurrentState() const;
    
    // Get processed prompt with placeholders replaced
    std::string getProcessedPrompt() const;

    // Periodic autonomous update
    void performAutonomousUpdate();
    void processLLMCallback(const json &response, const std::string &result_type, const json &function_result);
    
    // Get bound LLM provider
    std::shared_ptr<CLLMProvider> getLLMProvider() const { return llm_provider; }
    
    json toJson() const;
};
