//
// Created by rain on 8/2/25.
//

#ifndef CLLMPROVIDER_H
#define CLLMPROVIDER_H

#include <string>

class CLLMProvider {
public:
    CLLMProvider(const std::string& name);
    CLLMProvider(int dbId, const std::string& name, const std::string& apiKey, 
                 const std::string& baseUrl, const std::string& model);
    ~CLLMProvider();

    // Getters
    int getDbId() const;
    std::string getName() const;
    std::string getApiKey() const;
    std::string getBaseUrl() const;
    std::string getModel() const;
    std::string getCreatedAt() const;

    // Setters
    void setDbId(int newDbId);
    void setName(const std::string& newName);
    void setApiKey(const std::string& newApiKey);
    void setBaseUrl(const std::string& newBaseUrl);
    void setModel(const std::string& newModel);
    void setCreatedAt(const std::string& newCreatedAt);

private:
    int dbId;
    std::string name;
    std::string apiKey;
    std::string baseUrl;
    std::string model;
    std::string createdAt;
};

#endif //CLLMPROVIDER_H