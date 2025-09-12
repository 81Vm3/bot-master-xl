//
// Created by rain on 8/1/25.
//

#include "CServer.h"
#include <spdlog/spdlog.h>
#include <hv/hsocket.h>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "utils/TextConverter.h"

CServer::CServer(const std::string& host, unsigned short port) 
    : host(host), port(port), name(""), mode(""), language(""), 
      players(0), maxPlayers(0), password(false), ping(0.0f), version(""),
      lastUpdate(""), timeoutMs(DEFAULT_TIMEOUT_MS) {
    
    udpClient = std::make_unique<UdpClient>();
}

CServer::~CServer() {
}

int CServer::getDbId() const {
    return dbid;
}

void CServer::setDbId(int newDbId) {
    dbid = newDbId;
}

// Basic getters
std::string CServer::getHost() const {
    return host;
}

unsigned short CServer::getPort() const {
    return port;
}

std::string CServer::getName() const {
    return name;
}

std::string CServer::getMode() const {
    return mode;
}

std::string CServer::getLanguage() const {
    return language;
}

int CServer::getPlayers() const {
    return players;
}

int CServer::getMaxPlayers() const {
    return maxPlayers;
}

bool CServer::hasPassword() const {
    return password;
}

float CServer::getPing() const {
    return ping;
}

std::string CServer::getVersion() const {
    return version;
}

std::string CServer::getLastUpdate() const {
    return lastUpdate;
}

// Basic setters
void CServer::setHost(const std::string& newHost) {
    host = newHost;
}

void CServer::setPort(unsigned short newPort) {
    port = newPort;
}

void CServer::setName(const std::string& newName) {
    name = newName;
}

void CServer::setMode(const std::string& newMode) {
    mode = newMode;
}

void CServer::setLanguage(const std::string& newLanguage) {
    language = newLanguage;
}

void CServer::setPlayers(int newPlayers) {
    players = newPlayers;
}

void CServer::setMaxPlayers(int newMaxPlayers) {
    maxPlayers = newMaxPlayers;
}

void CServer::setPassword(bool newPassword) {
    password = newPassword;
}

void CServer::setPing(float newPing) {
    ping = newPing;
}

void CServer::setVersion(const std::string& newVersion) {
    version = newVersion;
}

void CServer::setLastUpdate(const std::string& newLastUpdate) {
    lastUpdate = newLastUpdate;
}

void CServer::setTimeout(int timeoutMs) {
    this->timeoutMs = timeoutMs;
}

int CServer::getTimeout() const {
    return timeoutMs;
}

// Server info update
void CServer::updateServerInfo(const std::string& name, const std::string& mode, 
                              const std::string& language, int players, int maxPlayers, 
                              bool password, const std::string& version) {
    this->name = name;
    this->mode = mode;
    this->language = language;
    this->players = players;
    this->maxPlayers = maxPlayers;
    this->password = password;
    this->version = version;
}

const std::vector<ServerPlayer>& CServer::getPlayerList() const {
    return playerList;
}

const ServerRules& CServer::getRules() const {
    return serverRules;
}

std::vector<unsigned char> CServer::createSampPacket(PacketType packetType, const std::string& rconPassword, const std::string& command) {
    std::vector<unsigned char> packet;
    
    // Add SAMP header
    packet.push_back('S');
    packet.push_back('A');
    packet.push_back('M');
    packet.push_back('P');
    
    // Resolve host to IP and add IP address (4 bytes)
    sockaddr_u addr;
    if (sockaddr_set_ipport(&addr, host.c_str(), port) != 0) {
        spdlog::error("Failed to resolve host: {}", host);
        return packet;
    }
    
    // Extract IP bytes from sockaddr
    uint32_t ip = ntohl(addr.sin.sin_addr.s_addr);
    packet.push_back(ip & 0xFF);
    packet.push_back((ip >> 8) & 0xFF);
    packet.push_back((ip >> 16) & 0xFF);
    packet.push_back((ip >> 24) & 0xFF);
    
    // Add port (2 bytes, little endian)
    packet.push_back(port & 0xFF);
    packet.push_back((port >> 8) & 0xFF);
    
    // Add packet type
    packet.push_back(static_cast<char>(packetType));
    
    // Add RCON specific data if needed
    if (packetType == RCON) {
        // Add password length and password
        unsigned short passLen = rconPassword.length();
        packet.push_back(passLen & 0xFF);
        packet.push_back((passLen >> 8) & 0xFF);
        
        for (char c : rconPassword) {
            packet.push_back(c);
        }
        
        // Add command length and command
        unsigned short cmdLen = command.length();
        packet.push_back(cmdLen & 0xFF);
        packet.push_back((cmdLen >> 8) & 0xFF);
        
        for (char c : command) {
            packet.push_back(c);
        }
    }
    
    return packet;
}

bool CServer::sendUdpPacketSync(const std::vector<unsigned char>& packet, std::vector<unsigned char>& response) {
    std::mutex mtx;
    std::condition_variable cv;
    bool completed = false;
    bool success = false;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    sendUdpPacketAsync(packet, [&](const std::vector<unsigned char>& resp) {
        std::lock_guard<std::mutex> lock(mtx);
        response = resp;
        success = !resp.empty();
        completed = true;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        ping = static_cast<float>(duration.count());
        
        cv.notify_one();
    });
    
    // Wait for completion with timeout
    std::unique_lock<std::mutex> lock(mtx);
    bool timedOut = !cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return completed; });
    
    if (timedOut) {
        spdlog::error("UDP query timed out for {}:{}", host, port);
        return false;
    }
    
    return success;
}

void CServer::sendUdpPacketAsync(const std::vector<unsigned char>& packet, std::function<void(const std::vector<unsigned char>&)> callback) {
    udpClient->onMessage = [callback](const SocketChannelPtr& channel, Buffer* buf) {
        // Convert Buffer to vector<unsigned char>
        const unsigned char* data = static_cast<const unsigned char*>(buf->data());
        std::vector<unsigned char> response(data, data + buf->size());
        callback(response);
    };
    
    udpClient->start();
    
    // Create sockaddr for the target
    sockaddr_u addr;
    if (sockaddr_set_ipport(&addr, host.c_str(), port) != 0) {
        spdlog::error("Failed to resolve host: {}", host);
        callback(std::vector<unsigned char>());
        return;
    }
    
    // Send the packet using the correct libhv API
    if (udpClient->sendto(packet.data(), packet.size(), &addr.sa) < 0) {
        spdlog::error("Failed to send UDP packet to {}:{}", host, port);
        callback(std::vector<unsigned char>());
        return;
    }
    
    // Set a timer for timeout
    udpClient->loop()->setTimeout(timeoutMs, [callback](TimerID timerID) {
        callback(std::vector<unsigned char>());
    });
}

bool CServer::queryServerInfo() {
    auto packet = createSampPacket(INFO);
    if (packet.empty()) return false;
    
    std::vector<unsigned char> response;
    if (!sendUdpPacketSync(packet, response)) return false;
    
    return parseServerInfoResponse(response);
}

bool CServer::queryServerPlayers() {
    auto packet = createSampPacket(PLAYERS);
    if (packet.empty()) return false;
    
    std::vector<unsigned char> response;
    if (!sendUdpPacketSync(packet, response)) return false;
    
    return parseServerPlayersResponse(response);
}

bool CServer::queryServerRules() {
    auto packet = createSampPacket(RULES);
    if (packet.empty()) return false;
    
    std::vector<unsigned char> response;
    if (!sendUdpPacketSync(packet, response)) return false;
    
    return parseServerRulesResponse(response);
}

std::string CServer::sendRconCommand(const std::string& command, const std::string& rconPassword) {
    auto packet = createSampPacket(RCON, rconPassword, command);
    if (packet.empty()) return "";
    
    std::vector<unsigned char> response;
    if (!sendUdpPacketSync(packet, response)) return "";
    
    return parseRconResponse(response);
}

void CServer::queryServerInfoAsync(std::function<void(bool)> callback) {
    auto packet = createSampPacket(INFO);
    if (packet.empty()) {
        callback(false);
        return;
    }
    
    sendUdpPacketAsync(packet, [this, callback](const std::vector<unsigned char>& response) {
        bool success = !response.empty() && parseServerInfoResponse(response);
        callback(success);
    });
}

void CServer::queryServerPlayersAsync(std::function<void(bool)> callback) {
    auto packet = createSampPacket(PLAYERS);
    if (packet.empty()) {
        callback(false);
        return;
    }
    
    sendUdpPacketAsync(packet, [this, callback](const std::vector<unsigned char>& response) {
        bool success = !response.empty() && parseServerPlayersResponse(response);
        callback(success);
    });
}

void CServer::queryServerRulesAsync(std::function<void(bool)> callback) {
    auto packet = createSampPacket(RULES);
    if (packet.empty()) {
        callback(false);
        return;
    }
    
    sendUdpPacketAsync(packet, [this, callback](const std::vector<unsigned char>& response) {
        bool success = !response.empty() && parseServerRulesResponse(response);
        callback(success);
    });
}

void CServer::sendRconCommandAsync(const std::string& command, const std::string& rconPassword, 
                                  std::function<void(const std::string&)> callback) {
    auto packet = createSampPacket(RCON, rconPassword, command);
    if (packet.empty()) {
        callback("");
        return;
    }
    
    sendUdpPacketAsync(packet, [this, callback](const std::vector<unsigned char>& response) {
        std::string result = response.empty() ? "" : parseRconResponse(response);
        callback(result);
    });
}

bool CServer::isOnline() {
    return queryServerInfo();
}

void CServer::isOnlineAsync(std::function<void(bool)> callback) {
    queryServerInfoAsync(callback);
}

bool CServer::parseServerInfoResponse(const std::vector<unsigned char>& data) {
    if (data.size() < 11) return false;
    
    const unsigned char* buffer = data.data();
    
    // Skip SAMP header (4) + IP (4) + port (2) + type (1) = 11 bytes
    buffer += 11;
    
    try {
        // Read password flag
        password = readUInt8FromBuffer(buffer) != 0;
        
        // Read player counts
        players = readUInt16FromBuffer(buffer);
        maxPlayers = readUInt16FromBuffer(buffer);
        
        // Read hostname length and hostname
        unsigned int hostnameLen = readUInt32FromBuffer(buffer);
        name = TextConverter::ensureUtf8(readStringFromBuffer(buffer, hostnameLen));
        
        // Read gamemode length and gamemode
        unsigned int gamemodeLen = readUInt32FromBuffer(buffer);
        mode = TextConverter::ensureUtf8(readStringFromBuffer(buffer, gamemodeLen));
        
        // Read language length and language
        unsigned int languageLen = readUInt32FromBuffer(buffer);
        language = TextConverter::ensureUtf8(readStringFromBuffer(buffer, languageLen));
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse server info response: {}", e.what());
        return false;
    }
}

bool CServer::parseServerPlayersResponse(const std::vector<unsigned char>& data) {
    if (data.size() < 11) return false;
    
    const unsigned char* buffer = data.data();
    
    // Skip SAMP header (4) + IP (4) + port (2) + type (1) = 11 bytes
    buffer += 11;
    
    try {
        playerList.clear();
        
        // Read player count
        unsigned short playerCount = readUInt16FromBuffer(buffer);
        
        for (int i = 0; i < playerCount; i++) {
            ServerPlayer player;
            
            // Read player ID
            player.playerId = readUInt8FromBuffer(buffer);
            
            // Read player name length and name
            unsigned char nameLen = readUInt8FromBuffer(buffer);
            player.playerName = readStringFromBuffer(buffer, nameLen);
            
            // Read player score and ping
            player.playerScore = readInt32FromBuffer(buffer);
            player.playerPing = readInt32FromBuffer(buffer);
            
            playerList.push_back(player);
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse server players response: {}", e.what());
        return false;
    }
}

bool CServer::parseServerRulesResponse(const std::vector<unsigned char>& data) {
    if (data.size() < 11) return false;
    
    const unsigned char* buffer = data.data();
    
    // Skip SAMP header (4) + IP (4) + port (2) + type (1) = 11 bytes
    buffer += 11;
    
    try {
        serverRules.rules.clear();
        
        // Read rules count
        unsigned short rulesCount = readUInt16FromBuffer(buffer);
        
        for (int i = 0; i < rulesCount; i++) {
            // Read rule name length and name
            unsigned char nameLen = readUInt8FromBuffer(buffer);
            std::string ruleName = readStringFromBuffer(buffer, nameLen);
            
            // Read rule value length and value
            unsigned char valueLen = readUInt8FromBuffer(buffer);
            std::string ruleValue = readStringFromBuffer(buffer, valueLen);
            
            serverRules.rules[ruleName] = ruleValue;
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse server rules response: {}", e.what());
        return false;
    }
}

std::string CServer::parseRconResponse(const std::vector<unsigned char>& data) {
    if (data.size() < 11) return "";
    
    const unsigned char* buffer = data.data();
    
    // Skip SAMP header (4) + IP (4) + port (2) + type (1) = 11 bytes
    buffer += 11;
    
    std::string result;
    
    try {
        unsigned short length;
        while ((length = readUInt16FromBuffer(buffer)) != 0) {
            result += readStringFromBuffer(buffer, length) + "\n";
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse RCON response: {}", e.what());
        return "";
    }
    
    return result;
}

// Utility methods
std::string CServer::readStringFromBuffer(const unsigned char*& buffer, int length) {
    std::string result(reinterpret_cast<const char*>(buffer), length);
    buffer += length;
    return result;
}

unsigned short CServer::readUInt16FromBuffer(const unsigned char*& buffer) {
    unsigned short value = buffer[0] | (buffer[1] << 8);
    buffer += 2;
    return value;
}

unsigned int CServer::readUInt32FromBuffer(const unsigned char*& buffer) {
    unsigned int value = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    buffer += 4;
    return value;
}

int CServer::readInt32FromBuffer(const unsigned char*& buffer) {
    int value = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    buffer += 4;
    return value;
}

unsigned char CServer::readUInt8FromBuffer(const unsigned char*& buffer) {
    unsigned char value = buffer[0];
    buffer += 1;
    return value;
}