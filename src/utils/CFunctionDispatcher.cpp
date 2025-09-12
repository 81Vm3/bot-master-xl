#include "CFunctionDispatcher.h"
#include "../models/CBot.h"
#include "../CApp.h"
#include <hv/hlog.h>
#include <sstream>
#include <spdlog/spdlog.h>

#include "core/CLLMBotSessionManager.h"
#include "core/CLogger.h"

CFunctionDispatcher::CFunctionDispatcher() {
    // No default LLM configuration needed
    http_client.setTimeout(30);  // Set 30-second timeout
}

void CFunctionDispatcher::registerFunction(const std::string& name, 
                                          const std::string& description,
                                          const json& parameters,
                                          std::function<json(const json&, const std::string&)> function) {
    registered_functions[name] = function;
    
    FunctionDefinition def;
    def.name = name;
    def.description = description;
    def.parameters = parameters;
    
    function_definitions.push_back(def);
}

json CFunctionDispatcher::executeFunction(const std::string& name, const json& arguments, const std::string& session_id) {
    auto it = registered_functions.find(name);
    if (it == registered_functions.end()) {
        return json{{"error", "Function not found: " + name}};
    }
    
    // Check cooldown using centralized session manager if session_id is provided
    if (!session_id.empty()) {
        auto sessionManager = CApp::getInstance()->getLLMSessionManager();
        if (sessionManager && !sessionManager->checkActionCooldown(session_id, name)) {
            return json{{"error", "Action " + name + " is on cooldown"}};
        }
    }
    
    try {
        json result = it->second(arguments, session_id);
        
        // Set cooldown and update activity using centralized session manager
        if (!session_id.empty()) {
            auto sessionManager = CApp::getInstance()->getLLMSessionManager();
            if (sessionManager) {
                sessionManager->setActionCooldown(session_id, name);
                sessionManager->updateSessionActivity(session_id);
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        spdlog::error("Error executing function %s: %s", name.c_str(), e.what());
        return json{{"error", "Function execution failed: " + std::string(e.what())}};
    }
}

std::vector<FunctionDefinition> CFunctionDispatcher::getFunctionDefinitions() const {
    return function_definitions;
}


json CFunctionDispatcher::handleFunctionCalls(const json& llm_response, const std::string& session_id) {
    json result = llm_response;
    json& message = result["choices"][0]["message"];
    
    if (!message.contains("tool_calls")) {
        return result;
    }
    
    json function_results = json::array();
    
    for (const auto& tool_call : message["tool_calls"]) {
        if (tool_call["type"] == "function") {
            std::string function_name = tool_call["function"]["name"];
            json arguments;
            
            try {
                arguments = json::parse(tool_call["function"]["arguments"].get<std::string>());
            } catch (const std::exception& e) {
                arguments = tool_call["function"]["arguments"];
            }
            
            json function_result = executeFunction(function_name, arguments, session_id);
            function_results.push_back({
                {"tool_call_id", tool_call["id"]},
                {"function_name", function_name},
                {"result", function_result}
            });
        }
    }

    return function_results;
}

json CFunctionDispatcher::createFunctionCallMessage(const json& result) {
    json message = json::array();
    for (auto& it : result) {
        message.push_back({
                {"role","tool"},
                {"tool_call_id", it["tool_call_id"]},
                {"content", it["result"].dump()} //必须是text而不是object
        });
    }
    return message;
}

json CFunctionDispatcher::createToolsArray() const {
    json tools = json::array();
    
    for (const auto& func_def : function_definitions) {
        json tool = {
            {"type", "function"},
            {"function", {
                {"name", func_def.name},
                {"description", func_def.description},
                {"parameters", func_def.parameters}
            }}
        };
        tools.push_back(tool);
    }
    
    return tools;
}

void CFunctionDispatcher::callLLMWithFunctionsAsync(const std::vector<json>& messages,
                                                    std::shared_ptr<CLLMProvider> llmProvider,
                                                    std::function<void(const json&, const std::string&, const json&)> callback,
                                                    const std::string& session_id) {
    if (!llmProvider) {
        callback(json{{"error", "No LLM provider specified"}}, "error", {});
        return;
    }
    
    json request_body = {
        {"model", llmProvider->getModel()},
        {"messages", messages},
        {"tools", createToolsArray()}
    };
    if (!function_definitions.empty()) {
        request_body["tool_choice"] = "auto";
    }

    auto req = std::make_shared<HttpRequest>();
    req->method = HTTP_POST;
    req->url = llmProvider->getBaseUrl();
    req->headers["Content-Type"] = "application/json";
    req->headers["Authorization"] = "Bearer " + llmProvider->getApiKey();
    req->body = request_body.dump();

    // Use member HTTP client
    http_client.sendAsync(req, [this, callback, session_id](const HttpResponsePtr& resp) {
        if (!resp) {
            callback(json{{"error", "Failed to send request to LLM API"}}, "", {});
            return;
        }
        if (resp->status_code != 200) {
            std::stringstream ss;
            ss << "LLM API error: " << resp->status_code << " - " << resp->body;
            callback(json{{"error", ss.str()}}, "", {});
            return;
        }
        try {
            json response = json::parse(resp->body);

            if (response.contains("choices") && !response["choices"].empty()) {
               json message = response["choices"][0]["message"];
               if (message.contains("tool_calls") && !message["tool_calls"].empty()) {
                   json function_results = handleFunctionCalls(response, session_id);
                   // 处理后会产生 function_results
                   callback(response, "function_calls_executed", function_results);
               } else {
                   callback(response, "message", {});
               }
           } else {
               callback(json{{"error", "Invalid response format from LLM"}}, "", {});
           }

        } catch (const std::exception& e) {
            callback(json{{"error", "Failed to parse LLM response: " + std::string(e.what())}}, "", {});
        }
    });
}
