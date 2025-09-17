//
// Created by rain on 8/1/25.
//

#include "CServerQuerier.h"
#include "CPersistentDataStorage.h"
#include "../database/querybuilder.h"
#include "../database/DBSchema.h"
#include <spdlog/spdlog.h>

#include "Bullet3OpenCL/RigidBody/kernels/solverUtils.h"

CServerQuerier::CServerQuerier() 
    : pDataStorage(nullptr), running(false), queryInterval(std::chrono::seconds(30)) {
}

CServerQuerier::~CServerQuerier() {
    stop();
}

void CServerQuerier::initialize(CPersistentDataStorage* dataStorage) {
    pDataStorage = dataStorage;
}

void CServerQuerier::start() {
    if (running.load() || !pDataStorage) {
        return;
    }
    
    running.store(true);
    queryThread = std::make_unique<std::thread>(&CServerQuerier::queryLoop, this);
}

void CServerQuerier::stop() {
    if (!running.load()) {
        return;
    }
    
    running.store(false);
    if (queryThread && queryThread->joinable()) {
        queryThread->join();
    }
    queryThread.reset();
    spdlog::info("CServerQuerier stopped");
}

bool CServerQuerier::isRunning() const {
    return running.load();
}

void CServerQuerier::setQueryInterval(std::chrono::seconds interval) {
    queryInterval = interval;
}

std::chrono::seconds CServerQuerier::getQueryInterval() const {
    return queryInterval;
}

void CServerQuerier::setOnServerUpdated(std::function<void(CServer*)> callback) {
    onServerUpdated = std::move(callback);
}

void CServerQuerier::setOnServerOffline(std::function<void(CServer*)> callback) {
    onServerOffline = std::move(callback);
}

void CServerQuerier::queryLoop() {
    while (running.load()) {
        if (!pDataStorage) {
            std::this_thread::sleep_for(queryInterval);
            continue;
        }
        
        // Query servers directly from database since vServers is removed
        std::vector<std::shared_ptr<CServer>> servers;
        std::string query = "SELECT * FROM " + DB::Tables::SERVERS;
        
        auto serverLoader = [&servers](const SQLiteRow& row) -> bool {
            try {
                int id = row.getInt(DB::Servers::ID);
                std::string host = row.getString(DB::Servers::HOST);
                int port = row.getInt(DB::Servers::PORT);
                std::string name = row.getString(DB::Servers::NAME);
                std::string gamemode = row.getString(DB::Servers::GAMEMODE);
                std::string language = row.getString(DB::Servers::LANGUAGE);
                std::string lastUpdate = row.getString(DB::Servers::LAST_UPDATE);
                
                if (host.empty() || port <= 0) {
                    return true; // Skip invalid servers
                }
                
                auto server = std::make_shared<CServer>(host, port);
                server->setDbId(id);
                server->setName(name);
                server->setMode(gamemode);
                server->setLanguage(language);
                server->setLastUpdate(lastUpdate);
                
                servers.push_back(std::move(server));
                return true;
            } catch (const std::exception& e) {
                spdlog::error("Error loading server for querying: {}", e.what());
                return true; // Continue processing
            }
        };
        
        if (!pDataStorage->executeQuery(query, serverLoader, "loading servers for querying")) {
            std::this_thread::sleep_for(queryInterval);
            continue;
        }
        
        spdlog::debug("Querying {} servers", servers.size());
        
        // Query each server asynchronously
        for (auto& server : servers) {
            if (!running.load()) {
                break;
            }
            queryServer(server.get());
        }
        
        // Wait for the specified interval
        auto sleepStart = std::chrono::steady_clock::now();
        while (running.load() && 
               std::chrono::steady_clock::now() - sleepStart < queryInterval) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void CServerQuerier::queryServer(CServer* server) {
    if (!server) {
        return;
    }
    // Query server info asynchronously
    server->queryServerInfoAsync([this, server](bool success) {
        if (success) {
            // Update last_update timestamp
            server->setLastUpdate(CPersistentDataStorage::getCurrentTimeString());
            
            // Update in database (including player count)
            updateServerInDatabase(server);
            
            // Call callback if set
            if (onServerUpdated) {
                onServerUpdated(server);
            }

        } else {
            // Call offline callback if set
            if (onServerOffline) {
                onServerOffline(server);
            }

            spdlog::debug("Server {}:{} is offline or unreachable",
                         server->getHost(), server->getPort());
        }
    });
}

void CServerQuerier::updateServerInDatabase(CServer* server) {
    if (!pDataStorage || !server || server->getDbId() <= 0) {
        return;
    }
    
    using namespace sql;
    
    try {
        UpdateModel um;
        um.update(DB::Tables::SERVERS)
          .set(DB::Servers::NAME, server->getName())
          .set(DB::Servers::GAMEMODE, server->getMode())
          .set(DB::Servers::LANGUAGE, server->getLanguage())
          .set(DB::Servers::PLAYERS, std::to_string(server->getPlayers()))
          .set(DB::Servers::MAX_PLAYERS, std::to_string(server->getMaxPlayers()))
          .set(DB::Servers::LAST_UPDATE, server->getLastUpdate())
          .where(DB::Servers::ID + " = " + std::to_string(server->getDbId()));
        
        char* error = nullptr;
        int rc = sqlite3_exec(pDataStorage->getDb(), um.str().c_str(), nullptr, nullptr, &error);
        
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to update server in database: {}", error);
            sqlite3_free(error);
        } else {
            spdlog::debug("Updated server {} in database", server->getDbId());
        }
    } catch (const std::exception& e) {
        spdlog::error("Exception updating server in database: {}", e.what());
    }
}