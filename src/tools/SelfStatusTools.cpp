#include "SelfStatusTools.h"
#include <glm/glm.hpp>

std::vector<tool> SelfStatusTools::createAllTools() {
    return {
        tool_builder("get_position")
        .with_description("Get the bot's current position coordinates")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            glm::vec3 pos = bot->getPosition();
            return ToolHelpers::createSuccess({
                {"x", pos.x},
                {"y", pos.y},
                {"z", pos.z}
            });
        })
        .build(),

        tool_builder("get_password")
        .with_description("Get the bot's server password")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            return ToolHelpers::createSuccess({{"password", bot->getPassword()}});
        })
        .build(),

        tool_builder("get_self_status")
        .with_description("Get comprehensive bot status information")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            glm::vec3 pos = bot->getPosition();
            glm::vec3 vel = bot->getVelocity();
            
            return ToolHelpers::createSuccess({
                {"name", bot->getName()},
                {"player_id", bot->getPlayerID()},
                {"status", bot->getStatusName()},
                {"position", {{"x", pos.x}, {"y", pos.y}, {"z", pos.z}}},
                {"velocity", {{"x", vel.x}, {"y", vel.y}, {"z", vel.z}}},
                {"health", bot->getHealth()},
                {"armor", bot->getArmor()},
                {"angle", bot->getAngle()},
                {"is_moving", bot->getFlag(CBot::IS_MOVING)},
                {"is_dead", bot->getFlag(CBot::IS_DEAD)},
                {"is_driving", bot->getFlag(CBot::IS_DRIVING)},
                {"is_connected", bot->isConnected()},
                {"dialog_active", bot->isDialogActive()}
            });
        })
        .build(),

        tool_builder("get_chatbox_history")
        .with_description("Get unread chat messages from the chatbox")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            auto messages = bot->getUnreadChatMessage();
            json chat_array = json::array();
            
            if (messages) {
                for (const auto& msg : *messages) {
                    chat_array.push_back(msg);
                }
                bot->clearUnreadChatMessage();
            }
            
            return ToolHelpers::createSuccess({{"messages", chat_array}});
        })
        .build()
    };
}