//
// Created by rain on 8/1/25.
//

#include "JsonResponse.h"
#include <chrono>

json JsonResponse::with_success(const json& data, const std::string& message) {
    json response;
    response["success"] = true;
    response["message"] = message;
    response["data"] = data;
    response["code"] = 200;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return response;
}

json JsonResponse::with_error(const std::string& message, const json& details) {
    json response;
    response["success"] = false;
    response["message"] = message;
    response["code"] = 500;
    response["data"] = nullptr;
    if (!details.is_null()) {
        response["details"] = details;
    }
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return response;
}

json JsonResponse::paginated(const json& data, int page, int pageSize, int total, const std::string& message) {
    json response;
    response["success"] = true;
    response["message"] = message;
    response["data"] = data;
    response["code"] = 200;
    response["pagination"] = {
        {"page", page},
        {"pageSize", pageSize},
        {"total", total},
        {"totalPages", (total + pageSize - 1) / pageSize}
    };
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return response;
}

json JsonResponse::internal_error() {
    json response;
    response["success"] = false;
    response["message"] = "500 SERVER INTERNAL ERROR";
    response["data"] = "";
    response["code"] = 500;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return response;
}

json JsonResponse::not_found() {
    json response;
    response["success"] = false;
    response["message"] = "404 NOT FOUND";
    response["data"] = "";
    response["code"] = 404;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return response;
}