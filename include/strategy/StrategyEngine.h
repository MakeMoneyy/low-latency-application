#pragma once

#include <aeron/Aeron.h>
#include <aeron/Subscription.h>
#include <aeron/Publication.h>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

#include "common/DCIndicator.h"
#include "common/TimeUtils.h"
#include "common/Logger.h"

namespace trading {

/**
 * @brief Trading signal types
 */
enum class SignalType {
    NONE,
    BUY,
    SELL,
    HOLD
};

/**
 * @brief Trading order structure
 */
struct TradingOrder {
    std::int64_t timestamp;
    SignalType signal;
    double price;
    double quantity;
    char symbol[16];
    std::int64_t strategy_latency_ns;  // Time from DC event to order generation
};

/**
 * @brief HMM state for market regime detection
 */
enum class MarketState {
    UNKNOWN,
    LOW_VOLATILITY,
    HIGH_VOLATILITY
};

/**
 * @brief Strategy Engine - processes DC signals and generates trading orders
 */
class StrategyEngine {
public:
    StrategyEngine();
    ~StrategyEngine();
    
    /**
     * @brief Initialize the strategy engine
     * @param aeron Aeron context
     * @param input_channel Input channel for DC signals
     * @param input_stream_id Input stream ID
     * @param output_channel Output channel for trading orders
     * @param output_stream_id Output stream ID
     * @return true if successful
     */
    bool initialize(std::shared_ptr<aeron::Aeron> aeron,
                   const std::string& input_channel,
                   std::int32_t input_stream_id,
                   const std::string& output_channel,
                   std::int32_t output_stream_id);
    
    /**
     * @brief Start the strategy engine
     */
    void start();
    
    /**
     * @brief Stop the strategy engine
     */
    void stop();
    
    /**
     * @brief Check if engine is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * @brief Enable/disable HMM regime detection
     * @param enable true to enable HMM
     */
    void enableHMM(bool enable) { hmm_enabled_ = enable; }
    
    /**
     * @brief Set leverage factor
     * @param leverage New leverage factor
     */
    void setLeverageFactor(double leverage) { leverage_factor_ = leverage; }
    
    /**
     * @brief Get strategy statistics
     */
    struct Statistics {
        std::uint64_t signals_processed;
        std::uint64_t orders_generated;
        std::uint64_t buy_signals;
        std::uint64_t sell_signals;
        std::int64_t avg_strategy_latency_ns;
        std::int64_t max_strategy_latency_ns;
        MarketState current_market_state;
    };
    
    Statistics getStatistics() const;

private:
    std::shared_ptr<aeron::Aeron> aeron_;
    std::shared_ptr<aeron::Subscription> input_subscription_;
    std::shared_ptr<aeron::Publication> output_publication_;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> processing_thread_;
    
    // Strategy parameters
    bool hmm_enabled_;
    double leverage_factor_;
    MarketState current_market_state_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics statistics_;
    
    // Processing methods
    void processLoop();
    void processDCSignal(const aeron::concurrent::AtomicBuffer& buffer,
                        util::index_t offset,
                        util::index_t length);
    
    SignalType generateTradingSignal(const DCSignalMessage& dc_signal);
    double calculateOrderQuantity(SignalType signal, double price);
    bool publishTradingOrder(const TradingOrder& order);
    
    // HMM-related methods
    void updateMarketState(const DCSignalMessage& dc_signal);
    double getVolatilityAdjustedLeverage() const;
    
    // Latency tracking
    void updateLatencyStats(std::int64_t latency_ns);
};

} // namespace trading 