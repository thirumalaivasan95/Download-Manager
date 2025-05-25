#ifndef DOWNLOAD_TASK_H
#define DOWNLOAD_TASK_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>
#include "core/SegmentDownloader.h"

namespace dm {
namespace core {

enum class DownloadStatus {
    NONE,           // Not started
    QUEUED,         // In queue, waiting to start
    CONNECTING,     // Connecting to server
    DOWNLOADING,    // Actively downloading
    PAUSED,         // Paused by user
    COMPLETED,      // Successfully completed
    ERROR,          // Error occurred
    CANCELED        // Canceled by user
};

enum class DownloadType {
    REGULAR,        // Regular file download
    STREAMING,      // Streaming content
    BATCH,          // Part of a batch download
    SCHEDULED       // Scheduled for later
};

class DownloadTask;
using StatusChangeCallback = std::function<void(std::shared_ptr<DownloadTask>, DownloadStatus, DownloadStatus)>;

/**
 * @brief Download priority enumeration
 */
enum class DownloadPriority {
    LOW,            // Low priority
    NORMAL,         // Normal priority
    HIGH            // High priority
};

/**
 * @brief Progress information structure
 */
struct ProgressInfo {
    int64_t totalBytes = 0;          // Total bytes to download
    int64_t downloadedBytes = 0;     // Bytes downloaded so far
    double progressPercent = 0.0;    // Progress percentage (0-100)
    double downloadSpeed = 0.0;      // Download speed in bytes/second
    double averageSpeed = 0.0;       // Average download speed
    int64_t timeElapsed = 0;         // Time elapsed in seconds
    int64_t timeRemaining = 0;       // Estimated time remaining in seconds
};

/**
 * @brief Progress callback function type
 */
using TaskProgressCallback = std::function<void(const ProgressInfo&)>;

/**
 * @brief Download task class
 * 
 * Represents a single download task with multiple segments
 */
class DownloadTask : public std::enable_shared_from_this<DownloadTask> {
public:
    /**
     * @brief Construct a new DownloadTask
     * 
     * @param url The URL to download
     * @param destinationPath The path to save the file
     * @param filename The filename to use (optional)
     */
    DownloadTask(const std::string& url, 
                const std::string& destinationPath,
                const std::string& filename = "");
    
    /**
     * @brief Destroy the DownloadTask
     */
    ~DownloadTask();
    
    /**
     * @brief Initialize the download task
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Start the download
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
     * @brief Set the number of segments
     * 
     * @param count The number of segments to use
     */
    void setSegmentCount(int count);
    
    /**
     * @brief Set the download priority
     * 
     * @param priority The priority level
     */
    void setPriority(DownloadPriority priority);
    
    /**
     * @brief Set the download type
     * 
     * @param type The download type
     */
    void setType(DownloadType type);
    
    /**
     * @brief Set the progress callback
     * 
     * @param callback The progress callback function
     */
    void setProgressCallback(TaskProgressCallback callback);
    
    /**
     * @brief Set the status change callback
     * 
     * @param callback The status change callback function
     */
    void setStatusChangeCallback(StatusChangeCallback callback);
    
    /**
     * @brief Get the current status
     * 
     * @return DownloadStatus The current status
     */
    DownloadStatus getStatus() const;
    
    /**
     * @brief Get the progress information
     * 
     * @return ProgressInfo The progress information
     */
    ProgressInfo getProgressInfo() const;
    
    /**
     * @brief Get the URL
     * 
     * @return const std::string& The URL
     */
    const std::string& getUrl() const;
    
    /**
     * @brief Get the destination path
     * 
     * @return const std::string& The destination path
     */
    const std::string& getDestinationPath() const;
    
    /**
     * @brief Get the filename
     * 
     * @return const std::string& The filename
     */
    const std::string& getFilename() const;
    
    /**
     * @brief Get the file size
     * 
     * @return int64_t The file size in bytes
     */
    int64_t getFileSize() const;
    
    /**
     * @brief Get the error message if an error occurred
     * 
     * @return const std::string& The error message
     */
    const std::string& getError() const;
    
    /**
     * @brief Get the download speed
     * 
     * @return double The download speed in bytes/second
     */
    double getDownloadSpeed() const;
    
    /**
     * @brief Get the start time
     * 
     * @return std::chrono::system_clock::time_point The start time
     */
    std::chrono::system_clock::time_point getStartTime() const;
    
    /**
     * @brief Check if the download supports resuming
     * 
     * @return true if the download supports resuming, false otherwise
     */
    bool supportsResume() const;
    
    /**
     * @brief Get the unique ID of the download
     * 
     * @return const std::string& The unique ID
     */
    const std::string& getId() const;
    
    /**
     * @brief Get the priority
     * 
     * @return DownloadPriority The priority level
     */
    DownloadPriority getPriority() const;
    
    /**
     * @brief Get the type
     * 
     * @return DownloadType The download type
     */
    DownloadType getType() const;
    
    /**
     * @brief Update the progress information
     */
    void updateProgress();
    
    // Set max retries for all segments
    void setSegmentMaxRetries(int retries);
    
private:
    /**
     * @brief Check if the server supports range requests
     * 
     * @return true if the server supports range requests, false otherwise
     */
    bool checkRangeSupport();
    
    /**
     * @brief Initialize the file
     * 
     * @return true if successful, false otherwise
     */
    bool initializeFile();
    
    /**
     * @brief Create download segments
     * 
     * @return true if successful, false otherwise
     */
    bool createSegments();
    
    /**
     * @brief Create a metadata file for resuming downloads
     * 
     * @return true if successful, false otherwise
     */
    bool createMetadataFile();
    
    /**
     * @brief Load metadata for resuming a download
     * 
     * @return true if successful, false otherwise
     */
    bool loadMetadata();
    
    /**
     * @brief Set the status
     * 
     * @param status The new status
     */
    void setStatus(DownloadStatus status);
    
    /**
     * @brief Task completion handler
     */
    void onTaskCompleted();
    
    /**
     * @brief Segment completion handler
     * 
     * @param segment The segment that completed
     */
    void onSegmentCompleted(std::shared_ptr<SegmentDownloader> segment);
    
    /**
     * @brief Segment error handler
     * 
     * @param segment The segment that encountered an error
     * @param error The error message
     */
    void onSegmentError(std::shared_ptr<SegmentDownloader> segment, const std::string& error);
    
    // Member variables
    std::string url_;
    std::string destinationPath_;
    std::string filename_;
    std::string id_;
    std::string error_;
    
    int64_t fileSize_ = 0;
    DownloadStatus status_ = DownloadStatus::NONE;
    DownloadPriority priority_ = DownloadPriority::NORMAL;
    DownloadType type_ = DownloadType::REGULAR;
    
    bool supportsResume_ = false;
    int segmentCount_ = 4;
    int segmentMaxRetries_ = 3;
    
    std::chrono::system_clock::time_point startTime_;
    std::chrono::system_clock::time_point lastUpdateTime_;
    
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<SegmentDownloader>> segments_;
    
    ProgressInfo progressInfo_;
    TaskProgressCallback progressCallback_ = nullptr;
    StatusChangeCallback statusChangeCallback_ = nullptr;
    
    // History of download speeds for calculating average
    std::vector<double> speedHistory_;
};

} // namespace core
} // namespace dm

#endif // DOWNLOAD_TASK_H