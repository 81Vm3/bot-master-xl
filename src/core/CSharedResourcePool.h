//
// Created by rain on 8/2/25.
//

#ifndef CSHAREDRESOURCEPOOL_H
#define CSHAREDRESOURCEPOOL_H
#include <map>
#include <unordered_set>
#include <vector>
#include <array>

#include "../samp.h"
#include "glm/vec3.hpp"
#include "../models/CServer.h"

struct stPlayer {
    std::string name;
    int id;
    float health, armor;
    glm::vec3 position;
    glm::vec3 velocity;
    bool is_driving;
    int specialAction;
    int currentAnimationID;
    int weapon;
    int vehicle_id;
    int skin;
    bool is_npc;
    bool valid = true;
    int stream_count = 0; // Number of bots streaming this player
};

struct stVehicle {
    int id;
    float health;
    glm::vec3 position;
    glm::vec3 velocity;
    int model;
    bool valid = true;
    int stream_count = 0; // Number of bots streaming this vehicle
};

// dynamic server resources, save them for sharing info despite being out of streaming range
// as long as there's at least one bot around these resources, the resources persist
struct stServerResources {
    //std::vector<stTextdraw> textdraws; // global textdraw
    // Note: pickups, objects, and labels are now handled per-bot via CStreamableResourcePool
    // Only players and vehicles are shared across all bots
    std::array<stPlayer, 2000> players;
    std::array<stVehicle, 2000> vehicles;
    size_t playerCount = 0;
    size_t vehicleCount = 0;

    std::unordered_set<std::size_t> player_hashes;
    std::unordered_set<std::size_t> vehicle_hashes;
};

using ServerAddress = std::pair<std::string, int>;

class CSharedResourcePool {
public:
    CSharedResourcePool();
    ~CSharedResourcePool();
    std::map<ServerAddress, stServerResources> serverResources;
    void addServer(CServer* server);

    void addPlayer(ServerAddress addr, const stPlayer& player);
    void addVehicle(ServerAddress addr, const stVehicle& vehicle);

    void updatePlayer(ServerAddress addr, unsigned short playerID, glm::vec3 position);
    void updatePlayer(ServerAddress addr, unsigned short playerID, const stOnFootData& onFootData);
    void updateVehicle(ServerAddress addr, unsigned short vehicleID, const stInCarData& inCarData);
    void updateVehicle(ServerAddress addr, unsigned short vehicleID, int modelid, glm::vec3 position);

    void incrementPlayerStreamCount(ServerAddress addr, int playerID);
    void decrementPlayerStreamCount(ServerAddress addr, int playerID);
    void incrementVehicleStreamCount(ServerAddress addr, int vehicleID);
    void decrementVehicleStreamCount(ServerAddress addr, int vehicleID);

    // Resource removal methods
    void removeServer(ServerAddress addr);
    void removePlayer(ServerAddress addr, const std::string& playerName);
    void removePlayer(ServerAddress addr, int id);
    void removeVehicle(ServerAddress addr, int vehicleId);
    void clearServerResources(ServerAddress addr);

    std::string getPlayerName(ServerAddress addr, int playerid);

    // Resource query methods
    const stServerResources* getServerResources(ServerAddress addr) const;
    std::vector<stPlayer> getPlayersInRange(ServerAddress addr, const glm::vec3& position, float range, bool npc_included) const;
    std::vector<stPlayer> getAllPlayer(ServerAddress addr, bool npc_included) const; //获取服务器全部玩家
    std::vector<stVehicle> getVehiclesInRange(ServerAddress addr, const glm::vec3& position, float range) const;
private:
    std::size_t calHashPlayer(const stPlayer& player);
    std::size_t calHashVehicle(const stVehicle& vehicle);
};

#endif //CSHAREDRESOURCEPOOL_H
