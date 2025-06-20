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
 * @brief Market data message structure
 */
struct MarketDataMessage {
    std::int64_t timestamp;
    double price;
    double volume;
    char symbol[16];
};

/**
 * @brief DC signal message structure for publishing
 */
struct DCSignalMessage {
    std::int64_t timestamp;
    DCEventType event_type;
    double price;
    double tmv_ext;
    std::int64_t duration;
    double time_adjusted_return;
    char symbol[16];
};

/**
 * @brief Market Data Processor - receives market data and detects DC events
 */
class MarketDataProcessor {
public:
    MarketDataProcessor();
    ~MarketDataProcessor();
    
    /**
     * @brief Initialize the processor with Aeron context
     * @param aeron Aeron context
     * @param input_channel Input channel for market data
     * @param input_stream_id Input stream ID
     * @param output_channel Output channel for DC signals
     * @param output_stream_id Output stream ID
     * @return true if successful
     */
    bool initialize(std::shared_ptr<aeron::Aeron> aeron,
                   const std::string& input_channel,
                   std::int32_t input_stream_id,
                   const std::string& output_channel,
                   std::int32_t output_stream_id);
    
    /**
     * @brief Start processing market data
     */
    void start();
    
    /**
     * @brief Stop processing
     */
    void stop();
    
    /**
     * @brief Check if processor is running
     * @return true if running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * @brief Set DC threshold
     * @param theta New threshold value
     */
    void setDCThreshold(double theta);
    
    /**
     * @brief Get processing statistics
     */
    struct Statistics {
        std::uint64_t messages_processed;
        std::uint64_t dc_events_detected;
        std::int64_t avg_processing_latency_ns;
        std::int64_t max_processing_latency_ns;
    };
    
    Statistics getStatistics() const;

private:
    std::shared_ptr<aeron::Aeron> aeron_;
    std::shared_ptr<aeron::Subscription> input_subscription_;
    std::shared_ptr<aeron::Publication> output_publication_;
    
    std::unique_ptr<DCIndicator> dc_indicator_;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> processing_thread_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics statistics_;
    
    // Processing methods
    void processLoop();
    void processMarketData(const aeron::concurrent::AtomicBuffer& buffer, 
                          util::index_t offset, 
                          util::index_t length);
    
    bool publishDCSignal(const DCEvent& dc_event, const std::string& symbol);
    
    // Latency tracking
    void updateLatencyStats(std::int64_t latency_ns);
};

} // namespace trading 