#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "common/TimeUtils_simple.h"
#include "common/DCIndicator_simple.h"

using namespace std;

int main() {
    cout << "=== Low-Latency Trading System - Core Function Test ===" << endl;
    
    // Test time utilities
    cout << "\n1. Testing Time Utilities..." << endl;
    TimeUtils timeUtils;
    
    auto start = timeUtils.getCurrentTimeNanos();
    // Simulate some work
    for(volatile int i = 0; i < 1000; ++i) {}
    auto end = timeUtils.getCurrentTimeNanos();
    
    cout << "Time measurement: " << (end - start) << " nanoseconds" << endl;
    
    // Test DC indicator
    cout << "\n2. Testing DC Indicator Calculation..." << endl;
    DCIndicator dcIndicator(0.004); // 0.4% threshold
    
    // Simulate price data
    vector<double> prices = {100.0, 100.5, 101.0, 100.8, 101.2, 101.8, 101.5, 102.0};
    
    cout << "Price sequence: ";
    for(double price : prices) {
        cout << price << " ";
    }
    cout << endl;
    
    // Process prices one by one and detect DC events
    for(size_t i = 0; i < prices.size(); ++i) {
        uint64_t timestamp = timeUtils.getCurrentTimeNanos();
        dcIndicator.updatePrice(prices[i], timestamp);
        
        if(dcIndicator.isDCEvent()) {
            cout << "DC Event Detected! Price: " << prices[i] 
                 << ", TMV_EXT: " << dcIndicator.getTMVExt() 
                 << ", Time Adjusted Return: " << dcIndicator.getTimeAdjustedReturn() << endl;
        }
    }
    
    // Test latency tracking
    cout << "\n3. Testing Latency Tracking..." << endl;
    timeUtils.startLatencyMeasurement("test_operation");
    
    // Simulate operation
    this_thread::sleep_for(chrono::microseconds(10));
    
    auto latency = timeUtils.endLatencyMeasurement("test_operation");
    cout << "Operation latency: " << latency << " nanoseconds" << endl;
    
    cout << "\n=== Test Complete ===" << endl;
    return 0;
} 