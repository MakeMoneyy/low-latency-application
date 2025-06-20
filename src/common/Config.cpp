#include "common/Config.h"
#include <fstream>
#include <iostream>

namespace trading {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadConfig(const std::string& config_file) {
    try {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << config_file << std::endl;
            setDefaults();
            return false;
        }
        
        nlohmann::json json_config;
        file >> json_config;
        
        // Load Aeron configurations
        if (json_config.contains("aeron")) {
            auto& aeron_config = json_config["aeron"];
            
            if (aeron_config.contains("market_data")) {
                auto& md_config = aeron_config["market_data"];
                market_data_config_.channel = md_config.value("channel", "aeron:ipc");
                market_data_config_.stream_id = md_config.value("stream_id", 1001);
                market_data_config_.directory = md_config.value("directory", "/tmp/aeron");
                market_data_config_.timeout_ms = md_config.value("timeout_ms", 5000);
            }
            
            if (aeron_config.contains("strategy")) {
                auto& strategy_config = aeron_config["strategy"];
                strategy_config_.channel = strategy_config.value("channel", "aeron:ipc");
                strategy_config_.stream_id = strategy_config.value("stream_id", 1002);
                strategy_config_.directory = strategy_config.value("directory", "/tmp/aeron");
                strategy_config_.timeout_ms = strategy_config.value("timeout_ms", 5000);
            }
            
            if (aeron_config.contains("execution")) {
                auto& exec_config = aeron_config["execution"];
                execution_config_.channel = exec_config.value("channel", "aeron:ipc");
                execution_config_.stream_id = exec_config.value("stream_id", 1003);
                execution_config_.directory = exec_config.value("directory", "/tmp/aeron");
                execution_config_.timeout_ms = exec_config.value("timeout_ms", 5000);
            }
        }
        
        // Load DC strategy configuration
        if (json_config.contains("dc_strategy")) {
            auto& dc_config = json_config["dc_strategy"];
            dc_config_.theta = dc_config.value("theta", 0.004);
            dc_config_.enable_tmv_calculation = dc_config.value("enable_tmv_calculation", true);
            dc_config_.enable_time_adjustment = dc_config.value("enable_time_adjustment", true);
        }
        
        // Load strategy settings
        if (json_config.contains("strategy_settings")) {
            auto& strategy_settings = json_config["strategy_settings"];
            strategy_settings_.name = strategy_settings.value("name", "DC_Strategy_v1");
            strategy_settings_.enable_hmm = strategy_settings.value("enable_hmm", false);
            strategy_settings_.hmm_states = strategy_settings.value("hmm_states", 2);
            strategy_settings_.hmm_max_iterations = strategy_settings.value("hmm_max_iterations", 200);
            strategy_settings_.leverage_factor = strategy_settings.value("leverage_factor", 1.0);
        }
        
        // Load performance configuration
        if (json_config.contains("performance")) {
            auto& perf_config = json_config["performance"];
            performance_config_.enable_latency_tracking = perf_config.value("enable_latency_tracking", true);
            performance_config_.enable_performance_metrics = perf_config.value("enable_performance_metrics", true);
            performance_config_.output_file = perf_config.value("output_file", "performance_report.json");
        }
        
        std::cout << "Configuration loaded successfully from: " << config_file << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        setDefaults();
        return false;
    }
}

void Config::setDefaults() {
    // Set default Aeron configurations
    market_data_config_.channel = "aeron:ipc";
    market_data_config_.stream_id = 1001;
    market_data_config_.directory = "/tmp/aeron";
    market_data_config_.timeout_ms = 5000;
    
    strategy_config_.channel = "aeron:ipc";
    strategy_config_.stream_id = 1002;
    strategy_config_.directory = "/tmp/aeron";
    strategy_config_.timeout_ms = 5000;
    
    execution_config_.channel = "aeron:ipc";
    execution_config_.stream_id = 1003;
    execution_config_.directory = "/tmp/aeron";
    execution_config_.timeout_ms = 5000;
    
    // Set default DC configuration
    dc_config_.theta = 0.004;  // 0.4%
    dc_config_.enable_tmv_calculation = true;
    dc_config_.enable_time_adjustment = true;
    
    // Set default strategy settings
    strategy_settings_.name = "DC_Strategy_v1";
    strategy_settings_.enable_hmm = false;
    strategy_settings_.hmm_states = 2;
    strategy_settings_.hmm_max_iterations = 200;
    strategy_settings_.leverage_factor = 1.0;
    
    // Set default performance configuration
    performance_config_.enable_latency_tracking = true;
    performance_config_.enable_performance_metrics = true;
    performance_config_.output_file = "performance_report.json";
}

} // namespace trading 