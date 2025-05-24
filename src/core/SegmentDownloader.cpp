#include "core/SegmentDownloader.h"
#include "utils/Logger.h"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace dm {
namespace core {

SegmentDownloader::SegmentDownloader(const std::string& url, 
                                   const std::string& filePath,
                                   int64_t startByte, 
                                   int64_t endByte,
                                   int id)
    : url_(url), filePath_(filePath), startByte_(startByte), endByte_(endByte), id_(id),
      status_(SegmentStatus::NONE), stopRequested_(false), downloadedBytes_(0), downloadSpeed_(0) {
    
    // Create HTTP client
    httpClient_ = std::make_unique<HttpClient>();
    
    // Initialize timestamps
    lastSpeedUpdateTime_ = std::chrono::system_clock::now();
    
    // Log segment creation
    std::ostringstream log;
    log << "Created segment " << id_ << " for " << url_ 
        << " [" << startByte_ << "-" << endByte_ << "]";
    dm::utils::Logger::debug(log.str());
}

SegmentDownloader::~SegmentDownloader() {
    // Make sure thread is stopped
    if (thread_ && thread_->joinable()) {
        stopRequested_ = true;
        thread_->join();
    }
    
    // Log segment destruction
    std::ostringstream log;
    log << "Destroyed segment " << id_ << " for " << url_;
    dm::utils::Logger::debug(log.str());
}

bool SegmentDownloader::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already running
    if (status_ == SegmentStatus::DOWNLOADING) {
        return true;
    }
    
    // Reset stop flag
    stopRequested_ = false;
    
    // Create download thread
    try {
        thread_ = std::make_unique<std::thread>(&SegmentDownloader::downloadThread, this);
        
        // Log segment start
        std::ostringstream log;
        log << "Started segment " << id_ << " for " << url_;
        dm::utils::Logger::debug(log.str());
        
        return true;
    } catch (const std::exception& e) {
        dm::utils::Logger::error("Failed to create download thread: " + std::string(e.what()));
        return false;
    }
}

bool SegmentDownloader::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if downloading
    if (status_ != SegmentStatus::DOWNLOADING) {
        return false;
    }
    
    // Set stop flag
    stopRequested_ = true;
    
    // Wait for thread to finish
    if (thread_ && thread_->joinable()) {
        thread_->join();
        thread_.reset();
    }
    
    // Set status to paused
    setStatus(SegmentStatus::PAUSED);
    
    // Log segment pause
    std::ostringstream log;
    log << "Paused segment " << id_ << " for " << url_;
    dm::utils::Logger::debug(log.str());
    
    return true;
}

bool SegmentDownloader::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if paused
    if (status_ != SegmentStatus::PAUSED) {
        return false;
    }
    
    // Start download
    return start();
}

bool SegmentDownloader::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Set stop flag
    stopRequested_ = true;
    
    // Wait for thread to finish
    if (thread_ && thread_->joinable()) {
        thread_->join();
        thread_.reset();
    }
    
    // Log segment cancellation
    std::ostringstream log;
    log << "Canceled segment " << id_ << " for " << url_;
    dm::utils::Logger::debug(log.str());
    
    return true;
}

void SegmentDownloader::setCompletionCallback(SegmentCompletionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    completionCallback_ = callback;
}

void SegmentDownloader::setErrorCallback(SegmentErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    errorCallback_ = callback;
}

SegmentStatus SegmentDownloader::getStatus() const {
    return status_;
}

int64_t SegmentDownloader::getDownloadedBytes() const {
    return downloadedBytes_;
}

int64_t SegmentDownloader::getTotalBytes() const {
    return endByte_ - startByte_ + 1;
}

double SegmentDownloader::getDownloadSpeed() const {
    return downloadSpeed_;
}

int SegmentDownloader::getId() const {
    return id_;
}

int64_t SegmentDownloader::getStartByte() const {
    return startByte_;
}

int64_t SegmentDownloader::getEndByte() const {
    return endByte_;
}

const std::string& SegmentDownloader::getFilePath() const {
    return filePath_;
}

const std::string& SegmentDownloader::getUrl() const {
    return url_;
}

void SegmentDownloader::downloadThread() {
    // Set status to downloading
    setStatus(SegmentStatus::DOWNLOADING);
    
    try {
        // Calculate bytes to download
        int64_t bytesToDownload = endByte_ - startByte_ + 1;
        
        // Set downloaded bytes to zero
        downloadedBytes_ = 0;
        lastDownloadedBytes_ = 0;
        
        // Set progress callback
        httpClient_->setProgressCallback(
            [this](int64_t downloadTotal, int64_t downloadedNow, int64_t uploadTotal, int64_t uploadedNow) -> bool {
                return this->onProgress(downloadTotal, downloadedNow, uploadTotal, uploadedNow);
            }
        );
        
        // Download the segment
        bool success = httpClient_->downloadFileSegment(
            url_,
            filePath_,
            startByte_,
            endByte_,
            nullptr // We're using the progress callback directly
        );
        
        // Check if download was successful
        if (success && !stopRequested_) {
            // Set status to completed
            setStatus(SegmentStatus::COMPLETED);
            
            // Set downloaded bytes to total
            downloadedBytes_ = bytesToDownload;
            
            // Call completion callback
            if (completionCallback_) {
                completionCallback_(shared_from_this());
            }
            
            // Log segment completion
            std::ostringstream log;
            log << "Completed segment " << id_ << " for " << url_;
            dm::utils::Logger::debug(log.str());
        } else if (stopRequested_) {
            // Set status to paused
            setStatus(SegmentStatus::PAUSED);
            
            // Log segment pause
            std::ostringstream log;
            log << "Paused segment " << id_ << " for " << url_;
            dm::utils::Logger::debug(log.str());
        } else {
            // Set status to error
            setStatus(SegmentStatus::ERROR);
            
            // Call error callback
            if (errorCallback_) {
                errorCallback_(shared_from_this(), "Download failed");
            }
            
            // Log segment error
            std::ostringstream log;
            log << "Failed to download segment " << id_ << " for " << url_;
            dm::utils::Logger::error(log.str());
        }
    } catch (const std::exception& e) {
        // Set status to error
        setStatus(SegmentStatus::ERROR);
        
        // Call error callback
        if (errorCallback_) {
            errorCallback_(shared_from_this(), e.what());
        }
        
        // Log segment error
        std::ostringstream log;
        log << "Exception in segment " << id_ << " for " << url_ << ": " << e.what();
        dm::utils::Logger::error(log.str());
    }
}

void SegmentDownloader::setStatus(SegmentStatus status) {
    status_ = status;
}

void SegmentDownloader::updateDownloadSpeed() {
    auto now = std::chrono::system_clock::now();
    
    // Calculate time difference in seconds
    double timeDiff = std::chrono::duration<double>(now - lastSpeedUpdateTime_).count();
    
    // Update speed if at least 1 second has passed
    if (timeDiff >= 1.0) {
        // Calculate bytes difference
        int64_t bytesDiff = downloadedBytes_ - lastDownloadedBytes_;
        
        // Calculate speed in bytes per second
        downloadSpeed_ = bytesDiff / timeDiff;
        
        // Update timestamps and previous bytes
        lastSpeedUpdateTime_ = now;
        lastDownloadedBytes_ = downloadedBytes_;
    }
}

bool SegmentDownloader::onProgress(int64_t downloadTotal, int64_t downloadedNow, 
                                 int64_t uploadTotal, int64_t uploadedNow) {
    // Check if download should be stopped
    if (stopRequested_) {
        return false;
    }
    
    // Update downloaded bytes
    downloadedBytes_ = downloadedNow;
    
    // Update download speed
    updateDownloadSpeed();
    
    return true;
}

} // namespace core
} // namespace dm