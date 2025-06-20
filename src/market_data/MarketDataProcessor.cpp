#include "market_data/MarketDataProcessor.h"
#include <iostream>
#include <cstring>

namespace trading {

MarketDataProcessor::MarketDataProcessor() 
    : running_(false)
    , statistics_{0, 0, 0, 0}
{
    dc_indicator_ = std::make_unique<DCIndicator>(0.004); // Default 0.4% threshold
}

MarketDataProcessor::~MarketDataProcessor() {
    stop();
}

bool MarketDataProcessor::initialize(std::shared_ptr<aeron::Aeron> aeron,
                                   const std::string& input_channel,
                                   std::int32_t input_stream_id,
                                   const std::string& output_channel,
                                   std::int32_t output_stream_id) {
    try {
        aeron_ = aeron;
        
        // Create input subscription for market data
        LOG_MARKET_DATA("Creating subscription for market data: {} stream {}", 
                       input_channel, input_stream_id);
        
        input_subscription_ = aeron_->addSubscription(input_channel, input_stream_id);
        
        // Wait for subscription to connect
        while (!input_subscription_->isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Create output publication for DC signals
        LOG_MARKET_DATA("Creating publication for DC signals: {} stream {}", 
                       output_channel, output_stream_id);
        
        output_publication_ = aeron_->addPublication(output_channel, output_stream_id);
        
        // Wait for publication to connect
        while (!output_publication_->isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        LOG_MARKET_DATA("Market data processor initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR_MARKET_DATA("Failed to initialize market data processor: {}", e.what());
        return false;
    }
}

void MarketDataProcessor::start() {
    if (running_.load()) {
        LOG_MARKET_DATA("Market data processor is already running");
        return;
    }
    
    running_.store(true);
    processing_thread_ = std::make_unique<std::thread>(&MarketDataProcessor::processLoop, this);
    
    LOG_MARKET_DATA("Market data processor started");
}

void MarketDataProcessor::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (processing_thread_ && processing_thread_->joinable()) {
        processing_thread_->join();
    }
    
    LOG_MARKET_DATA("Market data processor stopped");
}

void MarketDataProcessor::setDCThreshold(double theta) {
    if (dc_indicator_) {
        dc_indicator_->setTheta(theta);
        LOG_MARKET_DATA("DC threshold set to {}", theta);
    }
}

MarketDataProcessor::Statistics MarketDataProcessor::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

void MarketDataProcessor::processLoop() {
    LOG_MARKET_DATA("Market data processing loop started");
    
    aeron::concurrent::SleepingIdleStrategy idleStrategy(std::chrono::milliseconds(1));
    
    while (running_.load()) {
        const int fragmentsRead = input_subscription_->poll(
            [this](const aeron::concurrent::AtomicBuffer& buffer, 
                   util::index_t offset, 
                   util::index_t length, 
                   const aeron::Header& header) {
                processMarketData(buffer, offset, length);
            }, 
            10);  // Poll up to 10 fragments at a time
        
        idleStrategy.idle(fragmentsRead);
    }
    
    LOG_MARKET_DATA("Market data processing loop ended");
}

void MarketDataProcessor::processMarketData(const aeron::concurrent::AtomicBuffer& buffer, 
                                          util::index_t offset, 
                                          util::index_t length) {
    auto start_time = TimeUtils::getCurrentTime();
    
    if (length < sizeof(MarketDataMessage)) {
        LOG_ERROR_MARKET_DATA("Invalid market data message size: {}", length);
        return;
    }
    
    // Extract market data message
    MarketDataMessage market_data;
    std::memcpy(&market_data, buffer.buffer() + offset, sizeof(MarketDataMessage));
    
    // Create market data point for DC processing
    MarketDataPoint data_point(market_data.timestamp, market_data.price, market_data.volume);
    
    // Process through DC indicator
    DCEvent dc_event = dc_indicator_->processDataPoint(data_point);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        statistics_.messages_processed++;
        
        auto latency_ns = TimeUtils::getDurationNs(start_time, TimeUtils::getCurrentTime());
        updateLatencyStats(latency_ns);
    }
    
    // If DC event detected, publish signal
    if (dc_event.type != DCEventType::NONE) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            statistics_.dc_events_detected++;
        }
        
        std::string symbol(market_data.symbol);
        publishDCSignal(dc_event, symbol);
        
        LOG_DEBUG_MARKET_DATA("DC event detected: type={}, price={}, tmv={}", 
                             static_cast<int>(dc_event.type), 
                             dc_event.price, 
                             dc_event.tmv_ext);
    }
}

bool MarketDataProcessor::publishDCSignal(const DCEvent& dc_event, const std::string& symbol) {
    DCSignalMessage signal_msg;
    signal_msg.timestamp = dc_event.timestamp;
    signal_msg.event_type = dc_event.type;
    signal_msg.price = dc_event.price;
    signal_msg.tmv_ext = dc_event.tmv_ext;
    signal_msg.duration = dc_event.duration;
    signal_msg.time_adjusted_return = dc_event.time_adjusted_return;
    
    // Copy symbol (ensure null termination)
    std::strncpy(signal_msg.symbol, symbol.c_str(), sizeof(signal_msg.symbol) - 1);
    signal_msg.symbol[sizeof(signal_msg.symbol) - 1] = '\0';
    
    // Publish the signal
    aeron::concurrent::AtomicBuffer buffer(reinterpret_cast<std::uint8_t*>(&signal_msg), 
                                          sizeof(signal_msg));
    
    std::int64_t result = output_publication_->offer(buffer, 0, sizeof(signal_msg));
    
    if (result > 0) {
        LOG_DEBUG_MARKET_DATA("DC signal published successfully");
        return true;
    } else {
        // Handle back pressure or other issues
        if (result == aeron::NOT_CONNECTED) {
            LOG_ERROR_MARKET_DATA("Publication not connected");
        } else if (result == aeron::BACK_PRESSURED) {
            LOG_DEBUG_MARKET_DATA("Publication back pressured, retrying...");
            // Could implement retry logic here
        } else {
            LOG_ERROR_MARKET_DATA("Failed to publish DC signal, result: {}", result);
        }
        return false;
    }
}

void MarketDataProcessor::updateLatencyStats(std::int64_t latency_ns) {
    // Update running average (simple moving average for now)
    if (statistics_.messages_processed == 1) {
        statistics_.avg_processing_latency_ns = latency_ns;
    } else {
        statistics_.avg_processing_latency_ns = 
            (statistics_.avg_processing_latency_ns * 0.9) + (latency_ns * 0.1);
    }
    
    // Update max latency
    if (latency_ns > statistics_.max_processing_latency_ns) {
        statistics_.max_processing_latency_ns = latency_ns;
    }
}

} // namespace trading 