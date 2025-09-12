//
// Created by rain on 8/4/25.
//

#ifndef TOOL_BUILDER_H
#define TOOL_BUILDER_H

#include <functional>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include "hv/json.hpp"

using json = nlohmann::json;

// MCP Tool definition
struct tool {
    std::string name;
    std::string description;
    json parameters_schema;
    std::function<json(const json&, const std::string&)> handler;

    // Convert to JSON for API documentation
    json to_json() const {
        return {
            {"name", name},
            {"description", description},
            {"inputSchema", parameters_schema} // You may need `parameters` instead of `inputSchema` for OAI format
        };
    }
};

/**
 * @class tool_builder
 * @brief Utility class for building tools with a fluent API
 *
 * The tool_builder class provides a simple way to create tools with
 * a fluent (chain-based) API.
 */
class tool_builder {
public:
    /**
     * @brief Constructor
     * @param name The name of the tool
     */
    explicit tool_builder(const std::string& name);

    /**
     * @brief Set the tool description
     * @param description The description
     * @return Reference to this builder
     */
    tool_builder& with_description(const std::string& description);

    /**
     * @brief Add a string parameter
     * @param name The parameter name
     * @param description The parameter description
     * @param required Whether the parameter is required
     * @return Reference to this builder
     */
    tool_builder& with_string_param(const std::string& name,
                                   const std::string& description,
                                   bool required = true);

    /**
     * @brief Add a number parameter
     * @param name The parameter name
     * @param description The parameter description
     * @param required Whether the parameter is required
     * @return Reference to this builder
     */
    tool_builder& with_number_param(const std::string& name,
                                   const std::string& description,
                                   bool required = true);

    /**
     * @brief Add a boolean parameter
     * @param name The parameter name
     * @param description The parameter description
     * @param required Whether the parameter is required
     * @return Reference to this builder
     */
    tool_builder& with_boolean_param(const std::string& name,
                                    const std::string& description,
                                    bool required = true);

    /**
     * @brief Add an array parameter
     * @param name The parameter name
     * @param description The parameter description
     * @param item_type The type of the array items ("string", "number", "object", etc.)
     * @param required Whether the parameter is required
     * @return Reference to this builder
     */
    tool_builder& with_array_param(const std::string& name,
                                  const std::string& description,
                                  const std::string& item_type,
                                  bool required = true);

    /**
     * @brief Add an object parameter
     * @param name The parameter name
     * @param description The parameter description
     * @param properties JSON schema for the object properties
     * @param required Whether the parameter is required
     * @return Reference to this builder
     */
    tool_builder& with_object_param(const std::string& name,
                                   const std::string& description,
                                   const json& properties,
                                   bool required = true);

    /**
     * @brief Set a function handler for the tool
     * @param func The function to bind
     * @return Reference to this builder
     */
    tool_builder& with_function(std::function<json(const json&, const std::string&)> func);

    /**
     * @brief Build the tool
     * @return The constructed tool
     */
    tool build() const;

private:
    std::string name_;
    std::string description_;
    json parameters_;
    std::vector<std::string> required_params_;
    std::function<json(const json&, const std::string&)> handler_;

    // Helper to add a parameter of any type
    tool_builder& add_param(const std::string& name,
                           const std::string& description,
                           const std::string& type,
                           bool required);
};

/**
 * @brief Create a simple tool with a function-based approach
 * @param name Tool name
 * @param description Tool description
 * @param parameter_definitions A vector of parameter definitions as {name, description, type, required}
 * @param handler Function to handle tool invocations
 * @return The created tool
 */
inline tool create_tool(
    const std::string& name,
    const std::string& description,
    const std::vector<std::tuple<std::string, std::string, std::string, bool>>& parameter_definitions,
    std::function<json(const json&, const std::string&)> handler = nullptr) {

    tool_builder builder(name);
    builder.with_description(description);

    for (const auto& [param_name, param_desc, param_type, required] : parameter_definitions) {
        if (param_type == "string") {
            builder.with_string_param(param_name, param_desc, required);
        } else if (param_type == "number") {
            builder.with_number_param(param_name, param_desc, required);
        } else if (param_type == "boolean") {
            builder.with_boolean_param(param_name, param_desc, required);
        } else if (param_type == "array") {
            builder.with_array_param(param_name, param_desc, "string", required);
        } else if (param_type == "object") {
            builder.with_object_param(param_name, param_desc, json::object(), required);
        }
    }

    if (handler) {
        builder.with_function(handler);
    }

    return builder.build();
}


#endif //TOOL_BUILDER_H
