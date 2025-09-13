//
// Created by rain on 7/13/25.
//

#ifndef CCONFIG_H
#define CCONFIG_H

#include <string>
#include <hv/json.hpp>
#include <fstream>
#include <iostream>

#include "models/CConnectionQueue.h"

class CConfig {
public:
    CConfig();

    bool loadConfigFile(const std::string& filename);
    bool saveConfigFile(const std::string& filename) const;
    bool loadBaseInternalPrompt(const std::string& filename);

    int api_port;
    eConnectionPolicy connection_policy;
    std::string base_internal_prompt;
    std::string message_encoding;

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

#endif //CCONFIG_H
