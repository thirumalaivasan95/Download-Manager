#include "../../include/core/BatchDownloader.h"
#include "../../include/core/DownloadManager.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/UrlParser.h"
#include "../../include/utils/FileUtils.h"

#include <fstream>
#include <regex>
#include <thread>
#include <algorithm>

namespace DownloadManager {
namespace Core {

BatchDownloader::BatchDownloader() :
    m_isRunning(false),
    m_maxConcurrentDownloads(3),
    m_parsePatterns(true),
    m_maxTotalDownloads(-1),
    m_processingThread(nullptr)
{
}

BatchDownloader::~BatchDownloader() {
    stop();
}

bool BatchDownloader::addBatchUrls(const std::vector<std::string>& urls, const std::string& destinationDir) {
    if (urls.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // Parse each URL
    for (const auto& url : urls) {
        BatchItem item;
        item.url = url;
        item.status = BatchItemStatus::PENDING;
        item.destinationDir = destinationDir;
        
        // Parse URL for filename if possible
        Utils::UrlParser urlParser;
        auto parsedUrl = urlParser.parse(url);
        
        if (parsedUrl.valid && !parsedUrl.path.empty()) {
            size_t lastSlash = parsedUrl.path.find_last_of('/');
            if (lastSlash != std::string::npos && lastSlash < parsedUrl.path.length() - 1) {
                item.suggestedFilename = parsedUrl.path.substr(lastSlash + 1);
            }
        }
        
        m_batchItems.push_back(item);
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Added " + std::to_string(urls.size()) + " URLs to batch queue");
    
    return true;
}

bool BatchDownloader::addBatchFromFile(const std::string& filePath, const std::string& destinationDir) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to open batch file: " + filePath);
            return false;
        }
        
        std::vector<std::string> urls;
        std::string line;
        
        while (std::getline(file, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Add valid URL to list
            if (isValidUrl(line)) {
                urls.push_back(line);
            } else {
                Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                    "Skipping invalid URL in batch file: " + line);
            }
        }
        
        file.close();
        
        // Add valid URLs to the batch
        if (!urls.empty()) {
            return addBatchUrls(urls, destinationDir);
        } else {
            Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                "No valid URLs found in batch file: " + filePath);
            return false;
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception reading batch file: " + std::string(e.what()));
        return false;
    }
}

bool BatchDownloader::addBatchFromPattern(const std::string& patternUrl, int start, int end, 
                                           int step, int padding, const std::string& destinationDir) {
    if (!m_parsePatterns) {
        Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
            "Pattern parsing is disabled");
        return false;
    }
    
    // Validate parameters
    if (start > end || step <= 0) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Invalid pattern parameters");
        return false;
    }
    
    try {
        // Find the pattern placeholder in the URL
        std::string pattern = "{$PATTERN}";
        size_t patternPos = patternUrl.find(pattern);
        
        if (patternPos == std::string::npos) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Pattern placeholder not found in URL: " + patternUrl);
            return false;
        }
        
        std::vector<std::string> urls;
        
        // Generate URLs based on the pattern
        for (int i = start; i <= end; i += step) {
            std::string formattedNumber;
            
            if (padding > 0) {
                // Format number with padding
                std::ostringstream ss;
                ss << std::setw(padding) << std::setfill('0') << i;
                formattedNumber = ss.str();
            } else {
                formattedNumber = std::to_string(i);
            }
            
            // Replace pattern in URL
            std::string url = patternUrl;
            url.replace(patternPos, pattern.length(), formattedNumber);
            
            // Validate URL
            if (isValidUrl(url)) {
                urls.push_back(url);
            } else {
                Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                    "Skipping invalid pattern-generated URL: " + url);
            }
        }
        
        // Add generated URLs to the batch
        if (!urls.empty()) {
            return addBatchUrls(urls, destinationDir);
        } else {
            Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                "No valid URLs generated from pattern");
            return false;
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in pattern generation: " + std::string(e.what()));
        return false;
    }
}

void BatchDownloader::start() {
    if (m_isRunning || m_batchItems.empty()) {
        return;
    }
    
    m_isRunning = true;
    m_processingThread = std::make_unique<std::thread>(&BatchDownloader::processQueue, this);
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Batch downloader started with " + std::to_string(m_batchItems.size()) + " items");
}

void BatchDownloader::stop() {
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    if (m_processingThread && m_processingThread->joinable()) {
        m_processingThread->join();
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Batch downloader stopped");
}

void BatchDownloader::pause() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_isPaused = true;
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Batch downloader paused");
}

void BatchDownloader::resume() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_isPaused = false;
    m_queueCV.notify_all();
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Batch downloader resumed");
}

bool BatchDownloader::isRunning() const {
    return m_isRunning;
}

bool BatchDownloader::isPaused() const {
    return m_isPaused;
}

void BatchDownloader::setMaxConcurrentDownloads(int max) {
    m_maxConcurrentDownloads = max > 0 ? max : 1;
}

int BatchDownloader::getMaxConcurrentDownloads() const {
    return m_maxConcurrentDownloads;
}

void BatchDownloader::setMaxTotalDownloads(int max) {
    m_maxTotalDownloads = max;
}

int BatchDownloader::getMaxTotalDownloads() const {
    return m_maxTotalDownloads;
}

void BatchDownloader::setParsePatterns(bool enable) {
    m_parsePatterns = enable;
}

bool BatchDownloader::getParsePatterns() const {
    return m_parsePatterns;
}

int BatchDownloader::getTotalItems() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<int>(m_batchItems.size());
}

int BatchDownloader::getCompletedItems() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return std::count_if(m_batchItems.begin(), m_batchItems.end(), 
                        [](const BatchItem& item) { 
                            return item.status == BatchItemStatus::COMPLETED; 
                        });
}

int BatchDownloader::getFailedItems() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return std::count_if(m_batchItems.begin(), m_batchItems.end(), 
                        [](const BatchItem& item) { 
                            return item.status == BatchItemStatus::FAILED; 
                        });
}

int BatchDownloader::getPendingItems() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return std::count_if(m_batchItems.begin(), m_batchItems.end(), 
                        [](const BatchItem& item) { 
                            return item.status == BatchItemStatus::PENDING; 
                        });
}

int BatchDownloader::getActiveItems() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return std::count_if(m_batchItems.begin(), m_batchItems.end(), 
                        [](const BatchItem& item) { 
                            return item.status == BatchItemStatus::ACTIVE; 
                        });
}

std::vector<BatchItem> BatchDownloader::getBatchItems() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_batchItems;
}

void BatchDownloader::clearQueue() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_batchItems.clear();
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Batch queue cleared");
}

void BatchDownloader::removeItem(int index) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    if (index >= 0 && index < static_cast<int>(m_batchItems.size())) {
        // If the item is active, we can't safely remove it
        if (m_batchItems[index].status == BatchItemStatus::ACTIVE) {
            Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                "Cannot remove active batch item");
            return;
        }
        
        m_batchItems.erase(m_batchItems.begin() + index);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Removed item " + std::to_string(index) + " from batch queue");
    }
}

void BatchDownloader::addBatchProgressCallback(const BatchProgressCallback& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_progressCallbacks.push_back(callback);
}

void BatchDownloader::addBatchItemCallback(const BatchItemCallback& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_itemCallbacks.push_back(callback);
}

void BatchDownloader::processQueue() {
    DownloadManager& downloadManager = DownloadManager::instance();
    int totalProcessed = 0;
    
    while (m_isRunning) {
        // Check if we've reached the maximum number of downloads
        if (m_maxTotalDownloads > 0 && totalProcessed >= m_maxTotalDownloads) {
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "Reached maximum total downloads limit: " + std::to_string(m_maxTotalDownloads));
            break;
        }
        
        // Wait if paused
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_isPaused) {
                m_queueCV.wait(lock, [this] { return !m_isPaused || !m_isRunning; });
                if (!m_isRunning) break;
            }
        }
        
        // Find next pending item and active count
        int activeCount = 0;
        int nextIndex = -1;
        
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            
            for (size_t i = 0; i < m_batchItems.size(); i++) {
                if (m_batchItems[i].status == BatchItemStatus::ACTIVE) {
                    activeCount++;
                } 
                else if (m_batchItems[i].status == BatchItemStatus::PENDING && nextIndex == -1) {
                    nextIndex = static_cast<int>(i);
                }
            }
            
            // No more pending items
            if (nextIndex == -1) {
                // If there are no active items either, we're done
                if (activeCount == 0) {
                    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                        "Batch download queue completed");
                    break;
                }
                
                // Wait for active downloads to finish
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
            
            // Check if we can start more downloads
            if (activeCount >= m_maxConcurrentDownloads) {
                // Wait and check again
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
            
            // Set item as active
            m_batchItems[nextIndex].status = BatchItemStatus::ACTIVE;
        }
        
        // Get the next item
        BatchItem& item = m_batchItems[nextIndex];
        
        // Notify progress
        notifyBatchProgress();
        
        // Notify item started
        notifyBatchItem(nextIndex, BatchItemStatus::ACTIVE);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Starting batch download for URL: " + item.url);
        
        // Create download options
        DownloadOptions options;
        options.url = item.url;
        
        // If destination directory is not specified, use default
        if (item.destinationDir.empty()) {
            options.destination = downloadManager.getDefaultDownloadDirectory() + "/" + item.suggestedFilename;
        } else {
            // Ensure directory path ends with a separator
            std::string dir = item.destinationDir;
            if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') {
                dir += '/';
            }
            
            options.destination = dir + item.suggestedFilename;
        }
        
        try {
            // Add download to manager
            std::shared_ptr<DownloadTask> task = downloadManager.addDownload(options);
            
            if (task) {
                // Store task ID for later reference
                item.taskId = task->getId();
                
                // Wait for download to complete
                downloadManager.waitForDownload(task->getId());
                
                // Check final status
                DownloadStatus status = task->getStatus();
                
                if (status == DownloadStatus::COMPLETED) {
                    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                        "Batch download completed for URL: " + item.url);
                    
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    item.status = BatchItemStatus::COMPLETED;
                    totalProcessed++;
                } 
                else {
                    Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                        "Batch download failed for URL: " + item.url + 
                        " (Status: " + std::to_string(static_cast<int>(status)) + ")");
                    
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    item.status = BatchItemStatus::FAILED;
                    item.errorMessage = "Download failed with status: " + 
                                     std::to_string(static_cast<int>(status));
                }
            } 
            else {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to create download task for URL: " + item.url);
                
                std::lock_guard<std::mutex> lock(m_queueMutex);
                item.status = BatchItemStatus::FAILED;
                item.errorMessage = "Failed to create download task";
            }
        } 
        catch (const std::exception& e) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Exception in batch download: " + std::string(e.what()));
            
            std::lock_guard<std::mutex> lock(m_queueMutex);
            item.status = BatchItemStatus::FAILED;
            item.errorMessage = e.what();
        }
        
        // Notify item completed or failed
        notifyBatchItem(nextIndex, item.status);
        
        // Notify overall progress
        notifyBatchProgress();
    }
    
    m_isRunning = false;
}

bool BatchDownloader::isValidUrl(const std::string& url) {
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(url);
    return parsedUrl.valid;
}

void BatchDownloader::notifyBatchProgress() {
    int total = getTotalItems();
    int completed = getCompletedItems();
    int failed = getFailedItems();
    int active = getActiveItems();
    int pending = getPendingItems();
    
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_progressCallbacks) {
        callback(total, completed, failed, active, pending);
    }
}

void BatchDownloader::notifyBatchItem(int index, BatchItemStatus status) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_itemCallbacks) {
        callback(index, status);
    }
}

} // namespace Core
} // namespace DownloadManager