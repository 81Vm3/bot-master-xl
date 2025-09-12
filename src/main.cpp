#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <csignal>
#include <atomic>
#include <hv/HttpServer.h>

#include "utils/uuid.h"
#include "CApp.h"
#include "models/CBot.h"
#include "core/CConfig.h"
#include "core/CConsole.h"
#include "core/CLLMBotSessionManager.h"
#include "core/CLogger.h"
#include "core/CPersistentDataStorage.h"
#include "core/CSharedResourcePool.h""
#include "models/CConnectionQueue.h"
#include "models/CServer.h"
#include "physics/Raycast.h"
#include "spdlog/spdlog.h"
#include "utils/defines.h"

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    if (signal == SIGINT) {
        spdlog::info("Received SIGINT (Ctrl+C), initiating shutdown...");
        g_running.store(false);
    }
}

int main() {
    using namespace std;
    using namespace hv;
    // Setup signal handler for Ctrl+C
    std::signal(SIGINT, signalHandler);

    CApp::getInstance()->init();

    // main thread is for raknet bots..
    auto& bots = CApp::getInstance()->getDatabase()->vBots;

    // auto test_bot = std::make_shared<CBot>("qwen_ai_flash");
    // test_bot->setPassword("kawaiiangel2025");
    // bots.emplace_back(test_bot);
    // test_bot->setHost("gta-sa.cn");
    // test_bot->setPort(2023);
    //
    // auto llm_provider = std::make_shared<CLLMProvider>(-1,
    //     "智谱AI",
    //     "2ba8c820921e47a8ad6139c6e5ba2430.UZrmwEp1t9tSzL5G",
    //     "http://open.bigmodel.cn/api/paas/v4/chat/completions",
    //     "GLM-4.5-Flash");
    // auto llm_provider = std::make_shared<CLLMProvider>(-1,
    //         "Qwen",
    //         "sk-d10d5313d3f0433a931d9e606e41d0b8",
    //         "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions",
    //         "qwen-plus");
    // auto llm_provider = std::make_shared<CLLMProvider>(-1,
    //     "Deepseek",
    //     "sk-90dbe10ac1854fa9aae46484160014c2",
    //     "https://api.deepseek.com/chat/completions",
    //     "deepseek-chat");

    // /auto llm_session = CApp::getInstance()->getLLMSessionManager()->createSession(test_bot, llm_provider);

    CConnectionQueue queue(CApp::getInstance()->getConfig()->connection_policy);

    while (g_running.load()) {
        queue.try_connect();
        for (auto& bot : bots) {
            if (!g_running.load()) {
                break;
            }
            bot->process();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Graceful shutdown
    spdlog::info("Shutting down...");
    
    // Disconnect all bots
    for (auto& bot : bots) {
        if (bot->isConnected()) {
            bot->disconnect();
        }
    }
    
    spdlog::info("BotMasterXL shutdown complete.");
    return 0;
}
