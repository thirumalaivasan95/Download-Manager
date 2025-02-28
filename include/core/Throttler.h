#ifndef THROTTLER_H
#define THROTTLER_H

#include <mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>

namespace dm {
namespace core {

/**
 * @brief Throttler class for bandwidth limiting
 * 
 * Uses a token bucket algorithm to limit bandwidth
 */
class Throttler {
public:
    /**
     * @brief Construct a new Throttler
     * 
     * @param bytesPerSecond Maximum bytes per second (0 for unlimited)
     * @param burstSize Maximum burst size in bytes
     */
    explicit Throttler(int64_t bytesPerSecond = 0, int64_t burstSize = 0);
    
    /**
     * @brief Destroy the Throttler
     */
    ~Throttler();
    
    /**
     * @brief Set the maximum bandwidth
     * 
     * @param bytesPerSecond Maximum bytes per second (0 for unlimited)
     */
    void setMaxBandwidth(int64_t bytesPerSecond);
    
    /**
     * @brief Get the maximum bandwidth
     * 
     * @return int64_t Maximum bytes per second (0 for unlimited)
     */
    int64_t getMaxBandwidth() const;
    
    /**
     * @brief Set the burst size
     * 
     * @param burstSize Maximum burst size in bytes
     */
    void setBurstSize(int64_t burstSize);
    
    /**
     * @brief Get the burst size
     * 
     * @return int64_t Maximum burst size in bytes
     */
    int64_t getBurstSize() const;
    
    /**
     * @brief Request bandwidth
     * 
     * This method will block until the requested bandwidth is available
     * 
     * @param bytes Number of bytes requested
     */
    void request(int64_t bytes);
    
    /**
     * @brief Request bandwidth with a timeout
     * 
     * This method will block until the requested bandwidth is available or the timeout expires
     * 
     * @param bytes Number of bytes requested
     * @param timeoutMs Timeout in milliseconds
     * @return true if the request was successful, false if the timeout expired
     */
    bool requestWithTimeout(int64_t bytes, int timeoutMs);
    
    /**
     * @brief Reset the throttler
     * 
     * This will reset the token bucket to its initial state
     */
    void reset();
    
    /**
     * @brief Enable or disable the throttler
     * 
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if the throttler is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;
    
private:
    /**
     * @brief Fill the token bucket
     * 
     * This will add tokens to the bucket based on the time elapsed
     * since the last fill
     */
    void fillBucket();
    
    /**
     * @brief Get the maximum number of tokens that can be in the bucket
     * 
     * @return int64_t Maximum number of tokens
     */
    int64_t getMaxTokens() const;
    
    // Member variables
    std::atomic<int64_t> maxBandwidth_;
    std::atomic<int64_t> burstSize_;
    std::atomic<int64_t> tokens_;
    std::atomic<bool> enabled_;
    
    std::chrono::steady_clock::time_point lastFillTime_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace core
} // namespace dm

#endif // THROTTLER_H