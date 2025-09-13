//
// Created by rain on 8/1/25.
//

#include "CRakBot.h"

#include "RakClientInterface.h"
#include "RakNetworkFactory.h"
#include "BitStream.h"
#include <spdlog/spdlog.h>

#include <cstring>

#include "CApp.h"
#include "StringCompressor.h"
#include "../core/CSharedResourcePool.h"
#include "../core/CLogger.h"
#include "../utils/GetTickCount.h"
#include "../utils/UUIDUtil.h"

#include "../authKey.h"
#include "utils/ObjectNameUtil.h"
#include "utils/TextConverter.h"

CRakBot::CRakBot(std::string identifier) : name(identifier), playerID(0xFFFF), status(DISCONNECTED),
                                           reconnect_tick(0), update_tick(0) {
    client.SetMTUSize(MTU_SIZE);
    client.SetOccasionalPing(false);
    client.SetIncomingPassword(nullptr, 0);

    setupRPC();
    gameInited = false;

    uuid = UUIDUtil::generate_uuid();
    CLogger::getInstance()->bot->info("[{}:{}] Bot created", name, uuid);
}

CRakBot::~CRakBot() {
    if (status != DISCONNECTED) {
        disconnect();
    }
    CLogger::getInstance()->bot->info("[{}:{}] Bot destroyed", name, uuid);
}

void CRakBot::resetConnectionStatus() {
    status = DISCONNECTED;
    playerID = 0xFFFF;
    reconnect_tick = GetTickCount();
    update_tick = 0;
    gameInited = false;
    streamableResources.clear();
}

void CRakBot::setupRPC() {
    using namespace std::placeholders;
    client.AddRPCHandler(std::bind(&CRakBot::on_receive_rpc, this, _1, _2));
}

std::string CRakBot::getName() {
    return name;
}

unsigned short CRakBot::getPlayerID() {
    return playerID;
}

int CRakBot::getStatus() {
    return status;
}

std::string CRakBot::getStatusName() {
    switch (status) {
        case DISCONNECTED: return "DISCONNECTED";
        case CONNECTING: return "CONNECTING";
        case CONNECTED: return "CONNECTED";
        case WAIT_FOR_JOIN: return "WAIT_FOR_JOIN";
        case SPAWNED: return "SPAWNED";
        default: return "UNKNOWN";
    }
}

void CRakBot::setHost(const std::string &host) {
    this->host = host;
}

void CRakBot::setPort(int port) {
    this->port = port;
}

std::string CRakBot::getHost() {
    return host;
}

int CRakBot::getPort() {
    return port;
}

// Getters
std::string CRakBot::getUuid() const {
    return uuid;
}

unsigned int CRakBot::getReconnectTick() const {
    return reconnect_tick;
}

unsigned int CRakBot::getUpdateTick() const {
    return update_tick;
}

// Setters
void CRakBot::setName(const std::string &newName) {
    name = newName;
}

void CRakBot::setUuid(const std::string &newUuid) {
    uuid = newUuid;
}

void CRakBot::setPlayerID(unsigned short newPlayerID) {
    playerID = newPlayerID;
}

void CRakBot::setStatus(int newStatus) {
    status = newStatus;
}

void CRakBot::setReconnectTick(unsigned int newReconnectTick) {
    reconnect_tick = newReconnectTick;
}

void CRakBot::setUpdateTick(unsigned int newUpdateTick) {
    update_tick = newUpdateTick;
}

bool CRakBot::isConnected() {
    return status == CONNECTED || status == SPAWNED;
}

bool CRakBot::isGameInited() {
    return gameInited;
}

bool CRakBot::checkConectionDelay() {
    return GetTickCount() - reconnect_tick > CONNECTION_TIMEOUT;
}

void CRakBot::connect(std::string host, unsigned short port) {
    if (status != DISCONNECTED) return;

    this->host = host;
    this->port = port;

    client.Connect(host.c_str(), port, 0, 0, 0);
    status = CONNECTING;
    CLogger::getInstance()->bot->info("[{}:{}] Connecting to {}:{}", name, uuid, host, port);
}

void CRakBot::connect() {
    connect(this->host, this->port);
}

void CRakBot::disconnect() {
    if (status != DISCONNECTED) {
        client.Disconnect(0);
        resetConnectionStatus();
        CLogger::getInstance()->bot->info("[{}:{}] Disconnected from server", name, uuid);
    }
}

void CRakBot::process() {
    unsigned int currentTick = GetTickCount();
    receive();

    // if (status == SPAWNED && (currentTick - update_tick) > 100) {
    //     updateOnfoot();
    //     update_tick = currentTick;
    // }
}

void CRakBot::receive() {
    unsigned char packetIdentifier;
    Packet *pkt;
    while (pkt = client.Receive()) {
        if (!pkt || !pkt->data) {
            if (pkt) client.DeallocatePacket(pkt);
            continue;
        }
        if ((unsigned char) pkt->data[0] == ID_TIMESTAMP) {
            if (pkt->length > sizeof(unsigned char) + sizeof(unsigned int))
                packetIdentifier = (unsigned char) pkt->data[sizeof(unsigned char) + sizeof(unsigned int)];
            else {
                client.DeallocatePacket(pkt);
                continue;
            }
        } else
            packetIdentifier = (unsigned char) pkt->data[0];

        switch (packetIdentifier) {
            case ID_DISCONNECTION_NOTIFICATION:
                CLogger::getInstance()->bot->warn("[{}:{}] Server closed connection", name, uuid);
                resetConnectionStatus();
                break;
            case ID_CONNECTION_BANNED:
                CLogger::getInstance()->bot->error("[{}:{}] Connection banned by server", name, uuid);
                resetConnectionStatus();
                break;
            case ID_CONNECTION_ATTEMPT_FAILED:
                CLogger::getInstance()->bot->error("[{}:{}] Connection attempt failed", name, uuid);
                resetConnectionStatus();
                break;
            case ID_NO_FREE_INCOMING_CONNECTIONS:
                CLogger::getInstance()->bot->error("[{}:{}] Server full - no free connections", name, uuid);
                resetConnectionStatus();
                break;
            case ID_INVALID_PASSWORD:
                CLogger::getInstance()->bot->error("[{}:{}] Invalid password", name, uuid);
                resetConnectionStatus();
                break;
            case ID_CONNECTION_LOST:
                CLogger::getInstance()->bot->warn("[{}:{}] Connection lost", name, uuid);
                resetConnectionStatus();
                break;
            case ID_CONNECTION_REQUEST_ACCEPTED:
                CLogger::getInstance()->bot->info("[{}:{}] Connection accepted by server, joining...", name, uuid);
                sendClientJoin(pkt);
                status = CONNECTED;
                break;
            case ID_AUTH_KEY:
                sendAuthInfo(pkt);
                status = WAIT_FOR_JOIN;
                break;
            case ID_PLAYER_SYNC: {
                RakNet::BitStream bsPlayerSync((unsigned char *) pkt->data, pkt->length, false);
                unsigned short PlayerID;
                bsPlayerSync.IgnoreBits(8); //PACKET ID
                bsPlayerSync.Read(PlayerID);

                stOnFootData onFootData{};
                bsPlayerSync.Read((PCHAR) &onFootData, sizeof(stOnFootData));

                // sync to resources
                CApp::getInstance()->getResourceManager()->updatePlayer({host, port}, playerID, onFootData);

                on_onfoot_data(&onFootData);
                break;
            }
            case ID_VEHICLE_SYNC: {
                RakNet::BitStream bsVehicleSync((unsigned char *) pkt->data, pkt->length, false);
                unsigned short PlayerID;
                bsVehicleSync.IgnoreBits(8); //PACKET ID
                bsVehicleSync.Read(PlayerID);

                stInCarData inCarData{};
                bsVehicleSync.Read((PCHAR) &inCarData, sizeof(stInCarData));

                CApp::getInstance()->getResourceManager()->updateVehicle({host, port}, inCarData.sVehicleID, inCarData);

                on_incar_data(&inCarData);
                break;
            }
            case ID_PASSENGER_SYNC: {
                RakNet::BitStream bsPassengerSync((unsigned char *) pkt->data, pkt->length, false);
                unsigned short PlayerID;
                bsPassengerSync.IgnoreBits(8); //PACKET ID
                bsPassengerSync.Read(PlayerID);

                stPassengerData passengerData{};
                bsPassengerSync.Read((PCHAR) &passengerData, sizeof(stPassengerData));
                on_passenger_data(&passengerData);
                break;
            }
            case ID_AIM_SYNC:
                break;
            case ID_TRAILER_SYNC: {
                RakNet::BitStream bsTrailerSync((unsigned char *) pkt->data, pkt->length, false);
                unsigned short PlayerID;
                bsTrailerSync.IgnoreBits(8); //PACKET ID
                bsTrailerSync.Read(PlayerID);

                stTrailerData trailerData{};
                bsTrailerSync.Read((PCHAR) &trailerData, sizeof(stTrailerData));
                on_trailer_data(&trailerData);
                break;
            }
            case ID_UNOCCUPIED_SYNC: {
                RakNet::BitStream bsUnoccupiedSync((unsigned char *) pkt->data, pkt->length, false);
                unsigned short PlayerID;
                bsUnoccupiedSync.IgnoreBits(8); //PACKET ID
                bsUnoccupiedSync.Read(PlayerID);

                stUnoccupiedData unoccupiedData{};
                bsUnoccupiedSync.Read((PCHAR) &unoccupiedData, sizeof(stUnoccupiedData));
                on_unoccupied_data(&unoccupiedData);
                break;
            }
            case ID_MARKERS_SYNC:
                break;
            case ID_BULLET_SYNC: {
                RakNet::BitStream bsBulletSync((unsigned char *) pkt->data, pkt->length, false);
                unsigned short PlayerID;
                bsBulletSync.IgnoreBits(8); //PACKET ID
                bsBulletSync.Read(PlayerID);

                stBulletData bulletData{};
                bsBulletSync.Read((PCHAR) &bulletData, sizeof(stBulletData));
                on_bullet_data(&bulletData);
                break;
            }
        }
        client.DeallocatePacket(pkt);
    }
}

void CRakBot::sendRPC(int id, RakNet::BitStream *bs) {
    client.RPC(&id, bs, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

void CRakBot::sendAuthInfo(Packet *packet) {
    char *auth_key;
    bool found_key = false;
    for (int x = 0; x < 512; x++) {
        if (!strcmp(((char *) packet->data + 2), AuthKeyTable[x][0])) {
            auth_key = AuthKeyTable[x][1];
            found_key = true;
        }
    }
    if (found_key) {
        RakNet::BitStream bsKey;
        BYTE byteAuthKeyLen;

        byteAuthKeyLen = (BYTE) strlen(auth_key);
        bsKey.Write((BYTE) ID_AUTH_KEY);
        bsKey.Write((BYTE) byteAuthKeyLen);
        bsKey.Write(auth_key, byteAuthKeyLen);
        client.Send(&bsKey, SYSTEM_PRIORITY, RELIABLE, 0);
        CLogger::getInstance()->bot->debug("[{}:{}] Sent auth key response", name, uuid);
    } else {
        CLogger::getInstance()->bot->error("[{}:{}] Unknown auth key requested: {}", name, uuid,
                                           (char *) packet->data + 2);
    }
}

void CRakBot::sendClientJoin(Packet *packet) {
    RakNet::BitStream bsSuccAuth((unsigned char *) packet->data, packet->length, false);
    decltype(playerID) myPlayerID;
    unsigned int uiChallenge;

    bsSuccAuth.IgnoreBits(8); // ID_CONNECTION_REQUEST_ACCEPTED
    bsSuccAuth.IgnoreBits(32); // binaryAddress
    bsSuccAuth.IgnoreBits(16); // port

    bsSuccAuth.Read(myPlayerID);
    bsSuccAuth.Read(uiChallenge);

    playerID = myPlayerID;

    int iVersion = 4057;
    unsigned int uiClientChallengeResponse = uiChallenge ^ iVersion;
    BYTE byteMod = 1;

    char auth_bs[64];
    gen_gpci(auth_bs, 0x3e9);

    auto my_version = std::string("0.3.7");

    BYTE byteAuthBSLen = (BYTE) strlen(auth_bs);
    BYTE byteNameLen = (BYTE) name.size();
    BYTE iClientVerLen = (BYTE) my_version.size();

    RakNet::BitStream bsSend;
    bsSend.Reset();

    bsSend.Write(iVersion);
    bsSend.Write(byteMod);
    bsSend.Write(byteNameLen);
    bsSend.Write(name.c_str(), byteNameLen);
    bsSend.Write(uiClientChallengeResponse);
    bsSend.Write(byteAuthBSLen);
    bsSend.Write(auth_bs, byteAuthBSLen);
    bsSend.Write(iClientVerLen);
    bsSend.Write(my_version.c_str(), iClientVerLen);

    sendRPC(RPC_ClientJoin, &bsSend);
}

void CRakBot::sendBullet(stBulletData *data) {
    if (!data) return;
    RakNet::BitStream bs;
    bs.Reset();
    bs.Write((BYTE) ID_BULLET_SYNC);
    bs.Write((PCHAR) data, sizeof(stBulletData));
    client.Send(&bs, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
}

void CRakBot::sendAim(stAimData *data) {
    if (!data) return;

    RakNet::BitStream bs;
    bs.Reset();
    bs.Write((BYTE) ID_AIM_SYNC);
    bs.Write((PCHAR) data, sizeof(stAimData));
    client.Send(&bs, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
}

void CRakBot::sendInCar(stInCarData *data) {
    if (!data) return;

    RakNet::BitStream bs;
    bs.Reset();
    bs.Write((BYTE) ID_VEHICLE_SYNC);
    bs.Write((PCHAR) data, sizeof(stInCarData));
    client.Send(&bs, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
}

void CRakBot::sendEnterVehicle(int vehicleid) {
    RakNet::BitStream FkbsRPC;
    UINT16 wVehicleID = vehicleid;
    UINT8 bIsPassenger = false;
    FkbsRPC.Reset();
    FkbsRPC.Write(vehicleid);
    FkbsRPC.Write(bIsPassenger);
    int r = RPC_EnterVehicle;
    client.RPC(&r, &FkbsRPC, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

void CRakBot::sendClassRequest(int classid) {
    RakNet::BitStream bsSpawnRequest;
    bsSpawnRequest.Reset();
    bsSpawnRequest.Write(classid);
    int r = RPC_RequestClass;
    client.RPC(&r, &bsSpawnRequest, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

void CRakBot::sendSpawnRequest() {
    RakNet::BitStream bsSendRequestSpawn;
    bsSendRequestSpawn.Reset();
    int r = RPC_RequestSpawn;
    client.RPC(&r, &bsSendRequestSpawn, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

void CRakBot::sendChat(std::string text) {
    if (text.size() == 0)
        return;
    RakNet::BitStream bs;
    bs.Reset();
    if (text[0] == '/') {
        //send server command
        bs.Write((UINT32) text.size());
        bs.Write(text.data(), text.size());
        int r = RPC_ServerCommand;
        client.RPC(&r, &bs, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
    } else {
        bs.Write((UINT8) text.size());
        bs.Write(text.data(), text.size());
        int r = RPC_Chat;
        client.RPC(&r, &bs, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
    }
}

void CRakBot::sendSpawn() {
    RakNet::BitStream bsSendSpawn;
    bsSendSpawn.Reset();
    int r = RPC_Spawn;
    client.RPC(&r, &bsSendSpawn, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
    status = SPAWNED;

    CLogger::getInstance()->bot->info("[{}:{}] Spawned successfully", name, uuid);
}

void CRakBot::sendOnfootSync(stOnFootData *onfoot) {
    if (!onfoot) return;

    RakNet::BitStream bs;
    bs.Reset();
    bs.Write((BYTE) ID_PLAYER_SYNC);
    bs.Write((PCHAR) onfoot, sizeof(stOnFootData));
    client.Send(&bs, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
}


void CRakBot::on_receive_rpc(int id, RakNet::BitStream *bs) {
    CLogger::getInstance()->bot->trace("[{}:{}] Received RPC {}", name, uuid, id);
    bs->ResetReadPointer();
    switch (id) {
        case RPC_InitGame: {
            CLogger::getInstance()->bot->info("[{}:{}] Joined to the server with success (Player ID: {})", name, uuid,
                                              playerID);
            gameInited = true;
            sendClassRequest(0);
            break;
        }
        case RPC_WorldPlayerAdd: {
            UINT16 wPlayerID;
            UINT8 team;
            UINT32 dSkinId;
            float PosX;
            float PosY;
            float PosZ;
            float facing_angle;
            bs->Read(wPlayerID);
            bs->Read(team);
            bs->Read(dSkinId);
            bs->Read(PosX);
            bs->Read(PosY);
            bs->Read(PosZ);
            bs->Read(facing_angle);

            // Increment stream count - this player is now visible to this bot
            CApp::getInstance()->getResourceManager()->updatePlayer({host, port}, wPlayerID,
                                                                    glm::vec3{PosX, PosY, PosZ});
            CApp::getInstance()->getResourceManager()->incrementPlayerStreamCount({host, port}, wPlayerID);
            break;
        }
        case RPC_WorldPlayerRemove: {
            UINT16 wPlayerID;
            bs->Read(wPlayerID);

            // Decrement stream count - this player is no longer visible to this bot
            CApp::getInstance()->getResourceManager()->decrementPlayerStreamCount(
                std::make_pair(host, port), wPlayerID);
            break;
        }

        case RPC_ServerJoin: {
            char szPlayerName[256];
            UINT16 playerId;
            BYTE byteNameLen = 0;

            bs->Read(playerId);
            int iUnk = 0;
            bs->Read(iUnk);
            BYTE bIsNPC = 0;
            bs->Read(bIsNPC);
            bs->Read(byteNameLen);
            if (byteNameLen > 20) return;
            bs->Read(szPlayerName, byteNameLen);
            szPlayerName[byteNameLen] = '\0';

            auto cc = TextConverter::ensureUtf8(std::string(szPlayerName));
            CApp::getInstance()->getResourceManager()->addPlayer(std::make_pair(host, port), {
                 cc,
                 playerId,
                 100,
                 0,
                 {0x3f3f3f3f, 0x3f3f3f3f, 0x3f3f3f3f}, //一开始设置为无限大
                 {},
                 false,
                 0,
                 0,
                 0,
                 -1,
                 0,
                 (bool) bIsNPC
             });
            break;
        }
        case RPC_ServerQuit: {
            UINT16 playerId;
            bs->Read(playerId);
            CApp::getInstance()->getResourceManager()->removePlayer(std::make_pair(host, port), playerId);
            break;
        }
        case RPC_Create3DTextLabel: {
            //UINT16 wLabelID, UINT32 color, float x, float y, float z, float DrawDistance, UINT8 testLOS, UINT16 attachedPlayer, UINT16 attachedVehicle, CSTRING text[4096]
            UINT16 wLabelID;
            UINT32 color;
            float x, y, z, DrawDistance;
            UINT8 testLOS;
            UINT16 attachedPlayer, attachedVehicle;
            char text[4096];

            bs->Read(wLabelID);
            bs->Read(color);
            bs->Read(x);
            bs->Read(y);
            bs->Read(z);
            bs->Read(DrawDistance);
            bs->Read(testLOS);
            bs->Read(attachedPlayer);
            bs->Read(attachedVehicle);
            stringCompressor->DecodeString(text, 4096, bs);

            // Add label to streamable resources (per-bot)
            streamableResources.addLabel({
                wLabelID,
                {x, y, z},
                (int) attachedPlayer,
                (int) attachedVehicle,
                TextConverter::ensureUtf8(std::string(text)),
                DrawDistance,
                testLOS != 0
            });

            break;
        }
        case RPC_Update3DTextLabel: {
            UINT16 wLabelID;
            bs->Read(wLabelID);
            streamableResources.removeLabel(wLabelID);
            break;
        }
        case RPC_WorldVehicleAdd: {
            UINT16 wVehicleID;
            int subtype;
            float PosX;
            float PosY;
            float PosZ;
            float RotW;
            UINT8 iColor1;
            UINT8 iColor2;
            float fHealth;

            bs->Read(wVehicleID);
            bs->Read(subtype);
            bs->Read(PosX);
            bs->Read(PosY);
            bs->Read(PosZ);
            bs->Read(RotW);
            bs->Read(iColor1);
            bs->Read(iColor2);
            bs->Read(fHealth);

            // Add vehicle to shared resource manager
            CApp::getInstance()->getResourceManager()->updateVehicle({host, port}, wVehicleID, subtype,
                                                                     {PosX, PosY, PosZ});
            CApp::getInstance()->getResourceManager()->addVehicle(
                std::make_pair(host, port), {
                    wVehicleID,
                    fHealth,
                    {PosX, PosY, PosZ},
                    {},
                    (int) subtype
                });

            // Increment stream count - this vehicle is now visible to this bot
            CApp::getInstance()->getResourceManager()->incrementVehicleStreamCount(
                std::make_pair(host, port), wVehicleID);
            break;
        }
        case RPC_WorldVehicleRemove: {
            UINT16 wVehicleID;
            bs->Read(wVehicleID);

            // Decrement stream count - this vehicle is no longer visible to this bot
            CApp::getInstance()->getResourceManager()->decrementVehicleStreamCount(
                std::make_pair(host, port), wVehicleID);
            break;
        }

        case RPC_CreatePickup: {
            INT32 iPickupID;
            UINT32 iModel;
            UINT32 iType;
            float fX, fY, fZ;
            bs->Read(iPickupID);
            bs->Read(iModel);
            bs->Read(iType);
            bs->Read(fX);
            bs->Read(fY);
            bs->Read(fZ);
            // Add pickup to streamable resources (per-bot)
            streamableResources.addPickup({
                iPickupID,
                (int) iModel,
                {fX, fY, fZ}
            });
            break;
        }

        case RPC_DestroyPickup: {
            INT32 iPickupID;
            bs->Read(iPickupID);
            streamableResources.removePickup(iPickupID);
        }

        case RPC_CreateObject: {
            UINT16 wObjectID;
            UINT32 iModel;
            float fX, fY, fZ, fRotX, fRotY, fRotZ, fDrawDistance;
            bs->Read(wObjectID);
            bs->Read(iModel);
            bs->Read(fX);
            bs->Read(fY);
            bs->Read(fZ);
            bs->Read(fRotX);
            bs->Read(fRotY);
            bs->Read(fRotZ);
            bs->Read(fDrawDistance);

            // CLogger::getInstance()->system->info("{} {} {} {}", CApp::getInstance()->getObjectNameUtil()->getObjectName(iModel), fX, fY, fZ);
            // Add object to streamable resources (per-bot)
            streamableResources.addObject({
                wObjectID,
                (int) iModel,
                {fX, fY, fZ},
                {fRotX, fRotY, fRotZ},
                fDrawDistance,
                ""
            });
            break;
        }
    }
}

// Packet callback implementations
void CRakBot::on_bullet_data(stBulletData *data) {
    // Default implementation - do nothing
    // Subclasses can override this to handle bullet data
}

void CRakBot::on_onfoot_data(stOnFootData *data) {
    // Default implementation - do nothing
    // Subclasses can override this to handle onfoot data
}

void CRakBot::on_incar_data(stInCarData *data) {
    // Default implementation - do nothing
    // Subclasses can override this to handle in-car data
}

void CRakBot::on_passenger_data(stPassengerData *data) {
    // Default implementation - do nothing
    // Subclasses can override this to handle passenger data
}

void CRakBot::on_trailer_data(stTrailerData *data) {
    // Default implementation - do nothing
    // Subclasses can override this to handle trailer data
}

void CRakBot::on_unoccupied_data(stUnoccupiedData *data) {
    // Default implementation - do nothing
    // Subclasses can override this to handle unoccupied vehicle data
}
