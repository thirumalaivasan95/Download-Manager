#include "include/core/DownloadTask.h"
#include "include/core/HttpClient.h"
#include "include/utils/UrlParser.h"
#include "include/utils/Logger.h"
#include "include/utils/FileUtils.h"

#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <thread>
#include <algorithm>
#include <cmath>

namespace dm {
namespace core {

// Helper function to generate a unique ID
std::string generateUniqueId() {
    // Create a random device and engine
    std::random_device rd;
    std::mt19937_64 generator(rd());
    
    // Create a distribution
    std::uniform_int_distribution<uint64_t> distribution;
    
    // Generate a random number
    uint64_t value = distribution(generator);
    
    // Convert to string with fixed width (16 hex characters)
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << value;
    
    return ss.str();
}

DownloadTask::DownloadTask(const std::string& url, 
                         const std::string& destinationPath,
                         const std::string& filename)
    : url_(url), destinationPath_(destinationPath) {
    
    // Generate a unique ID
    id_ = generateUniqueId();
    
    // Parse the URL to extract filename if not provided
    if (filename.empty()) {
        filename_ = dm::utils::UrlParser::extractFilename(url);
        if (filename_.empty()) {
            // Use the ID as fallback filename
            filename_ = "download_" + id_;
        }
    } else {
        filename_ = filename;
    }
    
    // Initialize start time
    startTime_ = std::chrono::system_clock::now();
    lastUpdateTime_ = startTime_;
    
    // Log creation of download task
    dm::utils::Logger::info("Created download task: " + url + " -> " + destinationPath + "/" + filename_);
}

DownloadTask::~DownloadTask() {
    // Make sure all segments are stopped
    cancel();
}

bool DownloadTask::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already initialized
    if (status_ != DownloadStatus::NONE) {
        return true;
    }
    
    try {
        // Check if the server supports range requests
        supportsResume_ = checkRangeSupport();
        
        // Get file size
        HttpClient client;
        fileSize_ = client.getContentLength(url_);
        
        // Initialize the file
        if (!initializeFile()) {
            error_ = "Failed to initialize file";
            setStatus(DownloadStatus::ERROR);
            return false;
        }
        
        // Create metadata file
        if (!createMetadataFile()) {
            dm::utils::Logger::warning("Failed to create metadata file");
            // Continue anyway, metadata is not critical
        }
        
        // Set status to queued
        setStatus(DownloadStatus::QUEUED);
        
        return true;
    } catch (const std::exception& e) {
        error_ = "Initialization error: " + std::string(e.what());
        setStatus(DownloadStatus::ERROR);
        return false;
    }
}

bool DownloadTask::start() {
    // Initialize if needed
    if (status_ == DownloadStatus::NONE && !initialize()) {
        return false;
    }
    
    // Check if already downloading
    if (status_ == DownloadStatus::DOWNLOADING) {
        return true;
    }
    
    // Set status to connecting
    setStatus(DownloadStatus::CONNECTING);
    
    try {
        // Create segments
        if (!createSegments()) {
            error_ = "Failed to create download segments";
            setStatus(DownloadStatus::ERROR);
            return false;
        }
        
        // Update start time
        startTime_ = std::chrono::system_clock::now();
        lastUpdateTime_ = startTime_;
        
        // Set status to downloading
        setStatus(DownloadStatus::DOWNLOADING);
        
        // Start all segments
        for (auto& segment : segments_) {
            segment->start();
        }
        
        return true;
    } catch (const std::exception& e) {
        error_ = "Start error: " + std::string(e.what());
        setStatus(DownloadStatus::ERROR);
        return false;
    }
}

bool DownloadTask::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if downloading
    if (status_ != DownloadStatus::DOWNLOADING) {
        return false;
    }
    
    try {
        // Pause all segments
        for (auto& segment : segments_) {
            segment->pause();
        }
        
        // Set status to paused
        setStatus(DownloadStatus::PAUSED);
        
        return true;
    } catch (const std::exception& e) {
        error_ = "Pause error: " + std::string(e.what());
        return false;
    }
}

bool DownloadTask::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if paused
    if (status_ != DownloadStatus::PAUSED) {
        return false;
    }
    
    try {
        // Resume all segments
        for (auto& segment : segments_) {
            segment->resume();
        }
        
        // Set status to downloading
        setStatus(DownloadStatus::DOWNLOADING);
        
        return true;
    } catch (const std::exception& e) {
        error_ = "Resume error: " + std::string(e.what());
        return false;
    }
}

bool DownloadTask::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already canceled or completed
    if (status_ == DownloadStatus::CANCELED || status_ == DownloadStatus::COMPLETED) {
        return false;
    }
    
    try {
        // Cancel all segments
        for (auto& segment : segments_) {
            segment->cancel();
        }
        
        // Clear segments
        segments_.clear();
        
        // Set status to canceled
        setStatus(DownloadStatus::CANCELED);
        
        return true;
    } catch (const std::exception& e) {
        error_ = "Cancel error: " + std::string(e.what());
        return false;
    }
}

void DownloadTask::setSegmentCount(int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if download has started
    if (status_ != DownloadStatus::NONE && status_ != DownloadStatus::QUEUED) {
        return;
    }
    
    // Set segment count
    segmentCount_ = count;
}

void DownloadTask::setPriority(DownloadPriority priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    priority_ = priority;
}

void DownloadTask::setType(DownloadType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    type_ = type;
}

void DownloadTask::setProgressCallback(TaskProgressCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    progressCallback_ = callback;
}

void DownloadTask::setStatusChangeCallback(StatusChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    statusChangeCallback_ = callback;
}

DownloadStatus DownloadTask::getStatus() const {
    return status_;
}

ProgressInfo DownloadTask::getProgressInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return progressInfo_;
}

const std::string& DownloadTask::getUrl() const {
    return url_;
}

const std::string& DownloadTask::getDestinationPath() const {
    return destinationPath_;
}

const std::string& DownloadTask::getFilename() const {
    return filename_;
}

int64_t DownloadTask::getFileSize() const {
    return fileSize_;
}

const std::string& DownloadTask::getError() const {
    return error_;
}

double DownloadTask::getDownloadSpeed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return progressInfo_.downloadSpeed;
}

std::chrono::system_clock::time_point DownloadTask::getStartTime() const {
    return startTime_;
}

bool DownloadTask::supportsResume() const {
    return supportsResume_;
}

const std::string& DownloadTask::getId() const {
    return id_;
}

DownloadPriority DownloadTask::getPriority() const {
    return priority_;
}

DownloadType DownloadTask::getType() const {
    return type_;
}

void DownloadTask::updateProgress() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if downloading
    if (status_ != DownloadStatus::DOWNLOADING) {
        return;
    }
    
    // Calculate total downloaded bytes
    int64_t totalDownloaded = 0;
    double totalSpeed = 0.0;
    bool allCompleted = true;
    
    for (auto& segment : segments_) {
        totalDownloaded += segment->getDownloadedBytes();
        totalSpeed += segment->getDownloadSpeed();
        
        if (segment->getStatus() != SegmentStatus::COMPLETED) {
            allCompleted = false;
        }
    }
    
    // Update progress info
    progressInfo_.downloadedBytes = totalDownloaded;
    progressInfo_.downloadSpeed = totalSpeed;
    
    // Calculate percentage
    if (fileSize_ > 0) {
        progressInfo_.progressPercent = static_cast<double>(totalDownloaded) / fileSize_ * 100.0;
    } else {
        progressInfo_.progressPercent = 0.0;
    }
    
    // Calculate time elapsed
    auto now = std::chrono::system_clock::now();
    progressInfo_.timeElapsed = 
        std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();
    
    // Calculate estimated time remaining
    if (totalSpeed > 0 && fileSize_ > 0) {
        int64_t bytesRemaining = fileSize_ - totalDownloaded;
        progressInfo_.timeRemaining = static_cast<int64_t>(bytesRemaining / totalSpeed);
    } else {
        progressInfo_.timeRemaining = 0;
    }
    
    // Update speed history for calculating average speed
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime_).count() >= 1) {
        // Add current speed to history
        speedHistory_.push_back(totalSpeed);
        
        // Keep only the last 10 speed measurements
        if (speedHistory_.size() > 10) {
            speedHistory_.erase(speedHistory_.begin());
        }
        
        // Calculate average speed
        if (!speedHistory_.empty()) {
            double sum = 0.0;
            for (double speed : speedHistory_) {
                sum += speed;
            }
            progressInfo_.averageSpeed = sum / speedHistory_.size();
        }
        
        lastUpdateTime_ = now;
    }
    
    // Call progress callback if provided
    if (progressCallback_) {
        progressCallback_(progressInfo_);
    }
    
    // Check if all segments are completed
    if (allCompleted) {
        onTaskCompleted();
    }
}

bool DownloadTask::checkRangeSupport() {
    HttpClient client;
    return client.supportsRangeRequests(url_);
}

bool DownloadTask::initializeFile() {
    // Create destination directory if it doesn't exist
    if (!dm::utils::FileUtils::createDirectory(destinationPath_)) {
        return false;
    }
    
    // If destination path doesn't end with a separator, add one
    std::string fullPath = destinationPath_;
    if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
        fullPath += "/";
    }
    fullPath += filename_;
    
    // Create or truncate the file if it doesn't support resume or if we're starting a new download
    if (!supportsResume_ || status_ == DownloadStatus::NONE) {
        std::ofstream file(fullPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }
        
        // If file size is known, pre-allocate space
        if (fileSize_ > 0) {
            file.seekp(fileSize_ - 1);
            file.put(0);
        }
        
        file.close();
    }
    
    return true;
}

bool DownloadTask::createSegments() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clear existing segments
    segments_.clear();
    
    // If file size is unknown or no range support, use a single segment
    if (fileSize_ <= 0 || !supportsResume_ || segmentCount_ <= 1) {
        auto segment = std::make_shared<SegmentDownloader>(
            url_,
            destinationPath_ + "/" + filename_,
            0,
            fileSize_ - 1,
            0
        );
        
        // Set callbacks
        segment->setCompletionCallback(std::bind(&DownloadTask::onSegmentCompleted, this, std::placeholders::_1));
        segment->setErrorCallback(std::bind(&DownloadTask::onSegmentError, this, std::placeholders::_1, std::placeholders::_2));
        
        segments_.push_back(segment);
    } else {
        // Calculate segment size
        int64_t segmentSize = fileSize_ / segmentCount_;
        
        // Create segments
        for (int i = 0; i < segmentCount_; i++) {
            int64_t startByte = i * segmentSize;
            int64_t endByte;
            
            if (i == segmentCount_ - 1) {
                endByte = fileSize_ - 1;  // Last segment gets all remaining bytes
            } else {
                endByte = (i + 1) * segmentSize - 1;
            }
            
            auto segment = std::make_shared<SegmentDownloader>(
                url_,
                destinationPath_ + "/" + filename_,
                startByte,
                endByte,
                i
            );
            
            // Set callbacks
            segment->setCompletionCallback(std::bind(&DownloadTask::onSegmentCompleted, this, std::placeholders::_1));
            segment->setErrorCallback(std::bind(&DownloadTask::onSegmentError, this, std::placeholders::_1, std::placeholders::_2));
            
            segments_.push_back(segment);
        }
    }
    
    return true;
}

bool DownloadTask::createMetadataFile() {
    // Create metadata file path
    std::string metadataPath = destinationPath_ + "/" + filename_ + ".meta";
    
    // Open file for writing
    std::ofstream file(metadataPath, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    
    // Write metadata
    file << "id=" << id_ << std::endl;
    file << "url=" << url_ << std::endl;
    file << "file_size=" << fileSize_ << std::endl;
    file << "supports_resume=" << (supportsResume_ ? "true" : "false") << std::endl;
    file << "segment_count=" << segmentCount_ << std::endl;
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    file << "timestamp=" << time << std::endl;
    
    file.close();
    return true;
}

bool DownloadTask::loadMetadata() {
    // Not implemented in this version
    return false;
}

void DownloadTask::setStatus(DownloadStatus status) {
    // Update status
    DownloadStatus oldStatus = status_;
    status_ = status;
    
    // Log status change
    dm::utils::Logger::info("Download status changed: " + url_ + " -> " + 
                          std::to_string(static_cast<int>(status)));
    
    // Call status change callback if provided
    if (statusChangeCallback_ && oldStatus != status) {
        statusChangeCallback_(status);
    }
}

void DownloadTask::onTaskCompleted() {
    // Set status to completed
    setStatus(DownloadStatus::COMPLETED);
    
    // Update progress to 100%
    progressInfo_.downloadedBytes = fileSize_;
    progressInfo_.progressPercent = 100.0;
    progressInfo_.downloadSpeed = 0.0;
    progressInfo_.timeRemaining = 0;
    
    // Call progress callback if provided
    if (progressCallback_) {
        progressCallback_(progressInfo_);
    }
    
    // Log completion
    dm::utils::Logger::info("Download completed: " + url_ + " -> " + 
                          destinationPath_ + "/" + filename_);
}

void DownloadTask::onSegmentCompleted(std::shared_ptr<SegmentDownloader> segment) {
    // Log segment completion
    dm::utils::Logger::debug("Segment completed: " + std::to_string(segment->getId()) + 
                           " of download " + id_);
    
    // Check if all segments are completed
    bool allCompleted = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& seg : segments_) {
            if (seg->getStatus() != SegmentStatus::COMPLETED) {
                allCompleted = false;
                break;
            }
        }
    }
    
    // If all segments are completed, mark the task as completed
    if (allCompleted) {
        onTaskCompleted();
    }
}

void DownloadTask::onSegmentError(std::shared_ptr<SegmentDownloader> segment, const std::string& error) {
    // Log segment error
    dm::utils::Logger::error("Segment error: " + std::to_string(segment->getId()) + 
                           " of download " + id_ + " - " + error);
    
    // Set error message
    error_ = "Segment " + std::to_string(segment->getId()) + " error: " + error;
    
    // Set status to error
    setStatus(DownloadStatus::ERROR);
}

} // namespace core
} // namespace dm