//
// CLogger - Centralized logging system for BotMasterXL
//

#include "CLogger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// Initialize the static member
CLogger* CLogger::instance = nullptr;

// Definition of the private constructor
CLogger::CLogger() {
}

// Definition of the static getInstance method
CLogger* CLogger::getInstance() {
    // Lazy initialization: instance is created only when it's first requested
    if (instance == nullptr) {
        instance = new CLogger();
    }
    return instance;
}

void CLogger::init() {
    // Initialize specialized loggers
    // Create color sinks
    auto bot_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto system_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto api_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto llm_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // Set color per logger name
    bot_sink->set_color(spdlog::level::info, bot_sink->cyan);
    system_sink->set_color(spdlog::level::info, system_sink->green);
    api_sink->set_color(spdlog::level::info, system_sink->yellow);
    llm_sink->set_color(spdlog::level::info, system_sink->magenta);

    // Create loggers with their respective sinks
    bot = std::make_shared<spdlog::logger>("bot", bot_sink);
    system = std::make_shared<spdlog::logger>("system", system_sink);
    api = std::make_shared<spdlog::logger>("api", api_sink);
    llm = std::make_shared<spdlog::logger>("llm", llm_sink);

    // Set format
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%n%$] %v";
    bot->set_pattern(pattern);
    system->set_pattern(pattern);
    api->set_pattern(pattern);
    llm->set_pattern(pattern);

    // Register loggers globally if needed
    spdlog::register_logger(bot);
    spdlog::register_logger(system);
    spdlog::register_logger(api);
    spdlog::register_logger(llm);
}

CLogger::~CLogger() {
    // Cleanup is handled automatically by spdlog
}