#pragma once

#include "tool_builder.h"
#include "../models/CBot.h"
#include "../core/CSharedResourcePool.h"
#include "CApp.h"
#include <string>

class ToolHelpers {
public:
    static CBot* getBotBySessionId(const std::string& session_id);
    static ServerAddress getServerAddress(CBot* bot);
    static json createError(const std::string& message);
    static json createSuccess(const json& data = json::object());
};