#include <iostream>
#include <memory>
#include <thread>
#include <random>
#include <chrono>
#include <signal.h>

#include <aeron/Aeron.h>
#include <aeron/Context.h>
#include <aeron/Publication.h>

#include "common/Config.h"
#include "common/Logger.h"
#include "common/TimeUtils.h"
#include "market_data/MarketDataProcessor.h"

namespace {
    volatile bool running = true;
    
    void signalHandler(int signal) {
        std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
        running = false;
    }
}

/**
 * @brief Market data simulator that generates realistic price data
 */
class MarketDataSimulator {
public:
    MarketDataSimulator() 
        : price_(150.0)
        , trend_(0.0)
        , volatility_(0.02)
        , message_count_(0)
    {
        // Initialize random generators
        std::random_device rd;
        gen_.seed(rd());
        price_dist_ = std::normal_distribution<double>(0.0, 1.0);
        trend_dist_ = std::normal_distribution<double>(0.0, 0.001);
    }
    
    bool initialize(std::shared_ptr<aeron::Aeron> aeron,
                   const std::string& channel,
                   std::int32_t stream_id) {
        try {
            aeron_ = aeron;
            publication_ = aeron_->addPublication(channel, stream_id);
            
            // Wait for publication to connect
            while (!publication_->isConnected()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            
            std::cout << "Market data simulator initialized on " << channel 
                     << " stream " << stream_id << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize simulator: " << e.what() << std::endl;
            return false;
        }
    }
    
    void start(int messages_per_second = 1000) {
        std::cout << "Starting market data simulation at " << messages_per_second 
                 << " messages/second" << std::endl;
        
        auto interval = std::chrono::microseconds(1000000 / messages_per_second);
        auto next_send_time = std::chrono::high_resolution_clock::now();
        
        while (running) {
            // Generate next price
            generateNextPrice();
            
            // Create market data message
            trading::MarketDataMessage market_data;
            market_data.timestamp = trading::TimeUtils::getCurrentTimestampNs();
            market_data.price = price_;
            market_data.volume = generateVolume();
            std::strncpy(market_data.symbol, "EURUSD", sizeof(market_data.symbol) - 1);
            market_data.symbol[sizeof(market_data.symbol) - 1] = '\0';
            
            // Publish the message
            publishMarketData(market_data);
            
            message_count_++;
            
            // Print progress every 1000 messages
            if (message_count_ % 1000 == 0) {
                std::cout << "Sent " << message_count_ << " messages, current price: " 
                         << std::fixed << std::setprecision(5) << price_ << std::endl;
            }
            
            // Wait for next interval
            next_send_time += interval;
            std::this_thread::sleep_until(next_send_time);
        }
        
        std::cout << "Market data simulation stopped. Total messages: " << message_count_ << std::endl;
    }

private:
    void generateNextPrice() {
        // Update trend with some persistence and mean reversion
        trend_ = trend_ * 0.99 + trend_dist_(gen_);
        
        // Generate price change with trend and volatility
        double price_change = trend_ + volatility_ * price_dist_(gen_) * price_ * 0.0001;
        price_ += price_change;
        
        // Keep price in reasonable range
        price_ = std::max(100.0, std::min(200.0, price_));
        
        // Occasionally introduce larger moves (DC events)
        if (price_dist_(gen_) > 2.5) {  // ~1% chance
            double large_move = (price_dist_(gen_) > 0 ? 1 : -1) * price_ * 0.005;  // 0.5% move
            price_ += large_move;
            std::cout << "Large move: " << large_move << " new price: " << price_ << std::endl;
        }
    }
    
    double generateVolume() {
        // Generate realistic volume
        static std::uniform_real_distribution<double> volume_dist(1000.0, 10000.0);
        return volume_dist(gen_);
    }
    
    bool publishMarketData(const trading::MarketDataMessage& market_data) {
        aeron::concurrent::AtomicBuffer buffer(
            reinterpret_cast<const std::uint8_t*>(&market_data), 
            sizeof(market_data));
        
        std::int64_t result = publication_->offer(buffer, 0, sizeof(market_data));
        
        if (result > 0) {
            return true;
        } else {
            if (result == aeron::BACK_PRESSURED) {
                // Handle back pressure - could implement retry logic
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            } else {
                std::cerr << "Failed to publish market data, result: " << result << std::endl;
            }
            return false;
        }
    }
    
    std::shared_ptr<aeron::Aeron> aeron_;
    std::shared_ptr<aeron::Publication> publication_;
    
    // Price simulation
    double price_;
    double trend_;
    double volatility_;
    std::uint64_t message_count_;
    
    // Random generators
    std::mt19937 gen_;
    std::normal_distribution<double> price_dist_;
    std::normal_distribution<double> trend_dist_;
};

int main(int argc, char* argv[]) {
    // Install signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Initialize logging
        trading::Logger::initialize("market_data_simulator.log", spdlog::level::info);
        
        std::cout << "=== Market Data Simulator Starting ===" << std::endl;
        
        // Load configuration
        auto& config = trading::Config::getInstance();
        std::string config_file = (argc > 1) ? argv[1] : "config/system_config.json";
        
        if (!config.loadConfig(config_file)) {
            std::cerr << "Failed to load configuration from: " << config_file << std::endl;
            return 1;
        }
        
        // Initialize Aeron context
        aeron::Context aeronContext;
        aeronContext.aeronDir(config.getMarketDataConfig().directory);
        aeronContext.errorHandler([](const std::exception& exception) {
            std::cerr << "Aeron error: " << exception.what() << std::endl;
        });
        
        auto aeron = aeron::Aeron::connect(aeronContext);
        std::cout << "Aeron connection established" << std::endl;
        
        // Initialize market data simulator
        MarketDataSimulator simulator;
        if (!simulator.initialize(
                aeron,
                config.getMarketDataConfig().channel,
                config.getMarketDataConfig().stream_id)) {
            std::cerr << "Failed to initialize market data simulator" << std::endl;
            return 1;
        }
        
        // Start simulation
        int messages_per_second = (argc > 2) ? std::atoi(argv[2]) : 1000;
        simulator.start(messages_per_second);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Market data simulator shutdown complete" << std::endl;
    return 0;
} 