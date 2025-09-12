#include "CLLMBotSession.h"
#include "../CApp.h"
#include "../utils/CFunctionDispatcher.h"
#include "core/CConfig.h"
#include "core/CLogger.h"
#include "core/CPersistentDataStorage.h"
#include "utils/TextConverter.h"

CLLMBotSession::CLLMBotSession(const std::string &sid, std::shared_ptr<CBot> bot_ptr,
                               std::shared_ptr<CLLMProvider> provider_ptr)
    : session_id(sid), bot(bot_ptr), llm_provider(provider_ptr), last_activity(std::chrono::steady_clock::now()),
      is_active(true), is_idle_waiting_llm(false) {
    // No need to initialize function dispatcher - using global one
}

void CLLMBotSession::updateActivity() {
    last_activity = std::chrono::steady_clock::now();
}

void CLLMBotSession::addToConversationHistory(const json &message) {
    conversation_history.push_back(message);
    if (conversation_history.size() > 20) {
        conversation_history.pop_front();
    }
}

bool CLLMBotSession::isExpired(std::chrono::minutes timeout) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - last_activity);
    return elapsed > timeout;
}

bool CLLMBotSession::checkActionCooldown(const std::string &action, std::chrono::seconds cooldown) const {
    auto cooldown_it = action_cooldowns.find(action);

    if (cooldown_it == action_cooldowns.end()) {
        return true; // No cooldown set, action allowed
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - cooldown_it->second);

    return elapsed >= cooldown;
}

void CLLMBotSession::setActionCooldown(const std::string &action) {
    action_cooldowns[action] = std::chrono::steady_clock::now();
}

void CLLMBotSession::deactivate() {
    is_active = false;
}

json CLLMBotSession::toJson() const {
    return json{
        {"session_id", session_id},
        {"bot_id", bot ? bot->getName() : "unknown"},
        {"is_active", is_active},
        {"is_idle_waiting_llm", is_idle_waiting_llm},
        {"conversation_length", conversation_history.size()},
        {
            "last_activity", std::chrono::duration_cast<std::chrono::seconds>(
                last_activity.time_since_epoch()).count()
        }
    };
}

json CLLMBotSession::getCurrentState() const {
    // deprecated
    return {};
}

std::string CLLMBotSession::getProcessedPrompt() const {
    if (!bot) {
        return "";
    }

    std::string processed_prompt = CApp::getInstance()->getConfig()->base_internal_prompt;

    static auto replace_str = [&processed_prompt](const std::string &src, const std::string &dst) {
        size_t start_pos = 0;
        while ((start_pos = processed_prompt.find(src, start_pos)) != std::string::npos) {
            processed_prompt.replace(start_pos, src.length(), dst);
            start_pos += dst.length(); // advance past the new substring
        }
    };

    replace_str("[NAME]", bot->getName());
    replace_str("[SESSION_ID]", session_id);
    replace_str("[PASSWORD]", bot->getPassword());

    return processed_prompt;
}

void CLLMBotSession::performAutonomousUpdate() {
    if (!bot) {
        return;
    }
    auto dispatcher = CApp::getInstance()->getFunctionDispatcher();
    if (!dispatcher) {
        return;
    }
    if (!llm_provider) {
        return;
    }

    std::vector<json> messages;
    // 1. 加入基础系统 prompt
    messages.emplace_back(json{
        {"role", "system"},
        {"content", getProcessedPrompt()}
    });

    // 用户定义的 prompt
    if (!bot->getSystemPrompt().empty()) {
        messages.emplace_back(json{
            {"role", "system"},
            {"content", bot->getSystemPrompt()}
        });
    }

    // 2. 在上下文中加入对话历史
    for (auto& it : conversation_history)
        messages.push_back(it);

    // 3. 加入当前状态（作为 user 消息）
    // 不等待 function_calls_executed，每轮都要提供状态
    auto state = bot->generateStateJson();
    CLogger::getInstance()->llm->info("state {}", state.dump());
    json user_message = {
        {"role", "user"},
        {"content", state.dump() }
    };
    messages.push_back(user_message);

    // 把当前状态加入历史
    addToConversationHistory(user_message);

    // 4. 标记为等待中
    is_idle_waiting_llm = true;

    // 5. 异步调用
    using namespace std::placeholders;
    dispatcher->callLLMWithFunctionsAsync(
        messages,
        llm_provider,
        std::bind(&CLLMBotSession::processLLMCallback, this, _1, _2, _3),
        session_id
    );

}

void CLLMBotSession::processLLMCallback(const json &response, const std::string &result_type, const json &function_results) {

    // 解除等待
    is_idle_waiting_llm = false;

    spdlog::info(response.dump());

    if (result_type == "function_calls_executed") {
        CLogger::getInstance()->llm->info("=== LLM Round Complete for Session {} ===", session_id);
        
        // Log LLM's message content
        if (response.contains("choices") && !response["choices"].empty()) {
            json message = response["choices"][0]["message"];
            
            // Log the tool calls if present
            if (message.contains("tool_calls")) {
                CLogger::getInstance()->llm->info("LLM requested {} tool calls", message["tool_calls"].size());
                for (const auto& tool_call : message["tool_calls"]) {
                    CLogger::getInstance()->llm->info("Tool call: {} with args: {}", 
                        tool_call.value("function", json{}).value("name", "unknown"),
                        tool_call.value("function", json{}).value("arguments", "{}"));
                }
                // log function result
                CLogger::getInstance()->llm->info("Function results: {}", function_results.dump());
            }
            std::string content = message["content"].get<std::string>();
            CLogger::getInstance()->llm->info("LLM Message: {}", content);
            
            // 把LLM的函数调用加到上下文中
            addToConversationHistory(message);
        }
        
        // 每次执行完函数之后，CFunctionDispatcher会在response里面加上function_results
        // 此时应该将函数调用的结果加到上下文中
        auto function_messages = CApp::getInstance()->getFunctionDispatcher()->createFunctionCallMessage(function_results);
        for (auto& it : function_messages) {
            addToConversationHistory(it);
        }
    } else if (result_type == "message") {
        CLogger::getInstance()->llm->info("=== LLM Round Complete for Session {} (No Tools) ===", session_id);
        
        // Add the LLM response to conversation history if it contains content
        if (response.contains("choices") && !response["choices"].empty()) {
            json message = response["choices"][0]["message"];
            
            if (message.contains("content") && !message["content"].get<std::string>().empty()) {
                std::string content = message["content"].get<std::string>();
                CLogger::getInstance()->llm->info("LLM Message: {}", content);

                json conversation_entry = {
                    {"role", "assistant"},
                    {"content", message["content"].get<std::string>()}
                };
                addToConversationHistory(conversation_entry);
            }
        }
    } else {
        CLogger::getInstance()->llm->error(
            "LLM update failed for session {}: {}",
            session_id,
            response.value("error", "Unknown error"));
    }
}
