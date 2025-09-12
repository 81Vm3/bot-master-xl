//
// Created by rain on 8/1/25.
//

#ifndef CBOTSERVICE_H
#define CBOTSERVICE_H
#include "CBaseService.h"
#include "hv/HttpService.h"


class CBotService : public CBaseService {
public:
    CBotService(const std::string& uri);
    void on_install(HttpService *router) override;

private:
    static int list_bots(HttpRequest* req, HttpResponse* resp);
    static int create_bot(HttpRequest* req, HttpResponse* resp);
    static int delete_bot(HttpRequest* req, HttpResponse* resp);
    static int set_password(HttpRequest* req, HttpResponse* resp);
    static int reconnect_bot(HttpRequest* req, HttpResponse* resp);
    static int enable_llm_session(HttpRequest* req, HttpResponse* resp);
    static int disable_llm_session(HttpRequest* req, HttpResponse* resp);
    static int update_system_prompt(HttpRequest* req, HttpResponse* resp);
    
    // Helper method for LLM session creation
    static std::string createLLMSessionForBot(const std::string& botUuid, int providerId);
    
    // Helper method for LLM session deletion
    static bool deleteLLMSessionForBot(const std::string& botUuid);
};



#endif //CBOTSERVICE_H
