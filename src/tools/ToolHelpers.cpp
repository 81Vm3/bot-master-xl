#include "ToolHelpers.h"

#include "core/CLLMBotSessionManager.h"

CBot* ToolHelpers::getBotBySessionId(const std::string& session_id) {
    if (session_id.empty()) {
        return nullptr;
    }
    return CApp::getInstance()->getLLMSessionManager()->getBotFromLLMSession(session_id);
}

ServerAddress ToolHelpers::getServerAddress(CBot* bot) {
    return std::make_pair(bot->getHost(), bot->getPort());
}

json ToolHelpers::createError(const std::string& message) {
    return {{"error", message}};
}

json ToolHelpers::createSuccess(const json& data) {
    json result = {{"success", true}};
    if (!data.is_null() && !data.empty()) {
        result["data"] = data;
    }
    return result;
}