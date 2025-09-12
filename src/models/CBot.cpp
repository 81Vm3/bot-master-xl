#include "CBot.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <cstring>
#include <random>
#include <glm/gtc/quaternion.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "CApp.h"
#include "StringCompressor.h"
#include "../utils/bit_manipulation.h"
#include "../utils/GetTickCount.h"
#include "../utils/defines.h"
#include "../utils/weapon_config.h"
#include "../utils/TextConverter.h"
#include "../core/CLogger.h"
#include "core/CSharedResourcePool.h"
#include "spdlog/formatter.h"
#include "utils/map_zones.h"
#include "../physics/CPathFinder.h"
#include "physics/Raycast.h"

CBot::CBot(std::string identifier) : CRakBot(identifier) {
    memset(&data_onfoot, 0, sizeof(stOnFootData));
    position = glm::vec3(0.0f);
    velocity = glm::vec3(0.0f);
    quaternion = glm::quat();
    angle = 0.0f;
    armor = 0.0f;
    health = 100.0f;
    invulnerable = false;
    flag = 0;
    deathTick = 0;
    
    // Initialize pathfinder
    pathFinder = std::make_shared<CPathFinder>();
    
    // Initialize dialog system
    dialogActive = false;
    dialogID = 0;
    dialogStyle = 0;
    
    // Initialize movepath system
    currentWaypointIndex = 0;
    movepathStatus = MOVEPATH_INACTIVE;
    movepathLooping = false;
    waypointReached = false;
    
    // Initialize movement data
    m_iMovePath = 0;
    m_iMovePoint = 0;
    m_iMoveType = 0;
    m_iMoveMode = 0;
    m_iMovePathfinding = 0;
    m_fMoveRadius = 0.0f;
    m_bMoveSetAngle = false;
    m_fMoveSpeed = 0.0f;
    m_fDistOffset = 0.0f;
    m_iNodeMoveType = 0;
    m_iNodeMoveMode = 0;
    m_fNodeMoveRadius = 0.0f;
    m_bNodeMoveSetAngle = false;
    m_fNodeMoveSpeed = 0.0f;
    m_dwMoveStartTime = 0;
    m_dwMoveTime = 0;
    m_dwMoveStopDelay = 0;
    m_dwMoveTickCount = 0;
}

CBot::~CBot() {
}

std::string CBot::getPassword() const {
    return password;
}

std::string CBot::getSystemPrompt() const {
    return systemPrompt;
}

void CBot::setPassword(const std::string &newPassword) {
    password = newPassword;
}

void CBot::setSystemPrompt(const std::string &newSystemPrompt) {
    systemPrompt = newSystemPrompt;
}

// CBot-specific getters
glm::vec3 CBot::getPosition() const {
    return position;
}

glm::vec3 CBot::getVelocity() const {
    return velocity;
}

glm::quat CBot::getQuaternion() const {
    return quaternion;
}

float CBot::getArmor() const {
    return armor;
}

float CBot::getHealth() const {
    return health;
}

// CBot-specific setters
void CBot::setPosition(const glm::vec3 &newPosition) {
    position = newPosition;
}

void CBot::setVelocity(const glm::vec3 &newVelocity) {
    velocity = newVelocity;
}

void CBot::setQuaternion(const glm::quat &newQuaternion) {
    quaternion = newQuaternion;
}

void CBot::setArmor(float newArmor) {
    armor = newArmor;
}

void CBot::setHealth(float newHealth) {
    health = newHealth;
}

void CBot::toggleFlag(int bit) {
    flag = togglebit(flag, bit);
}

bool CBot::getFlag(int bit) const {
    return checkbit(flag, bit);
}

void CBot::setFlag(int bit, bool state) {
    if (state) flag = onbit(flag, bit);
    else flag = offbit(flag, bit);
}

void CBot::setAngle(float newAngle) {
    angle = newAngle;
    // Convert to GTA:SA angle system (angles are reversed: 360 - angle)
    // Then convert degrees to radians and create rotation quaternion around Z-axis
    float gtaAngle = 360.0f - newAngle;
    if (gtaAngle >= 360.0f) {
        gtaAngle -= 360.0f;
    }
    float radians = glm::radians(gtaAngle);
    quaternion = glm::angleAxis(radians, glm::vec3(0.0f, 0.0f, 1.0f));
}

float CBot::getAngle() const {
    return angle;
}

void CBot::updateMovingData(glm::vec3 vecDestination, float fRadius, bool bSetAngle, float fSpeed, float fDistOffset) {
    // Add random radius offset if specified
    if (fRadius > 0.0f) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-fRadius, fRadius);

        vecDestination.x += dis(gen);
        vecDestination.y += dis(gen);
        // Z coordinate offset is 0 (ground level movement)
    }

    glm::vec3 vecPosition = position;
    float fDistance = glm::distance(vecPosition, vecDestination);

    glm::vec3 vecFront = glm::vec3(0.0f);
    if (fDistance > 0.0f) {
        vecFront = glm::normalize(vecDestination - vecPosition);
    }

    // Calculate angle in degrees from direction vector (matching CMath::GetAngle behavior)
    float fAngle = glm::degrees(glm::atan(vecFront.y, vecFront.x)) + 270.0f;
    if (fAngle >= 360.0f) {
        fAngle -= 360.0f;
    } else if (fAngle < 0.0f) {
        fAngle += 360.0f;
    }

    // Apply distance offset if specified
    if (fDistOffset != 0.0f) {
        fDistance += fDistOffset;
        // Recalculate destination with offset
        float angleRad = glm::radians(fAngle);
        vecDestination.x = vecPosition.x + glm::cos(angleRad) * fDistance;
        vecDestination.y = vecPosition.y + glm::sin(angleRad) * fDistance;

        // Recalculate direction vector
        if (fDistance > 0.0f) {
            vecFront = glm::normalize(vecDestination - vecPosition);
        }
    }

    if (bSetAngle) {
        setAngle(fAngle);
    }

    // Set the moving velocity
    vecFront *= (fSpeed / 100.0f); // Step per 1ms
    setVelocity(vecFront);

    // Calculate the moving time
    if (glm::length(vecFront) != 0.0) {
        m_dwMoveTime = static_cast<DWORD>(fDistance / glm::length(vecFront));
    } else {
        m_dwMoveTime = 0;
    }
    // Get the start move tick
    m_dwMoveTickCount = m_dwMoveStartTime = GetTickCount();

    // Save the movement parameters
    this->vecDestination = vecDestination;
    m_fMoveRadius = fRadius;
    m_bMoveSetAngle = bSetAngle;
    m_fMoveSpeed = fSpeed;
    m_fDistOffset = fDistOffset;
}

void CBot::setKeys(WORD wUDAnalog, WORD wLRAnalog, DWORD dwKeys) {
    data_onfoot.sUpDownKeys = wUDAnalog;
    data_onfoot.sLeftRightKeys = wLRAnalog;
    data_onfoot.sKeys = dwKeys;
}

void CBot::getKeys(WORD *pwUDAnalog, WORD *pwLRAnalog, DWORD *pdwKeys) {
    *pwUDAnalog = data_onfoot.sUpDownKeys;
    *pwLRAnalog = data_onfoot.sLeftRightKeys;
    *pdwKeys = data_onfoot.sKeys;
}

void CBot::go(const glm::vec3 &vecPoint, int iType, float fRadius, bool bSetAngle, float fSpeed,
              float fDistOffset, int dwStopDelay) {
    // CLogger::getInstance()->logger_bot->info("vecPoint {} {} {}", vecPoint.x, vecPoint.y, vecPoint.z);

    // Get the moving type key and speed
    WORD wUDKey = data_onfoot.sUpDownKeys;
    WORD wLRKey = data_onfoot.sLeftRightKeys;
    DWORD dwMoveKey = data_onfoot.sKeys & ~(KEY_WALK | KEY_SPRINT);

    if (iType == MOVE_TYPE_AUTO || iType == MOVE_TYPE_WALK || iType == MOVE_TYPE_RUN || iType == MOVE_TYPE_SPRINT) {
        wUDKey |= static_cast<WORD>(KEY_UP);
        if (iType == MOVE_TYPE_AUTO && glm::abs(fSpeed - MOVE_SPEED_AUTO) < 0.001f) {
            iType = MOVE_TYPE_RUN;
        }
        if (glm::abs(fSpeed - MOVE_SPEED_AUTO) < 0.001f) {
            if (iType == MOVE_TYPE_RUN) {
                fSpeed = MOVE_SPEED_RUN;
            } else if (iType == MOVE_TYPE_WALK) {
                fSpeed = MOVE_SPEED_WALK;
            } else if (iType == MOVE_TYPE_SPRINT) {
                fSpeed = MOVE_SPEED_SPRINT;
            }
        } else if (iType == MOVE_TYPE_AUTO) {
            // Find the nearest speed value
            float fSpeedValues[] = {MOVE_SPEED_WALK, MOVE_SPEED_RUN, MOVE_SPEED_SPRINT};
            float fNearestSpeed = fSpeedValues[0];
            float fMinDiff = glm::abs(fSpeed - fSpeedValues[0]);
            for (int i = 1; i < 3; i++) {
                float fDiff = glm::abs(fSpeed - fSpeedValues[i]);
                if (fDiff < fMinDiff) {
                    fMinDiff = fDiff;
                    fNearestSpeed = fSpeedValues[i];
                }
            }
            if (glm::abs(fNearestSpeed - MOVE_SPEED_SPRINT) < 0.001f) {
                iType = MOVE_TYPE_SPRINT;
            } else if (glm::abs(fNearestSpeed - MOVE_SPEED_RUN) < 0.001f) {
                iType = MOVE_TYPE_RUN;
            } else if (glm::abs(fNearestSpeed - MOVE_SPEED_WALK) < 0.001f) {
                iType = MOVE_TYPE_WALK;
            }
        }
        if (iType == MOVE_TYPE_RUN) {
            dwMoveKey |= KEY_NONE;
        } else if (iType == MOVE_TYPE_WALK) {
            dwMoveKey |= KEY_WALK;
        } else if (iType == MOVE_TYPE_SPRINT) {
            dwMoveKey |= KEY_SPRINT;
        }
    } else if (iType == MOVE_TYPE_DRIVE) {
        //-V547
        dwMoveKey |= KEY_SPRINT;
        if (glm::abs(fSpeed - MOVE_SPEED_AUTO) < 0.001f) {
            fSpeed = 1.0f;
        }
    }
    // Set the moving keys
    // Set the moving keys
    setKeys(wUDKey, wLRKey, dwMoveKey);
    // Update moving data
    updateMovingData(vecPoint, fRadius, bSetAngle, fSpeed, fDistOffset);
    // Mark as moving
    setFlag(IS_MOVING, true);
    // Save the data
    m_iMoveType = iType;
    m_dwMoveStopDelay = dwStopDelay;
}

void CBot::stop() {
    // Make sure the player is moving
    if (!getFlag(IS_MOVING)) {
        return;
    }
    // Reset moving flag
    setFlag(IS_MOVING, false);
    m_iMovePoint = 0;
    
    // Reset the player data
    setVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
    
    WORD wUDKey = data_onfoot.sUpDownKeys;
    WORD wLRKey = data_onfoot.sLeftRightKeys;
    
    if (getFlag(IS_DRIVING)) {
        setKeys(wUDKey, wLRKey, data_onfoot.sKeys & ~KEY_SPRINT);
    } else {
        setKeys(wUDKey & ~KEY_UP, wLRKey, data_onfoot.sKeys & ~(KEY_WALK | KEY_SPRINT));
    }
    
    // Reset other moving variables
    m_dwMoveTime = 0;
    m_dwMoveStartTime = 0;
}

void CBot::kill(int bReason, int wKillerID) {
    if (getFlag(IS_MOVING)) stop();
    setFlag(IS_DEAD, true);

    health = 0;
    // 强制同步health
    updateOnfoot();

    RakNet::BitStream bs;
    bs.Reset();
    bs.Write(static_cast<UINT8>(bReason));
    bs.Write(static_cast<UINT16>(wKillerID));
    // 发送死亡rpc
    sendRPC(RPC_Death, &bs);

    // 延迟重生
    deathTick = GetTickCount();
}

void CBot::updateOnfoot() {
    data_onfoot.fPosition[0] = position.x;
    data_onfoot.fPosition[1] = position.y;
    data_onfoot.fPosition[2] = position.z;

    data_onfoot.fQuaternion[0] = quaternion.w;
    data_onfoot.fQuaternion[1] = quaternion.x;
    data_onfoot.fQuaternion[2] = quaternion.y;
    data_onfoot.fQuaternion[3] = quaternion.z;

    data_onfoot.fMoveSpeed[0] = velocity.x;
    data_onfoot.fMoveSpeed[1] = velocity.y;
    data_onfoot.fMoveSpeed[2] = velocity.z;

    data_onfoot.byteHealth = (unsigned char) (health);
    data_onfoot.byteArmor = (unsigned char) (armor);

    sendOnfootSync(&data_onfoot);
}

void CBot::go_with_path(const glm::vec3& destination, int iType, float fSpeed) {
    // Clear any existing movepath first
    clearMovepath();

    glm::vec3 raycastResult;
    if (!raycast(position, destination, &raycastResult)) {
        if (abs(position.z - destination.z) < 3.0) {
            go(destination, iType, 0.0, true, fSpeed, 0, 0);
            return;
        }
    }

    // Get current position
    glm::vec3 currentPos = getPosition();
    
    // Use pathfinder to calculate optimal route
    try {
        spdlog::info("CBot: Finding path from ({}, {}, {}) to ({}, {}, {})", 
                    currentPos.x, currentPos.y, currentPos.z,
                    destination.x, destination.y, destination.z);
        
        std::vector<glm::vec3> pathWaypoints = pathFinder->findPath(currentPos, destination);
        
        if (pathWaypoints.empty()) {
            importantEvents.emplace_back("Pathfinder failed! Target too far or the goal too complex!");
            return;
        }
        
        spdlog::info("CBot: Pathfinder found route with {} waypoints", pathWaypoints.size());
        
        // Create movepath from the calculated waypoints
        createMovepath(pathWaypoints, false); // No looping for pathfinding
        
        // Store movement parameters for consistent behavior
        m_iMoveType = iType;
        m_fMoveSpeed = fSpeed;
        
        // Start the movepath
        startMovepath();
    } catch (const std::exception& e) {
        importantEvents.emplace_back("Pathfinder failed!");
    }
}

void CBot::on_spawned() {
    setFlag(IS_DEAD, false);
    go(glm::vec3(300, 200, 13.5622), MOVE_TYPE_WALK, 0, true, MOVE_SPEED_RUN, 0.0, 0);
}

bool CBot::on_take_damage(int weapon, float damage) {
    if (invulnerable) return false;

    return true;
}

void CBot::on_receive_rpc(int id, RakNet::BitStream *bs) {
    CRakBot::on_receive_rpc(id, bs);
    bs->ResetReadPointer();
    switch (id) {
        case RPC_InitGame: {
            // 尝试spawn
            sendSpawnRequest();
            break;
        }
        case RPC_SetPlayerPos: {
            if (!getFlag(IS_UNMOVING)) {
                float x, y, z;
                bs->Read(x);
                bs->Read(y);
                bs->Read(z);
                position.x = x;
                position.y = y;
                position.z = z;
                importantEvents.emplace_back(fmt::format("Your position was set to {},{},{}", position.x, position.y,
                                                         position.z));
            }
            break;
        }
        case RPC_SetPlayerHealth: {
            if (!invulnerable) {
                bs->Read(health);
                importantEvents.emplace_back(fmt::format("Your health was set to {}", health));
            }
            break;
        }
        case RPC_SetPlayerArmour: {
            if (!invulnerable) {
                bs->Read(armor);
                importantEvents.emplace_back(fmt::format("Your armor was set to {}", armor));
            }
            break;
        }
        case RPC_SetSpawnInfo: {
            status = CONNECTED;
            UINT8 byteTeam;
            UINT32 dSkin;
            UINT8 unused;
            float X, Y, Z;
            bs->Read(byteTeam);
            bs->Read(dSkin);
            bs->Read(unused);
            bs->Read(X);
            bs->Read(Y);
            bs->Read(Z);
            position.x = X;
            position.y = Y;
            position.z = Z;
            break;
        }
        case RPC_RequestClass: {
            status = CONNECTED;
            UINT8 bRequestResponse;
            UINT8 byteTeam;
            UINT32 dSkin;
            UINT8 unused;
            float X, Y, Z;
            bs->Read(bRequestResponse);
            bs->Read(byteTeam);
            bs->Read(dSkin);
            bs->Read(unused);
            bs->Read(X);
            bs->Read(Y);
            bs->Read(Z);
            position.x = X;
            position.y = Y;
            position.z = Z;

            sendSpawnRequest();
            break;
        }
        case RPC_RequestSpawn: {
            UINT8 bRequestResponse;
            bs->Read(bRequestResponse);
            if (bRequestResponse) {
                sendSpawn();
            }
            break;
        }
        case RPC_ClientMessage: {
            DWORD dwStrLen, dwColor;
            char szMsg[257];
            memset(szMsg, 0, 257);
            bs->Read(dwColor);
            bs->Read(dwStrLen);
            if (dwStrLen > 256) return;
            bs->Read(szMsg, dwStrLen);
            szMsg[dwStrLen] = 0;

            // Convert GB2312 to UTF-8
            std::string utf8Message = TextConverter::ensureUtf8(std::string(szMsg));
            addMessageToChatbox(utf8Message);

            unreadChatMessage.emplace_back(utf8Message);
            break;
        }
        case RPC_Chat: {
            UINT16 playerId;
            BYTE byteTextLen;
            char szText[256];
            memset(szText, 0, 256);
            bs->Read(playerId);
            bs->Read(byteTextLen);
            bs->Read((char *) szText, byteTextLen);
            szText[byteTextLen] = 0;
            if (playerId < 0 || playerId >= MAX_PLAYERS)
                break;

            // Convert GB2312 to UTF-8
            std::string utf8ChatText = TextConverter::ensureUtf8(std::string(szText));
            addMessageToChatbox(utf8ChatText);

            unreadChatMessage.emplace_back(utf8ChatText);
            break;
        }
        case RPC_ShowDialog: {
            // Read dialog ID and style
            INT16 wDialogID;
            UINT8 bDialogStyle;
            bs->Read(wDialogID);
            dialogID = wDialogID;
            bs->Read(bDialogStyle);
            dialogStyle = bDialogStyle;

            // Read title
            uint8_t titleLength;
            bs->Read(titleLength);
            char titleBuffer[256];
            bs->Read(titleBuffer, titleLength);
            titleBuffer[titleLength] = 0;
            dialogTitle = TextConverter::ensureUtf8(std::string(titleBuffer));

            // Read button 1 (left button)
            uint8_t button1Length;
            bs->Read(button1Length);
            char button1Buffer[256];
            bs->Read(button1Buffer, button1Length);
            button1Buffer[button1Length] = 0;
            dialogButtonLeft = TextConverter::ensureUtf8(std::string(button1Buffer));

            // Read button 2 (right button)
            uint8_t button2Length;
            bs->Read(button2Length);
            char button2Buffer[256];
            bs->Read(button2Buffer, button2Length);
            button2Buffer[button2Length] = 0;
            dialogButtonRight = TextConverter::ensureUtf8(std::string(button2Buffer));

            // Read dialog content (compressed)
            char contentBuffer[4096];
            stringCompressor->DecodeString(contentBuffer, sizeof(contentBuffer), bs);
            dialogContent = TextConverter::ensureUtf8(std::string(contentBuffer));

            // Set dialog as active
            dialogActive = true;
            // CLogger::getInstance()->bot->info("Bot {} received dialog: ID={}, Style={}, Title='{}', Content='{}'",
            //                                   name, dialogID, dialogStyle, dialogTitle, dialogContent);
            break;
        }
        case RPC_CreateExplosion: {
            float X;
            float Y;
            float Z;
            bs->Read(X);
            bs->Read(Y);
            bs->Read(Z);
            if (glm::distance({X, Y, Z}, position) < 100)
                importantEvents.emplace_back(fmt::format("An explostion appear at {} {} {}", X, Y, Z));

            break;
        }
        case RPC_ApplyAnimation: {
            break;
        }
        case RPC_GivePlayerMoney: {
            //importantEvents.emplace_back(fmt::format("You've given money"));
            break;
        }
        case RPC_GivePlayerWeapon: {
            //importantEvents.emplace_back(fmt::format("You've given gta weapon {} (ID:{}) with ammo"));
            break;
        }
        case RPC_ResetPlayerWeapons: {
            //importantEvents.emplace_back(fmt::format("Your weapon was reset"));
            break;
        }
        case RPC_WorldPlayerAdd: {
            unsigned short wPlayerID;
            BYTE team;
            int dSkinId;
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
            importantEvents.emplace_back(fmt::format("Player {} (ID:{}) has enter your streaming range at {} {} {}",
                                                     CApp::getInstance()->getResourceManager()->getPlayerName({host, port}, playerID), playerID, PosX, PosY, PosZ));
            break;
        }
        case RPC_WorldPlayerRemove: {
            UINT16 wPlayerID;
            bs->Read(wPlayerID);
            importantEvents.emplace_back(fmt::format("Player {} (ID:{}) has leave your streaming range",
                                                     CApp::getInstance()->getResourceManager()->getPlayerName({host, port}, playerID), playerID));
            break;
        }
    }
}

void CBot::process() {
    CRakBot::process();
    auto dwThisTick = GetTickCount();
    if (getStatus() == SPAWNED) {
        // BOT 是否死了
        if (getFlag(IS_DEAD)) {
            if (dwThisTick - deathTick > 4000) {
                // 复活
                health = 100;
                sendSpawn();
            }
        } else {
            //活着
            // BOT 有没有移动
            if (getFlag(IS_MOVING)) {
                DWORD dwMoveTick = dwThisTick - m_dwMoveStartTime;
                if (dwMoveTick < m_dwMoveTime) {
                    auto vecNewPosition = getPosition();
                    auto vecVelocity = getVelocity();
                    int iTickDiff = dwThisTick - m_dwMoveTickCount;
                    if (iTickDiff > 0) {
                        vecNewPosition += vecVelocity * static_cast<float>(iTickDiff);
                    }
                    setPosition(vecNewPosition);
                    m_dwMoveTickCount = dwThisTick;
                } else if (dwMoveTick > m_dwMoveTime + m_dwMoveStopDelay) {
                    // Check if we're following a movepath
                    if (movepathStatus == MOVEPATH_ACTIVE && !movepath.empty()) {
                        // Mark current waypoint as reached
                        waypointReached = true;
                        
                        // Move to next waypoint
                        currentWaypointIndex++;
                        
                        // Check if we've reached the end
                        if (currentWaypointIndex >= movepath.size()) {
                            if (movepathLooping) {
                                // Loop back to first waypoint
                                currentWaypointIndex = 0;
                            } else {
                                // Movepath completed
                                movepathStatus = MOVEPATH_COMPLETED;
                                stop();
                                return;
                            }
                        }
                        
                        // Start moving to next waypoint using stored parameters
                        const glm::vec3& nextWaypoint = movepath[currentWaypointIndex];
                        int moveType = (m_iMoveType != 0) ? m_iMoveType : MOVE_TYPE_RUN;
                        float moveSpeed = (m_fMoveSpeed > 0) ? m_fMoveSpeed : MOVE_SPEED_RUN;
                        
                        go(nextWaypoint, moveType, 0.0f, true, moveSpeed, 0.0f, 0);
                    } else {
                        // Regular movement stop
                        stop();
                    }
                }
            } // MOVING?

            // 更新onfoot数据到服务器
            auto updateTick = dwThisTick - getUpdateTick();
            if (updateTick > 40) {
                updateOnfoot();
                setUpdateTick(dwThisTick);
            }
        } // DEAD?
    }
}

void CBot::on_bullet_data(stBulletData *data) {
    CRakBot::on_bullet_data(data);
    auto damage_amount = WeaponConfig::GetWeaponDamage(data->byteWeaponID);
    if (on_take_damage(data->byteWeaponID, damage_amount)) {
        // 模拟受伤过程
        if (armor > 0) {
            armor -= damage_amount;
            if (armor < 0)
                health += armor; // 负数
        } else {
            health -= damage_amount;
        }
        if (health <= 0) {
            // 生命变0就杀死
            kill(data->byteWeaponID, playerID);
        }
    }
}

void CBot::sendDialogResponse(bool leftButton, std::string content, int listitem) {
    RakNet::BitStream response;
    response.Reset();
    response.Write(static_cast<INT16>(dialogID));
    response.Write(static_cast<UINT8>(leftButton));
    response.Write(static_cast<INT16>(listitem));
    response.Write(static_cast<UINT8>(content.size()));
    response.Write(content.data(), content.size());
    sendRPC(RPC_DialogResponse, &response);
    dialogActive = false;
}

void CBot::sendPickup(int pickupid) {
    RakNet::BitStream bsSend;
    bsSend.Write(pickupid);
    sendRPC(RPC_PickedUpPickup, &bsSend);
}

std::vector<std::string> *CBot::getImportantEvents() {
    return &importantEvents;
}

void CBot::clearImportantEvents() {
    importantEvents.clear();
}

std::vector<std::string> *CBot::getUnreadChatMessage() {
    return &unreadChatMessage;
}

void CBot::clearUnreadChatMessage() {
    unreadChatMessage.clear();
}

bool CBot::isDialogActive() {
    return dialogActive;
}

nlohmann::json CBot::getDialogJson() {
    std::string s;
    switch (dialogStyle) {
        case DIALOG_STYLE_MSGBOX: {
            s = "message_box";
            break;
        }
        case DIALOG_STYLE_INPUT: {
            s = "input_box";
            break;
        }
        case DIALOG_STYLE_PASSWORD: {
            s = "input_password_box";
            break;
        }
        case DIALOG_STYLE_LIST: {
            s = "list_box";
            break;
        }
        case DIALOG_STYLE_TABLIST:
        case DIALOG_STYLE_TABLIST_HEADERS: {
            s = "tablist_box";
            break;
        }
        default: s = "Unknown";
    }
    return nlohmann::json{
        {"title", dialogTitle},
        {"type",s},
        {"content", dialogContent},
        {"left_button", dialogButtonLeft},
        {"right_button", dialogButtonRight}
    };
}

void CBot::addMessageToChatbox(const std::string &content) {
    chatbox.emplace_back(content);
    if (chatbox.size() > MAX_CHATBOX_SIZE) {
        chatbox.pop_front();
    }
}



nlohmann::json CBot::generateStateJson() {
    using json = nlohmann::json;

    json state;

    state["position"] = {
        {"x", round_to(position.x)},
        {"y", round_to(position.y)},
        {"z", round_to(position.z)},
        {"zone", MapZones::GetMapZoneName(MapZones::GetMapZoneAtPoint2D(position.x, position.y))}
    };
    state["status"] = getStatusName();
    state["health"] = round_to(health);
    state["armor"] = round_to(armor);
    auto addr = std::make_pair(host, port);
    auto P = CApp::getInstance()->getResourceManager()->getPlayersInRange(addr, position, 300.0f, true);
    if (P.empty()) {
        state["streamed_players"] = {};
    } else {
        for (auto& player : P) {
            state["streamed_players"].emplace_back(json {
                {"name", player.name},
                {"health", round_to(player.health)},
                {"weapon", WeaponConfig::GetWeaponName(player.weapon)},
                {"distance", round_to(glm::distance(player.position, position))},
                {"x", round_to(position.x)},
                {"y", round_to(position.y)},
                {"z", round_to(position.z)},
            });
        }
    }
    state["streamed_vehicles"] =  CApp::getInstance()->getResourceManager()->getVehiclesInRange(addr, position, 300.0f).size();
    state["streamed_pickups"] = getStreamableResources().getPickupsInRange(position, 300.0f).size();
    state["streamed_3d_labels"] = getStreamableResources().getLabelsInRange(position, 300.0f).size();

    state["is_moving"] = getFlag(IS_MOVING);
    for (const auto &it: *getUnreadChatMessage()) {
        state["new_chat_message"].emplace_back(it);
    }
    for (const auto &it: *getImportantEvents()) {
        state["important_events"].emplace_back(it);
    }
    // 为下一次清除
    clearImportantEvents();
    clearUnreadChatMessage();
    if (isDialogActive()) {
        state["dialog"] = getDialogJson();
        state["has_active_dialog"] = true;
    } else {
        state["has_active_dialog"] = false;
    }
    return state;
}

// =================================================================
// MOVEPATH SYSTEM IMPLEMENTATION
// =================================================================

void CBot::createMovepath(const std::vector<glm::vec3>& waypoints, bool loop) {
    movepath = waypoints;
    currentWaypointIndex = 0;
    movepathLooping = loop;
    movepathStatus = MOVEPATH_INACTIVE;
    waypointReached = false;
}

void CBot::addWaypoint(const glm::vec3& position) {
    movepath.push_back(position);
}

void CBot::clearMovepath() {
    movepath.clear();
    currentWaypointIndex = 0;
    movepathStatus = MOVEPATH_INACTIVE;
    waypointReached = false;
    
    // Stop current movement if active
    if (getFlag(IS_MOVING)) {
        stop();
    }
}

void CBot::startMovepath() {
    if (movepath.empty()) {
        spdlog::warn("Cannot start movepath: no waypoints defined");
        return;
    }
    
    currentWaypointIndex = 0;
    movepathStatus = MOVEPATH_ACTIVE;
    waypointReached = false;
    
    // Start moving to first waypoint using existing movement logic
    // Use stored parameters if available, otherwise use defaults
    const glm::vec3& waypoint = movepath[currentWaypointIndex];
    int moveType = (m_iMoveType != 0) ? m_iMoveType : MOVE_TYPE_RUN;
    float moveSpeed = (m_fMoveSpeed > 0) ? m_fMoveSpeed : MOVE_SPEED_RUN;
    
    go(waypoint, moveType, 0.0f, true, moveSpeed, 0.0f, 0);
}

void CBot::pauseMovepath() {
    if (movepathStatus == MOVEPATH_ACTIVE) {
        movepathStatus = MOVEPATH_PAUSED;
        stop();
    }
}

void CBot::resumeMovepath() {
    if (movepathStatus == MOVEPATH_PAUSED && !movepath.empty() && currentWaypointIndex < movepath.size()) {
        movepathStatus = MOVEPATH_ACTIVE;
        
        // Resume movement to current waypoint using stored parameters
        const glm::vec3& waypoint = movepath[currentWaypointIndex];
        int moveType = (m_iMoveType != 0) ? m_iMoveType : MOVE_TYPE_RUN;
        float moveSpeed = (m_fMoveSpeed > 0) ? m_fMoveSpeed : MOVE_SPEED_RUN;
        
        go(waypoint, moveType, 0.0f, true, moveSpeed, 0.0f, 0);
    }
}

void CBot::stopMovepath() {
    movepathStatus = MOVEPATH_INACTIVE;
    waypointReached = false;
    
    if (getFlag(IS_MOVING)) {
        stop();
    }
}

CBot::eMovepathStatus CBot::getMovepathStatus() const {
    return movepathStatus;
}

size_t CBot::getCurrentWaypointIndex() const {
    return currentWaypointIndex;
}

size_t CBot::getWaypointCount() const {
    return movepath.size();
}

bool CBot::isMovepathLooping() const {
    return movepathLooping;
}

float CBot::getDistanceToCurrentWaypoint() const {
    if (movepath.empty() || currentWaypointIndex >= movepath.size()) {
        return -1.0f;
    }
    
    return glm::distance(position, movepath[currentWaypointIndex]);
}

const glm::vec3* CBot::getCurrentWaypoint() const {
    if (movepath.empty() || currentWaypointIndex >= movepath.size()) {
        return nullptr;
    }
    
    return &movepath[currentWaypointIndex];
}
