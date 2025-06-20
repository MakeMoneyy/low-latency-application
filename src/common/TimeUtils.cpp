#include "common/TimeUtils.h"
#include "common/Logger.h"
#include <thread>
#include <iomanip>
#include <sstream>

namespace trading {

TimeUtils::TimePoint TimeUtils::getCurrentTime() {
    return std::chrono::high_resolution_clock::now();
}

std::int64_t TimeUtils::getCurrentTimestampNs() {
    auto now = getCurrentTime();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
}

std::int64_t TimeUtils::getCurrentTimestampUs() {
    auto now = getCurrentTime();
    return std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
}

std::int64_t TimeUtils::getDurationNs(const TimePoint& start, const TimePoint& end) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

std::int64_t TimeUtils::getDurationUs(const TimePoint& start, const TimePoint& end) {
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

std::string TimeUtils::timestampToString(std::int64_t timestamp_ns) {
    auto time_point = std::chrono::time_point<std::chrono::high_resolution_clock>(
        std::chrono::nanoseconds(timestamp_ns));
    
    auto time_t = std::chrono::high_resolution_clock::to_time_t(time_point);
    auto tm = *std::localtime(&time_t);
    
    auto ns_part = timestamp_ns % 1000000000;
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(9) << ns_part;
    
    return oss.str();
}

void TimeUtils::busySleepNs(std::int64_t nanoseconds) {
    auto start = getCurrentTime();
    auto target = start + std::chrono::nanoseconds(nanoseconds);
    
    // Busy wait for high precision
    while (getCurrentTime() < target) {
        // Busy loop - use CPU cycles for precision
        std::this_thread::yield();
    }
}

// LatencyMeasurer implementation
LatencyMeasurer::LatencyMeasurer(const std::string& operation_name)
    : operation_name_(operation_name)
    , start_time_(TimeUtils::getCurrentTime())
{
}

LatencyMeasurer::~LatencyMeasurer() {
    auto end_time = TimeUtils::getCurrentTime();
    auto latency_ns = TimeUtils::getDurationNs(start_time_, end_time);
    auto latency_us = latency_ns / 1000;
    
    LOG_PERFORMANCE("Operation '{}' completed in {} ns ({} us)", 
                   operation_name_, latency_ns, latency_us);
}

std::int64_t LatencyMeasurer::getElapsedNs() const {
    auto current_time = TimeUtils::getCurrentTime();
    return TimeUtils::getDurationNs(start_time_, current_time);
}

std::int64_t LatencyMeasurer::getElapsedUs() const {
    auto current_time = TimeUtils::getCurrentTime();
    return TimeUtils::getDurationUs(start_time_, current_time);
}

} // namespace trading 