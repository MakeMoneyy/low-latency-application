#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace trading {

/**
 * @brief Configuration manager for the trading system
 */
class Config {
public:
    struct AeronConfig {
        std::string channel;
        std::int32_t stream_id;
        std::string directory;
        std::int64_t timeout_ms;
    };

    struct DCConfig {
        double theta;              // DC threshold (e.g., 0.004 for 0.4%)
        bool enable_tmv_calculation;
        bool enable_time_adjustment;
    };

    struct StrategyConfig {
        std::string name;
        bool enable_hmm;
        int hmm_states;
        int hmm_max_iterations;
        double leverage_factor;
    };

    struct PerformanceConfig {
        bool enable_latency_tracking;
        bool enable_performance_metrics;
        std::string output_file;
    };

    static Config& getInstance();
    
    bool loadConfig(const std::string& config_file);
    
    // Getters
    const AeronConfig& getMarketDataConfig() const { return market_data_config_; }
    const AeronConfig& getStrategyConfig() const { return strategy_config_; }
    const AeronConfig& getExecutionConfig() const { return execution_config_; }
    const DCConfig& getDCConfig() const { return dc_config_; }
    const StrategyConfig& getStrategySettings() const { return strategy_settings_; }
    const PerformanceConfig& getPerformanceConfig() const { return performance_config_; }

private:
    Config() = default;
    
    AeronConfig market_data_config_;
    AeronConfig strategy_config_;
    AeronConfig execution_config_;
    DCConfig dc_config_;
    StrategyConfig strategy_settings_;
    PerformanceConfig performance_config_;
    
    void setDefaults();
};

} // namespace trading 