#include "WorldInteractionTools.h"
#include <glm/glm.hpp>
#include "core/CLogger.h"
#include "physics/Raycast.h"
#include "utils/defines.h"
#include "utils/math_function.h"

std::vector<tool> WorldInteractionTools::createAllTools() {
    return {
        // goto with path finder
        // in ai's view, walk is normal
        // but in gta, walk is pretty slow and no player choose to walk with their alt key pressed all the time
        tool_builder("goto")
        .with_description("Move to specified coordinates within 150m")
        .with_number_param("x", "X coordinate", true)
        .with_number_param("y", "Y coordinate", true)
        .with_number_param("z", "Z coordinate", true)
        .with_string_param("move_type", "Movement type: walk, run", true)
        .with_number_param("radius", "Arrival radius", false)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            if (!args.contains("x") || !args.contains("y") || !args.contains("z")) {
                return ToolHelpers::createError("X, Y, Z coordinates required");
            }

            float x = args["x"];
            float y = args["y"];
            float z = args["z"];
            
            float speed = MOVE_SPEED_RUN; // Default to run
            int type = MOVE_SPEED_RUN;
            if (args.contains("move_type")) {
                std::string move_type = args["move_type"];
                if (move_type == "walk") {
                    speed = MOVE_SPEED_RUN;
                    type = MOVE_TYPE_RUN;
                } else if (move_type == "run") {
                    speed = MOVE_SPEED_SPRINT;
                    type = MOVE_TYPE_SPRINT;
                } else {
                    return ToolHelpers::createError("Invalid move_type. Use 'walk', 'run'");
                }
            }
            
            float radius = args.contains("radius") ? (float)args["radius"] : 1.0f;
            glm::vec3 destination(x, y, z);
            bot->go_with_path(destination, type, speed);
            return ToolHelpers::createSuccess({{"destination", {{"x", x}, {"y", y}, {"z", z}}}});
        })
        .build(),

        tool_builder("forced_goto")
        .with_description("Move to specified coordinates within 150m, ignoring collision")
        .with_number_param("x", "X coordinate", true)
        .with_number_param("y", "Y coordinate", true)
        .with_number_param("z", "Z coordinate", true)
        .with_string_param("move_type", "Movement type: walk, run", true)
        .with_number_param("radius", "Arrival radius", false)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }

            if (!args.contains("x") || !args.contains("y") || !args.contains("z")) {
                return ToolHelpers::createError("X, Y, Z coordinates required");
            }

            float x = args["x"];
            float y = args["y"];
            float z = args["z"];

            float speed = MOVE_SPEED_RUN; // Default to run
            int type = MOVE_SPEED_RUN;
            if (args.contains("move_type")) {
                std::string move_type = args["move_type"];
                if (move_type == "walk") {
                    speed = MOVE_SPEED_RUN;
                    type = MOVE_TYPE_RUN;
                } else if (move_type == "run") {
                    speed = MOVE_SPEED_SPRINT;
                    type = MOVE_TYPE_SPRINT;
                } else {
                    return ToolHelpers::createError("Invalid move_type. Use 'walk', 'run'");
                }
            }
            float radius = args.contains("radius") ? (float)args["radius"] : 1.0f;
            glm::vec3 destination(x, y, z);
            bot->go(destination, type, radius, true, speed, 0.0f, 0);
            return ToolHelpers::createSuccess({{"destination", {{"x", x}, {"y", y}, {"z", z}}}});
        })
        .build(),

        tool_builder("random_explore")
        .with_description("Move to a random 3D position nearby")
        .with_string_param("move_type", "Movement type: walk, run", true)
        .with_number_param("dist", "Exploration distance", true)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }

            if (!args.contains("dist")) {
                return ToolHelpers::createError("Distance parameter required");
            }
            float distance = args["dist"];
            if (distance > 150) {
                return ToolHelpers::createError("Distance parameter out of range");
            }

            float speed = MOVE_SPEED_RUN; // Default to run
            int type = MOVE_SPEED_RUN;
            if (args.contains("move_type")) {
                std::string move_type = args["move_type"];
                if (move_type == "walk") {
                    speed = MOVE_SPEED_RUN;
                    type = MOVE_TYPE_RUN;
                } else if (move_type == "run") {
                    speed = MOVE_SPEED_SPRINT;
                    type = MOVE_TYPE_SPRINT;
                } else {
                    return ToolHelpers::createError("Invalid move_type. Use 'walk', 'run'");
                }
            }

            auto pos = bot->getPosition();
            float x = pos.x + RandomFloat(-distance, distance), y = pos.y + RandomFloat(-distance, distance);
            float z = raycast_ground_z(x, y);
            bot->go_with_path({x, y, z}, type, speed);
            return ToolHelpers::createSuccess({{"destination", {{"x", x}, {"y", y}, {"z", z}}}});
        })
        .build(),

        tool_builder("chat")
        .with_description("Type/Send a chat message")
        .with_string_param("msg", "Message to send in chat", true)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            if (!args.contains("msg")) {
                return ToolHelpers::createError("Message parameter required");
            }
            
            std::string message = args["msg"];
            bot->sendChat(message);
            
            return ToolHelpers::createSuccess({{"message_sent", message}});
        })
        .build(),

        tool_builder("command")
        .with_description("Type/Send a command (like /help etc.)")
        .with_string_param("cmd", "Command to execute", true)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            if (!args.contains("cmd")) {
                return ToolHelpers::createError("Command parameter required");
            }
            
            std::string command = args["cmd"];
            // Prepend '/' if not present
            if (!command.empty() && command[0] != '/') {
                command = "/" + command;
            }
            bot->sendChat(command);
            
            return ToolHelpers::createSuccess({{"command_sent", command}});
        })
        .build(),

        tool_builder("dialog_response")
        .with_description("Respond to an active dialog")
        .with_boolean_param("button", "True for left button, false for right button", true)
        .with_number_param("listitem", "Selected list item index (-1 if none), required for dialog type of list_box", false)
        .with_string_param("input", "Input text for input dialogs", false)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            if (!bot->isDialogActive()) {
                return ToolHelpers::createError("No active dialog to respond to");
            }
            
            if (!args.contains("button")) {
                return ToolHelpers::createError("Button parameter required");
            }
            
            bool leftButton = args["button"];
            int listitem = args.contains("listitem") ? (int)args["listitem"] : -1;
            std::string input = args.contains("input") ? (std::string)args["input"] : "";
            
            bot->sendDialogResponse(leftButton, input, listitem);
            
            return ToolHelpers::createSuccess({
                {"button_clicked", leftButton ? "left" : "right"},
                {"listitem", listitem},
                {"input", input}
            });
        })
        .build(),

        tool_builder("send_pickup")
        .with_description("Pick up an item by pickup ID within 3m")
        .with_number_param("pickup_id", "ID of the pickup to collect", true)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            
            if (!args.contains("pickup_id")) {
                return ToolHelpers::createError("Pickup ID parameter required");
            }

            int pickupId = args["pickup_id"];
            float dis = glm::distance(bot->getStreamableResources().getPickupPosition(pickupId), bot->getPosition());
            if (dis > 3) {
                return ToolHelpers::createError(fmt::format("Pickup is too far, distance: {:.2f}", dis));
            }

            bot->sendPickup(pickupId);
            return ToolHelpers::createSuccess({{"pickup_id", pickupId}});
        })
        .build(),
    };
}
