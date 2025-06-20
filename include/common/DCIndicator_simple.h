#pragma once

#include <cmath>
#include <vector>

class DCIndicator {
private:
    double theta;                    // DC阈值
    double currentPrice;            // 当前价格
    double extremePrice;            // 极值价格
    uint64_t lastTimestamp;         // 上次时间戳
    uint64_t extremeTimestamp;      // 极值时间戳
    bool isUptrend;                 // 是否上升趋势
    bool dcEventDetected;           // DC事件检测标志
    double tmvExt;                  // TMV_EXT值
    double timeAdjustedReturn;      // 时间调整回报
    
    std::vector<double> priceHistory;
    std::vector<uint64_t> timestampHistory;
    
public:
    DCIndicator(double threshold = 0.004) : theta(threshold) {
        reset();
    }
    
    void reset() {
        currentPrice = 0.0;
        extremePrice = 0.0;
        lastTimestamp = 0;
        extremeTimestamp = 0;
        isUptrend = true;
        dcEventDetected = false;
        tmvExt = 0.0;
        timeAdjustedReturn = 0.0;
        priceHistory.clear();
        timestampHistory.clear();
    }
    
    void updatePrice(double price, uint64_t timestamp) {
        priceHistory.push_back(price);
        timestampHistory.push_back(timestamp);
        
        if (priceHistory.size() == 1) {
            currentPrice = price;
            extremePrice = price;
            lastTimestamp = timestamp;
            extremeTimestamp = timestamp;
            return;
        }
        
        double prevPrice = currentPrice;
        currentPrice = price;
        lastTimestamp = timestamp;
        
        dcEventDetected = false;
        
        // 检测DC事件
        if (isUptrend) {
            if (price > extremePrice) {
                extremePrice = price;
                extremeTimestamp = timestamp;
            } else {
                double decline = (extremePrice - price) / extremePrice;
                if (decline >= theta) {
                    // 检测到下降DC事件
                    dcEventDetected = true;
                    isUptrend = false;
                    calculateIndicators(timestamp);
                }
            }
        } else {
            if (price < extremePrice) {
                extremePrice = price;
                extremeTimestamp = timestamp;
            } else {
                double rise = (price - extremePrice) / extremePrice;
                if (rise >= theta) {
                    // 检测到上升DC事件
                    dcEventDetected = true;
                    isUptrend = true;
                    calculateIndicators(timestamp);
                }
            }
        }
    }
    
    bool isDCEvent() const { return dcEventDetected; }
    double getTMVExt() const { return tmvExt; }
    double getTimeAdjustedReturn() const { return timeAdjustedReturn; }
    bool getIsUptrend() const { return isUptrend; }
    double getCurrentPrice() const { return currentPrice; }
    double getExtremePrice() const { return extremePrice; }
    
private:
    void calculateIndicators(uint64_t timestamp) {
        if (priceHistory.size() < 2) return;
        
        // 计算TMV_EXT
        double prevExtreme = (priceHistory.size() >= 2) ? priceHistory[priceHistory.size()-2] : extremePrice;
        if (prevExtreme != 0) {
            tmvExt = (extremePrice - prevExtreme) / (prevExtreme * theta);
        }
        
        // 计算时间调整回报
        uint64_t timeDiff = timestamp - extremeTimestamp;
        if (timeDiff > 0) {
            double timeInSeconds = timeDiff / 1e9; // 转换为秒
            timeAdjustedReturn = tmvExt / timeInSeconds * theta;
        }
    }
}; 