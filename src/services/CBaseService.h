//
// Created by rain on 8/1/25.
//

#ifndef CBASESERVICE_H
#define CBASESERVICE_H
#include "hv/HttpServer.h"

class CBaseService {
public:
    CBaseService(const std::string& uri);
    virtual ~CBaseService() = default;

    virtual void on_install(::HttpService* router);
protected:
    std::string getRelativePath(const std::string& path);
private:
    std::string m_uri;
};


#endif //CBASESERVICE_H
