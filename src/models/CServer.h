//
// Created by rain on 8/1/25.
//

#ifndef CSERVER_H
#define CSERVER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <hv/UdpClient.h>
#include <hv/htime.h>

using namespace hv;

// Forward declarations
struct ServerPlayer {
    unsigned char playerId;
    std::string playerName;
    int playerScore;
    int playerPing;
};

struct ServerRules {
    std::map<std::string, std::string> rules;
};

class CServer {
public:
    static const unsigned short DEFAULT_SAMP_PORT = 7777;
    static const int DEFAULT_TIMEOUT_MS = 5000;
    static const int RECEIVE_BUFFER_SIZE = 2048;

    // Packet types for SAMP query protocol
    enum PacketType : char {
        INFO = 'i',
        PLAYERS = 'c', 
        RULES = 'r',
        RCON = 'x'
    };

    CServer(const std::string& host, unsigned short port = DEFAULT_SAMP_PORT);
    ~CServer();

    // Basic getters
    int getDbId() const;
    std::string getHost() const;
    unsigned short getPort() const;
    std::string getName() const;
    std::string getMode() const;
    std::string getLanguage() const;
    int getPlayers() const;
    int getMaxPlayers() const;
    bool hasPassword() const;
    float getPing() const;
    std::string getVersion() const;
    std::string getLastUpdate() const;

    // Basic setters
    void setDbId(int newDbId);
    void setHost(const std::string& newHost);
    void setPort(unsigned short newPort);
    void setName(const std::string& newName);
    void setMode(const std::string& newMode);
    void setLanguage(const std::string& newLanguage);
    void setPlayers(int newPlayers);
    void setMaxPlayers(int newMaxPlayers);
    void setPassword(bool newPassword);
    void setPing(float newPing);
    void setVersion(const std::string& newVersion);
    void setLastUpdate(const std::string& newLastUpdate);

    // Server info update
    void updateServerInfo(const std::string& name, const std::string& mode, 
                         const std::string& language, int players, int maxPlayers, 
                         bool password, const std::string& version);

    // SAMP Query functionality
    bool queryServerInfo();
    bool queryServerPlayers();
    bool queryServerRules();
    std::string sendRconCommand(const std::string& command, const std::string& rconPassword);

    // Async query methods
    void queryServerInfoAsync(std::function<void(bool)> callback);
    void queryServerPlayersAsync(std::function<void(bool)> callback);
    void queryServerRulesAsync(std::function<void(bool)> callback);
    void sendRconCommandAsync(const std::string& command, const std::string& rconPassword, 
                             std::function<void(const std::string&)> callback);

    // Get queried data
    const std::vector<ServerPlayer>& getPlayerList() const;
    const ServerRules& getRules() const;
    
    // Connection test
    bool isOnline();
    void isOnlineAsync(std::function<void(bool)> callback);

    // Timeout settings
    void setTimeout(int timeoutMs);
    int getTimeout() const;

private:
    // Basic server info
    int dbid;
    std::string host;
    unsigned short port;
    std::string name;
    std::string mode;
    std::string language;
    int players;
    int maxPlayers;
    bool password;
    float ping;
    std::string version;
    std::string lastUpdate;

    // Query settings
    int timeoutMs;

    // Queried data
    std::vector<ServerPlayer> playerList;
    ServerRules serverRules;

    // libhv UDP client
    std::unique_ptr<UdpClient> udpClient;

    // Internal methods
    std::vector<unsigned char> createSampPacket(PacketType packetType, const std::string& rconPassword = "", const std::string& command = "");
    bool sendUdpPacketSync(const std::vector<unsigned char>& packet, std::vector<unsigned char>& response);
    void sendUdpPacketAsync(const std::vector<unsigned char>& packet, std::function<void(const std::vector<unsigned char>&)> callback);
    
    bool parseServerInfoResponse(const std::vector<unsigned char>& data);
    bool parseServerPlayersResponse(const std::vector<unsigned char>& data);
    bool parseServerRulesResponse(const std::vector<unsigned char>& data);
    std::string parseRconResponse(const std::vector<unsigned char>& data);
    
    // Utility methods
    std::string readStringFromBuffer(const unsigned char*& buffer, int length);
    unsigned short readUInt16FromBuffer(const unsigned char*& buffer);
    unsigned int readUInt32FromBuffer(const unsigned char*& buffer);
    int readInt32FromBuffer(const unsigned char*& buffer);
    unsigned char readUInt8FromBuffer(const unsigned char*& buffer);
};

#endif //CSERVER_H