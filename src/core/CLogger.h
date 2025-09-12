//
// CLogger - Centralized logging system for BotMasterXL
//

#ifndef CLOGGER_H
#define CLOGGER_H

#include <memory>
#include "spdlog/spdlog.h"

class CLogger {
private:
    static CLogger* instance;
    CLogger();
    CLogger(const CLogger&) = delete;
    CLogger& operator=(const CLogger&) = delete;

public:
    static CLogger* getInstance();
    void init();
    
    std::shared_ptr<spdlog::logger> system;
    std::shared_ptr<spdlog::logger> bot;
    std::shared_ptr<spdlog::logger> api;
    std::shared_ptr<spdlog::logger> llm;
    
    // Convenience methods for LLM logging
    template<typename... Args>
    void llmerror(const std::string& fmt, Args&&... args) {
        if (llm) llm->error(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void llminfo(const std::string& fmt, Args&&... args) {
        if (llm) llm->info(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void llmwarn(const std::string& fmt, Args&&... args) {
        if (llm) llm->warn(fmt, std::forward<Args>(args)...);
    }

    ~CLogger();
};

#endif //CLOGGER_H