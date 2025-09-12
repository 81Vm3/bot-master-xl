//
// Created by rain on 8/1/25.
//

#ifndef CSERVERQUERIER_H
#define CSERVERQUERIER_H

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include "../models/CServer.h"

class CPersistentDataStorage;

class CServerQuerier {
public:
    CServerQuerier();
    ~CServerQuerier();
    
    // Initialize with database reference
    void initialize(CPersistentDataStorage* dataStorage);
    
    // Start/stop the querier
    void start();
    void stop();
    bool isRunning() const;
    
    // Set query interval (default 30 seconds)
    void setQueryInterval(std::chrono::seconds interval);
    std::chrono::seconds getQueryInterval() const;
    
    // Callbacks for query results
    void setOnServerUpdated(std::function<void(CServer*)> callback);
    void setOnServerOffline(std::function<void(CServer*)> callback);

    // Query a single server asynchronously
    void queryServer(CServer* server);

    // Update server's last_update in database
    void updateServerInDatabase(CServer* server);

private:
    CPersistentDataStorage* pDataStorage;
    std::atomic<bool> running;
    std::unique_ptr<std::thread> queryThread;
    std::chrono::seconds queryInterval;
    
    // Callbacks
    std::function<void(CServer*)> onServerUpdated;
    std::function<void(CServer*)> onServerOffline;

    // Main query loop
    void queryLoop();
};

#endif //CSERVERQUERIER_H
