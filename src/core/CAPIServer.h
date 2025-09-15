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
#include "utils/ds/Trie.h"
#include <map>

class CAPIServer {
public:
    CAPIServer();
    ~CAPIServer();

    void start_async();
    void stop_async();
    bool is_running() const;

    void load_website_dist(const std::string& path);
    int on_static_request(HttpRequest* req, HttpResponse* resp);

private:
    std::unique_ptr<hv::HttpService> router;
    std::unique_ptr<hv::HttpServer> server;
    std::vector<std::unique_ptr<CBaseService>> services;
    bool running = false;

    // Static file serving
    Trie trie;
    static std::map<std::string, std::string> mime_types;
    std::string get_mime_type(const std::string& path);
};



#endif //CAPISERVER_H
