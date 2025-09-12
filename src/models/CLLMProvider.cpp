//
// Created by rain on 8/2/25.
//

#include "CLLMProvider.h"

CLLMProvider::CLLMProvider(const std::string& name) 
    : dbId(-1), name(name), apiKey(""), baseUrl(""), model(""), createdAt("") {
}

CLLMProvider::CLLMProvider(int dbId, const std::string& name, const std::string& apiKey, 
                           const std::string& baseUrl, const std::string& model)
    : dbId(dbId), name(name), apiKey(apiKey), baseUrl(baseUrl), model(model), 
      createdAt("") {
}

CLLMProvider::~CLLMProvider() {
}

// Getters
int CLLMProvider::getDbId() const {
    return dbId;
}

std::string CLLMProvider::getName() const {
    return name;
}

std::string CLLMProvider::getApiKey() const {
    return apiKey;
}

std::string CLLMProvider::getBaseUrl() const {
    return baseUrl;
}

std::string CLLMProvider::getModel() const {
    return model;
}

std::string CLLMProvider::getCreatedAt() const {
    return createdAt;
}

// Setters
void CLLMProvider::setDbId(int newDbId) {
    dbId = newDbId;
}

void CLLMProvider::setName(const std::string& newName) {
    name = newName;
}

void CLLMProvider::setApiKey(const std::string& newApiKey) {
    apiKey = newApiKey;
}

void CLLMProvider::setBaseUrl(const std::string& newBaseUrl) {
    baseUrl = newBaseUrl;
}

void CLLMProvider::setModel(const std::string& newModel) {
    model = newModel;
}

void CLLMProvider::setCreatedAt(const std::string& newCreatedAt) {
    createdAt = newCreatedAt;
}