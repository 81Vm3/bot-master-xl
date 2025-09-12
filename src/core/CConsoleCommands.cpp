#include "CConsoleCommands.h"
#include "../CApp.h"
#include "../models/CBot.h"
#include "../models/CLLMBotSession.h"
#include "../core/CLLMBotSessionManager.h"
#include "../core/CPersistentDataStorage.h"
#include "../core/CConfig.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

void CConsoleCommands::registerBotCommands(CConsole* console) {
    console->registerCommand("bots", {
        "List all bots with their status",
        [](const std::vector<std::string>&) {
            auto& bots = CApp::getInstance()->getDatabase()->vBots;
            auto console = CApp::getInstance()->getConsole();
            
            if (bots.empty()) {
                console->println("No bots available.");
                return;
            }
            
            console->println("\n=== Bot Status ===");
            std::ostringstream header;
            header << std::left << std::setw(20) << "Name" 
                   << std::setw(15) << "Status" 
                   << std::setw(20) << "Server"
                   << std::setw(10) << "Port";
            console->println(header.str());
            console->println(std::string(65, '-'));
            
            for (const auto& bot : bots) {
                std::string status = bot->isConnected() ? "Connected" : "Disconnected";
                std::ostringstream row;
                row << std::left << std::setw(20) << bot->getName()
                    << std::setw(15) << status
                    << std::setw(20) << bot->getHost()
                    << std::setw(10) << std::to_string(bot->getPort());
                console->println(row.str());
            }
            console->println("");
        },
        "bots"
    });
    
    console->registerCommand("bot", {
        "Show detailed information about a specific bot",
        [](const std::vector<std::string>& args) {
            auto console = CApp::getInstance()->getConsole();
            
            if (args.size() < 2) {
                console->println("Usage: bot <bot_name>");
                return;
            }
            
            auto& bots = CApp::getInstance()->getDatabase()->vBots;
            std::string botName = args[1];
            
            auto it = std::find_if(bots.begin(), bots.end(),
                [&botName](const std::shared_ptr<CBot>& bot) {
                    return bot->getName() == botName;
                });
            
            if (it == bots.end()) {
                console->println("Bot '" + botName + "' not found.");
                return;
            }
            
            auto bot = *it;
            console->println("\n=== Bot Details: " + botName + " ===");
            console->println("Status: " + std::string(bot->isConnected() ? "Connected" : "Disconnected"));
            console->println("Host: " + bot->getHost());
            console->println("Port: " + std::to_string(bot->getPort()));
            console->println("UUID: " + bot->getUuid());
            
            auto sessionMgr = CApp::getInstance()->getLLMSessionManager();
            CLLMBotSession* session = sessionMgr->getLLMSessionFromBot(bot);
            if (session != nullptr) {
                console->println("LLM Session: Active");
            } else {
                console->println("LLM Session: None");
            }
            console->println("");
        },
        "bot <bot_name>"
    });
    
    console->registerCommand("connect", {
        "Connect a bot to its configured server",
        [](const std::vector<std::string>& args) {
            auto console = CApp::getInstance()->getConsole();
            
            if (args.size() < 2) {
                console->println("Usage: connect <bot_name>");
                return;
            }
            
            auto& bots = CApp::getInstance()->getDatabase()->vBots;
            std::string botName = args[1];
            
            auto it = std::find_if(bots.begin(), bots.end(),
                [&botName](const std::shared_ptr<CBot>& bot) {
                    return bot->getName() == botName;
                });
            
            if (it == bots.end()) {
                console->println("Bot '" + botName + "' not found.");
                return;
            }
            
            auto bot = *it;
            if (bot->isConnected()) {
                console->println("Bot '" + botName + "' is already connected.");
                return;
            }
            
            console->println("Attempting to connect bot '" + botName + "'...");
            // Note: Actual connection logic handled by the main loop and connection queue
        },
        "connect <bot_name>"
    });
    
    console->registerCommand("disconnect", {
        "Disconnect a bot from its server",
        [](const std::vector<std::string>& args) {
            auto console = CApp::getInstance()->getConsole();
            
            if (args.size() < 2) {
                console->println("Usage: disconnect <bot_name>");
                return;
            }
            
            auto& bots = CApp::getInstance()->getDatabase()->vBots;
            std::string botName = args[1];
            
            auto it = std::find_if(bots.begin(), bots.end(),  
                [&botName](const std::shared_ptr<CBot>& bot) {
                    return bot->getName() == botName;
                });
            
            if (it == bots.end()) {
                console->println("Bot '" + botName + "' not found.");
                return;
            }
            
            auto bot = *it;
            if (!bot->isConnected()) {
                console->println("Bot '" + botName + "' is already disconnected.");
                return;
            }
            
            bot->disconnect();
            console->println("Bot '" + botName + "' disconnected.");
        },
        "disconnect <bot_name>"
    });
}

void CConsoleCommands::registerSystemCommands(CConsole* console) {
    console->registerCommand("status", {
        "Show system status and statistics",
        [](const std::vector<std::string>&) {
            auto console = CApp::getInstance()->getConsole();
            auto app = CApp::getInstance();
            
            console->println("\n=== System Status ===");
            console->println("Uptime: " + std::to_string(app->getUptime().count()) + "ms");
            
            auto& bots = app->getDatabase()->vBots;
            int connectedBots = std::count_if(bots.begin(), bots.end(),
                [](const std::shared_ptr<CBot>& bot) { return bot->isConnected(); });
            
            console->println("Total Bots: " + std::to_string(bots.size()));
            console->println("Connected Bots: " + std::to_string(connectedBots));
            console->println("Disconnected Bots: " + std::to_string(bots.size() - connectedBots));
            
            auto sessionMgr = app->getLLMSessionManager();
            console->println("Active LLM Sessions: " + std::to_string(sessionMgr->getActiveSessionCount()));
            console->println("");
        },
        "status"
    });
    
    console->registerCommand("config", {
        "Show current configuration",
        [](const std::vector<std::string>&) {
            auto console = CApp::getInstance()->getConsole();
            auto config = CApp::getInstance()->getConfig();
            
            console->println("\n=== Configuration ===");
            console->println("Connection Policy: " + std::to_string(static_cast<int>(config->connection_policy)));
            console->println("");
        },
        "config"
    });
    
    console->registerCommand("threads", {
        "Show thread information",
        [](const std::vector<std::string>&) {
            auto console = CApp::getInstance()->getConsole();
            
            console->println("\n=== Thread Information ===");
            console->println("Main Thread: Bot processing loop");
            console->println("API Server Thread: HTTP API server");
            console->println("Console Thread: Debug console (current)");
            console->println("");
        },
        "threads"
    });
}

void CConsoleCommands::registerLLMCommands(CConsole* console) {
    console->registerCommand("llm", {
        "Show LLM session information",
        [](const std::vector<std::string>& args) {
            auto console = CApp::getInstance()->getConsole();
            auto sessionMgr = CApp::getInstance()->getLLMSessionManager();
            
            if (args.size() > 1 && args[1] == "sessions") {
                console->println("\n=== LLM Sessions ===");
                console->println("Active sessions: " + std::to_string(sessionMgr->getActiveSessionCount()));
                console->println("");
            } else {
                console->println("\n=== LLM Information ===");
                console->println("Session Manager: Active");
                console->println("Total Sessions: " + std::to_string(sessionMgr->getActiveSessionCount()));
                console->println("");
                console->println("Usage: llm sessions - Show detailed session info");
            }
        },
        "llm [sessions]"
    });
}