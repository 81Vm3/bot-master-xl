//
// Created by rain on 8/1/25.
//

#include "CAPIServer.h"

#include "../CApp.h"
#include "CConfig.h"
#include "../services/CServerService.h"
#include "../services/CBotService.h"
#include "services/CDashboardService.h"
#include "services/CLLMProviderService.h"

CAPIServer::CAPIServer() {
    router = std::make_unique<hv::HttpService>();
    server = std::make_unique<hv::HttpServer>();

    services.emplace_back(std::make_unique<CServerService>("server"));
    services.emplace_back(std::make_unique<CBotService>("bot"));
    services.emplace_back(std::make_unique<CDashboardService>("dashboard"));
    services.emplace_back(std::make_unique<CLLMProviderService>("llm"));

    for (auto& service : services) {
        service->on_install(router.get());
    }

    // Enable CORS for all endpoints
    router->AllowCORS();

    server->setPort(CApp::getInstance()->getConfig()->api_port);
    server->setThreadNum(4);
    server->service = router.get();
}

CAPIServer::~CAPIServer() {
    if (running) {
        stop_async();
    }
}

void CAPIServer::start_async() {
    if (!running) {
        server->start();
        running = true;
    }
}

void CAPIServer::stop_async() {
    if (running) {
        server->stop();
        running = false;
    }
}

bool CAPIServer::is_running() const {
    return running;
}
