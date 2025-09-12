//
// Created by rain on 8/2/25.
//

#include "CSharedResourcePool.h"
#include <algorithm>

#include "glm/detail/func_geometric.inl"
#include "spdlog/spdlog.h"

CSharedResourcePool::CSharedResourcePool() {
}

CSharedResourcePool::~CSharedResourcePool() {
}

void CSharedResourcePool::addServer(CServer *server) {
    ServerAddress addr = std::make_pair(server->getHost(), server->getPort());
    serverResources[addr] = stServerResources{};
}

void CSharedResourcePool::addPlayer(ServerAddress addr, const stPlayer &player) {
    auto &resources = serverResources[addr];
    std::size_t hash = calHashPlayer(player);

    // Check if player already exists
    if (resources.player_hashes.find(hash) != resources.player_hashes.end()) {
        return; // Already exists, don't add duplicate
    }

    // Check if we have space
    if (resources.playerCount >= resources.players.size()) {
        return; // No more space
    }

    // Add player and its hash
    resources.players[resources.playerCount] = player;
    resources.player_hashes.insert(hash);
    resources.playerCount++;
}

void CSharedResourcePool::addVehicle(ServerAddress addr, const stVehicle &vehicle) {
    auto &resources = serverResources[addr];
    std::size_t hash = calHashVehicle(vehicle);

    // Check if vehicle already exists
    if (resources.vehicle_hashes.find(hash) != resources.vehicle_hashes.end()) {
        return; // Already exists, don't add duplicate
    }

    // Check if we have space
    if (resources.vehicleCount >= resources.vehicles.size()) {
        return; // No more space
    }

    // Add vehicle and its hash
    resources.vehicles[resources.vehicleCount] = vehicle;
    resources.vehicle_hashes.insert(hash);
    resources.vehicleCount++;
}

void CSharedResourcePool::updatePlayer(ServerAddress addr, unsigned short playerID, glm::vec3 position) {
    auto &resources = serverResources[addr];
    
    // Find player by ID and update position
    for (size_t i = 0; i < resources.playerCount; i++) {
        if (resources.players[i].id == playerID) {
            resources.players[i].position = position;
            break;
        }
    }
}

void CSharedResourcePool::updatePlayer(ServerAddress addr, unsigned short playerID, const stOnFootData &onFootData) {
    auto &resources = serverResources[addr];
    
    // Find player by ID and update with onfoot data
    for (size_t i = 0; i < resources.playerCount; i++) {
        if (resources.players[i].id == playerID) {
            resources.players[i].position = glm::vec3{onFootData.fPosition[0], onFootData.fPosition[1], onFootData.fPosition[2]};
            resources.players[i].velocity = glm::vec3{onFootData.fMoveSpeed[0], onFootData.fMoveSpeed[1], onFootData.fMoveSpeed[2]};
            resources.players[i].health = onFootData.byteHealth;
            resources.players[i].armor = onFootData.byteArmor;
            resources.players[i].weapon = onFootData.byteCurrentWeapon;
            resources.players[i].specialAction = onFootData.byteSpecialAction;
            break;
        }
    }
}

void CSharedResourcePool::updateVehicle(ServerAddress addr, unsigned short vehicleID, const stInCarData &inCarData) {
    auto &resources = serverResources[addr];
    
    // Find vehicle by ID and update with incar data
    for (size_t i = 0; i < resources.vehicleCount; i++) {
        if (resources.vehicles[i].id == vehicleID) {
            resources.vehicles[i].position = glm::vec3{inCarData.fPosition[0], inCarData.fPosition[1], inCarData.fPosition[2]};
            resources.vehicles[i].velocity = glm::vec3{inCarData.fMoveSpeed[0], inCarData.fMoveSpeed[1], inCarData.fMoveSpeed[2]};
            resources.vehicles[i].health = inCarData.fVehicleHealth;
            break;
        }
    }
}

void CSharedResourcePool::updateVehicle(ServerAddress addr, unsigned short vehicleID, int modelid, glm::vec3 position) {
    auto &resources = serverResources[addr];
    
    // Find vehicle by ID and update
    for (size_t i = 0; i < resources.vehicleCount; i++) {
        if (resources.vehicles[i].id == vehicleID) {
            resources.vehicles[i].model = modelid;
            resources.vehicles[i].position = position;
            break;
        }
    }
}

void CSharedResourcePool::incrementPlayerStreamCount(ServerAddress addr, int playerID) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.playerCount; i++) {
        if (resources.players[i].id == playerID) {
            resources.players[i].stream_count++;
            break;
        }
    }
}

void CSharedResourcePool::decrementPlayerStreamCount(ServerAddress addr, int playerID) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.playerCount; i++) {
        if (resources.players[i].id == playerID) {
            resources.players[i].stream_count--;
            if (resources.players[i].stream_count <= 0) {
                // Remove player if no bots are streaming it
                std::size_t hash = calHashPlayer(resources.players[i]);
                resources.player_hashes.erase(hash);
                
                if (i < resources.playerCount - 1) {
                    resources.players[i] = resources.players[resources.playerCount - 1];
                }
                resources.playerCount--;
            }
            break;
        }
    }
}

void CSharedResourcePool::incrementVehicleStreamCount(ServerAddress addr, int vehicleID) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.vehicleCount; i++) {
        if (resources.vehicles[i].id == vehicleID) {
            resources.vehicles[i].stream_count++;
            break;
        }
    }
}

void CSharedResourcePool::decrementVehicleStreamCount(ServerAddress addr, int vehicleID) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.vehicleCount; i++) {
        if (resources.vehicles[i].id == vehicleID) {
            resources.vehicles[i].stream_count--;
            if (resources.vehicles[i].stream_count <= 0) {
                // Remove vehicle if no bots are streaming it
                std::size_t hash = calHashVehicle(resources.vehicles[i]);
                resources.vehicle_hashes.erase(hash);
                
                if (i < resources.vehicleCount - 1) {
                    resources.vehicles[i] = resources.vehicles[resources.vehicleCount - 1];
                }
                resources.vehicleCount--;
            }
            break;
        }
    }
}

void CSharedResourcePool::removeServer(ServerAddress addr) {
    serverResources.erase(addr);
}

void CSharedResourcePool::removePlayer(ServerAddress addr, const std::string &playerName) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.playerCount; i++) {
        if (resources.players[i].name == playerName) {
            std::size_t hash = calHashPlayer(resources.players[i]);
            resources.player_hashes.erase(hash);
            
            if (i < resources.playerCount - 1) {
                resources.players[i] = resources.players[resources.playerCount - 1];
            }
            resources.playerCount--;
            break;
        }
    }
}

void CSharedResourcePool::removePlayer(ServerAddress addr, int id) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.playerCount; i++) {
        if (resources.players[i].id == id) {
            std::size_t hash = calHashPlayer(resources.players[i]);
            resources.player_hashes.erase(hash);
            
            if (i < resources.playerCount - 1) {
                resources.players[i] = resources.players[resources.playerCount - 1];
            }
            resources.playerCount--;
            break;
        }
    }
}

void CSharedResourcePool::removeVehicle(ServerAddress addr, int vehicleId) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.vehicleCount; i++) {
        if (resources.vehicles[i].id == vehicleId) {
            std::size_t hash = calHashVehicle(resources.vehicles[i]);
            resources.vehicle_hashes.erase(hash);
            
            if (i < resources.vehicleCount - 1) {
                resources.vehicles[i] = resources.vehicles[resources.vehicleCount - 1];
            }
            resources.vehicleCount--;
            break;
        }
    }
}

void CSharedResourcePool::clearServerResources(ServerAddress addr) {
    auto &resources = serverResources[addr];
    resources.playerCount = 0;
    resources.vehicleCount = 0;
    resources.player_hashes.clear();
    resources.vehicle_hashes.clear();
}

std::string CSharedResourcePool::getPlayerName(ServerAddress addr, int playerid) {
    auto &resources = serverResources[addr];
    
    for (size_t i = 0; i < resources.playerCount; i++) {
        if (resources.players[i].id == playerid) {
            return resources.players[i].name;
        }
    }
    return "";
}

const stServerResources* CSharedResourcePool::getServerResources(ServerAddress addr) const {
    auto it = serverResources.find(addr);
    if (it != serverResources.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<stPlayer> CSharedResourcePool::getPlayersInRange(ServerAddress addr, const glm::vec3 &position, float range, bool npc_included) const {
    std::vector<stPlayer> result;
    auto it = serverResources.find(addr);
    if (it == serverResources.end()) return result;
    
    const auto &resources = it->second;
    float rangeSq = range * range;
    
    for (size_t i = 0; i < resources.playerCount; i++) {
        const auto &player = resources.players[i];
        if (!npc_included && player.is_npc) continue;
        
        glm::vec3 diff = player.position - position;
        if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= rangeSq) {
            result.push_back(player);
        }
    }
    
    return result;
}

std::vector<stPlayer> CSharedResourcePool::getAllPlayer(ServerAddress addr, bool npc_included) const {
    std::vector<stPlayer> result;
    auto it = serverResources.find(addr);
    if (it == serverResources.end()) return result;
    
    const auto &resources = it->second;
    
    for (size_t i = 0; i < resources.playerCount; i++) {
        const auto &player = resources.players[i];
        if (!npc_included && player.is_npc) continue;
        result.push_back(player);
    }
    
    return result;
}

std::vector<stVehicle> CSharedResourcePool::getVehiclesInRange(ServerAddress addr, const glm::vec3 &position, float range) const {
    std::vector<stVehicle> result;
    auto it = serverResources.find(addr);
    if (it == serverResources.end()) return result;
    
    const auto &resources = it->second;
    float rangeSq = range * range;
    
    for (size_t i = 0; i < resources.vehicleCount; i++) {
        const auto &vehicle = resources.vehicles[i];
        glm::vec3 diff = vehicle.position - position;
        if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= rangeSq) {
            result.push_back(vehicle);
        }
    }
    
    return result;
}

std::size_t CSharedResourcePool::calHashPlayer(const stPlayer &player) {
    std::size_t hash = std::hash<int>{}(player.id);
    hash ^= std::hash<std::string>{}(player.name) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

std::size_t CSharedResourcePool::calHashVehicle(const stVehicle &vehicle) {
    std::size_t hash = std::hash<int>{}(vehicle.id);
    hash ^= std::hash<int>{}(vehicle.model) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}