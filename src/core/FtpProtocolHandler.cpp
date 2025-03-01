#include "../../include/core/FtpProtocolHandler.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/UrlParser.h"

#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <cstring>

namespace DownloadManager {
namespace Core {

// Structure to hold FTP transfer data
struct FtpTransferData {
    std::shared_ptr<DownloadTask> task;
    std::ofstream* outputFile;
    ProgressCallback progressCallback;
    int64_t downloadedSize;
    std::mutex mutex;
};

// Callback function for receiving data from libcurl
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    FtpTransferData* transferData = static_cast<FtpTransferData*>(userp);
    
    // Lock to ensure thread safety when updating shared data
    std::lock_guard<std::mutex> lock(transferData->mutex);
    
    if (transferData && transferData->outputFile) {
        transferData->outputFile->write(static_cast<char*>(contents), realSize);
        transferData->downloadedSize += realSize;
        
        // Update task progress
        if (transferData->task) {
            transferData->task->setDownloadedBytes(transferData->downloadedSize);
        }
        
        // Call progress callback if provided
        if (transferData->progressCallback) {
            transferData->progressCallback(transferData->downloadedSize, transferData->task->getFileSize());
        }
    }
    
    return realSize;
}

// Callback function for progress updates from libcurl
static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    FtpTransferData* transferData = static_cast<FtpTransferData*>(clientp);
    
    // Call progress callback if provided
    if (transferData && transferData->progressCallback) {
        transferData->progressCallback(dlnow, dltotal);
    }
    
    return 0; // Return 0 to continue, non-zero to abort
}

FtpProtocolHandler::FtpProtocolHandler() : ProtocolHandler("FTP") {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
}

FtpProtocolHandler::~FtpProtocolHandler() {
    // Cleanup libcurl
    curl_global_cleanup();
}

std::vector<std::string> FtpProtocolHandler::getSupportedSchemes() const {
    return {"ftp", "ftps"};
}

bool FtpProtocolHandler::canHandleUrl(const std::string& url) const {
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(url);
    
    return parsedUrl.valid && supportsScheme(parsedUrl.scheme);
}

std::shared_ptr<DownloadTask> FtpProtocolHandler::createDownloadTask(const std::string& url, const std::string& destination) {
    // Validate URL
    if (!canHandleUrl(url)) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "URL not supported by FTP handler: " + url);
        return nullptr;
    }
    
    // Create download task
    auto task = std::make_shared<DownloadTask>(url, destination);
    
    // Initialize FTP-specific task parameters
    task->setProtocolType("FTP");
    
    return task;
}

bool FtpProtocolHandler::getFileInfo(const std::string& url, DownloadFileInfo& fileInfo) {
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Getting file info for: " + url);
        
        // Parse URL to extract components
        Utils::UrlParser urlParser;
        auto parsedUrl = urlParser.parse(url);
        
        if (!parsedUrl.valid) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Invalid URL: " + url);
            return false;
        }
        
        // Initialize CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to initialize libcurl");
            return false;
        }
        
        // Set up CURL options for FTP SIZE command
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 1L);
        
        // Set authentication if provided in URL
        if (!parsedUrl.username.empty()) {
            std::string userpass = parsedUrl.username;
            if (!parsedUrl.password.empty()) {
                userpass += ":" + parsedUrl.password;
            }
            curl_easy_setopt(curl, CURLOPT_USERPWD, userpass.c_str());
        }
        
        // Set timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            // Get file size
            double fileSize;
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fileSize);
            if (res == CURLE_OK && fileSize > 0) {
                fileInfo.size = static_cast<int64_t>(fileSize);
            } else {
                fileInfo.size = -1; // Unknown size
            }
            
            // Get file modification time
            long filetime;
            res = curl_easy_getinfo(curl, CURLINFO_FILETIME, &filetime);
            if (res == CURLE_OK && filetime > 0) {
                // Convert time_t to string
                char timeBuffer[50];
                struct tm* timeinfo = localtime(&filetime);
                strftime(timeBuffer, sizeof(timeBuffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
                fileInfo.lastModified = timeBuffer;
            }
            
            // Extract filename from URL
            if (!parsedUrl.path.empty()) {
                size_t lastSlash = parsedUrl.path.find_last_of('/');
                if (lastSlash != std::string::npos && lastSlash < parsedUrl.path.length() - 1) {
                    fileInfo.filename = parsedUrl.path.substr(lastSlash + 1);
                } else {
                    // Use a default name if we couldn't extract one
                    fileInfo.filename = "download";
                }
            } else {
                fileInfo.filename = "download";
            }
            
            // FTP is generally resumable
            fileInfo.resumable = true;
            
            curl_easy_cleanup(curl);
            return true;
        } else {
            // Handle error
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "libcurl error: " + std::string(curl_easy_strerror(res)));
            curl_easy_cleanup(curl);
            return false;
        }
    } catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in getFileInfo: " + std::string(e.what()));
        return false;
    }
}

bool FtpProtocolHandler::download(std::shared_ptr<DownloadTask> task, const ProgressCallback& progressCallback) {
    if (!task) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, "Invalid download task");
        return false;
    }
    
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Starting FTP download for: " + task->getUrl());
        
        // Get file info if not already available
        if (task->getFileSize() <= 0) {
            DownloadFileInfo fileInfo;
            if (getFileInfo(task->getUrl(), fileInfo)) {
                task->setFileSize(fileInfo.size);
                task->setResumable(fileInfo.resumable);
                
                // Update destination filename if not explicitly set
                if (task->getDestination().find_last_of('/') == task->getDestination().length() - 1) {
                    task->setDestination(task->getDestination() + fileInfo.filename);
                }
            }
        }
        
        // Parse URL to extract components
        Utils::UrlParser urlParser;
        auto parsedUrl = urlParser.parse(task->getUrl());
        
        if (!parsedUrl.valid) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Invalid URL: " + task->getUrl());
            return false;
        }
        
        // Open output file
        std::ofstream outputFile;
        if (task->isResumable() && task->getDownloadedSize() > 0) {
            // Open file for appending (resuming download)
            outputFile.open(task->getDestination(), std::ios::binary | std::ios::app);
        } else {
            // Create new file
            outputFile.open(task->getDestination(), std::ios::binary);
            task->setDownloadedBytes(0);
        }
        
        if (!outputFile.is_open()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to open output file: " + task->getDestination());
            return false;
        }
        
        // Prepare transfer data for callbacks
        FtpTransferData transferData;
        transferData.task = task;
        transferData.outputFile = &outputFile;
        transferData.progressCallback = progressCallback;
        transferData.downloadedSize = task->getDownloadedSize();
        
        // Initialize CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to initialize libcurl");
            outputFile.close();
            return false;
        }
        
        // Set up CURL options
        curl_easy_setopt(curl, CURLOPT_URL, task->getUrl().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &transferData);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &transferData);
        curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 1L);
        
        // Set authentication if provided in URL
        if (!parsedUrl.username.empty()) {
            std::string userpass = parsedUrl.username;
            if (!parsedUrl.password.empty()) {
                userpass += ":" + parsedUrl.password;
            }
            curl_easy_setopt(curl, CURLOPT_USERPWD, userpass.c_str());
        }
        
        // Set resume point if resuming download
        if (task->isResumable() && task->getDownloadedSize() > 0) {
            curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(task->getDownloadedSize()));
        }
        
        // Set timeout
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1000L);  // 1KB/s minimum speed
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);     // Abort if below minimum speed for 30 seconds
        
        // Perform the download
        CURLcode res = curl_easy_perform(curl);
        
        // Close file
        outputFile.close();
        
        // Check result
        if (res == CURLE_OK) {
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "FTP download completed successfully: " + task->getUrl());
            curl_easy_cleanup(curl);
            return true;
        } else {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "FTP download failed: " + std::string(curl_easy_strerror(res)));
            curl_easy_cleanup(curl);
            return false;
        }
    } catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in download: " + std::string(e.what()));
        return false;
    }
}

bool FtpProtocolHandler::cancelDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    // For FTP, cancellation is handled by the download manager by stopping the thread
    // that runs the download() method
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "FTP download canceled: " + task->getUrl());
    
    return true;
}

bool FtpProtocolHandler::pauseDownload(std::shared_ptr<DownloadTask> task) {
    if (!task || !task->isResumable()) {
        return false;
    }
    
    // For FTP, pausing is similar to canceling, but we keep track of the downloaded bytes
    // so we can resume later
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "FTP download paused: " + task->getUrl());
    
    return true;
}

} // namespace Core
} // namespace DownloadManager