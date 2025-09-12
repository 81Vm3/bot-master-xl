#pragma once

#include "CBaseService.h"
#include <hv/json.hpp>
#include <string>
#include <vector>
#include <memory>

using json = nlohmann::json;

class CLLMProviderService : public CBaseService {
public:
    CLLMProviderService(const std::string& uri);
    void on_install(HttpService *router) override;

private:
    static int list_providers(HttpRequest* req, HttpResponse* resp);
    static int create_provider(HttpRequest* req, HttpResponse* resp);
    static int update_provider(HttpRequest* req, HttpResponse* resp);
    static int delete_provider(HttpRequest* req, HttpResponse* resp);
    static int get_provider(HttpRequest* req, HttpResponse* resp);
};