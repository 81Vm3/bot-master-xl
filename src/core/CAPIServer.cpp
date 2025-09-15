//
// Created by rain on 8/1/25.
//

#include "CAPIServer.h"

#include "../CApp.h"
#include "CConfig.h"
#include "../services/CServerService.h"
#include "../services/CBotService.h"
#include "services/CDashboardService.h"
#include "services/CLLMProviderService.h"
#include <miniz.h>
#include <fstream>

std::map<std::string, std::string> CAPIServer::mime_types = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"}
};

CAPIServer::CAPIServer() {
    router = std::make_unique<hv::HttpService>();
    server = std::make_unique<hv::HttpServer>();

    using namespace std::placeholders;
    router->GET("/web/*", std::bind(&CAPIServer::on_static_request, this, _1, _2));
    router->GET("/web", std::bind(&CAPIServer::on_static_request, this, _1, _2));
    load_website_dist("data/dist.zip");

    services.emplace_back(std::make_unique<CBotService>("api/bot"));
    services.emplace_back(std::make_unique<CLLMProviderService>("api/llm"));
    services.emplace_back(std::make_unique<CServerService>("api/server"));
    services.emplace_back(std::make_unique<CDashboardService>("api/dashboard"));

    for (auto& service : services) {
        service->on_install(router.get());
    }

    // Enable CORS for all endpoints
    router->AllowCORS();

    server->setPort(CApp::getInstance()->getConfig()->api_port);
    server->setThreadNum(4);
    server->service = router.get();
}

CAPIServer::~CAPIServer() {
    if (running) {
        stop_async();
    }
}

void CAPIServer::start_async() {
    if (!running) {
        server->start();
        running = true;
    }
}

void CAPIServer::stop_async() {
    if (running) {
        server->stop();
        running = false;
    }
}

bool CAPIServer::is_running() const {
    return running;
}

int CAPIServer::on_static_request(HttpRequest* req, HttpResponse* resp) {
    std::string path = req->Path();

    // Remove /web prefix if present
    if (path.substr(0, 4) == "/web") {
        path = path.substr(4);
    }
    if (path.empty()) path = "/";

    if (path == "/" || path.empty()) path = "index.html"; // Default to index.html for root
    // Remove leading slash for map lookup  
    else if (path[0] == '/') path = path.substr(1);

    // Look up file in memory trie
    auto it = trie.find(path);
    if (it && !it->empty()) {
        // Set content and MIME type
        std::string data(reinterpret_cast<const char*>(it->data()), it->size());
        resp->SetBody(data);
        resp->SetContentType(get_mime_type(path).c_str());
        return 200; // OK
    }
    // SPA fallback: if file not found and path doesn't have extension, serve index.html
    // if (path.find('.') == std::string::npos) {
    //     auto index_it = trie.find("index.html");
    //     if (index_it && !index_it->empty()) {
    //         std::string data(reinterpret_cast<const char*>(index_it->data()), index_it->size());
    //         resp->SetBody(data);
    //         resp->SetContentType("text/html");
    //         return 200; // OK
    //     }
    // }
    resp->SetBody("File Not Found");
    resp->SetContentType("text/plain");
    return 404;
}

std::string CAPIServer::get_mime_type(const std::string &path) {
    size_t pos = path.find_last_of('.');
    if (pos != std::string::npos) {
        std::string ext = path.substr(pos);
        auto it = mime_types.find(ext);
        if (it != mime_types.end()) {
            return it->second;
        }
    }
    return "application/octet-stream"; // Default
}

void CAPIServer::load_website_dist(const std::string &path) {
    // Read the zip file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> zip_data(size);
    if (!file.read(zip_data.data(), size)) {
        return;
    }
    
    // Initialize miniz
    mz_zip_archive zip_archive = {};
    
    if (!mz_zip_reader_init_mem(&zip_archive, zip_data.data(), size, 0)) {
        return;
    }
    
    // Extract each file
    int num_files = static_cast<int>(mz_zip_reader_get_num_files(&zip_archive));
    
    for (int i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            continue;
        }
        
        // Skip directories
        if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            continue;
        }
        
        // Extract file data
        size_t uncompressed_size;
        void* file_data = mz_zip_reader_extract_to_heap(&zip_archive, i, &uncompressed_size, 0);
        if (!file_data) {
            continue;
        }
        
        // Create DataNode and store in trie
        auto data_node = std::make_unique<std::vector<std::byte>>();
        data_node->resize(uncompressed_size);
        std::memcpy(data_node->data(), file_data, uncompressed_size);
        
        // Insert into trie with filename as key
        std::string filename = file_stat.m_filename;
        trie.insert(filename, std::move(data_node));
        
        // Free extracted data
        mz_free(file_data);
    }
    
    mz_zip_reader_end(&zip_archive);
}
