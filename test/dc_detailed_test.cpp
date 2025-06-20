#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include "common/TimeUtils_simple.h"
#include "common/DCIndicator_simple.h"

using namespace std;

int main() {
    cout << "=== DC Event Detection Comprehensive Test ===" << endl;
    
    TimeUtils timeUtils;
    DCIndicator dcIndicator(0.01); // 1% threshold for easier testing
    
    // Test case 1: Rising trend with DC event
    cout << "\nTest Case 1: Rising trend with DC event" << endl;
    cout << "Expected: Price rises to 103.00, then drops to 101.50 (>1% decline)" << endl;
    vector<double> prices1 = {100.0, 101.0, 102.0, 103.0, 101.5}; // Last price drops >1%
    
    for(size_t i = 0; i < prices1.size(); ++i) {
        uint64_t timestamp = timeUtils.getCurrentTimeNanos();
        dcIndicator.updatePrice(prices1[i], timestamp);
        
        cout << "Price: " << fixed << setprecision(2) << prices1[i];
        if(dcIndicator.isDCEvent()) {
            cout << " -> DC EVENT! Trend: " << (dcIndicator.getIsUptrend() ? "UPWARD" : "DOWNWARD")
                 << ", TMV_EXT: " << setprecision(4) << dcIndicator.getTMVExt()
                 << ", Time Adj Return: " << dcIndicator.getTimeAdjustedReturn();
        }
        cout << endl;
    }
    
    // Test case 2: Falling trend with DC event
    cout << "\nTest Case 2: Falling trend with DC event" << endl;
    cout << "Expected: Price falls to 97.00, then rises to 98.50 (>1% rise)" << endl;
    dcIndicator.reset();
    vector<double> prices2 = {100.0, 99.0, 98.0, 97.0, 98.5}; // Last price rises >1%
    
    for(size_t i = 0; i < prices2.size(); ++i) {
        uint64_t timestamp = timeUtils.getCurrentTimeNanos();
        dcIndicator.updatePrice(prices2[i], timestamp);
        
        cout << "Price: " << fixed << setprecision(2) << prices2[i];
        if(dcIndicator.isDCEvent()) {
            cout << " -> DC EVENT! Trend: " << (dcIndicator.getIsUptrend() ? "UPWARD" : "DOWNWARD")
                 << ", TMV_EXT: " << setprecision(4) << dcIndicator.getTMVExt()
                 << ", Time Adj Return: " << dcIndicator.getTimeAdjustedReturn();
        }
        cout << endl;
    }
    
    // Test case 3: Multiple DC events
    cout << "\nTest Case 3: Multiple DC events in sequence" << endl;
    dcIndicator.reset();
    vector<double> prices3 = {100.0, 102.0, 100.8, 102.5, 101.2, 103.0, 101.9}; 
    
    for(size_t i = 0; i < prices3.size(); ++i) {
        uint64_t timestamp = timeUtils.getCurrentTimeNanos();
        dcIndicator.updatePrice(prices3[i], timestamp);
        
        cout << "Price: " << fixed << setprecision(2) << prices3[i];
        if(dcIndicator.isDCEvent()) {
            cout << " -> DC EVENT! Trend: " << (dcIndicator.getIsUptrend() ? "UPWARD" : "DOWNWARD")
                 << ", Extreme: " << dcIndicator.getExtremePrice()
                 << ", TMV_EXT: " << setprecision(4) << dcIndicator.getTMVExt();
        }
        cout << endl;
    }
    
    // Performance test
    cout << "\nPerformance Test:" << endl;
    cout << "Processing 100,000 price updates..." << endl;
    dcIndicator.reset();
    
    timeUtils.startLatencyMeasurement("dc_processing");
    
    int dcEventCount = 0;
    // Process 100000 price updates
    for(int i = 0; i < 100000; ++i) {
        double price = 100.0 + (rand() % 1000) / 100.0; // Random price 100-110
        uint64_t timestamp = timeUtils.getCurrentTimeNanos();
        dcIndicator.updatePrice(price, timestamp);
        if(dcIndicator.isDCEvent()) {
            dcEventCount++;
        }
    }
    
    auto totalLatency = timeUtils.endLatencyMeasurement("dc_processing");
    cout << "Processed 100,000 price updates in " << totalLatency << " nanoseconds" << endl;
    cout << "Average latency per update: " << (totalLatency / 100000) << " nanoseconds" << endl;
    cout << "Total DC events detected: " << dcEventCount << endl;
    cout << "DC event rate: " << (dcEventCount * 100.0 / 100000) << "%" << endl;
    
    // Latency benchmark
    cout << "\nLatency Benchmark:" << endl;
    dcIndicator.reset();
    
    vector<uint64_t> latencies;
    for(int i = 0; i < 1000; ++i) {
        double price = 100.0 + (i % 100) / 10.0;
        
        auto start = timeUtils.getCurrentTimeNanos();
        dcIndicator.updatePrice(price, timeUtils.getCurrentTimeNanos());
        auto end = timeUtils.getCurrentTimeNanos();
        
        latencies.push_back(end - start);
    }
    
    // Calculate statistics
    uint64_t minLatency = *min_element(latencies.begin(), latencies.end());
    uint64_t maxLatency = *max_element(latencies.begin(), latencies.end());
    uint64_t avgLatency = 0;
    for(uint64_t lat : latencies) avgLatency += lat;
    avgLatency /= latencies.size();
    
    cout << "Latency Statistics (1000 samples):" << endl;
    cout << "  Minimum: " << minLatency << " ns" << endl;
    cout << "  Maximum: " << maxLatency << " ns" << endl;
    cout << "  Average: " << avgLatency << " ns" << endl;
    cout << "  Target: < 100,000 ns (100 μs)" << endl;
    cout << "  Status: " << (avgLatency < 100000 ? "PASS ✓" : "FAIL ✗") << endl;
    
    cout << "\n=== Test Complete ===" << endl;
    return 0;
} 