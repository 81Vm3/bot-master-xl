#include "CLLMProviderService.h"
#include "../utils/JsonResponse.h"
#include "../CApp.h"
#include "../core/CPersistentDataStorage.h"
#include "../database/DBSchema.h"
#include "../database/querybuilder.h"
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <algorithm>

CLLMProviderService::CLLMProviderService(const std::string& uri) : CBaseService(uri) {
}

void CLLMProviderService::on_install(HttpService *router) {
    router->GET(getRelativePath("list").c_str(), CLLMProviderService::list_providers);
    router->POST(getRelativePath("create").c_str(), CLLMProviderService::create_provider);
    router->POST(getRelativePath("update").c_str(), CLLMProviderService::update_provider);
    router->POST(getRelativePath("delete").c_str(), CLLMProviderService::delete_provider);
    router->GET(getRelativePath("get").c_str(), CLLMProviderService::get_provider);
}

int CLLMProviderService::list_providers(HttpRequest* req, HttpResponse* resp) {
    try {
        json providers = json::array();
        auto database = CApp::getInstance()->getDatabase();
        
        for (const auto& provider : database->vLLMProvider) {
            json providerJson = {
                {"id", provider->getDbId()},
                {"name", provider->getName()},
                {"api_key", provider->getApiKey()},
                {"base_url", provider->getBaseUrl()},
                {"model", provider->getModel()},
                {"created_at", provider->getCreatedAt()}
            };
            providers.push_back(providerJson);
        }
        
        return resp->Json(JsonResponse::with_success(providers, "LLM providers retrieved successfully"));
    } catch (const std::exception& e) {
        spdlog::error("Error in list_providers: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CLLMProviderService::create_provider(HttpRequest* req, HttpResponse* resp) {
    try {
        json body = json::parse(req->Body());
        
        if (!body.contains("name") || !body.contains("base_url") || !body.contains("model")) {
            return resp->Json(JsonResponse::with_error("Missing required fields: name, base_url, model"));
        }
        
        std::string name = body["name"];
        std::string base_url = body["base_url"];
        std::string api_key = body.value("api_key", "");
        std::string model = body["model"];
        
        auto db = CApp::getInstance()->getDatabase()->getDb();
        
        sql::InsertModel im;
        im.insert(DB::LLMProviders::NAME, name)
          .insert(DB::LLMProviders::API_KEY, api_key)
          .insert(DB::LLMProviders::BASE_URL, base_url)
          .insert(DB::LLMProviders::MODEL, model)
          .into(DB::Tables::LLM_PROVIDERS);
        
        char* error_msg = nullptr;
        int rc = sqlite3_exec(db, im.str().c_str(), nullptr, nullptr, &error_msg);
        
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to create provider: {}", error_msg ? error_msg : "Unknown error");
            if (error_msg) sqlite3_free(error_msg);
            return resp->Json(JsonResponse::internal_error());
        }
        
        int provider_id = sqlite3_last_insert_rowid(db);
        
        // Add to memory and hash map
        auto database = CApp::getInstance()->getDatabase();
        if (database) {
            auto llmProvider = std::make_shared<CLLMProvider>(provider_id, name, api_key, base_url, model);

            database->vLLMProvider.push_back(llmProvider);
            database->llmProvidersById[provider_id] = llmProvider;
        }
        
        json response_data = {
            {"id", provider_id},
            {"name", name},
            {"base_url", base_url},
            {"model", model}
        };
        
        return resp->Json(JsonResponse::with_success(response_data, "LLM provider created successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        spdlog::error("Error in create_provider: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CLLMProviderService::update_provider(HttpRequest* req, HttpResponse* resp) {
    try {
        json body = json::parse(req->Body());
        
        if (!body.contains("id")) {
            return resp->Json(JsonResponse::with_error("Missing required field: id"));
        }
        
        int id = body["id"];
        auto db = CApp::getInstance()->getDatabase()->getDb();
        
        std::vector<std::string> updates;
        std::vector<std::string> values;
        
        if (body.contains("name")) {
            updates.push_back(DB::LLMProviders::NAME + " = ?");
            values.push_back(body["name"]);
        }
        if (body.contains("base_url")) {
            updates.push_back(DB::LLMProviders::BASE_URL + " = ?");
            values.push_back(body["base_url"]);
        }
        if (body.contains("api_key")) {
            updates.push_back(DB::LLMProviders::API_KEY + " = ?");
            values.push_back(body["api_key"]);
        }
        if (body.contains("model")) {
            updates.push_back(DB::LLMProviders::MODEL + " = ?");
            values.push_back(body["model"]);
        }
        
        if (updates.empty()) {
            return resp->Json(JsonResponse::with_error("No fields to update"));
        }
        
        std::string sql = "UPDATE " + DB::Tables::LLM_PROVIDERS + " SET ";
        for (size_t i = 0; i < updates.size(); ++i) {
            sql += updates[i];
            if (i < updates.size() - 1) sql += ", ";
        }
        sql += " WHERE id = ?";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
            return resp->Json(JsonResponse::internal_error());
        }
        
        for (size_t i = 0; i < values.size(); ++i) {
            sqlite3_bind_text(stmt, i + 1, values[i].c_str(), -1, SQLITE_STATIC);
        }
        sqlite3_bind_int(stmt, values.size() + 1, id);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            spdlog::error("Failed to update provider: {}", sqlite3_errmsg(db));
            return resp->Json(JsonResponse::internal_error());
        }
        
        return resp->Json(JsonResponse::with_success(nullptr, "LLM provider updated successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        spdlog::error("Error in update_provider: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CLLMProviderService::delete_provider(HttpRequest* req, HttpResponse* resp) {
    try {
        json body = json::parse(req->Body());
        if (!body.contains("id")) {
            return resp->Json(JsonResponse::not_found());
        }
        
        int id = body["id"];
        auto database = CApp::getInstance()->getDatabase();
        auto db = database->getDb();
        
        // Check for foreign key constraint - active LLM sessions using this provider
        sql::SelectModel sm;
        sm.select("COUNT(*)").from(DB::Tables::LLM_SESSIONS)
          .where(DB::LLMSessions::PROVIDER_ID + " = " + std::to_string(id) 
                 + " AND " + DB::LLMSessions::IS_ACTIVE + " = 1");
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sm.str().c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare check query: {}", sqlite3_errmsg(db));
            return resp->Json(JsonResponse::internal_error());
        }
        
        rc = sqlite3_step(stmt);
        int active_sessions_count = (rc == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : 0;
        sqlite3_finalize(stmt);
        
        if (active_sessions_count > 0) {
            return resp->Json(JsonResponse::with_error(
                "Cannot delete LLM provider: " + std::to_string(active_sessions_count) + 
                " active session(s) are using this provider"));
        }
        
        // Delete from database
        sql::DeleteModel dm;
        dm.from(DB::Tables::LLM_PROVIDERS).where(DB::LLMProviders::ID + " = " + std::to_string(id));
        
        char* error;
        rc = sqlite3_exec(db, dm.str().c_str(), nullptr, nullptr, &error);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to delete provider: {}", error);
            sqlite3_free(error);
            return resp->Json(JsonResponse::internal_error());
        }
        
        // Remove from memory using ID-based hash map
        if (database) {
            // Remove from hash map by ID
            database->llmProvidersById.erase(id);
            
            // Remove from vector
            auto& providers = database->vLLMProvider;
            providers.erase(std::remove_if(providers.begin(), providers.end(),
                [id](const std::shared_ptr<CLLMProvider>& provider) {
                    return provider->getDbId() == id;
                }), providers.end());
        }
        
        return resp->Json(JsonResponse::with_success(nullptr, "LLM provider deleted successfully"));
    } catch (const json::parse_error& e) {
        return resp->Json(JsonResponse::with_error("Invalid JSON format"));
    } catch (const std::exception& e) {
        spdlog::error("Error in delete_provider: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}

int CLLMProviderService::get_provider(HttpRequest* req, HttpResponse* resp) {
    try {
        std::string id_param = req->GetParam("id");
        if (id_param.empty()) {
            return resp->Json(JsonResponse::with_error("Missing id parameter"));
        }
        
        int id = std::stoi(id_param);
        auto database = CApp::getInstance()->getDatabase();
        
        auto it = database->llmProvidersById.find(id);
        if (it != database->llmProvidersById.end()) {
            auto provider = it->second;
            json providerJson = {
                {"id", provider->getDbId()},
                {"name", provider->getName()},
                {"api_key", provider->getApiKey()},
                {"base_url", provider->getBaseUrl()},
                {"model", provider->getModel()},
                {"created_at", provider->getCreatedAt()}
            };
            return resp->Json(JsonResponse::with_success(providerJson, "LLM provider retrieved successfully"));
        } else {
            return resp->Json(JsonResponse::not_found());
        }
    } catch (const std::invalid_argument& e) {
        return resp->Json(JsonResponse::with_error("Invalid id parameter"));
    } catch (const std::exception& e) {
        spdlog::error("Error in get_provider: {}", e.what());
        return resp->Json(JsonResponse::internal_error());
    }
}
