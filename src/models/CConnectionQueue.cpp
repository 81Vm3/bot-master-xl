//
// Created by rain on 8/3/25.
//

#include "CConnectionQueue.h"
#include "CApp.h"
#include "core/CPersistentDataStorage.h"

#include "CBot.h"
#include <map>
#include <vector>

CConnectionQueue::CConnectionQueue(eConnectionPolicy policy) {
    this->policy = policy;
}

int CConnectionQueue::try_connect() {
    auto& bots = CApp::getInstance()->getDatabase()->vBots;
    int ans = 0;

    // bot that is connecting or disconnected to the server
    std::map<std::pair<std::string, int>, std::vector<CBot*>> serverBotMap;


    // check for connecting bot
    for (auto& bot : bots) {
        if (!bot->isGameInited() && bot->getStatus() != CBot::DISCONNECTED)
            serverBotMap[{bot->getHost(), bot->getPort()}].emplace_back(bot.get());
    }

    for (auto& bot : bots) {
        if (bot->getStatus() == CBot::DISCONNECTED) {
            if (!bot->checkConectionDelay())
                continue;

            if (policy == QUEUED) {
                // 同一服务器同一时间只能连一个bot
                if (serverBotMap[{bot->getHost(), bot->getPort()}].empty()) {
                    serverBotMap[{bot->getHost(), bot->getPort()}].emplace_back(bot.get());
                }
            } else {
                // AGGRESSIVE - unlimited connections
                serverBotMap[{bot->getHost(), bot->getPort()}].emplace_back(bot.get());
            }
        }
    }

    // make disconnected bot connect to the server
    for (auto& serverEntry : serverBotMap) {
        for (auto& bot : serverEntry.second) {
            if (bot->getStatus() == CBot::DISCONNECTED) {
                ans++;
                bot->connect();
            }
        }
    }

    return ans;
}
