#include "BotLLMTools.h"
#include "CApp.h"

#include "SelfStatusTools.h"
#include "SituationAwarenessTools.h"
#include "WorldInteractionTools.h"

#include "utils/CFunctionDispatcher.h"
#include "core/CLogger.h"

void BotLLMTools::registerAllBotTools() {
    registerToolsWithDispatcher(SelfStatusTools::createAllTools());
    registerToolsWithDispatcher(SituationAwarenessTools::createAllTools());
    registerToolsWithDispatcher(WorldInteractionTools::createAllTools());
}

void BotLLMTools::registerToolsWithDispatcher(const std::vector<tool>& tools) {
    auto dispatcher = CApp::getInstance()->getFunctionDispatcher();
    
    for (const auto& tool : tools) {
        dispatcher->registerFunction(
            tool.name,
            tool.description,
            tool.parameters_schema,
            tool.handler
        );
    }
}