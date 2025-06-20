#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>

#include <aeron/Aeron.h>
#include <aeron/Context.h>

#include "common/Config.h"
#include "common/Logger.h"
#include "market_data/MarketDataProcessor.h"
#include "strategy/StrategyEngine.h"
#include "execution/ExecutionEngine.h"

namespace {
    volatile bool running = true;
    
    void signalHandler(int signal) {
        std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
        running = false;
    }
}

int main(int argc, char* argv[]) {
    // Install signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Initialize logging
        trading::Logger::initialize("trading_system.log", spdlog::level::info);
        
        std::cout << "=== Low Latency Trading System Starting ===" << std::endl;
        
        // Load configuration
        auto& config = trading::Config::getInstance();
        std::string config_file = (argc > 1) ? argv[1] : "config/system_config.json";
        
        if (!config.loadConfig(config_file)) {
            std::cerr << "Failed to load configuration from: " << config_file << std::endl;
            return 1;
        }
        
        std::cout << "Configuration loaded successfully" << std::endl;
        
        // Initialize Aeron context
        aeron::Context aeronContext;
        aeronContext.aeronDir(config.getMarketDataConfig().directory);
        
        // Set error handler
        aeronContext.errorHandler([](const std::exception& exception) {
            std::cerr << "Aeron error: " << exception.what() << std::endl;
        });
        
        // Set idle strategy for low latency
        aeronContext.idleStrategy(std::make_shared<aeron::BusySpinIdleStrategy>());
        
        auto aeron = aeron::Aeron::connect(aeronContext);
        std::cout << "Aeron connection established" << std::endl;
        
        // Initialize components
        trading::MarketDataProcessor market_data_processor;
        trading::StrategyEngine strategy_engine;
        trading::ExecutionEngine execution_engine;
        
        // Configure market data processor
        if (!market_data_processor.initialize(
                aeron,
                config.getMarketDataConfig().channel,
                config.getMarketDataConfig().stream_id,
                config.getStrategyConfig().channel,
                config.getStrategyConfig().stream_id)) {
            std::cerr << "Failed to initialize market data processor" << std::endl;
            return 1;
        }
        market_data_processor.setDCThreshold(config.getDCConfig().theta);
        
        // Configure strategy engine
        if (!strategy_engine.initialize(
                aeron,
                config.getStrategyConfig().channel,
                config.getStrategyConfig().stream_id,
                config.getExecutionConfig().channel,
                config.getExecutionConfig().stream_id)) {
            std::cerr << "Failed to initialize strategy engine" << std::endl;
            return 1;
        }
        strategy_engine.enableHMM(config.getStrategySettings().enable_hmm);
        strategy_engine.setLeverageFactor(config.getStrategySettings().leverage_factor);
        
        // Configure execution engine
        if (!execution_engine.initialize(
                aeron,
                config.getExecutionConfig().channel,
                config.getExecutionConfig().stream_id)) {
            std::cerr << "Failed to initialize execution engine" << std::endl;
            return 1;
        }
        execution_engine.setSimulationMode(true);  // Default to simulation mode
        execution_engine.setInitialCapital(100000.0);
        
        std::cout << "All components initialized successfully" << std::endl;
        
        // Start all components
        market_data_processor.start();
        strategy_engine.start();
        execution_engine.start();
        
        std::cout << "All components started successfully" << std::endl;
        std::cout << "Trading system is running... Press Ctrl+C to stop" << std::endl;
        
        // Main loop - monitor system and print statistics
        auto last_stats_time = std::chrono::steady_clock::now();
        const auto stats_interval = std::chrono::seconds(10);
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            auto now = std::chrono::steady_clock::now();
            if (now - last_stats_time >= stats_interval) {
                // Print system statistics
                auto md_stats = market_data_processor.getStatistics();
                auto strategy_stats = strategy_engine.getStatistics();
                auto execution_stats = execution_engine.getPerformanceMetrics();
                
                std::cout << "\n=== System Statistics ===" << std::endl;
                std::cout << "Market Data: " << md_stats.messages_processed 
                         << " messages, " << md_stats.dc_events_detected 
                         << " DC events, Avg latency: " << md_stats.avg_processing_latency_ns << " ns" << std::endl;
                
                std::cout << "Strategy: " << strategy_stats.signals_processed 
                         << " signals, " << strategy_stats.orders_generated 
                         << " orders, Avg latency: " << strategy_stats.avg_strategy_latency_ns << " ns" << std::endl;
                
                std::cout << "Execution: " << execution_stats.total_trades 
                         << " trades, PnL: $" << execution_stats.total_pnl 
                         << ", Win rate: " << (execution_stats.win_rate * 100) << "%" << std::endl;
                
                last_stats_time = now;
            }
        }
        
        std::cout << "\nShutting down components..." << std::endl;
        
        // Stop all components
        market_data_processor.stop();
        strategy_engine.stop();
        execution_engine.stop();
        
        std::cout << "All components stopped successfully" << std::endl;
        
        // Print final performance report
        auto final_stats = execution_engine.getPerformanceMetrics();
        std::cout << "\n=== Final Performance Report ===" << std::endl;
        std::cout << "Total Trades: " << final_stats.total_trades << std::endl;
        std::cout << "Total PnL: $" << final_stats.total_pnl << std::endl;
        std::cout << "Win Rate: " << (final_stats.win_rate * 100) << "%" << std::endl;
        std::cout << "Sharpe Ratio: " << final_stats.sharpe_ratio << std::endl;
        std::cout << "Max Drawdown: " << (final_stats.max_drawdown * 100) << "%" << std::endl;
        std::cout << "Average Execution Latency: " << final_stats.avg_execution_latency_ns << " ns" << std::endl;
        
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Trading system shutdown complete" << std::endl;
    return 0;
}