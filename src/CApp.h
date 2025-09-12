//
// Created by rain on 7/13/25.
//

#ifndef CAPP_H
#define CAPP_H
#include <memory>
#include <vector>
#include <chrono>

#include "models/CServer.h"
#include "spdlog/spdlog.h"

class ColAndreasWorld;
class CServerQuerier;
class CSharedResourcePool;
class CPersistentDataStorage;
class CAPIServer;
class CConfig;
class CFunctionDispatcher;
class CLLMBotSessionManager;
class ObjectNameUtil;
class CConsole;

class CApp {
private:
    static CApp* instance;
    CApp();
    CApp(const CApp&) = delete;
    CApp& operator=(const CApp&) = delete;

    std::unique_ptr<CConfig> pConfig;
    std::unique_ptr<CAPIServer> pAPIServer;
    std::unique_ptr<CPersistentDataStorage> pDataStorage;
    std::unique_ptr<CServerQuerier> pServerQuerier;
    std::unique_ptr<CFunctionDispatcher> pFunctionDispatcher;
    std::unique_ptr<CSharedResourcePool> pResourceManager;
    std::unique_ptr<CLLMBotSessionManager> pLLMSessionManager;
    std::unique_ptr<ObjectNameUtil> pObjectNameUtil;
    std::unique_ptr<CConsole> pConsole;
    ColAndreasWorld* pColAndreasWorld;

    // Runtime tracking
    std::chrono::steady_clock::time_point startTime;

public:
    static CApp* getInstance();
    void run();
    void init();

    CConfig* getConfig();
    CAPIServer* getAPIServer();
    CPersistentDataStorage* getDatabase();
    CServerQuerier* getServerQuerier();
    CFunctionDispatcher* getFunctionDispatcher();
    CSharedResourcePool* getResourceManager();
    CLLMBotSessionManager* getLLMSessionManager();
    ObjectNameUtil* getObjectNameUtil();
    CConsole* getConsole();
    ColAndreasWorld* getColAndreas();

    // Runtime tracking
    std::chrono::milliseconds getUptime() const;

    ~CApp();
};


#endif //CAPP_H
