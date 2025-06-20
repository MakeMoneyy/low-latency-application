#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <random>
#include <functional>
#include <algorithm>
#include <cmath>

// Include our simplified headers
#include "common/TimeUtils_simple.h"
#include "common/DCIndicator_simple.h"

using namespace std;

// Mock trading system components
class MockMarketDataFeed {
private:
    vector<double> prices;
    size_t currentIndex;
    bool running;
    thread feedThread;
    
public:
    function<void(double, uint64_t)> onPriceUpdate;
    
    MockMarketDataFeed() : currentIndex(0), running(false) {
        generateRealisticPriceData();
    }
    
    void generateRealisticPriceData() {
        prices.clear();
        random_device rd;
        mt19937 gen(rd());
        normal_distribution<> price_change(-0.001, 0.005); // Small price changes
        
        double basePrice = 100.0;
        prices.push_back(basePrice);
        
        // Generate 10000 realistic price points
        for (int i = 1; i < 10000; ++i) {
            double change = price_change(gen);
            double newPrice = prices.back() * (1.0 + change);
            
            // Add some trending behavior
            if (i % 1000 < 300) {
                newPrice += 0.002; // Slight upward trend
            } else if (i % 1000 < 600) {
                newPrice -= 0.001; // Slight downward trend
            }
            
            prices.push_back(max(newPrice, 50.0)); // Floor at 50
        }
        
        cout << "Generated " << prices.size() << " price points" << endl;
        cout << "Price range: " << *min_element(prices.begin(), prices.end()) 
             << " to " << *max_element(prices.begin(), prices.end()) << endl;
    }
    
    void start() {
        if (running) return;
        
        running = true;
        currentIndex = 0;
        feedThread = thread(&MockMarketDataFeed::feedLoop, this);
    }
    
    void stop() {
        running = false;
        if (feedThread.joinable()) {
            feedThread.join();
        }
    }
    
private:
    void feedLoop() {
        TimeUtils timeUtils;
        
        while (running && currentIndex < prices.size()) {
            if (onPriceUpdate) {
                uint64_t timestamp = timeUtils.getCurrentTimeNanos();
                onPriceUpdate(prices[currentIndex], timestamp);
            }
            
            currentIndex++;
            this_thread::sleep_for(chrono::microseconds(10)); // 100k updates/sec
        }
    }
};

class TradingStrategy {
private:
    DCIndicator dcIndicator;
    TimeUtils timeUtils;
    
    // Portfolio state
    double cash;
    double position; // Number of shares
    double currentPrice;
    vector<double> portfolioValues;
    vector<string> tradeLog;
    
    // Statistics
    int totalTrades;
    int winningTrades;
    double totalPnL;
    
public:
    TradingStrategy(double dcThreshold = 0.008) 
        : dcIndicator(dcThreshold), cash(100000.0), position(0.0), 
          currentPrice(100.0), totalTrades(0), winningTrades(0), totalPnL(0.0) {
    }
    
    void onPriceUpdate(double price, uint64_t timestamp) {
        double prevPortfolioValue = getPortfolioValue();
        currentPrice = price;
        
        dcIndicator.updatePrice(price, timestamp);
        
        if (dcIndicator.isDCEvent()) {
            executeTrade(price, timestamp);
        }
        
        // Track portfolio value
        double currentPortfolioValue = getPortfolioValue();
        portfolioValues.push_back(currentPortfolioValue);
        
        // Update PnL
        if (prevPortfolioValue > 0) {
            double returnPct = (currentPortfolioValue - prevPortfolioValue) / prevPortfolioValue;
            totalPnL += returnPct;
            if (returnPct > 0) winningTrades++;
        }
    }
    
    void executeTrade(double price, uint64_t timestamp) {
        bool isUptrend = dcIndicator.getIsUptrend();
        double tradeSize = 1000.0; // Fixed trade size
        
        string action;
        
        if (isUptrend && position <= 0) {
            // Buy signal
            if (position < 0) {
                // Close short position
                cash -= position * price; // position is negative
                position = 0;
                action += "COVER_SHORT ";
            }
            
            // Go long
            double sharesToBuy = min(tradeSize, cash / price);
            if (sharesToBuy > 0) {
                cash -= sharesToBuy * price;
                position += sharesToBuy;
                action += "BUY";
                totalTrades++;
            }
        } else if (!isUptrend && position >= 0) {
            // Sell signal
            if (position > 0) {
                // Close long position
                cash += position * price;
                position = 0;
                action += "SELL ";
            }
            
            // Go short
            double sharesToShort = min(tradeSize, cash / price);
            if (sharesToShort > 0) {
                cash += sharesToShort * price;
                position -= sharesToShort;
                action += "SHORT";
                totalTrades++;
            }
        }
        
        if (!action.empty()) {
            string logEntry = "Time: " + to_string(timestamp) + 
                            ", Action: " + action + 
                            ", Price: " + to_string(price) + 
                            ", Position: " + to_string(position) + 
                            ", Cash: " + to_string(cash);
            tradeLog.push_back(logEntry);
            
            cout << "TRADE: " << action << " at " << fixed << setprecision(2) 
                 << price << ", Position: " << position << endl;
        }
    }
    
    double getPortfolioValue() const {
        return cash + position * currentPrice;
    }
    
    void printStatistics() {
        cout << "\n=== Trading Strategy Performance ===" << endl;
        cout << "Initial Capital: $100,000.00" << endl;
        cout << "Final Portfolio Value: $" << fixed << setprecision(2) << getPortfolioValue() << endl;
        cout << "Total Return: " << setprecision(4) << ((getPortfolioValue() - 100000.0) / 100000.0 * 100) << "%" << endl;
        cout << "Total Trades: " << totalTrades << endl;
        cout << "Winning Trades: " << winningTrades << endl;
        cout << "Win Rate: " << setprecision(2) << (totalTrades > 0 ? (winningTrades * 100.0 / totalTrades) : 0) << "%" << endl;
        
        if (portfolioValues.size() > 1) {
            // Calculate volatility
            double mean = 0;
            for (size_t i = 1; i < portfolioValues.size(); ++i) {
                double ret = (portfolioValues[i] - portfolioValues[i-1]) / portfolioValues[i-1];
                mean += ret;
            }
            mean /= (portfolioValues.size() - 1);
            
            double variance = 0;
            for (size_t i = 1; i < portfolioValues.size(); ++i) {
                double ret = (portfolioValues[i] - portfolioValues[i-1]) / portfolioValues[i-1];
                variance += (ret - mean) * (ret - mean);
            }
            variance /= (portfolioValues.size() - 2);
            double volatility = sqrt(variance);
            
            double sharpeRatio = (volatility > 0) ? mean / volatility : 0;
            
            cout << "Volatility: " << setprecision(4) << (volatility * 100) << "%" << endl;
            cout << "Sharpe Ratio: " << setprecision(3) << sharpeRatio << endl;
        }
        
        // Save trade log
        ofstream logFile("trade_log.txt");
        for (const auto& entry : tradeLog) {
            logFile << entry << endl;
        }
        logFile.close();
        cout << "Trade log saved to trade_log.txt" << endl;
    }
};

class PerformanceMonitor {
private:
    TimeUtils timeUtils;
    uint64_t startTime;
    uint64_t totalMessages;
    vector<uint64_t> latencies;
    
public:
    PerformanceMonitor() : totalMessages(0) {
        startTime = timeUtils.getCurrentTimeNanos();
    }
    
    void recordMessage(uint64_t processingLatency = 0) {
        totalMessages++;
        if (processingLatency > 0) {
            latencies.push_back(processingLatency);
        }
    }
    
    void printStatistics() {
        uint64_t endTime = timeUtils.getCurrentTimeNanos();
        uint64_t totalTime = endTime - startTime;
        
        cout << "\n=== Performance Statistics ===" << endl;
        cout << "Total Messages Processed: " << totalMessages << endl;
        cout << "Total Processing Time: " << (totalTime / 1000000.0) << " ms" << endl;
        cout << "Average Throughput: " << (totalMessages * 1000000000ULL / totalTime) << " msg/sec" << endl;
        
        if (!latencies.empty()) {
            sort(latencies.begin(), latencies.end());
            cout << "Latency Statistics:" << endl;
            cout << "  Min: " << latencies.front() << " ns" << endl;
            cout << "  Max: " << latencies.back() << " ns" << endl;
            cout << "  Median: " << latencies[latencies.size()/2] << " ns" << endl;
            
            uint64_t sum = 0;
            for (uint64_t lat : latencies) sum += lat;
            cout << "  Average: " << (sum / latencies.size()) << " ns" << endl;
            cout << "  95th percentile: " << latencies[latencies.size() * 95 / 100] << " ns" << endl;
            cout << "  99th percentile: " << latencies[latencies.size() * 99 / 100] << " ns" << endl;
        }
    }
};

int main() {
    cout << "=== Low-Latency Trading System - Integration Test ===" << endl;
    
    // Initialize components
    MockMarketDataFeed marketFeed;
    TradingStrategy strategy(0.008); // 0.8% DC threshold
    PerformanceMonitor perfMonitor;
    TimeUtils timeUtils;
    
    // Wire up the system
    marketFeed.onPriceUpdate = [&](double price, uint64_t timestamp) {
        uint64_t startTime = timeUtils.getCurrentTimeNanos();
        
        strategy.onPriceUpdate(price, timestamp);
        
        uint64_t endTime = timeUtils.getCurrentTimeNanos();
        perfMonitor.recordMessage(endTime - startTime);
    };
    
    cout << "\nStarting market data feed..." << endl;
    marketFeed.start();
    
    // Let it run for a while
    cout << "Running integration test for 30 seconds..." << endl;
    this_thread::sleep_for(chrono::seconds(30));
    
    cout << "\nStopping market data feed..." << endl;
    marketFeed.stop();
    
    // Print results
    strategy.printStatistics();
    perfMonitor.printStatistics();
    
    cout << "\n=== Integration Test Complete ===" << endl;
    
    return 0;
} 