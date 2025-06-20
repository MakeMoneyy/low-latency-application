#include "execution/ExecutionEngine.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <random>

namespace trading {

ExecutionEngine::ExecutionEngine()
    : running_(false)
    , simulation_mode_(true)
    , initial_capital_(100000.0)
    , current_capital_(100000.0)
    , current_position_(0.0)
    , order_counter_(0)
    , peak_capital_(100000.0)
{
    performance_metrics_ = {0.0, 0.0, 0, 0, 0, 0.0, 0.0, 0.0, 0, 0};
}

ExecutionEngine::~ExecutionEngine() {
    stop();
}

bool ExecutionEngine::initialize(std::shared_ptr<aeron::Aeron> aeron,
                                const std::string& input_channel,
                                std::int32_t input_stream_id) {
    try {
        aeron_ = aeron;
        
        // Create input subscription for trading orders
        LOG_EXECUTION("Creating subscription for trading orders: {} stream {}", 
                     input_channel, input_stream_id);
        
        input_subscription_ = aeron_->addSubscription(input_channel, input_stream_id);
        
        // Wait for subscription to connect
        while (!input_subscription_->isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        LOG_EXECUTION("Execution engine initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR_EXECUTION("Failed to initialize execution engine: {}", e.what());
        return false;
    }
}

void ExecutionEngine::start() {
    if (running_.load()) {
        LOG_EXECUTION("Execution engine is already running");
        return;
    }
    
    running_.store(true);
    processing_thread_ = std::make_unique<std::thread>(&ExecutionEngine::processLoop, this);
    
    LOG_EXECUTION("Execution engine started in {} mode", 
                 simulation_mode_ ? "simulation" : "live");
}

void ExecutionEngine::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (processing_thread_ && processing_thread_->joinable()) {
        processing_thread_->join();
    }
    
    LOG_EXECUTION("Execution engine stopped");
}

PerformanceMetrics ExecutionEngine::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(performance_mutex_);
    return performance_metrics_;
}

std::vector<TradeExecution> ExecutionEngine::getTradeHistory() const {
    std::lock_guard<std::mutex> lock(trades_mutex_);
    return trade_history_;
}

void ExecutionEngine::resetPerformanceTracking() {
    std::lock_guard<std::mutex> lock1(performance_mutex_);
    std::lock_guard<std::mutex> lock2(trades_mutex_);
    
    current_capital_ = initial_capital_;
    current_position_ = 0.0;
    peak_capital_ = initial_capital_;
    
    performance_metrics_ = {0.0, 0.0, 0, 0, 0, 0.0, 0.0, 0.0, 0, 0};
    trade_history_.clear();
    daily_returns_.clear();
    
    LOG_EXECUTION("Performance tracking reset");
}

void ExecutionEngine::processLoop() {
    LOG_EXECUTION("Execution processing loop started");
    
    aeron::concurrent::SleepingIdleStrategy idleStrategy(std::chrono::milliseconds(1));
    
    while (running_.load()) {
        const int fragmentsRead = input_subscription_->poll(
            [this](const aeron::concurrent::AtomicBuffer& buffer, 
                   util::index_t offset, 
                   util::index_t length, 
                   const aeron::Header& header) {
                processOrder(buffer, offset, length);
            }, 
            10);  // Poll up to 10 fragments at a time
        
        idleStrategy.idle(fragmentsRead);
    }
    
    LOG_EXECUTION("Execution processing loop ended");
}

void ExecutionEngine::processOrder(const aeron::concurrent::AtomicBuffer& buffer,
                                  util::index_t offset,
                                  util::index_t length) {
    if (length < sizeof(TradingOrder)) {
        LOG_ERROR_EXECUTION("Invalid trading order message size: {}", length);
        return;
    }
    
    // Extract trading order message
    TradingOrder order;
    std::memcpy(&order, buffer.buffer() + offset, sizeof(TradingOrder));
    
    // Execute the order
    TradeExecution execution = executeOrder(order);
    
    // Store execution record
    {
        std::lock_guard<std::mutex> lock(trades_mutex_);
        trade_history_.push_back(execution);
    }
    
    // Update performance metrics
    updatePerformanceMetrics(execution);
    
    LOG_DEBUG_EXECUTION("Order executed: signal={}, price={}, quantity={}, status={}", 
                       static_cast<int>(execution.signal),
                       execution.executed_price,
                       execution.executed_quantity,
                       static_cast<int>(execution.status));
}

TradeExecution ExecutionEngine::executeOrder(const TradingOrder& order) {
    if (simulation_mode_) {
        return simulateExecution(order);
    } else {
        return executeLiveOrder(order);
    }
}

TradeExecution ExecutionEngine::simulateExecution(const TradingOrder& order) {
    auto execution_start = TimeUtils::getCurrentTime();
    
    TradeExecution execution;
    execution.execution_timestamp = TimeUtils::getCurrentTimestampNs();
    execution.order_id = generateOrderId();
    execution.signal = order.signal;
    execution.executed_price = order.price;
    execution.executed_quantity = order.quantity;
    execution.status = ExecutionStatus::FILLED;  // Assume all orders fill in simulation
    std::strncpy(execution.symbol, order.symbol, sizeof(execution.symbol) - 1);
    execution.symbol[sizeof(execution.symbol) - 1] = '\0';
    
    // Add some realistic execution latency simulation (10-100 microseconds)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> latency_dist(10000, 100000);  // 10-100 microseconds in nanoseconds
    
    auto simulated_latency = std::chrono::nanoseconds(latency_dist(gen));
    std::this_thread::sleep_for(simulated_latency);
    
    execution.execution_latency_ns = TimeUtils::getDurationNs(execution_start, TimeUtils::getCurrentTime());
    
    // Add small price slippage simulation
    std::uniform_real_distribution<> slippage_dist(-0.0001, 0.0001);  // ±0.01% slippage
    double slippage = slippage_dist(gen);
    execution.executed_price *= (1.0 + slippage);
    
    return execution;
}

TradeExecution ExecutionEngine::executeLiveOrder(const TradingOrder& order) {
    // This would implement actual order execution via broker API
    // For now, return a placeholder implementation
    
    TradeExecution execution;
    execution.execution_timestamp = TimeUtils::getCurrentTimestampNs();
    execution.order_id = generateOrderId();
    execution.signal = order.signal;
    execution.executed_price = order.price;
    execution.executed_quantity = order.quantity;
    execution.status = ExecutionStatus::PENDING;  // Would be updated by broker callback
    std::strncpy(execution.symbol, order.symbol, sizeof(execution.symbol) - 1);
    execution.symbol[sizeof(execution.symbol) - 1] = '\0';
    execution.execution_latency_ns = 0;  // Would be set when execution completes
    
    LOG_EXECUTION("Live order execution not implemented - placeholder returned");
    
    return execution;
}

void ExecutionEngine::updatePerformanceMetrics(const TradeExecution& execution) {
    if (execution.status != ExecutionStatus::FILLED) {
        return;  // Only update metrics for filled orders
    }
    
    std::lock_guard<std::mutex> lock(performance_mutex_);
    
    // Calculate P&L for this trade
    double trade_pnl = calculatePnL(execution);
    
    // Update position
    if (execution.signal == SignalType::BUY) {
        current_position_ += execution.executed_quantity;
    } else if (execution.signal == SignalType::SELL) {
        current_position_ -= execution.executed_quantity;
    }
    
    // Update capital
    current_capital_ += trade_pnl;
    
    // Update performance metrics
    performance_metrics_.total_pnl += trade_pnl;
    performance_metrics_.total_trades++;
    
    if (trade_pnl > 0) {
        performance_metrics_.winning_trades++;
    } else if (trade_pnl < 0) {
        performance_metrics_.losing_trades++;
    }
    
    // Update win rate
    if (performance_metrics_.total_trades > 0) {
        performance_metrics_.win_rate = 
            static_cast<double>(performance_metrics_.winning_trades) / performance_metrics_.total_trades;
    }
    
    // Update average trade P&L
    performance_metrics_.avg_trade_pnl = 
        performance_metrics_.total_pnl / performance_metrics_.total_trades;
    
    // Update drawdown
    updateDrawdown(current_capital_);
    
    // Update execution latency stats
    if (execution.execution_latency_ns > 0) {
        if (performance_metrics_.total_trades == 1) {
            performance_metrics_.avg_execution_latency_ns = execution.execution_latency_ns;
        } else {
            performance_metrics_.avg_execution_latency_ns = 
                (performance_metrics_.avg_execution_latency_ns * 0.9) + 
                (execution.execution_latency_ns * 0.1);
        }
        
        if (execution.execution_latency_ns > performance_metrics_.max_execution_latency_ns) {
            performance_metrics_.max_execution_latency_ns = execution.execution_latency_ns;
        }
    }
    
    // Update Sharpe ratio (simplified calculation)
    daily_returns_.push_back(trade_pnl / initial_capital_);
    if (daily_returns_.size() > 252) {  // Keep one year of data
        daily_returns_.erase(daily_returns_.begin());
    }
    performance_metrics_.sharpe_ratio = calculateSharpeRatio();
}

double ExecutionEngine::calculatePnL(const TradeExecution& execution) const {
    // Simplified P&L calculation
    // In a real system, this would consider position sizing, entry/exit prices, etc.
    
    static double last_price = execution.executed_price;
    
    double pnl = 0.0;
    
    if (execution.signal == SignalType::BUY) {
        // For simplicity, assume we're buying at current price
        // P&L would be realized when we sell
        pnl = 0.0;  // Unrealized P&L
    } else if (execution.signal == SignalType::SELL) {
        // Assume we're selling position bought at last price
        pnl = (execution.executed_price - last_price) * execution.executed_quantity;
    }
    
    last_price = execution.executed_price;
    return pnl;
}

void ExecutionEngine::updateDrawdown(double current_pnl) {
    if (current_capital_ > peak_capital_) {
        peak_capital_ = current_capital_;
    }
    
    double current_drawdown = (peak_capital_ - current_capital_) / peak_capital_;
    
    if (current_drawdown > performance_metrics_.max_drawdown) {
        performance_metrics_.max_drawdown = current_drawdown;
    }
}

double ExecutionEngine::calculateSharpeRatio() const {
    if (daily_returns_.size() < 2) {
        return 0.0;
    }
    
    // Calculate mean return
    double mean_return = 0.0;
    for (double ret : daily_returns_) {
        mean_return += ret;
    }
    mean_return /= daily_returns_.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double ret : daily_returns_) {
        variance += (ret - mean_return) * (ret - mean_return);
    }
    variance /= (daily_returns_.size() - 1);
    double std_dev = std::sqrt(variance);
    
    // Calculate Sharpe ratio (assuming risk-free rate = 0)
    if (std_dev > 0.0) {
        return (mean_return * std::sqrt(252)) / (std_dev * std::sqrt(252));  // Annualized
    }
    
    return 0.0;
}

std::string ExecutionEngine::generateOrderId() {
    return "ORDER_" + std::to_string(++order_counter_) + "_" + 
           std::to_string(TimeUtils::getCurrentTimestampUs());
}

double ExecutionEngine::getMarketPrice(const std::string& symbol) const {
    // Placeholder implementation for simulation
    // In a real system, this would fetch current market price
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> price_dist(100.0, 200.0);
    
    return price_dist(gen);
}

} // namespace trading