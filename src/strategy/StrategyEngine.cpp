#include "strategy/StrategyEngine.h"
#include <iostream>
#include <cstring>

namespace trading {

StrategyEngine::StrategyEngine()
    : running_(false)
    , hmm_enabled_(false)
    , leverage_factor_(1.0)
    , current_market_state_(MarketState::UNKNOWN)
    , statistics_{0, 0, 0, 0, 0, 0, MarketState::UNKNOWN}
{
}

StrategyEngine::~StrategyEngine() {
    stop();
}

bool StrategyEngine::initialize(std::shared_ptr<aeron::Aeron> aeron,
                              const std::string& input_channel,
                              std::int32_t input_stream_id,
                              const std::string& output_channel,
                              std::int32_t output_stream_id) {
    try {
        aeron_ = aeron;
        
        // Create input subscription for DC signals
        LOG_STRATEGY("Creating subscription for DC signals: {} stream {}", 
                    input_channel, input_stream_id);
        
        input_subscription_ = aeron_->addSubscription(input_channel, input_stream_id);
        
        // Wait for subscription to connect
        while (!input_subscription_->isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Create output publication for trading orders
        LOG_STRATEGY("Creating publication for trading orders: {} stream {}", 
                    output_channel, output_stream_id);
        
        output_publication_ = aeron_->addPublication(output_channel, output_stream_id);
        
        // Wait for publication to connect
        while (!output_publication_->isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        LOG_STRATEGY("Strategy engine initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR_STRATEGY("Failed to initialize strategy engine: {}", e.what());
        return false;
    }
}

void StrategyEngine::start() {
    if (running_.load()) {
        LOG_STRATEGY("Strategy engine is already running");
        return;
    }
    
    running_.store(true);
    processing_thread_ = std::make_unique<std::thread>(&StrategyEngine::processLoop, this);
    
    LOG_STRATEGY("Strategy engine started");
}

void StrategyEngine::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (processing_thread_ && processing_thread_->joinable()) {
        processing_thread_->join();
    }
    
    LOG_STRATEGY("Strategy engine stopped");
}

StrategyEngine::Statistics StrategyEngine::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

void StrategyEngine::processLoop() {
    LOG_STRATEGY("Strategy processing loop started");
    
    aeron::concurrent::SleepingIdleStrategy idleStrategy(std::chrono::milliseconds(1));
    
    while (running_.load()) {
        const int fragmentsRead = input_subscription_->poll(
            [this](const aeron::concurrent::AtomicBuffer& buffer, 
                   util::index_t offset, 
                   util::index_t length, 
                   const aeron::Header& header) {
                processDCSignal(buffer, offset, length);
            }, 
            10);  // Poll up to 10 fragments at a time
        
        idleStrategy.idle(fragmentsRead);
    }
    
    LOG_STRATEGY("Strategy processing loop ended");
}

void StrategyEngine::processDCSignal(const aeron::concurrent::AtomicBuffer& buffer,
                                   util::index_t offset,
                                   util::index_t length) {
    auto start_time = TimeUtils::getCurrentTime();
    
    if (length < sizeof(DCSignalMessage)) {
        LOG_ERROR_STRATEGY("Invalid DC signal message size: {}", length);
        return;
    }
    
    // Extract DC signal message
    DCSignalMessage dc_signal;
    std::memcpy(&dc_signal, buffer.buffer() + offset, sizeof(DCSignalMessage));
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        statistics_.signals_processed++;
    }
    
    // Update market state if HMM is enabled
    if (hmm_enabled_) {
        updateMarketState(dc_signal);
    }
    
    // Generate trading signal based on DC event
    SignalType trading_signal = generateTradingSignal(dc_signal);
    
    if (trading_signal != SignalType::NONE) {
        // Create trading order
        TradingOrder order;
        order.timestamp = TimeUtils::getCurrentTimestampNs();
        order.signal = trading_signal;
        order.price = dc_signal.price;
        order.quantity = calculateOrderQuantity(trading_signal, dc_signal.price);
        order.strategy_latency_ns = TimeUtils::getDurationNs(
            TimeUtils::TimePoint(std::chrono::nanoseconds(dc_signal.timestamp)), 
            TimeUtils::getCurrentTime());
        
        // Copy symbol
        std::strncpy(order.symbol, dc_signal.symbol, sizeof(order.symbol) - 1);
        order.symbol[sizeof(order.symbol) - 1] = '\0';
        
        // Publish trading order
        if (publishTradingOrder(order)) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            statistics_.orders_generated++;
            
            if (trading_signal == SignalType::BUY) {
                statistics_.buy_signals++;
            } else if (trading_signal == SignalType::SELL) {
                statistics_.sell_signals++;
            }
        }
        
        LOG_DEBUG_STRATEGY("Trading order generated: signal={}, price={}, quantity={}", 
                          static_cast<int>(trading_signal), order.price, order.quantity);
    }
    
    // Update latency statistics
    auto latency_ns = TimeUtils::getDurationNs(start_time, TimeUtils::getCurrentTime());
    updateLatencyStats(latency_ns);
}

SignalType StrategyEngine::generateTradingSignal(const DCSignalMessage& dc_signal) {
    // Basic DC strategy: 
    // - Upward DC event -> Buy signal
    // - Downward DC event -> Sell signal
    
    switch (dc_signal.event_type) {
        case DCEventType::UPTURN:
            // Consider time-adjusted return and market state
            if (dc_signal.time_adjusted_return > 0.0) {
                // Stronger signal if HMM indicates low volatility
                if (hmm_enabled_ && current_market_state_ == MarketState::LOW_VOLATILITY) {
                    LOG_DEBUG_STRATEGY("Strong BUY signal in low volatility state");
                }
                return SignalType::BUY;
            }
            break;
            
        case DCEventType::DOWNTURN:
            // Consider time-adjusted return and market state
            if (dc_signal.time_adjusted_return < 0.0) {
                // Stronger signal if HMM indicates low volatility
                if (hmm_enabled_ && current_market_state_ == MarketState::LOW_VOLATILITY) {
                    LOG_DEBUG_STRATEGY("Strong SELL signal in low volatility state");
                }
                return SignalType::SELL;
            }
            break;
            
        default:
            break;
    }
    
    return SignalType::NONE;
}

double StrategyEngine::calculateOrderQuantity(SignalType signal, double price) {
    double base_quantity = 100.0;  // Base quantity
    
    // Apply leverage factor
    double quantity = base_quantity * leverage_factor_;
    
    // Apply volatility-adjusted leverage if HMM is enabled
    if (hmm_enabled_) {
        double volatility_adjustment = getVolatilityAdjustedLeverage();
        quantity *= volatility_adjustment;
    }
    
    // Simple position sizing based on price (higher price = smaller quantity)
    if (price > 0.0) {
        quantity = std::min(quantity, 10000.0 / price);  // Max $10,000 per trade
    }
    
    return std::max(1.0, quantity);  // Minimum 1 unit
}

bool StrategyEngine::publishTradingOrder(const TradingOrder& order) {
    // Publish the order
    aeron::concurrent::AtomicBuffer buffer(reinterpret_cast<const std::uint8_t*>(&order), 
                                          sizeof(order));
    
    std::int64_t result = output_publication_->offer(buffer, 0, sizeof(order));
    
    if (result > 0) {
        LOG_DEBUG_STRATEGY("Trading order published successfully");
        return true;
    } else {
        // Handle back pressure or other issues
        if (result == aeron::NOT_CONNECTED) {
            LOG_ERROR_STRATEGY("Publication not connected");
        } else if (result == aeron::BACK_PRESSURED) {
            LOG_DEBUG_STRATEGY("Publication back pressured, retrying...");
            // Could implement retry logic here
        } else {
            LOG_ERROR_STRATEGY("Failed to publish trading order, result: {}", result);
        }
        return false;
    }
}

void StrategyEngine::updateMarketState(const DCSignalMessage& dc_signal) {
    // Simple HMM-like state detection based on TMV and duration
    // This is a simplified implementation - real HMM would use more sophisticated algorithms
    
    double volatility_indicator = std::abs(dc_signal.tmv_ext) / (dc_signal.duration / 1e9); // TMV per second
    
    // Threshold-based state classification
    const double low_volatility_threshold = 0.1;
    const double high_volatility_threshold = 0.5;
    
    MarketState new_state = current_market_state_;
    
    if (volatility_indicator < low_volatility_threshold) {
        new_state = MarketState::LOW_VOLATILITY;
    } else if (volatility_indicator > high_volatility_threshold) {
        new_state = MarketState::HIGH_VOLATILITY;
    }
    
    if (new_state != current_market_state_) {
        LOG_STRATEGY("Market state changed from {} to {}", 
                    static_cast<int>(current_market_state_), 
                    static_cast<int>(new_state));
        current_market_state_ = new_state;
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        statistics_.current_market_state = new_state;
    }
}

double StrategyEngine::getVolatilityAdjustedLeverage() const {
    switch (current_market_state_) {
        case MarketState::LOW_VOLATILITY:
            return 1.5;  // Increase leverage in low volatility
        case MarketState::HIGH_VOLATILITY:
            return 0.5;  // Reduce leverage in high volatility
        default:
            return 1.0;  // Default leverage
    }
}

void StrategyEngine::updateLatencyStats(std::int64_t latency_ns) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Update running average
    if (statistics_.signals_processed == 1) {
        statistics_.avg_strategy_latency_ns = latency_ns;
    } else {
        statistics_.avg_strategy_latency_ns = 
            (statistics_.avg_strategy_latency_ns * 0.9) + (latency_ns * 0.1);
    }
    
    // Update max latency
    if (latency_ns > statistics_.max_strategy_latency_ns) {
        statistics_.max_strategy_latency_ns = latency_ns;
    }
}

} // namespace trading 