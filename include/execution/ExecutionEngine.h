#pragma once

#include <aeron/Aeron.h>
#include <aeron/Subscription.h>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

#include "strategy/StrategyEngine.h"
#include "common/TimeUtils.h"
#include "common/Logger.h"

namespace trading {

/**
 * @brief Execution status
 */
enum class ExecutionStatus {
    PENDING,
    FILLED,
    PARTIALLY_FILLED,
    REJECTED,
    CANCELLED
};

/**
 * @brief Trade execution record
 */
struct TradeExecution {
    std::int64_t execution_timestamp;
    std::string order_id;
    SignalType signal;
    double executed_price;
    double executed_quantity;
    ExecutionStatus status;
    char symbol[16];
    std::int64_t execution_latency_ns;  // Time from order to execution
};

/**
 * @brief Performance metrics
 */
struct PerformanceMetrics {
    double total_pnl;
    double win_rate;
    std::uint64_t total_trades;
    std::uint64_t winning_trades;
    std::uint64_t losing_trades;
    double max_drawdown;
    double sharpe_ratio;
    double avg_trade_pnl;
    std::int64_t avg_execution_latency_ns;
    std::int64_t max_execution_latency_ns;
};

/**
 * @brief Execution Engine - executes trading orders and tracks performance
 */
class ExecutionEngine {
public:
    ExecutionEngine();
    ~ExecutionEngine();
    
    /**
     * @brief Initialize the execution engine
     * @param aeron Aeron context
     * @param input_channel Input channel for trading orders
     * @param input_stream_id Input stream ID
     * @return true if successful
     */
    bool initialize(std::shared_ptr<aeron::Aeron> aeron,
                   const std::string& input_channel,
                   std::int32_t input_stream_id);
    
    /**
     * @brief Start the execution engine
     */
    void start();
    
    /**
     * @brief Stop the execution engine
     */
    void stop();
    
    /**
     * @brief Check if engine is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * @brief Enable/disable simulation mode
     * @param enable true for simulation, false for live trading
     */
    void setSimulationMode(bool enable) { simulation_mode_ = enable; }
    
    /**
     * @brief Set initial capital for simulation
     * @param capital Initial capital amount
     */
    void setInitialCapital(double capital) { initial_capital_ = capital; }
    
    /**
     * @brief Get current performance metrics
     */
    PerformanceMetrics getPerformanceMetrics() const;
    
    /**
     * @brief Get trade history
     */
    std::vector<TradeExecution> getTradeHistory() const;
    
    /**
     * @brief Reset performance tracking
     */
    void resetPerformanceTracking();

private:
    std::shared_ptr<aeron::Aeron> aeron_;
    std::shared_ptr<aeron::Subscription> input_subscription_;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> processing_thread_;
    
    // Execution settings
    bool simulation_mode_;
    double initial_capital_;
    double current_capital_;
    double current_position_;  // Current position size
    
    // Trade tracking
    mutable std::mutex trades_mutex_;
    std::vector<TradeExecution> trade_history_;
    std::uint64_t order_counter_;
    
    // Performance tracking
    mutable std::mutex performance_mutex_;
    PerformanceMetrics performance_metrics_;
    std::vector<double> daily_returns_;
    double peak_capital_;
    
    // Processing methods
    void processLoop();
    void processOrder(const aeron::concurrent::AtomicBuffer& buffer,
                     util::index_t offset,
                     util::index_t length);
    
    // Execution methods
    TradeExecution executeOrder(const TradingOrder& order);
    TradeExecution simulateExecution(const TradingOrder& order);
    TradeExecution executeLiveOrder(const TradingOrder& order);
    
    // Performance calculation methods
    void updatePerformanceMetrics(const TradeExecution& execution);
    double calculatePnL(const TradeExecution& execution) const;
    void updateDrawdown(double current_pnl);
    double calculateSharpeRatio() const;
    
    // Utility methods
    std::string generateOrderId();
    double getMarketPrice(const std::string& symbol) const;  // For simulation
};

} // namespace trading 