#include "SituationAwarenessTools.h"
#include <glm/glm.hpp>
#include <cmath>

#include "utils/ObjectNameUtil.h"
#include "utils/weapon_config.h"

// Helper function to round a float to two decimal places
float round_to_two_places(float val) {
    return std::round(val * 100.0f) / 100.0f;
}

std::vector<tool> SituationAwarenessTools::createAllTools() {
    return {
        tool_builder("list_vehicles")
        .with_description("List all vehicles within 300m")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }

            float distance = 300.0f;
            glm::vec3 botPos = bot->getPosition();
            ServerAddress serverAddr = ToolHelpers::getServerAddress(bot);

            auto resourceManager = CApp::getInstance()->getResourceManager();
            auto vehicles = resourceManager->getVehiclesInRange(serverAddr, botPos, distance);

            json vehicle_array = json::array();
            for (const auto& vehicle : vehicles) {
                json vehicle_element = {
                    {"id", vehicle.id},
                    {"model_id", vehicle.model},
                    {"model_name", CApp::getInstance()->getObjectNameUtil()->getObjectName(vehicle.model)},
                    {"position", {{"x", round_to_two_places(vehicle.position.x)}, {"y", round_to_two_places(vehicle.position.y)}, {"z", round_to_two_places(vehicle.position.z)}}},
                    {"velocity", {{"x", round_to_two_places(vehicle.velocity.x)}, {"y", round_to_two_places(vehicle.velocity.y)}, {"z", round_to_two_places(vehicle.velocity.z)}}},
                    {"health", round_to_two_places(vehicle.health)}
                };
                // Get labels attached to this vehicle using O(1) hashmap query
                auto attachedLabels = bot->getStreamableResources().getLabelsAttachedToVehicle(vehicle.id);
                json label_array = json::array();

                // 只需要提供文字即可
                for (const auto& label : attachedLabels) {
                    label_array.push_back(label.text);
                }
                if (!label_array.empty())
                    vehicle_element["attached_labels"] = label_array;

                vehicle_array.push_back(vehicle_element);
            }
            return ToolHelpers::createSuccess({{"vehicles", vehicle_array}});
        })
        .build(),

        tool_builder("list_players")
        .with_description("List all players within 300m")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            float distance = 300.0f;
            glm::vec3 botPos = bot->getPosition();
            ServerAddress serverAddr = ToolHelpers::getServerAddress(bot);

            auto resourceManager = CApp::getInstance()->getResourceManager();
            auto players = resourceManager->getPlayersInRange(serverAddr, botPos, distance, true);

            json player_array = json::array();
            for (const auto& player : players) {
                json player_element = {
                    {"id", player.id},
                    {"name", player.name},
                    {"position", {{"x", round_to_two_places(player.position.x)}, {"y", round_to_two_places(player.position.y)}, {"z", round_to_two_places(player.position.z)}}},
                    {"velocity", {{"x", round_to_two_places(player.velocity.x)}, {"y", round_to_two_places(player.velocity.y)}, {"z", round_to_two_places(player.velocity.z)}}},
                    {"health", round_to_two_places(player.health)},
                    {"armor", round_to_two_places(player.armor)},
                    {"weapon", WeaponConfig::GetWeaponName(player.weapon)},
                    {"skin", player.skin},
                    {"is_npc", player.is_npc}
                };
                // Get labels attached to this player using O(1) hashmap query
                auto attachedLabels = bot->getStreamableResources().getLabelsAttachedToPlayer(player.id);
                json label_array = json::array();
                for (const auto& label : attachedLabels) {
                    label_array.push_back(label.text);
                }
                if (!label_array.empty())
                    player_element["attached_labels"] = label_array;
                
                player_array.push_back(player_element);
            }

            return ToolHelpers::createSuccess({{"players", player_array}});
        })
        .build(),

        tool_builder("list_objects")
        .with_description("List all objects within 300m")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }

            float distance = 300.0f;
            glm::vec3 botPos = bot->getPosition();
            ServerAddress serverAddr = ToolHelpers::getServerAddress(bot);

            auto resourceManager = CApp::getInstance()->getResourceManager();
            auto objects = bot->getStreamableResources().getObjectsInRange(botPos, distance);

            // Sort objects by distance and limit to 100 nearest
            std::vector<std::pair<float, decltype(objects)::value_type>> objectsWithDistance;
            for (const auto& obj : objects) {
                float dist = glm::distance(botPos, obj.position);
                objectsWithDistance.emplace_back(dist, obj);
            }
            
            std::sort(objectsWithDistance.begin(), objectsWithDistance.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });
            
            // Limit to 100 nearest objects
            size_t maxObjects = std::min(static_cast<size_t>(100), objectsWithDistance.size());

            json object_array = json::array();
            for (size_t i = 0; i < maxObjects; ++i) {
                const auto& obj = objectsWithDistance[i].second;
                // 在AI作为玩家的视角里面，知道obj id没有意义
                json object_element = {
                    {"model_name", CApp::getInstance()->getObjectNameUtil()->getObjectName(obj.model)},
                    {"position", {{"x", round_to_two_places(obj.position.x)}, {"y", round_to_two_places(obj.position.y)}, {"z", round_to_two_places(obj.position.z)}}}
                };
                // Get labels near this object (within 2.0 units) using spatial search
                auto nearbyLabels = bot->getStreamableResources().getLabelsInRange(obj.position, 2.0f);
                json label_array = json::array();
                for (const auto& label : nearbyLabels) {
                    label_array.push_back(label.text);
                }
                if (!label_array.empty())
                    object_element["attached_labels"] = label_array;

                object_array.push_back(object_element);
            }

            return ToolHelpers::createSuccess({{"objects", object_array}});
        })
        .build(),

        tool_builder("list_objects_text")
        .with_description("List all objects with text within 300m")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }

            float distance = 300.0f;
            glm::vec3 botPos = bot->getPosition();
            ServerAddress serverAddr = ToolHelpers::getServerAddress(bot);

            auto resourceManager = CApp::getInstance()->getResourceManager();
            auto objects = bot->getStreamableResources().getObjectsInRange(botPos, distance);

            json object_array = json::array();
            for (const auto& obj : objects) {
                // Only include objects that have material text
                if (!obj.materialText.empty()) {
                    object_array.push_back({
                        {"model_name", CApp::getInstance()->getObjectNameUtil()->getObjectName(obj.model)},
                        {"position", {{"x", round_to_two_places(obj.position.x)}, {"y", round_to_two_places(obj.position.y)}, {"z", round_to_two_places(obj.position.z)}}},
                        {"text", obj.materialText}
                    });
                }
            }
            return ToolHelpers::createSuccess({{"objects_with_text", object_array}});
        })
        .build(),

        tool_builder("list_pickups")
        .with_description("List all pickups within 300m")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }

            float distance = 300.0f;
            glm::vec3 botPos = bot->getPosition();
            ServerAddress serverAddr = ToolHelpers::getServerAddress(bot);

            auto resourceManager = CApp::getInstance()->getResourceManager();
            auto pickups = bot->getStreamableResources().getPickupsInRange(bot->getPosition(), distance);

            json pickup_array = json::array();
            for (const auto& pickup : pickups) {
                json pickup_element = {
                    {"id", pickup.id},
                    {"model_name", CApp::getInstance()->getObjectNameUtil()->getObjectName(pickup.model)},
                    {"position", {{"x", round_to_two_places(pickup.position.x)}, {"y", round_to_two_places(pickup.position.y)}, {"z", round_to_two_places(pickup.position.z)}}}
                };
                // check nearest labels for 3d space
                auto labels = bot->getStreamableResources().getLabelsInRangeLinear(pickup.position, 2.0);
                json label_array = json::array();
                for (auto& label : labels) {
                    label_array.push_back(label.text);
                }
                if (!label_array.empty())
                    pickup_element["attached_labels"] = label_array;

                pickup_array.push_back(pickup_element);
            }
            return ToolHelpers::createSuccess({{"pickups", pickup_array}});
        })
        .build(),

        tool_builder("list_labels")
        .with_description("List all 3D text labels within 300m")
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }

            float distance = 300.0f;
            glm::vec3 botPos = bot->getPosition();
            ServerAddress serverAddr = ToolHelpers::getServerAddress(bot);

            auto resourceManager = CApp::getInstance()->getResourceManager();
            auto labels = bot->getStreamableResources().getLabelsInRange(bot->getPosition(), distance);

            json label_array = json::array();

            for (const auto& label : labels) {
                try {
                    // Validate label data before adding to JSON
                    std::string safe_text = label.text;
                    if (safe_text.empty()) {
                        safe_text = "[empty]";
                    }
                    label_array.push_back({
                        {"id", label.id},
                        {"text", safe_text},
                        {"position", {{"x", round_to_two_places(label.position.x)}, {"y", round_to_two_places(label.position.y)}, {"z", round_to_two_places(label.position.z)}}},
                        {"attached_player", label.attachedPlayer},
                        {"attached_vehicle", label.attachedVehicle}
                    });
                } catch (const std::exception& e) {
                    spdlog::error("Error processing label {}: {}", label.id, e.what());
                    // Skip this label and continue
                }
            }
            return ToolHelpers::createSuccess({{"labels", label_array}});
        })
        .build(),

        tool_builder("list_server_player")
        .with_description("List all players in the server")
        .with_boolean_param("npc_included", "Whether to include server NPCs into your search or not", false)
        .with_function([](const json& args, const std::string& session_id) -> json {
            CBot* bot = ToolHelpers::getBotBySessionId(session_id);
            if (!bot) {
                return ToolHelpers::createError("Bot not found for session");
            }
            auto npc_included = args.contains("npc_included") ? (bool)args["npc_included"] : false;
            auto resourceManager = CApp::getInstance()->getResourceManager();

            ServerAddress serverAddr = ToolHelpers::getServerAddress(bot);
            auto players = resourceManager->getAllPlayer(serverAddr, npc_included);
            json player_array = json::array();
            for (const auto& player : players) {
                player_array.push_back({
                    {"id", player.id},
                    {"name", player.name},
                    {"is_npc", player.is_npc}
                });
            }
            return ToolHelpers::createSuccess({{"players", player_array}});
        })
        .build()
    };
}