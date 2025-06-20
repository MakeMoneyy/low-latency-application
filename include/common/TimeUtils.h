#pragma once

#include <chrono>
#include <cstdint>

namespace trading {

/**
 * @brief High-precision time utilities for latency measurement
 */
class TimeUtils {
public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using Duration = std::chrono::nanoseconds;
    
    /**
     * @brief Get current high-resolution timestamp
     * @return TimePoint representing current time
     */
    static TimePoint getCurrentTime();
    
    /**
     * @brief Get current timestamp as nanoseconds since epoch
     * @return int64_t timestamp in nanoseconds
     */
    static std::int64_t getCurrentTimestampNs();
    
    /**
     * @brief Get current timestamp as microseconds since epoch
     * @return int64_t timestamp in microseconds
     */
    static std::int64_t getCurrentTimestampUs();
    
    /**
     * @brief Calculate duration between two time points in nanoseconds
     * @param start Start time point
     * @param end End time point
     * @return Duration in nanoseconds
     */
    static std::int64_t getDurationNs(const TimePoint& start, const TimePoint& end);
    
    /**
     * @brief Calculate duration between two time points in microseconds
     * @param start Start time point
     * @param end End time point
     * @return Duration in microseconds
     */
    static std::int64_t getDurationUs(const TimePoint& start, const TimePoint& end);
    
    /**
     * @brief Convert nanoseconds timestamp to string format
     * @param timestamp_ns Timestamp in nanoseconds
     * @return String representation of timestamp
     */
    static std::string timestampToString(std::int64_t timestamp_ns);
    
    /**
     * @brief Sleep for specified nanoseconds (busy wait for high precision)
     * @param nanoseconds Duration to sleep
     */
    static void busySleepNs(std::int64_t nanoseconds);
};

/**
 * @brief RAII class for automatic latency measurement
 */
class LatencyMeasurer {
public:
    explicit LatencyMeasurer(const std::string& operation_name);
    ~LatencyMeasurer();
    
    // Get elapsed time without destroying the measurer
    std::int64_t getElapsedNs() const;
    std::int64_t getElapsedUs() const;

private:
    std::string operation_name_;
    TimeUtils::TimePoint start_time_;
};

} // namespace trading 