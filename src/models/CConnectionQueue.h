//
// Created by rain on 8/3/25.
//

#ifndef CCONNECTIONQUEUE_H
#define CCONNECTIONQUEUE_H
#include <map>
#include <string>

// Forward declarations
class CBot;

enum eConnectionPolicy {
    QUEUED,     // 同一时间只允许一个bot连接到同一服务器
    AGGRESSIVE  // bot的连接无限制
};

class CConnectionQueue {
public:
    CConnectionQueue(eConnectionPolicy policy);
    int try_connect();
private:
    eConnectionPolicy policy;
};

#endif //CCONNECTIONQUEUE_H
