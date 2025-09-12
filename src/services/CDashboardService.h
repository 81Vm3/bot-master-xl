//
// Created by rain on 8/3/25.
//

#ifndef CDASHBOARDSERVICE_H
#define CDASHBOARDSERVICE_H
#include "CBaseService.h"
#include "hv/HttpService.h"


class CDashboardService : public CBaseService {
public:
    CDashboardService(const std::string& uri);
    void on_install(HttpService *router) override;

private:
    static int get_runtime(HttpRequest* req, HttpResponse* resp);
    static int get_bot_stats(HttpRequest* req, HttpResponse* resp);
    static int get_server_stats(HttpRequest* req, HttpResponse* resp);
};



#endif //CDASHBOARDSERVICE_H