//
// Created by rain on 8/1/25.
//

#ifndef CRAKBOT_H
#define CRAKBOT_H

#include <string>
#include "BitStream.h"
#include "RakClientInterface.h"
#include "RakClient.h"
#include "../samp.h"
#include "../core/CStreamableResourcePool.h"

class CRakBot {
public:
    // === Type Definitions ===
    enum eBotStatus {
        DISCONNECTED,
        CONNECTING,
        WAIT_FOR_JOIN,
        CONNECTED,
        SPAWNED
    };

    // === Construction/Destruction ===
    CRakBot(std::string identifier);
    virtual ~CRakBot();

    // === Connection Management ===
    void connect(std::string host, unsigned short port);
    void connect();
    void disconnect();
    bool isConnected();
    bool checkConectionDelay(); // 检查连接延迟，如果过了4秒则满足

    // === Configuration ===
    void setHost(const std::string& host);
    void setPort(int port);
    std::string getHost();
    int getPort();

    // === State Access ===
    std::string getName();
    unsigned short getPlayerID();
    int getStatus();
    std::string getStatusName();
    bool isGameInited();

    // === Data Modification ===
    void setName(const std::string& newName);
    void setUuid(const std::string& newUuid);
    void setPlayerID(unsigned short newPlayerID);
    void setStatus(int newStatus);
    void setReconnectTick(unsigned int newReconnectTick);
    void setUpdateTick(unsigned int newUpdateTick);

    // === Advanced Getters ===
    std::string getUuid() const;
    unsigned int getReconnectTick() const;
    unsigned int getUpdateTick() const;
    CStreamableResourcePool& getStreamableResources() { return streamableResources; }
    const CStreamableResourcePool& getStreamableResources() const { return streamableResources; }

    // === Network Communication ===
    void sendRPC(int id, RakNet::BitStream *bs);
    void sendAuthInfo(Packet *packet);
    void sendClientJoin(Packet *packet);
    void sendBullet(stBulletData *data);
    void sendAim(stAimData *data);
    void sendInCar(stInCarData *data);
    void sendEnterVehicle(int vehicleid);
    void sendClassRequest(int classid);
    void sendSpawnRequest();
    void sendChat(std::string text);
    void sendSpawn();
    void sendOnfootSync(stOnFootData *onfoot);

    // === Virtual Event Handlers ===
    virtual void process();
    virtual void on_receive_rpc(int id, RakNet::BitStream *bs);
    
    // === Packet Callbacks ===
    virtual void on_bullet_data(stBulletData *data);
    virtual void on_onfoot_data(stOnFootData *data);
    virtual void on_incar_data(stInCarData *data);
    virtual void on_passenger_data(stPassengerData *data);
    virtual void on_trailer_data(stTrailerData *data);
    virtual void on_unoccupied_data(stUnoccupiedData *data);

protected:
    // === Core Identity ===
    std::string name;
    std::string uuid;
    unsigned short playerID;
    int status;

    // === Network Configuration ===
    std::string host; 
    unsigned short port;
    unsigned int reconnect_tick;
    unsigned int update_tick;
    bool gameInited;

    // === Network Components ===
    RakClient client;

    // Per-bot streamable resources (pickups, objects, labels)
    CStreamableResourcePool streamableResources;

    // === Helper Methods ===
    void resetConnectionStatus();
    void setupRPC();
    void receive();

private:
    // === Network Constants ===
    static constexpr int CONNECTION_TIMEOUT = 4000;
    static constexpr int MTU_SIZE = 576;
};

#endif //CRAKBOT_H
