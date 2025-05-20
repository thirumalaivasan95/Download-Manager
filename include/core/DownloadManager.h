#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

#include "core/DownloadTask.h"
#include "core/DownloadQueue.h"
#include "core/Settings.h"

namespace dm {
namespace core {

/**
 * @brief Task added callback function type
 */
using TaskAddedCallback = std::function<void(std::shared_ptr<DownloadTask>)>;

/**
 * @brief Task removed callback function type
 */
using TaskRemovedCallback = std::function<void(std::shared_ptr<DownloadTask>)>;

/**
 * @brief Task status changed callback function type
 */
using TaskStatusChangedCallback = std::function<void(std::shared_ptr<DownloadTask>, DownloadStatus)>;

/**
 * @brief Download manager class
 * 
 * Main class that manages all download tasks
 */
class DownloadManager {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return DownloadManager& The singleton instance
     */
    static DownloadManager& getInstance();
    
    /**
     * @brief Initialize the download manager
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Shutdown the download manager
     */
    void shutdown();
    
    /**
     * @brief Add a new download task
     * 
     * @param url The URL to download
     * @param destinationPath The path to save the file (optional)
     * @param filename The filename to use (optional)
     * @param start Whether to start the download immediately
     * @return std::shared_ptr<DownloadTask> The created task or nullptr on failure
     */
    std::shared_ptr<DownloadTask> addDownload(const std::string& url, 
                                            const std::string& destinationPath = "",
                                            const std::string& filename = "",
                                            bool start = true);
    
    /**
     * @brief Add a batch of downloads
     * 
     * @param urls The URLs to download
     * @param destinationPath The common destination path
     * @param start Whether to start the downloads immediately
     * @return std::vector<std::shared_ptr<DownloadTask>> The created tasks
     */
    std::vector<std::shared_ptr<DownloadTask>> addBatchDownload(
        const std::vector<std::string>& urls,
        const std::string& destinationPath = "",
        bool start = true);
    
    /**
     * @brief Start a download task
     * 
     * @param taskId The task ID
     * @return true if successful, false otherwise
     */
    bool startDownload(const std::string& taskId);
    
    /**
     * @brief Pause a download task
     * 
     * @param taskId The task ID
     * @return true if successful, false otherwise
     */
    bool pauseDownload(const std::string& taskId);
    
    /**
     * @brief Resume a download task
     * 
     * @param taskId The task ID
     * @return true if successful, false otherwise
     */
    bool resumeDownload(const std::string& taskId);
    
    /**
     * @brief Cancel a download task
     * 
     * @param taskId The task ID
     * @return true if successful, false otherwise
     */
    bool cancelDownload(const std::string& taskId);
    
    /**
     * @brief Remove a download task
     * 
     * @param taskId The task ID
     * @param deleteFile Whether to delete the downloaded file
     * @return true if successful, false otherwise
     */
    bool removeDownload(const std::string& taskId, bool deleteFile = false);
    
    /**
     * @brief Start all downloads
     */
    void startAllDownloads();
    
    /**
     * @brief Pause all downloads
     */
    void pauseAllDownloads();
    
    /**
     * @brief Resume all downloads
     */
    void resumeAllDownloads();
    
    /**
     * @brief Cancel all downloads
     */
    void cancelAllDownloads();
    
    /**
     * @brief Get a download task by ID
     * 
     * @param taskId The task ID
     * @return std::shared_ptr<DownloadTask> The task or nullptr if not found
     */
    std::shared_ptr<DownloadTask> getDownloadTask(const std::string& taskId);
    
    /**
     * @brief Get all download tasks
     * 
     * @return std::vector<std::shared_ptr<DownloadTask>> All tasks
     */
    std::vector<std::shared_ptr<DownloadTask>> getAllDownloadTasks();
    
    /**
     * @brief Get download tasks by status
     * 
     * @param status The status to filter by
     * @return std::vector<std::shared_ptr<DownloadTask>> The matching tasks
     */
    std::vector<std::shared_ptr<DownloadTask>> getDownloadTasksByStatus(DownloadStatus status);
    
    /**
     * @brief Set the task added callback
     * 
     * @param callback The callback function
     */
    void setTaskAddedCallback(TaskAddedCallback callback);
    
    /**
     * @brief Set the task removed callback
     * 
     * @param callback The callback function
     */
    void setTaskRemovedCallback(TaskRemovedCallback callback);
    
    /**
     * @brief Set the task status changed callback
     * 
     * @param callback The callback function
     */
    void setTaskStatusChangedCallback(TaskStatusChangedCallback callback);
    
    /**
     * @brief Get the settings
     * 
     * @return std::shared_ptr<Settings> The settings
     */
    std::shared_ptr<Settings> getSettings();
    
    /**
     * @brief Load tasks from disk
     * 
     * @return true if successful, false otherwise
     */
    bool loadTasks();
    
    /**
     * @brief Save tasks to disk
     * 
     * @return true if successful, false otherwise
     */
    bool saveTasks();
    
    /**
     * @brief Get the default download directory
     * 
     * @return std::string The default download directory
     */
    std::string getDefaultDownloadDirectory() const;
    
    /**
     * @brief Set the default download directory
     * 
     * @param directory The directory path
     */
    void setDefaultDownloadDirectory(const std::string& directory);
    
    /**
     * @brief Get the maximum concurrent downloads
     * 
     * @return int The maximum concurrent downloads
     */
    int getMaxConcurrentDownloads() const;
    
    /**
     * @brief Set the maximum concurrent downloads
     * 
     * @param max The maximum concurrent downloads
     */
    void setMaxConcurrentDownloads(int max);
    
private:
    /**
     * @brief Construct a new DownloadManager
     */
    DownloadManager();
    
    /**
     * @brief Destroy the DownloadManager
     */
    ~DownloadManager();
    
    // Prevent copying
    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;
    
    /**
     * @brief Task status change handler
     * 
     * @param task The task that changed status
     * @param status The new status
     */
    void onTaskStatusChanged(std::shared_ptr<DownloadTask> task, DownloadStatus status);
    
    /**
     * @brief Queue processor thread function
     */
    void queueProcessorThread();
    
    // Member variables
    std::shared_ptr<Settings> settings_;
    std::shared_ptr<DownloadQueue> queue_;
    
    std::map<std::string, std::shared_ptr<DownloadTask>> tasks_;
    mutable std::mutex tasksMutex_;
    
    std::atomic<bool> running_ = false;
    std::unique_ptr<std::thread> queueProcessorThread_;
    
    TaskAddedCallback taskAddedCallback_ = nullptr;
    TaskRemovedCallback taskRemovedCallback_ = nullptr;
    TaskStatusChangedCallback taskStatusChangedCallback_ = nullptr;
};

} // namespace core
} // namespace dm

#endif // DOWNLOAD_MANAGER_H