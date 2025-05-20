#ifndef SEGMENT_DOWNLOADER_H
#define SEGMENT_DOWNLOADER_H

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include "core/HttpClient.h"

namespace dm {
namespace core {

/**
 * @brief Segment status enumeration
 */
enum class SegmentStatus {
    NONE,           // Not started
    DOWNLOADING,    // Actively downloading
    PAUSED,         // Paused
    COMPLETED,      // Successfully completed
    ERROR           // Error occurred
};

/**
 * @brief Segment completion callback function type
 */
using SegmentCompletionCallback = std::function<void(std::shared_ptr<class SegmentDownloader>)>;

/**
 * @brief Segment error callback function type
 */
using SegmentErrorCallback = std::function<void(std::shared_ptr<class SegmentDownloader>, const std::string&)>;

/**
 * @brief Segment downloader class
 * 
 * Responsible for downloading a specific segment of a file
 */
class SegmentDownloader : public std::enable_shared_from_this<SegmentDownloader> {
public:
    /**
     * @brief Construct a new SegmentDownloader
     * 
     * @param url The URL to download
     * @param filePath The path to save the segment
     * @param startByte The starting byte offset
     * @param endByte The ending byte offset
     * @param id The segment ID
     */
    SegmentDownloader(const std::string& url, 
                     const std::string& filePath,
                     int64_t startByte, 
                     int64_t endByte,
                     int id);
    
    /**
     * @brief Destroy the SegmentDownloader
     */
    ~SegmentDownloader();
    
    /**
     * @brief Start downloading the segment
     * 
     * @return true if started, false otherwise
     */
    bool start();
    
    /**
     * @brief Pause the download
     * 
     * @return true if paused, false otherwise
     */
    bool pause();
    
    /**
     * @brief Resume the download
     * 
     * @return true if resumed, false otherwise
     */
    bool resume();
    
    /**
     * @brief Cancel the download
     * 
     * @return true if canceled, false otherwise
     */
    bool cancel();
    
    /**
     * @brief Set the completion callback
     * 
     * @param callback The completion callback function
     */
    void setCompletionCallback(SegmentCompletionCallback callback);
    
    /**
     * @brief Set the error callback
     * 
     * @param callback The error callback function
     */
    void setErrorCallback(SegmentErrorCallback callback);
    
    /**
     * @brief Get the current status
     * 
     * @return SegmentStatus The current status
     */
    SegmentStatus getStatus() const;
    
    /**
     * @brief Get the downloaded bytes
     * 
     * @return int64_t The downloaded bytes
     */
    int64_t getDownloadedBytes() const;
    
    /**
     * @brief Get the total bytes
     * 
     * @return int64_t The total bytes
     */
    int64_t getTotalBytes() const;
    
    /**
     * @brief Get the download speed
     * 
     * @return double The download speed in bytes/second
     */
    double getDownloadSpeed() const;
    
    /**
     * @brief Get the segment ID
     * 
     * @return int The segment ID
     */
    int getId() const;
    
    /**
     * @brief Get the start byte
     * 
     * @return int64_t The start byte
     */
    int64_t getStartByte() const;
    
    /**
     * @brief Get the end byte
     * 
     * @return int64_t The end byte
     */
    int64_t getEndByte() const;
    
    /**
     * @brief Get the file path
     * 
     * @return const std::string& The file path
     */
    const std::string& getFilePath() const;
    
    /**
     * @brief Get the URL
     * 
     * @return const std::string& The URL
     */
    const std::string& getUrl() const;
    
private:
    /**
     * @brief Download thread function
     */
    void downloadThread();
    
    /**
     * @brief Set the status
     * 
     * @param status The new status
     */
    void setStatus(SegmentStatus status);
    
    /**
     * @brief Update the download speed
     */
    void updateDownloadSpeed();
    
    /**
     * @brief Progress callback for the HTTP client
     * 
     * @param downloadTotal Total bytes to download
     * @param downloadedNow Bytes downloaded so far
     * @param uploadTotal Total bytes to upload
     * @param uploadedNow Bytes uploaded so far
     * @return true to continue, false to abort
     */
    bool onProgress(int64_t downloadTotal, int64_t downloadedNow, 
                   int64_t uploadTotal, int64_t uploadedNow);
    
    // Member variables
    std::string url_;
    std::string filePath_;
    int64_t startByte_;
    int64_t endByte_;
    int id_;
    
    std::atomic<SegmentStatus> status_ = SegmentStatus::NONE;
    std::atomic<bool> stopRequested_ = false;
    
    std::atomic<int64_t> downloadedBytes_ = 0;
    std::atomic<double> downloadSpeed_ = 0.0;
    
    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    
    std::chrono::system_clock::time_point lastSpeedUpdateTime_;
    int64_t lastDownloadedBytes_ = 0;
    
    std::unique_ptr<HttpClient> httpClient_;
    
    SegmentCompletionCallback completionCallback_ = nullptr;
    SegmentErrorCallback errorCallback_ = nullptr;
};

} // namespace core
} // namespace dm

#endif // SEGMENT_DOWNLOADER_H