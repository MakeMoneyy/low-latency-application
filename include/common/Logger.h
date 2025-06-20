#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/file_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace trading {

/**
 * @brief Centralized logging system using spdlog
 */
class Logger {
public:
    static void initialize(const std::string& log_file = "trading_system.log", 
                          spdlog::level::level_enum level = spdlog::level::info);
    
    static std::shared_ptr<spdlog::logger> getLogger(const std::string& name);
    
    // Convenience macros for different components
    static std::shared_ptr<spdlog::logger> getMarketDataLogger();
    static std::shared_ptr<spdlog::logger> getStrategyLogger();
    static std::shared_ptr<spdlog::logger> getExecutionLogger();
    static std::shared_ptr<spdlog::logger> getPerformanceLogger();

private:
    static bool initialized_;
    static std::shared_ptr<spdlog::sinks::file_sink_mt> file_sink_;
    static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
};

// Convenient logging macros
#define LOG_MARKET_DATA(...) trading::Logger::getMarketDataLogger()->info(__VA_ARGS__)
#define LOG_STRATEGY(...) trading::Logger::getStrategyLogger()->info(__VA_ARGS__)
#define LOG_EXECUTION(...) trading::Logger::getExecutionLogger()->info(__VA_ARGS__)
#define LOG_PERFORMANCE(...) trading::Logger::getPerformanceLogger()->info(__VA_ARGS__)

#define LOG_ERROR_MARKET_DATA(...) trading::Logger::getMarketDataLogger()->error(__VA_ARGS__)
#define LOG_ERROR_STRATEGY(...) trading::Logger::getStrategyLogger()->error(__VA_ARGS__)
#define LOG_ERROR_EXECUTION(...) trading::Logger::getExecutionLogger()->error(__VA_ARGS__)

#define LOG_DEBUG_MARKET_DATA(...) trading::Logger::getMarketDataLogger()->debug(__VA_ARGS__)
#define LOG_DEBUG_STRATEGY(...) trading::Logger::getStrategyLogger()->debug(__VA_ARGS__)
#define LOG_DEBUG_EXECUTION(...) trading::Logger::getExecutionLogger()->debug(__VA_ARGS__)

} // namespace trading 