#pragma once

#include "tool_builder.h"

class BotLLMTools {
public:
    static void registerAllBotTools();

private:
    static void registerToolsWithDispatcher(const std::vector<tool>& tools);
};