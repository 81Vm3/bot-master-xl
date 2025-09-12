#pragma once

#include <hv/json.hpp>
#include <hv/requests.h>
#include <hv/HttpClient.h>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "../models/CLLMProvider.h"

using json = nlohmann::json;

struct FunctionDefinition {
    std::string name;
    std::string description;
    json parameters;
};

struct FunctionCall {
    std::string name;
    json arguments;
};

class CFunctionDispatcher {
private:
    std::map<std::string, std::function<json(const json&, const std::string&)>> registered_functions;
    std::vector<FunctionDefinition> function_definitions;
    hv::HttpClient http_client;

public:
    CFunctionDispatcher();
    ~CFunctionDispatcher() = default;

    void registerFunction(const std::string& name, 
                         const std::string& description,
                         const json& parameters,
                         std::function<json(const json&, const std::string&)> function);

    json executeFunction(const std::string& name, const json& arguments, const std::string& session_id = "");
    
    std::vector<FunctionDefinition> getFunctionDefinitions() const;


    // 回调函数： LLM反馈，结果类型，工具结果的上下文信息
    void callLLMWithFunctionsAsync(const std::vector<json>& messages,
                                   std::shared_ptr<CLLMProvider> llmProvider,
                                   std::function<void(const json&, const std::string&, const json&)> callback,
                                   const std::string& session_id = "");
    json createFunctionCallMessage(const json& result);


private:
    // 处理工具，会返回工具结果的上下文信息
    json handleFunctionCalls(const json& llm_response, const std::string& session_id);
    json createToolsArray() const;
};