#pragma once

#ifdef MOCK_AERON

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

namespace aeron {

// Mock message handler
using fragment_handler_t = std::function<void(const uint8_t* buffer, size_t length, int64_t timestamp)>;

// Mock Aeron Context
class Context {
public:
    Context& aeronDir(const std::string& dir) { return *this; }
    Context& mediaDriverTimeout(int64_t timeout) { return *this; }
    Context& resourceLingerTimeout(int64_t timeout) { return *this; }
    Context& useConductorAgentInvoker(bool use) { return *this; }
};

// Mock Publication
class Publication {
private:
    std::string channel;
    int32_t streamId;
    
public:
    Publication(const std::string& ch, int32_t sid) : channel(ch), streamId(sid) {}
    
    int64_t offer(const uint8_t* buffer, size_t length) {
        // Mock successful publication
        return length;
    }
    
    bool isConnected() const { return true; }
    bool isClosed() const { return false; }
    std::string getChannel() const { return channel; }
    int32_t getStreamId() const { return streamId; }
};

// Mock Subscription  
class Subscription {
private:
    std::string channel;
    int32_t streamId;
    fragment_handler_t handler;
    std::queue<std::vector<uint8_t>> messageQueue;
    std::mutex queueMutex;
    
public:
    Subscription(const std::string& ch, int32_t sid) : channel(ch), streamId(sid) {}
    
    void setFragmentHandler(fragment_handler_t h) { handler = h; }
    
    int poll(int maxMessages = 10) {
        std::lock_guard<std::mutex> lock(queueMutex);
        int processed = 0;
        
        while (!messageQueue.empty() && processed < maxMessages) {
            auto& message = messageQueue.front();
            if (handler) {
                handler(message.data(), message.size(), 
                       std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            }
            messageQueue.pop();
            processed++;
        }
        
        return processed;
    }
    
    // Mock method to inject test messages
    void injectMessage(const std::vector<uint8_t>& message) {
        std::lock_guard<std::mutex> lock(queueMutex);
        messageQueue.push(message);
    }
    
    bool isConnected() const { return true; }
    bool isClosed() const { return false; }
    std::string getChannel() const { return channel; }
    int32_t getStreamId() const { return streamId; }
};

// Mock Aeron client
class Aeron {
private:
    std::vector<std::shared_ptr<Publication>> publications;
    std::vector<std::shared_ptr<Subscription>> subscriptions;
    
public:
    static std::shared_ptr<Aeron> connect(Context& context) {
        return std::make_shared<Aeron>();
    }
    
    std::shared_ptr<Publication> addPublication(const std::string& channel, int32_t streamId) {
        auto pub = std::make_shared<Publication>(channel, streamId);
        publications.push_back(pub);
        return pub;
    }
    
    std::shared_ptr<Subscription> addSubscription(const std::string& channel, int32_t streamId) {
        auto sub = std::make_shared<Subscription>(channel, streamId);
        subscriptions.push_back(sub);
        return sub;
    }
    
    void close() {
        publications.clear();
        subscriptions.clear();
    }
    
    // Mock method to get subscriptions for testing
    std::vector<std::shared_ptr<Subscription>>& getSubscriptions() {
        return subscriptions;
    }
};

// Mock IdleStrategy
class BusySpinIdleStrategy {
public:
    void idle(int workCount) {
        if (workCount == 0) {
            std::this_thread::yield();
        }
    }
};

class BackoffIdleStrategy {
public:
    void idle(int workCount) {
        if (workCount == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
};

} // namespace aeron

#endif // MOCK_AERON 