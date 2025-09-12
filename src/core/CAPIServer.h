//
// Created by rain on 8/1/25.
//

#ifndef CAPISERVER_H
#define CAPISERVER_H
#include <memory>
#include <vector>

#include <hv/HttpServer.h>
#include <hv/HttpService.h>

#include "../services/CBaseService.h"

class CAPIServer {
public:
    CAPIServer();
    ~CAPIServer();

    void start_async();
    void stop_async();
    bool is_running() const;

private:
    std::unique_ptr<hv::HttpService> router;
    std::unique_ptr<hv::HttpServer> server;
    std::vector<std::unique_ptr<CBaseService>> services;
    bool running = false;
};



#endif //CAPISERVER_H
