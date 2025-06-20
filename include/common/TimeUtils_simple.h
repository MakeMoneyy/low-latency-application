#pragma once

#include <chrono>
#include <unordered_map>
#include <string>

class TimeUtils {
private:
    std::unordered_map<std::string, uint64_t> latencyStartTimes;

public:
    // 获取当前时间（纳秒）
    uint64_t getCurrentTimeNanos() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
    }

    // 开始延迟测量
    void startLatencyMeasurement(const std::string& operationName) {
        latencyStartTimes[operationName] = getCurrentTimeNanos();
    }

    // 结束延迟测量并返回延迟（纳秒）
    uint64_t endLatencyMeasurement(const std::string& operationName) {
        uint64_t endTime = getCurrentTimeNanos();
        auto it = latencyStartTimes.find(operationName);
        if (it != latencyStartTimes.end()) {
            uint64_t latency = endTime - it->second;
            latencyStartTimes.erase(it);
            return latency;
        }
        return 0;
    }
}; 