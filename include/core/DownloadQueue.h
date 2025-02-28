#ifndef DOWNLOAD_QUEUE_H
#define DOWNLOAD_QUEUE_H

#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

#include "include/core/DownloadTask.h"

namespace dm {
namespace core {

/**
 * @brief Queue processor callback function type
 */
using QueueProcessorCallback = std::function<void()>;

/**
 * @brief Download queue class
 * 
 * Manages and schedules download tasks
 */
class DownloadQueue {
public:
    /**
     * @brief Construct a new DownloadQueue
     * 
     * @param maxConcurrentDownloads Maximum number of concurrent downloads
     */
    explicit DownloadQueue(int maxConcurrentDownloads = 3);
    
    /**
     * @brief Destroy the DownloadQueue
     */
    ~DownloadQueue();
    
    /**
     * @brief Add a download task to the queue
     * 
     * @param task The task to add
     */
    void addTask(std::shared_ptr<DownloadTask> task);
    
    /**
     * @brief Remove a download task from the queue
     * 
     * @param taskId The task ID to remove
     * @return true if removed, false if not found
     */
    bool removeTask(const std::string& taskId);
    
    /**
     * @brief Get a download task by ID
     * 
     * @param taskId The task ID
     * @return std::shared_ptr<DownloadTask> The task or nullptr if not found
     */
    std::shared_ptr<DownloadTask> getTask(const std::string& taskId);
    
    /**
     * @brief Get all download tasks
     * 
     * @return std::vector<std::shared_ptr<DownloadTask>> All tasks
     */
    std::vector<std::shared_ptr<DownloadTask>> getAllTasks();
    
    /**
     * @brief Get download tasks by status
     * 
     * @param status The status to filter by
     * @return std::vector<std::shared_ptr<DownloadTask>> The matching tasks
     */
    std::vector<std::shared_ptr<DownloadTask>> getTasksByStatus(DownloadStatus status);
    
    /**
     * @brief Start a download task
     * 
     * @param taskId The task ID
     * @return true if started, false otherwise
     */
    bool startTask(const std::string& taskId);
    
    /**
     * @brief Pause a download task
     * 
     * @param taskId The task ID
     * @return true if paused, false otherwise
     */
    bool pauseTask(const std::string& taskId);
    
    /**
     * @brief Resume a download task
     * 
     * @param taskId The task ID
     * @return true if resumed, false otherwise
     */
    bool resumeTask(const std::string& taskId);
    
    /**
     * @brief Cancel a download task
     * 
     * @param taskId The task ID
     * @return true if canceled, false otherwise
     */
    bool cancelTask(const std::string& taskId);
    
    /**
     * @brief Start all download tasks
     */
    void startAllTasks();
    
    /**
     * @brief Pause all download tasks
     */
    void pauseAllTasks();
    
    /**
     * @brief Resume all paused download tasks
     */
    void resumeAllTasks();
    
    /**
     * @brief Cancel all download tasks
     */
    void cancelAllTasks();
    
    /**
     * @brief Process the queue
     * 
     * Starts tasks if slots are available
     */
    void processQueue();
    
    /**
     * @brief Set the maximum number of concurrent downloads
     * 
     * @param maxConcurrentDownloads The maximum number
     */
    void setMaxConcurrentDownloads(int maxConcurrentDownloads);
    
    /**
     * @brief Get the maximum number of concurrent downloads
     * 
     * @return int The maximum number
     */
    int getMaxConcurrentDownloads() const;
    
    /**
     * @brief Get the number of active downloads
     * 
     * @return int The number of active downloads
     */
    int getActiveDownloadsCount() const;
    
    /**
     * @brief Set the queue processor callback
     * 
     * @param callback The callback function
     */
    void setQueueProcessorCallback(QueueProcessorCallback callback);
    
private:
    /**
     * @brief Task status change handler
     * 
     * @param task The task that changed status
     * @param status The new status
     */
    void onTaskStatusChanged(std::shared_ptr<DownloadTask> task, DownloadStatus status);
    
    /**
     * @brief Count active downloads
     * 
     * @return int The number of active downloads
     */
    int countActiveDownloads();
    
    // Member variables
    std::map<std::string, std::shared_ptr<DownloadTask>> tasks_;
    std::queue<std::string> pendingTasks_;
    mutable std::mutex mutex_;
    std::atomic<int> maxConcurrentDownloads_;
    std::atomic<int> activeDownloads_;
    
    QueueProcessorCallback queueProcessorCallback_ = nullptr;
};

} // namespace core
} // namespace dm

#endif // DOWNLOAD_QUEUE_H