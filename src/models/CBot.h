// Created by Administrator on 2024/10/9.
//

#ifndef CBOT_H
#define CBOT_H

#include <deque>
#include <string>
#include <vector>
#include <memory>

#include "CRakBot.h"
#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"
#include "hv/json.hpp"

class CBotManager;
class CPathFinder;

class CBot : public CRakBot {
public:
    // === Type Definitions ===
    enum eBotFlags {
        IS_MOVING = 1,
        IS_DEAD,
        IS_DRIVING,
        IS_AIMING,
        IS_RELOADING,
        IS_SHOOTING,
        IS_JACKING,
        IS_EXITING,
        IS_PLAYING,
        IS_MELEEATTACK,
        IS_UNMOVING
    };

    enum eMovepathStatus {
        MOVEPATH_INACTIVE = 0,
        MOVEPATH_ACTIVE,
        MOVEPATH_PAUSED,
        MOVEPATH_COMPLETED
    };

    // === Construction/Destruction ===
    CBot(std::string identifier);
    CBot(std::string identifier, std::string uuid);
    ~CBot();

    void init();

    // === Configuration ===
    std::string getPassword() const;
    void setPassword(const std::string& newPassword);
    std::string getSystemPrompt() const;
    void setSystemPrompt(const std::string& newSystemPrompt);

    // === Physical State Access ===
    glm::vec3 getPosition() const;
    glm::vec3 getVelocity() const;
    glm::quat getQuaternion() const;
    float getArmor() const;
    float getHealth() const;
    float getAngle() const;

    // === Physical State Modification ===
    void setPosition(const glm::vec3& newPosition);
    void setVelocity(const glm::vec3& newVelocity);
    void setQuaternion(const glm::quat& newQuaternion);
    void setArmor(float newArmor);
    void setHealth(float newHealth);
    void setAngle(float newAngle);

    // === Bot Flags ===
    bool getFlag(int bit) const;
    void toggleFlag(int bit);
    void setFlag(int bit, bool state);

    // === Movement System ===
    void updateMovingData(glm::vec3 vecDestination, float fRadius, bool bSetAngle, float fSpeed, float fDistOffset);
    void go(const glm::vec3 &vecPoint, int iType, float fRadius, bool bSetAngle, float fSpeed, float fDistOffset, int dwStopDelay);
    void stop();
    void updateOnfoot();

    // Pathfinding integration
    void go_with_path(const glm::vec3& destination, int iType = 1, float fSpeed = 0.56444f); // MOVE_TYPE_RUN, MOVE_SPEED_RUN

    // Movepath system
    void createMovepath(const std::vector<glm::vec3>& waypoints, bool loop = false);
    void addWaypoint(const glm::vec3& position);
    void clearMovepath();
    void startMovepath();
    void pauseMovepath();
    void resumeMovepath();
    void stopMovepath();

    // Movepath getters
    eMovepathStatus getMovepathStatus() const;
    size_t getCurrentWaypointIndex() const;
    size_t getWaypointCount() const;
    bool isMovepathLooping() const;
    float getDistanceToCurrentWaypoint() const;
    const glm::vec3* getCurrentWaypoint() const;

    // === Input/Control ===
    void setKeys(WORD wUDAnalog, WORD wLRAnalog, DWORD dwKeys);
    void getKeys(WORD *pwUDAnalog, WORD *pwLRAnalog, DWORD *pdwKeys);

    // === Game Actions ===
    void kill(int bReason = -1, int wKillerID = -1); // 杀死bot，等待重生
    void sendDialogResponse(bool leftButton, std::string content, int listitem);
    void sendPickup(int pickupid);

    // === Event Management ===
    std::vector<std::string>* getImportantEvents();
    void clearImportantEvents();
    std::vector<std::string>* getUnreadChatMessage();
    void clearUnreadChatMessage();

    // === UI State ===
    bool isDialogActive();
    nlohmann::json getDialogJson();

    // === State Serialization ===
    nlohmann::json generateStateJson();

    // === Custom Event Callbacks ===
    void on_spawned();
    bool on_take_damage(int weapon, float damage);

    // === Override Virtual Functions ===
    void on_receive_rpc(int id, RakNet::BitStream *bs) override;
    void on_bullet_data(stBulletData *data) override;
    void process() override;

private:
    // === Configuration Data ===
    std::string password; // if necessary
    std::string systemPrompt; // LLM system prompt
    
    // === Pathfinding ===
    std::shared_ptr<CPathFinder> pathFinder;

    // === Physical State ===
    glm::vec3 position;
    glm::vec3 velocity;
    glm::qua<float> quaternion;
    float angle;
    float armor;
    float health;
    bool invulnerable;
    std::vector<int> weapons;
    int skin;

    // === Movement State ===
    glm::vec3 vecDestination;
    int m_iMovePath;
    int m_iMovePoint;
    int m_iMoveType;
    int m_iMoveMode;
    int m_iMovePathfinding;
    float m_fMoveRadius;
    bool m_bMoveSetAngle;
    float m_fMoveSpeed;
    float m_fDistOffset;
    int m_iNodeMoveType;
    int m_iNodeMoveMode;
    float m_fNodeMoveRadius;
    bool m_bNodeMoveSetAngle;
    float m_fNodeMoveSpeed;
    int m_dwMoveStartTime;
    int m_dwMoveTime;
    int m_dwMoveStopDelay;
    int m_dwMoveTickCount;

    // === Movepath System ===
    std::vector<glm::vec3> movepath;
    size_t currentWaypointIndex;
    eMovepathStatus movepathStatus;
    bool movepathLooping;
    bool waypointReached;

    // === UI State ===
    std::deque<std::string> chatbox; // 模拟记录聊天框，最多64
    std::vector<std::string> unreadChatMessage; // 新的聊天消息
    int dialogID;
    int dialogStyle;
    std::string dialogTitle;
    std::string dialogContent;
    std::string dialogButtonLeft;
    std::string dialogButtonRight;
    bool dialogActive; //目前是否有对话框

    // === Bot State ===
    int flag;
    int deathTick;

    // === Cached Data ===
    stOnFootData data_onfoot;

    // === Event System ===
    std::vector<std::string> importantEvents; // 重要事件 用于构建llm 上下文

    // === Helper Methods ===
    void addMessageToChatbox(const std::string& content);

    // === Constants ===
    static constexpr int MAX_CHATBOX_SIZE = 64;
};

#endif //CBOT_H
