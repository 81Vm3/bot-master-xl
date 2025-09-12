//
// Created by rain on 8/3/25.
//

#include "CDashboardService.h"

#include <chrono>
#include <sqlite3.h>

#include "../utils/JsonResponse.h"
#include "../utils/timeutil.h"
#include "../database/DBSchema.h"
#include "../CApp.h"
#include "../core/CPersistentDataStorage.h"
#include "../models/CBot.h"
#include <hv/json.hpp>
#include "spdlog/spdlog.h"
#include "../core/CLogger.h"

using json = nlohmann::json;

CDashboardService::CDashboardService(const std::string& uri) : CBaseService(uri) {

}

void CDashboardService::on_install(HttpService *router) {
    router->GET(getRelativePath("runtime").c_str(), CDashboardService::get_runtime);
    router->GET(getRelativePath("bot_stats").c_str(), CDashboardService::get_bot_stats);
    router->GET(getRelativePath("server_stats").c_str(), CDashboardService::get_server_stats);
}

int CDashboardService::get_runtime(HttpRequest* req, HttpResponse* resp) {
    try {
        auto app = CApp::getInstance();
        if (!app) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        auto uptime = app->getUptime();
        auto uptimeSeconds = uptime.count() / 1000.0;
        
        // Calculate days, hours, minutes, seconds
        int days = static_cast<int>(uptimeSeconds / 86400);
        int hours = static_cast<int>((uptimeSeconds - days * 86400) / 3600);
        int minutes = static_cast<int>((uptimeSeconds - days * 86400 - hours * 3600) / 60);
        int seconds = static_cast<int>(uptimeSeconds) % 60;
        
        json runtime_data = {
            {"uptime_ms", uptime.count()},
            {"uptime_seconds", uptimeSeconds},
            {"formatted", {
                {"days", days},
                {"hours", hours},
                {"minutes", minutes},
                {"seconds", seconds}
            }},
            {"uptime_string", std::to_string(days) + "d " + 
                             std::to_string(hours) + "h " + 
                             std::to_string(minutes) + "m " + 
                             std::to_string(seconds) + "s"}
        };
        
        return resp->Json(JsonResponse::with_success(runtime_data, "Runtime retrieved successfully"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in get_runtime: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CDashboardService::get_bot_stats(HttpRequest* req, HttpResponse* resp) {
    try {
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        const auto& bots = database->vBots;
        
        int total_bots = bots.size();
        int connected_bots = 0;
        
        // Count connected bots
        for (const auto& bot : bots) {
            if (bot && bot->isConnected()) {
                connected_bots++;
            }
        }
        
        json bot_stats = {
            {"total", total_bots},
            {"connected", connected_bots},
            {"disconnected", total_bots - connected_bots}
        };
        
        return resp->Json(JsonResponse::with_success(bot_stats, "Bot statistics retrieved successfully"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in get_bot_stats: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CDashboardService::get_server_stats(HttpRequest* req, HttpResponse* resp) {
    try {
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        // Query servers directly from database
        auto db = database->getDb();
        int total_servers = 0;
        int online_servers = 0;
        int offline_servers = 0;
        
        std::string query = "SELECT " + DB::Servers::LAST_UPDATE + " FROM " + DB::Tables::SERVERS;
        
        auto serverCounter = [&](const SQLiteRow& row) -> bool {
            total_servers++;
            
            try {
                // Check if server is online based on last update time
                // If last update was within last 5 minutes, consider it online
                std::string lastUpdateStr = row.getString(DB::Servers::LAST_UPDATE);
                
                if (!lastUpdateStr.empty()) {
                    if (TimeUtil::isWithinMinutes(lastUpdateStr, 5)) {
                        online_servers++;
                    } else {
                        offline_servers++;
                    }
                } else {
                    offline_servers++;
                }
            } catch (const std::exception& e) {
                // If we can't parse the timestamp, consider server offline
                CLogger::getInstance()->api->warn("Failed to parse server last update time: {}", e.what());
                offline_servers++;
            }
            
            return true; // Continue processing
        };
        
        if (!database->executeQuery(query, serverCounter, "counting servers")) {
            return resp->Json(JsonResponse::internal_error());
        }
        
        json server_stats = {
            {"total", total_servers},
            {"online", online_servers},
            {"offline", offline_servers}
        };
        
        return resp->Json(JsonResponse::with_success(server_stats, "Server statistics retrieved successfully"));
    } catch (const std::exception& e) {
        CLogger::getInstance()->api->error("Error in get_server_stats: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}