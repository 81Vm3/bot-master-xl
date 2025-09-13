//
// Created by rain on 7/13/25.
//

#include "CConfig.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/os.h>

CConfig::CConfig() :
    api_port(7070),
    connection_policy(eConnectionPolicy::QUEUED),
    message_encoding("GBK") {
}

bool CConfig::loadConfigFile(const std::string &filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            spdlog::warn("Config file {} not found, creating with defaults", filename);
            if (!saveConfigFile(filename)) {
                spdlog::error("Failed to create default config file: {}", filename);
                return false;
            }
            spdlog::info("Default config created at {}", filename);
            return true;
        }

        nlohmann::json j;
        file >> j;
        fromJson(j);
        file.close();
        return true;
    } catch (const std::exception &e) {
        spdlog::error("Error loading config: {}", e.what());
        return false;
    }
}

bool CConfig::saveConfigFile(const std::string &filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            spdlog::error("Failed to create config file: {}", filename);

            return false;
        }

        nlohmann::json j = toJson();
        file << j.dump(4);
        file.close();
        spdlog::info("Config saved successfully to {}", filename);
        return true;
    } catch (const std::exception &e) {
        spdlog::error("Error saving config: {}", e.what());
        return false;
    }
}

bool CConfig::loadBaseInternalPrompt(const std::string &filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in) {
        spdlog::error("Error opening prompt file: {}", filename);
        return false;
    }

    in.seekg(0, std::ios::end);
    std::size_t size = in.tellg();
    in.seekg(0, std::ios::beg);

    base_internal_prompt.resize(size);
    in.read(&base_internal_prompt[0], size);
    return true;
}

nlohmann::json CConfig::toJson() const {
    nlohmann::json j;
    j["api_port"] = api_port;
    j["connection_policy"] = connection_policy;
    j["message_encoding"] = message_encoding;
    return j;
}

void CConfig::fromJson(const nlohmann::json &j) {
    api_port = j["api_port"];
    connection_policy = j["connection_policy"];
    message_encoding =  j["message_encoding"];
}
