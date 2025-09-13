#include <chrono>
#include <thread>
#include <memory>
#include <csignal>
#include <atomic>
#include <hv/HttpServer.h>

#include "CApp.h"
#include "models/CBot.h"
#include "core/CConfig.h"
#include "core/CLLMBotSessionManager.h"
#include "core/CPersistentDataStorage.h"
#include "models/CConnectionQueue.h"
#include "models/CServer.h"
#include "spdlog/spdlog.h"

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
