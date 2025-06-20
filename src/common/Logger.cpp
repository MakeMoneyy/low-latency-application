#include "common/Logger.h"
#include <iostream>

namespace trading {

bool Logger::initialized_ = false;
std::shared_ptr<spdlog::sinks::file_sink_mt> Logger::file_sink_ = nullptr;
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> Logger::console_sink_ = nullptr;

void Logger::initialize(const std::string& log_file, spdlog::level::level_enum level) {
    if (initialized_) {
        return;
    }
    
    try {
        // Create sinks
        file_sink_ = std::make_shared<spdlog::sinks::file_sink_mt>(log_file, true);
        console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        
        // Set log patterns
        file_sink_->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] %v");
        console_sink_->set_pattern("[%H:%M:%S.%f] [%n] [%^%l%$] %v");
        
        // Set log levels
        file_sink_->set_level(level);
        console_sink_->set_level(level);
        
        // Set global log level
        spdlog::set_level(level);
        
        initialized_ = true;
        
        std::cout << "Logger initialized with log file: " << log_file << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
    }
}

std::shared_ptr<spdlog::logger> Logger::getLogger(const std::string& name) {
    if (!initialized_) {
        initialize();
    }
    
    auto logger = spdlog::get(name);
    if (!logger) {
        std::vector<spdlog::sink_ptr> sinks;
        
        if (file_sink_) {
            sinks.push_back(file_sink_);
        }
        if (console_sink_) {
            sinks.push_back(console_sink_);
        }
        
        logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        spdlog::register_logger(logger);
    }
    
    return logger;
}

std::shared_ptr<spdlog::logger> Logger::getMarketDataLogger() {
    return getLogger("MarketData");
}

std::shared_ptr<spdlog::logger> Logger::getStrategyLogger() {
    return getLogger("Strategy");
}

std::shared_ptr<spdlog::logger> Logger::getExecutionLogger() {
    return getLogger("Execution");
}

std::shared_ptr<spdlog::logger> Logger::getPerformanceLogger() {
    return getLogger("Performance");
}

} // namespace trading 