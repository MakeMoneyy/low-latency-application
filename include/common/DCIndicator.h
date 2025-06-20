#pragma once

#include <vector>
#include <memory>
#include <cstdint>

namespace trading {

/**
 * @brief Structure to hold market data point
 */
struct MarketDataPoint {
    std::int64_t timestamp;  // Timestamp in nanoseconds
    double price;            // Price value
    double volume;           // Volume (optional)
    
    MarketDataPoint(std::int64_t ts, double p, double v = 0.0) 
        : timestamp(ts), price(p), volume(v) {}
};

/**
 * @brief DC event types
 */
enum class DCEventType {
    NONE,
    UPTURN,     // Upward DC event
    DOWNTURN    // Downward DC event
};

/**
 * @brief Structure to hold DC event information
 */
struct DCEvent {
    DCEventType type;
    std::int64_t timestamp;
    double price;
    double tmv_ext;          // Total Move Extent (TMV_EXT)
    std::int64_t duration;   // Time duration T(n)
    double time_adjusted_return;  // R(n)
    
    DCEvent() : type(DCEventType::NONE), timestamp(0), price(0.0), 
                tmv_ext(0.0), duration(0), time_adjusted_return(0.0) {}
};

/**
 * @brief Directional Change (DC) indicator calculator
 */
class DCIndicator {
public:
    explicit DCIndicator(double theta = 0.004);  // Default 0.4% threshold
    
    /**
     * @brief Process new market data point and detect DC events
     * @param data_point New market data point
     * @return DCEvent if detected, otherwise NONE type
     */
    DCEvent processDataPoint(const MarketDataPoint& data_point);
    
    /**
     * @brief Set DC threshold
     * @param theta New threshold value (e.g., 0.004 for 0.4%)
     */
    void setTheta(double theta) { theta_ = theta; }
    
    /**
     * @brief Get current theta value
     * @return Current threshold
     */
    double getTheta() const { return theta_; }
    
    /**
     * @brief Reset the indicator state
     */
    void reset();
    
    /**
     * @brief Get current trend direction
     * @return 1 for uptrend, -1 for downtrend, 0 for unknown
     */
    int getCurrentTrend() const { return current_trend_; }
    
    /**
     * @brief Get last DC event
     * @return Last detected DC event
     */
    const DCEvent& getLastDCEvent() const { return last_dc_event_; }

private:
    double theta_;                    // DC threshold
    int current_trend_;               // Current trend: 1=up, -1=down, 0=unknown
    
    // State tracking
    double extreme_price_;            // Current extreme price
    std::int64_t extreme_timestamp_;  // Timestamp of extreme price
    double last_dc_price_;            // Price at last DC event
    std::int64_t last_dc_timestamp_;  // Timestamp of last DC event
    
    DCEvent last_dc_event_;
    
    // Internal calculation methods
    double calculateTMV(double current_price, double previous_extreme) const;
    std::int64_t calculateDuration(std::int64_t current_time, std::int64_t previous_time) const;
    double calculateTimeAdjustedReturn(double tmv, std::int64_t duration) const;
    
    bool isUpwardDC(double current_price, double extreme_price) const;
    bool isDownwardDC(double current_price, double extreme_price) const;
};

} // namespace trading 