//
// Created by rain on 8/1/25.
//

#include "CBaseService.h"
#include <sstream>

CBaseService::CBaseService(const std::string& uri) {
    this->m_uri = uri;
}

void CBaseService::on_install(HttpService *router) {
}

std::string CBaseService::getRelativePath(const std::string& path) {
    std::stringstream ss;
    ss << '/' << m_uri << '/' << path;
    return ss.str();
}
