#include "common/DCIndicator.h"
#include <cmath>
#include <limits>

namespace trading {

DCIndicator::DCIndicator(double theta) 
    : theta_(theta)
    , current_trend_(0)
    , extreme_price_(std::numeric_limits<double>::quiet_NaN())
    , extreme_timestamp_(0)
    , last_dc_price_(std::numeric_limits<double>::quiet_NaN())
    , last_dc_timestamp_(0)
{
}

DCEvent DCIndicator::processDataPoint(const MarketDataPoint& data_point) {
    DCEvent event;
    
    // Initialize on first data point
    if (std::isnan(extreme_price_)) {
        extreme_price_ = data_point.price;
        extreme_timestamp_ = data_point.timestamp;
        last_dc_price_ = data_point.price;
        last_dc_timestamp_ = data_point.timestamp;
        return event;  // Return NONE event
    }
    
    bool dc_detected = false;
    
    if (current_trend_ >= 0) {  // In uptrend or unknown
        // Update extreme if new high
        if (data_point.price > extreme_price_) {
            extreme_price_ = data_point.price;
            extreme_timestamp_ = data_point.timestamp;
        }
        
        // Check for downward DC
        if (isDownwardDC(data_point.price, extreme_price_)) {
            event.type = DCEventType::DOWNTURN;
            current_trend_ = -1;
            dc_detected = true;
        }
    }
    else {  // In downtrend
        // Update extreme if new low
        if (data_point.price < extreme_price_) {
            extreme_price_ = data_point.price;
            extreme_timestamp_ = data_point.timestamp;
        }
        
        // Check for upward DC
        if (isUpwardDC(data_point.price, extreme_price_)) {
            event.type = DCEventType::UPTURN;
            current_trend_ = 1;
            dc_detected = true;
        }
    }
    
    if (dc_detected) {
        // Calculate DC indicators
        event.timestamp = data_point.timestamp;
        event.price = data_point.price;
        event.tmv_ext = calculateTMV(data_point.price, extreme_price_);
        event.duration = calculateDuration(extreme_timestamp_, last_dc_timestamp_);
        event.time_adjusted_return = calculateTimeAdjustedReturn(event.tmv_ext, event.duration);
        
        // Update state for next DC calculation
        last_dc_price_ = extreme_price_;
        last_dc_timestamp_ = extreme_timestamp_;
        extreme_price_ = data_point.price;
        extreme_timestamp_ = data_point.timestamp;
        
        last_dc_event_ = event;
    }
    
    return event;
}

void DCIndicator::reset() {
    current_trend_ = 0;
    extreme_price_ = std::numeric_limits<double>::quiet_NaN();
    extreme_timestamp_ = 0;
    last_dc_price_ = std::numeric_limits<double>::quiet_NaN();
    last_dc_timestamp_ = 0;
    last_dc_event_ = DCEvent();
}

double DCIndicator::calculateTMV(double current_price, double previous_extreme) const {
    if (std::isnan(previous_extreme) || previous_extreme == 0.0) {
        return 0.0;
    }
    
    // TMV_EXT(n) = (P_EXT(n) - P_EXT(n-1)) / (P_EXT(n-1) * theta)
    return std::abs(current_price - previous_extreme) / (previous_extreme * theta_);
}

std::int64_t DCIndicator::calculateDuration(std::int64_t current_time, std::int64_t previous_time) const {
    // T(n) = t_EXT(n) - t_EXT(n-1)
    return current_time - previous_time;
}

double DCIndicator::calculateTimeAdjustedReturn(double tmv, std::int64_t duration) const {
    if (duration <= 0) {
        return 0.0;
    }
    
    // R(n) = TMV_EXT(n) / T(n) * theta
    // Convert duration from nanoseconds to seconds for meaningful calculation
    double duration_seconds = static_cast<double>(duration) / 1e9;
    return (tmv / duration_seconds) * theta_;
}

bool DCIndicator::isUpwardDC(double current_price, double extreme_price) const {
    if (std::isnan(extreme_price)) {
        return false;
    }
    
    // Upward DC: price increases by theta% from the extreme low
    return (current_price - extreme_price) / extreme_price >= theta_;
}

bool DCIndicator::isDownwardDC(double current_price, double extreme_price) const {
    if (std::isnan(extreme_price)) {
        return false;
    }
    
    // Downward DC: price decreases by theta% from the extreme high
    return (extreme_price - current_price) / extreme_price >= theta_;
}

} // namespace trading 