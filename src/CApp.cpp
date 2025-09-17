//
// Created by rain on 7/13/25.
//

#include "CApp.h"

#include "singal.h"
#include "core/CAPIServer.h"
#include "core/CConfig.h"
#include "core/CSharedResourcePool.h"
#include "utils/CFunctionDispatcher.h"
#include "core/CPersistentDataStorage.h"
#include "core/CServerQuerier.h"
#include "core/CLLMBotSessionManager.h"
#include "utils/ObjectNameUtil.h"
#include "core/CLogger.h"
#include "tools/BotLLMTools.h"
#include "core/CConsole.h"
#include "core/CConsoleCommands.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "vendor/ColAndreas/DynamicWorld.h"

// ColAndreas stuff
bool colInit = false;
bool colDataLoaded = false;
ColAndreasWorld *collisionWorld;

// Initialize the static member outside the class definition.
// This is crucial for the linker to find the definition of the static member.
CApp* CApp::instance = nullptr;

// Definition of the private constructor
CApp::CApp() {
}

// Definition of the static getInstance method
CApp* CApp::getInstance() {
    // Lazy initialization: instance is created only when it's first requested
    if (instance == nullptr) {
        instance = new CApp();
    }
    return instance;
}

CConfig * CApp::getConfig() {
    return pConfig.get();
}

CAPIServer * CApp::getAPIServer() {
    return pAPIServer.get();
}

CPersistentDataStorage * CApp::getDatabase() {
    return pDataStorage.get();
}

CServerQuerier * CApp::getServerQuerier() {
    return pServerQuerier.get();
}

CFunctionDispatcher * CApp::getFunctionDispatcher() {
    return pFunctionDispatcher.get();
}

CSharedResourcePool * CApp::getResourceManager() {
    return pResourceManager.get();
}

CLLMBotSessionManager * CApp::getLLMSessionManager() {
    return pLLMSessionManager.get();
}

ObjectNameUtil * CApp::getObjectNameUtil() {
    return pObjectNameUtil.get();
}


CConsole* CApp::getConsole() {
    return pConsole.get();
}

ColAndreasWorld * CApp::getColAndreas() {
    return pColAndreasWorld;
}

std::chrono::milliseconds CApp::getUptime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
}

void CApp::init() {
    // Init loggers
    CLogger::getInstance()->init();

    // Record start time
    startTime = std::chrono::steady_clock::now();

    CLogger::getInstance()->system->info("[CONFIG]: Initializing configuration system");
    pConfig = std::make_unique<CConfig>();
    if (pConfig->loadConfigFile("data/config.json") && pConfig->loadBaseInternalPrompt("data/prompt.md"))  {
        CLogger::getInstance()->system->info("[CONFIG]: Configuration loaded successfully");
    } else {
        CLogger::getInstance()->system->error("[CONFIG]: Failed to load configuration");
    }

    CLogger::getInstance()->system->info("[API]: Starting API server with static file serving");
    pAPIServer = std::make_unique<CAPIServer>();
    pAPIServer->start_async();
    CLogger::getInstance()->system->info("[API]: API server started successfully");

    CLogger::getInstance()->system->info("[LLM]: Initializing LLM bot session manager");
    pLLMSessionManager = std::make_unique<CLLMBotSessionManager>();
    CLogger::getInstance()->system->info("[LLM]: LLM bot session manager initialized");

    CLogger::getInstance()->system->info("[DATABASE]: Initializing persistent data storage");
    pDataStorage = std::make_unique<CPersistentDataStorage>();
    if (pDataStorage->loadDatabase()) {
        CLogger::getInstance()->system->info("[DATABASE]: Database loaded successfully");
    } else {
        CLogger::getInstance()->system->error("[DATABASE]: Database could not be loaded");
    }

    CLogger::getInstance()->system->info("[QUERIER]: Initializing server querier");
    pServerQuerier = std::make_unique<CServerQuerier>();
    pServerQuerier->initialize(pDataStorage.get());
    pServerQuerier->start();
    CLogger::getInstance()->system->info("[QUERIER]: Server querier started successfully");

    CLogger::getInstance()->system->info("[DISPATCHER]: Initializing function dispatcher");
    pFunctionDispatcher = std::make_unique<CFunctionDispatcher>();
    CLogger::getInstance()->system->info("[DISPATCHER]: Function dispatcher initialized");

    BotLLMTools::registerAllBotTools();

    CLogger::getInstance()->system->info("[RESOURCE]: Initializing resource manager");
    pResourceManager = std::make_unique<CSharedResourcePool>();
    CLogger::getInstance()->system->info("[RESOURCE]: Resource manager initialized");
    
    CLogger::getInstance()->system->info("[OBJECTS]: Initializing object name utility");
    pObjectNameUtil = std::make_unique<ObjectNameUtil>();
    // Try to load object names from default location
    if (pObjectNameUtil->loadFromFile("data/objects.txt")) {
        CLogger::getInstance()->system->info("[OBJECTS]: Object names loaded successfully");
    } else {
        CLogger::getInstance()->system->info("[OBJECTS]: No object names file found");
    }

    CLogger::getInstance()->system->info("[CONSOLE]: Initializing debug console");
    pConsole = std::make_unique<CConsole>();
    CConsoleCommands::registerBotCommands(pConsole.get());
    CConsoleCommands::registerSystemCommands(pConsole.get());
    CConsoleCommands::registerLLMCommands(pConsole.get());
    pConsole->start();
    CLogger::getInstance()->system->info("[CONSOLE]: Debug console started successfully");

    pColAndreasWorld = new ColAndreasWorld;
    collisionWorld = pColAndreasWorld;

    if (pConfig->enable_colandreas) {
        CLogger::getInstance()->system->info("[PHYSICS] Loading collision data.");
        if (collisionWorld->loadCollisionData()) {
            CLogger::getInstance()->system->info("[PHYSICS] Initializing collision map...");
            collisionWorld->colandreasInitMap();
            CLogger::getInstance()->system->info("[PHYSICS] Collision map initialized successfully");
            colDataLoaded = true;
        } else {
            CLogger::getInstance()->system->info("[PHYSICS] No collision data found!");
        }
    }

    if (g_running)
        CLogger::getInstance()->system->info("[INIT]: Application initialization completed successfully");
}

CApp::~CApp() {
    if (pColAndreasWorld)
        delete pColAndreasWorld;
    if (pConsole) {
        pConsole->stop();
    }
}
