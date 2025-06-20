#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "common/TimeUtils_simple.h"
#include "common/DCIndicator_simple.h"

using namespace std;

class TradingSimulator {
private:
    TimeUtils timeUtils;
    DCIndicator dcIndicator;
    double currentPrice;
    double portfolio;
    double cash;
    int position; // 1 = long, -1 = short, 0 = flat
    vector<double> returns;
    
public:
    TradingSimulator(double threshold = 0.005) : dcIndicator(threshold) {
        reset();
    }
    
    void reset() {
        currentPrice = 100.0;
        portfolio = 10000.0; // Starting capital
        cash = portfolio;
        position = 0;
        returns.clear();
    }
    
    void processPrice(double price) {
        double prevPortfolio = portfolio;
        currentPrice = price;
        
        uint64_t timestamp = timeUtils.getCurrentTimeNanos();
        dcIndicator.updatePrice(price, timestamp);
        
        // Update portfolio value
        if (position != 0) {
            portfolio = cash + position * price;
        }
        
        // Trading logic based on DC events
        if (dcIndicator.isDCEvent()) {
            if (dcIndicator.getIsUptrend() && position <= 0) {
                // Go long on upward DC event
                if (position == -1) {
                    cash += price; // Close short position
                }
                cash -= price; // Buy long
                position = 1;
                cout << "LONG at " << fixed << setprecision(2) << price << endl;
            } else if (!dcIndicator.getIsUptrend() && position >= 0) {
                // Go short on downward DC event
                if (position == 1) {
                    cash += price; // Close long position
                }
                cash += price; // Sell short
                position = -1;
                cout << "SHORT at " << fixed << setprecision(2) << price << endl;
            }
        }
        
        // Calculate return
        if (prevPortfolio > 0) {
            double ret = (portfolio - prevPortfolio) / prevPortfolio;
            returns.push_back(ret);
        }
    }
    
    void printStatistics() {
        cout << "\n=== Trading Statistics ===" << endl;
        cout << "Final Portfolio Value: $" << fixed << setprecision(2) << portfolio << endl;
        cout << "Total Return: " << ((portfolio - 10000.0) / 10000.0 * 100) << "%" << endl;
        
        if (!returns.empty()) {
            double totalReturn = 0;
            double sumSquaredReturns = 0;
            for (double ret : returns) {
                totalReturn += ret;
                sumSquaredReturns += ret * ret;
            }
            
            double avgReturn = totalReturn / returns.size();
            double variance = (sumSquaredReturns / returns.size()) - (avgReturn * avgReturn);
            double volatility = sqrt(variance);
            double sharpeRatio = (volatility > 0) ? avgReturn / volatility : 0;
            
            cout << "Average Return per Trade: " << (avgReturn * 100) << "%" << endl;
            cout << "Volatility: " << (volatility * 100) << "%" << endl;
            cout << "Sharpe Ratio: " << sharpeRatio << endl;
            cout << "Number of Trades: " << returns.size() << endl;
        }
    }
};

int main() {
    cout << "=== Advanced Low-Latency Trading System Test ===" << endl;
    
    // Initialize random seed
    srand(static_cast<unsigned>(time(nullptr)));
    
    TimeUtils timeUtils;
    
    // Test 1: Basic DC Event Detection with Different Thresholds
    cout << "\n=== Test 1: DC Detection with Multiple Thresholds ===" << endl;
    
    vector<double> thresholds = {0.005, 0.01, 0.02}; // 0.5%, 1%, 2%
    vector<double> testPrices = {100.0, 101.0, 102.0, 103.0, 101.5, 100.0, 102.5, 101.0, 103.5, 102.0};
    
    for (double threshold : thresholds) {
        cout << "\nThreshold: " << (threshold * 100) << "%" << endl;
        DCIndicator indicator(threshold);
        
        int eventCount = 0;
        for (size_t i = 0; i < testPrices.size(); ++i) {
            uint64_t timestamp = timeUtils.getCurrentTimeNanos();
            indicator.updatePrice(testPrices[i], timestamp);
            
            if (indicator.isDCEvent()) {
                eventCount++;
                cout << "  DC Event #" << eventCount << " at price " << testPrices[i] 
                     << " (Trend: " << (indicator.getIsUptrend() ? "UP" : "DOWN") << ")" << endl;
            }
        }
        cout << "Total events: " << eventCount << endl;
    }
    
    // Test 2: Trading Simulation
    cout << "\n=== Test 2: Trading Strategy Simulation ===" << endl;
    
    TradingSimulator simulator(0.008); // 0.8% threshold
    
    // Generate realistic price series with trends and reversals
    vector<double> prices;
    double price = 100.0;
    
    // Uptrend phase
    for (int i = 0; i < 50; ++i) {
        price += (rand() % 100 - 30) / 1000.0; // Random walk with upward bias
        prices.push_back(max(price, 95.0)); // Floor at 95
    }
    
    // Downtrend phase
    for (int i = 0; i < 50; ++i) {
        price -= (rand() % 100 - 20) / 1000.0; // Random walk with downward bias
        prices.push_back(max(price, 90.0)); // Floor at 90
    }
    
    // Sideways phase
    for (int i = 0; i < 30; ++i) {
        price += (rand() % 100 - 50) / 1000.0; // Pure random walk
        prices.push_back(max(price, 85.0)); // Floor at 85
    }
    
    cout << "Simulating trading on " << prices.size() << " price points..." << endl;
    cout << "Price range: " << *min_element(prices.begin(), prices.end()) 
         << " to " << *max_element(prices.begin(), prices.end()) << endl;
    
    for (double p : prices) {
        simulator.processPrice(p);
    }
    
    simulator.printStatistics();
    
    // Test 3: Extreme Performance Test
    cout << "\n=== Test 3: Extreme Performance Benchmark ===" << endl;
    
    DCIndicator perfIndicator(0.01);
    
    cout << "Testing with 1,000,000 price updates..." << endl;
    
    timeUtils.startLatencyMeasurement("extreme_performance");
    
    int totalEvents = 0;
    for (int i = 0; i < 1000000; ++i) {
        double randomPrice = 100.0 + (rand() % 2000 - 1000) / 100.0; // Price 90-110
        uint64_t timestamp = timeUtils.getCurrentTimeNanos();
        
        perfIndicator.updatePrice(randomPrice, timestamp);
        
        if (perfIndicator.isDCEvent()) {
            totalEvents++;
        }
    }
    
    uint64_t totalTime = timeUtils.endLatencyMeasurement("extreme_performance");
    
    cout << "Results:" << endl;
    cout << "  Total processing time: " << totalTime << " nanoseconds" << endl;
    cout << "  Average latency per update: " << (totalTime / 1000000) << " ns" << endl;
    cout << "  Throughput: " << (1000000000000ULL / totalTime) << " updates/second" << endl;
    cout << "  DC events detected: " << totalEvents << endl;
    cout << "  Event rate: " << (totalEvents * 100.0 / 1000000) << "%" << endl;
    
    // Performance targets
    uint64_t avgLatency = totalTime / 1000000;
    cout << "\nPerformance Analysis:" << endl;
    cout << "  Target latency: < 100,000 ns (100 μs)" << endl;
    cout << "  Actual latency: " << avgLatency << " ns" << endl;
    cout << "  Performance ratio: " << (100000.0 / avgLatency) << "x better than target" << endl;
    cout << "  Status: " << (avgLatency < 100000 ? "EXCELLENT ✓" : "NEEDS IMPROVEMENT ✗") << endl;
    
    cout << "\n=== All Tests Complete ===" << endl;
    
    return 0;
} 