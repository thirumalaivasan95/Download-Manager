#include "include/core/Throttler.h"
#include "include/utils/Logger.h"

#include <thread>
#include <sstream>
#include <algorithm>

namespace dm {
namespace core {

Throttler::Throttler(int64_t bytesPerSecond, int64_t burstSize)
    : maxBandwidth_(bytesPerSecond), burstSize_(burstSize), tokens_(0), enabled_(bytesPerSecond > 0) {
    
    // Initialize the token bucket
    lastFillTime_ = std::chrono::steady_clock::now();
    tokens_ = getMaxTokens();
    
    // Log throttler creation
    std::ostringstream log;
    log << "Created throttler with max bandwidth: " << bytesPerSecond 
        << " bytes/s, burst size: " << burstSize << " bytes";
    dm::utils::Logger::debug(log.str());
}

Throttler::~Throttler() {
    // Log throttler destruction
    dm::utils::Logger::debug("Destroyed throttler");
}

void Throttler::setMaxBandwidth(int64_t bytesPerSecond) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Set max bandwidth
    maxBandwidth_ = bytesPerSecond;
    
    // Enable/disable throttling
    enabled_ = bytesPerSecond > 0;
    
    // Refill bucket
    fillBucket();
    
    // Wake up all waiting threads
    cv_.notify_all();
    
    // Log bandwidth change
    std::ostringstream log;
    log << "Throttler max bandwidth changed to " << bytesPerSecond << " bytes/s";
    dm::utils::Logger::debug(log.str());
}

int64_t Throttler::getMaxBandwidth() const {
    return maxBandwidth_;
}

void Throttler::setBurstSize(int64_t burstSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    burstSize_ = burstSize;
    
    // Log burst size change
    std::ostringstream log;
    log << "Throttler burst size changed to " << burstSize << " bytes";
    dm::utils::Logger::debug(log.str());
}

int64_t Throttler::getBurstSize() const {
    return burstSize_;
}

void Throttler::request(int64_t bytes) {
    // If throttling is disabled, return immediately
    if (!enabled_) {
        return;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Fill the bucket
    fillBucket();
    
    // Wait until enough tokens are available
    while (tokens_ < bytes && enabled_) {
        // Calculate time to wait for enough tokens
        int64_t tokensNeeded = bytes - tokens_;
        int64_t bw = maxBandwidth_.load();
        
        if (bw <= 0) {
            break; // Unlimited bandwidth
        }
        
        // Calculate milliseconds to wait
        int64_t millisecondsToWait = (tokensNeeded * 1000) / bw + 1;
        
        // Wait for tokens
        cv_.wait_for(lock, std::chrono::milliseconds(millisecondsToWait));
        
        // Refill bucket after wait
        fillBucket();
    }
    
    // If throttling is disabled, return immediately
    if (!enabled_) {
        return;
    }
    
    // Consume tokens
    tokens_ -= bytes;
}

bool Throttler::requestWithTimeout(int64_t bytes, int timeoutMs) {
    // If throttling is disabled, return immediately
    if (!enabled_) {
        return true;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Fill the bucket
    fillBucket();
    
    // Check if enough tokens are available immediately
    if (tokens_ >= bytes) {
        tokens_ -= bytes;
        return true;
    }
    
    // Calculate deadline
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    
    // Wait until enough tokens are available or timeout expires
    while (tokens_ < bytes && enabled_) {
        // Calculate time to wait for enough tokens
        int64_t tokensNeeded = bytes - tokens_;
        int64_t bw = maxBandwidth_.load();
        
        if (bw <= 0) {
            break; // Unlimited bandwidth
        }
        
        // Calculate milliseconds to wait
        int64_t millisecondsToWait = (tokensNeeded * 1000) / bw + 1;
        
        // Wait for tokens with timeout
        auto status = cv_.wait_until(lock, deadline);
        
        // Check if timeout expired
        if (status == std::cv_status::timeout || std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        
        // Refill bucket after wait
        fillBucket();
    }
    
    // If throttling is disabled, return immediately
    if (!enabled_) {
        return true;
    }
    
    // Consume tokens
    tokens_ -= bytes;
    return true;
}

void Throttler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Reset token bucket
    lastFillTime_ = std::chrono::steady_clock::now();
    tokens_ = getMaxTokens();
    
    // Wake up all waiting threads
    cv_.notify_all();
    
    // Log reset
    dm::utils::Logger::debug("Throttler reset");
}

void Throttler::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled && (maxBandwidth_ > 0);
    
    // Wake up all waiting threads if disabled
    if (!enabled_) {
        cv_.notify_all();
    }
    
    // Log enable/disable
    std::ostringstream log;
    log << "Throttler " << (enabled_ ? "enabled" : "disabled");
    dm::utils::Logger::debug(log.str());
}

bool Throttler::isEnabled() const {
    return enabled_;
}

void Throttler::fillBucket() {
    // Get current time
    auto now = std::chrono::steady_clock::now();
    
    // Calculate time difference in seconds
    double elapsedSeconds = std::chrono::duration<double>(now - lastFillTime_).count();
    
    // Calculate tokens to add
    int64_t bw = maxBandwidth_.load();
    int64_t tokensToAdd = static_cast<int64_t>(elapsedSeconds * bw);
    
    if (tokensToAdd > 0) {
        // Add tokens to bucket
        tokens_ = std::min(tokens_ + tokensToAdd, getMaxTokens());
        
        // Update last fill time
        lastFillTime_ = now;
    }
}

int64_t Throttler::getMaxTokens() const {
    int64_t bs = burstSize_.load();
    
    // If burst size is not set, use 1 second of bandwidth
    if (bs <= 0) {
        int64_t bw = maxBandwidth_.load();
        return bw > 0 ? bw : 0;
    }
    
    return bs;
}

} // namespace core
} // namespace dm