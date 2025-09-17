//
// Created by rain on 8/1/25.
//

#include "CServerService.h"

#include <chrono>
#include <sqlite3.h>

#include "../utils/JsonResponse.h"
#include "../database/DBSchema.h"
#include "../CApp.h"
#include "../core/CPersistentDataStorage.h"
#include <hv/json.hpp>
#include "../database/querybuilder.h"
#include "spdlog/spdlog.h"
#include "../core/CLogger.h"
#include "core/CServerQuerier.h"

using json = nlohmann::json;

CServerService::CServerService(const std::string &uri) : CBaseService(uri) {
}

void CServerService::on_install(HttpService *router) {
    router->POST(getRelativePath("delete").c_str(), CServerService::delete_server);
    router->POST(getRelativePath("add").c_str(), CServerService::add_server);
    router->GET(getRelativePath("list").c_str(), CServerService::list_servers);
    router->POST(getRelativePath("query").c_str(), CServerService::query_server);
}

int CServerService::delete_server(HttpRequest *req, HttpResponse *resp) {
    try {
        json body = json::parse(req->Body());
        if (!body.contains("dbid")) {
            return resp->Json(JsonResponse::not_found());
        }

        int dbid = body["dbid"];
        auto db = CApp::getInstance()->getDatabase()->getDb();

        // Also remove from database
        sql::DeleteModel dm;
        dm.from(DB::Tables::SERVERS).where(DB::Servers::ID + " = " + std::to_string(dbid));
        char *error;
        auto rc = sqlite3_exec(db, dm.str().c_str(), nullptr, nullptr, &error);
        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Failed to delete server: {}", error);
            sqlite3_free(error);
            return resp->Json(JsonResponse::internal_error());
        }

        return resp->Json(JsonResponse::with_success(nullptr, "Server deleted successfully"));
    } catch (const std::exception &e) {
        CLogger::getInstance()->api->error("Error in delete_server: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CServerService::add_server(HttpRequest *req, HttpResponse *resp) {
    try {
        json body = json::parse(req->Body());
        if (!body.contains("host") || !body.contains("port")) {
            return resp->Json(JsonResponse::with_error("Missing host or port"));
        }

        std::string host = body["host"];
        int port = body["port"];

        auto db = CApp::getInstance()->getDatabase()->getDb();

        // Check if server already exists
        sql::SelectModel sm;
        sm.select("id").from(DB::Tables::SERVERS)
                .where(DB::Servers::HOST + " = '" + host + "' AND " +
                       DB::Servers::PORT + " = " + std::to_string(port));

        char *error;
        bool serverExists = false;
        int serverId = -1;

        auto rc = sqlite3_exec(db, sm.str().c_str(),
                               [](void *data, int argc, char **argv, char **colName) -> int {
                                   bool *exists = static_cast<bool *>(data);
                                   *exists = true;
                                   return 0;
                               }, &serverExists, &error);

        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Error checking existing server: {}", error);
            sqlite3_free(error);
            return resp->Json(JsonResponse::internal_error());
        }

        if (serverExists) {
            return resp->Json(JsonResponse::with_error("Server already exists"));
        }

        // Add new server
        std::string query = "INSERT INTO " + DB::Tables::SERVERS + " (" +
                            DB::Servers::HOST + ", " + DB::Servers::PORT + ", " +
                            DB::Servers::NAME + ", " + DB::Servers::GAMEMODE + ", " +
                            DB::Servers::RULE + ", " + DB::Servers::LANGUAGE + ", " +
                            DB::Servers::PLAYERS + ", " + DB::Servers::MAX_PLAYERS + ", " +
                            DB::Servers::PING + ", " + DB::Servers::LAST_UPDATE + ") VALUES ('" + host + "', " +
                            std::to_string(port) + ", '', '', '', '', 0, 0, 0, '')";

        rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &error);
        if (rc != SQLITE_OK) {
            CLogger::getInstance()->api->error("Failed to add server: {}", error);
            sqlite3_free(error);
            return resp->Json(JsonResponse::internal_error());
        }

        // Get the new server ID
        serverId = sqlite3_last_insert_rowid(db);

        // Server created successfully and stored in database

        // Immediately query the server to get its information
        auto serverQuerier = CApp::getInstance()->getServerQuerier();
        if (serverQuerier) {
            // CLogger::getInstance()->api->info("Immediately querying newly added server {}:{}", host, port);

            // Create a temporary server object for querying
            auto server = std::make_shared<CServer>(host, port);
            server->setDbId(serverId);

            // Query server asynchronously
            serverQuerier->queryServer(server.get());
        } else {
            CLogger::getInstance()->api->warn("Server querier not available, server will be queried on next cycle");
        }

        json response;
        response["message"] = "Server added successfully and querying initiated";
        response["server_id"] = serverId;
        return resp->Json(JsonResponse::with_success(response));
    } catch (const std::exception &e) {
        CLogger::getInstance()->api->error("Error in add_server: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CServerService::list_servers(HttpRequest *req, HttpResponse *resp) {
    try {
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }

        // Query servers directly from database
        std::string query = "SELECT * FROM " + DB::Tables::SERVERS;
        json serverList = json::array();

        auto serverLoader = [&serverList](const SQLiteRow &row) -> bool {
            try {
                json serverJson;
                serverJson["id"] = row.getInt(DB::Servers::ID);
                serverJson["host"] = row.getString(DB::Servers::HOST);
                serverJson["port"] = row.getInt(DB::Servers::PORT);
                serverJson["name"] = row.getString(DB::Servers::NAME);
                serverJson["gamemode"] = row.getString(DB::Servers::GAMEMODE);
                serverJson["language"] = row.getString(DB::Servers::LANGUAGE);
                serverJson["rule"] = row.getString(DB::Servers::RULE);
                serverJson["last_update"] = row.getString(DB::Servers::LAST_UPDATE);
                // Dynamic data now stored in DB
                serverJson["players"] = row.getInt(DB::Servers::PLAYERS);
                serverJson["max_players"] = row.getInt(DB::Servers::MAX_PLAYERS);
                serverJson["ping"] = row.getInt(DB::Servers::PING);
                serverList.push_back(serverJson);
                return true;
            } catch (const std::exception &e) {
                CLogger::getInstance()->api->error("Error parsing server record: {}", e.what());
                return true; // Continue processing
            }
        };

        if (!database->executeQuery(query, serverLoader, "listing servers")) {
            return resp->Json(JsonResponse::internal_error());
        }

        json response;
        response["servers"] = serverList;
        response["count"] = serverList.size();

        return resp->Json(JsonResponse::with_success(response));
    } catch (const std::exception &e) {
        CLogger::getInstance()->api->error("Error in list_servers: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CServerService::query_server(HttpRequest *req, HttpResponse *resp) {
    try {
        json body = json::parse(req->Body());
        if (!body.contains("server_id")) {
            return resp->Json(JsonResponse::with_error("Missing server_id parameter"));
        }

        int serverId = body["server_id"];
        auto database = CApp::getInstance()->getDatabase();
        if (!database) {
            return resp->Json(JsonResponse::internal_error());
        }

        // Fetch server details from database
        std::string query = "SELECT * FROM " + DB::Tables::SERVERS + " WHERE " +
                            DB::Servers::ID + " = " + std::to_string(serverId);

        std::shared_ptr<CServer> server = nullptr;
        bool queryExecuted = database->executeQuery(query, [&server](const SQLiteRow &row) -> bool {
            try {
                std::string host = row.getString(DB::Servers::HOST);
                int port = row.getInt(DB::Servers::PORT);
                server = std::make_shared<CServer>(host, port);
                server->setDbId(row.getInt(DB::Servers::ID));
                server->setName(row.getString(DB::Servers::NAME));
                server->setMode(row.getString(DB::Servers::GAMEMODE));
                server->setLanguage(row.getString(DB::Servers::LANGUAGE));
                return true; // Continue processing (though we expect only one result)
            } catch (const std::exception &e) {
                CLogger::getInstance()->api->error("Error loading server: {}", e.what());
                return true; // Continue processing even on error
            }
        }, "fetching server for query");
        
        if (!queryExecuted) {
            return resp->Json(JsonResponse::internal_error());
        }
        if (!server) {
            return resp->Json(JsonResponse::with_error("Server not found"));
        }
        // Get server querier and initiate async query
        auto serverQuerier = CApp::getInstance()->getServerQuerier();
        if (!serverQuerier) {
            return resp->Json(JsonResponse::with_error("Server querier not available"));
        }
        // Query server asynchronously - this will automatically update database
        serverQuerier->queryServer(server.get());

        json response;
        response["message"] = "Server query initiated successfully";
        response["server_id"] = serverId;
        return resp->Json(JsonResponse::with_success(response));
    } catch (const std::exception &e) {
        CLogger::getInstance()->api->error("Error in query_server: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}
