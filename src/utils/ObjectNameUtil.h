//
// Created by rain on 2025/8/3.
//

#ifndef OBJECTNAMEUTIL_H
#define OBJECTNAMEUTIL_H

#include <string>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>

class ObjectNameUtil {
public:
    ObjectNameUtil();
    ~ObjectNameUtil();
    
    // Load object names from file in format: id,model_name
    bool loadFromFile(const std::string& filePath);
    
    // Get object name by model ID (O(1) lookup)
    const std::string& getObjectName(int modelId) const;
    
    // Check if a model ID exists
    bool hasModelId(int modelId) const;
    
    // Get the maximum model ID loaded
    int getMaxModelId() const;
    
    // Get total number of loaded objects
    size_t getObjectCount() const;
    
    // Clear all loaded data
    void clear();

private:
    std::unique_ptr<std::string[]> objectNames;  // Fixed array for O(1) lookup
    int maxModelId;                              // Maximum model ID found
    size_t objectCount;                          // Number of objects loaded
    static const std::string EMPTY_STRING;      // Returned for invalid IDs
    
    // Parse a line in format "id,model_name"
    bool parseLine(const std::string& line, int& id, std::string& name);
};

#endif //OBJECTNAMEUTIL_H