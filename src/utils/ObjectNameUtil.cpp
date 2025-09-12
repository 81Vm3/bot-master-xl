//
// Created by rain on 2025/8/3.
//

#include "ObjectNameUtil.h"
#include <fstream>
#include <sstream>
#include <algorithm>

const std::string ObjectNameUtil::EMPTY_STRING = "";

ObjectNameUtil::ObjectNameUtil() : objectNames(nullptr), maxModelId(-1), objectCount(0) {
}

ObjectNameUtil::~ObjectNameUtil() {
    clear();
}

bool ObjectNameUtil::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        spdlog::error("Failed to open object names file: {}", filePath);
        return false;
    }
    
    // First pass: find maximum model ID
    std::string line;
    int tempMaxId = -1;
    size_t tempCount = 0;
    std::vector<std::pair<int, std::string>> tempData;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        int id;
        std::string name;
        if (parseLine(line, id, name)) {
            tempData.emplace_back(id, name);
            tempMaxId = std::max(tempMaxId, id);
            tempCount++;
        } else {
            spdlog::warn("Invalid line format in {}: {}", filePath, line);
        }
    }
    
    file.close();
    
    if (tempMaxId < 0) {
        spdlog::error("No valid object names found in file: {}", filePath);
        return false;
    }
    
    // Clear existing data
    clear();
    
    // Allocate fixed array based on maximum ID
    maxModelId = tempMaxId;
    objectCount = tempCount;
    objectNames = std::make_unique<std::string[]>(maxModelId + 1);
    
    // Initialize all entries to empty string
    for (int i = 0; i <= maxModelId; ++i) {
        objectNames[i] = EMPTY_STRING;
    }
    
    // Populate the array
    for (const auto& pair : tempData) {
        objectNames[pair.first] = pair.second;
    }
    
    spdlog::info("Loaded {} object names with max ID {} from file: {}", 
                 objectCount, maxModelId, filePath);
    return true;
}

const std::string& ObjectNameUtil::getObjectName(int modelId) const {
    if (modelId < 0 || modelId > maxModelId || objectNames == nullptr) {
        return EMPTY_STRING;
    }
    return objectNames[modelId];
}

bool ObjectNameUtil::hasModelId(int modelId) const {
    if (modelId < 0 || modelId > maxModelId || objectNames == nullptr) {
        return false;
    }
    return !objectNames[modelId].empty();
}

int ObjectNameUtil::getMaxModelId() const {
    return maxModelId;
}

size_t ObjectNameUtil::getObjectCount() const {
    return objectCount;
}

void ObjectNameUtil::clear() {
    objectNames.reset();
    maxModelId = -1;
    objectCount = 0;
}

bool ObjectNameUtil::parseLine(const std::string& line, int& id, std::string& name) {
    // Find the first comma
    size_t commaPos = line.find(',');
    if (commaPos == std::string::npos) {
        return false;
    }
    
    // Extract ID part
    std::string idStr = line.substr(0, commaPos);
    
    // Extract name part (everything after the comma)
    name = line.substr(commaPos + 1);
    
    // Trim whitespace from both parts
    idStr.erase(0, idStr.find_first_not_of(" \t\r\n"));
    idStr.erase(idStr.find_last_not_of(" \t\r\n") + 1);
    name.erase(0, name.find_first_not_of(" \t\r\n"));
    name.erase(name.find_last_not_of(" \t\r\n") + 1);
    
    // Parse ID
    try {
        id = std::stoi(idStr);
        if (id < 0) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }
    
    // Name cannot be empty
    if (name.empty()) {
        return false;
    }
    
    return true;
}