//
// Created by rain on 8/1/25.
//

#ifndef JSONRESPONSE_H
#define JSONRESPONSE_H
#include <hv/json.hpp>
#include <string>

using json = nlohmann::json;

class JsonResponse {
public:
    static json with_success(const json& data = nullptr, const std::string& message = "Success");
    static json with_error(const std::string& message, const json& details = nullptr);
    static json paginated(const json& data, int page, int pageSize, int total, const std::string& message = "Success");
    static json internal_error();
    static json not_found();
};

#endif //JSONRESPONSE_H