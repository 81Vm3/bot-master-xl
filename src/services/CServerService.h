//
// Created by rain on 8/1/25.
//

#ifndef CSERVERSERVICE_H
#define CSERVERSERVICE_H
#include "CBaseService.h"
#include "hv/HttpService.h"


class CServerService : public CBaseService {
public:
    CServerService(const std::string& uri);
    void on_install(HttpService *router) override;

private:
    static int delete_server(HttpRequest* req, HttpResponse* resp);
    static int add_server(HttpRequest* req, HttpResponse* resp);
    static int list_servers(HttpRequest* req, HttpResponse* resp);
    static int query_server(HttpRequest* req, HttpResponse* resp);
};



#endif //CSERVERSERVICE_H
