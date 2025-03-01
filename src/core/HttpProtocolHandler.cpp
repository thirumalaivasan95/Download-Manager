#include "../../include/core/HttpProtocolHandler.h"
#include "../../include/core/HttpClient.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/UrlParser.h"

namespace DownloadManager {
namespace Core {

HttpProtocolHandler::HttpProtocolHandler() : ProtocolHandler("HTTP/HTTPS") {
    m_httpClient = std::make_unique<HttpClient>();
}

HttpProtocolHandler::~HttpProtocolHandler() {
}

std::vector<std::string> HttpProtocolHandler::getSupportedSchemes() const {
    return {"http", "https"};
}

bool HttpProtocolHandler::canHandleUrl(const std::string& url) const {
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(url);
    
    return parsedUrl.valid && supportsScheme(parsedUrl.scheme);
}

std::shared_ptr<DownloadTask> HttpProtocolHandler::createDownloadTask(const std::string& url, const std::string& destination) {
    // Validate URL
    if (!canHandleUrl(url)) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "URL not supported by HTTP handler: " + url);
        return nullptr;
    }
    
    // Create download task
    auto task = std::make_shared<DownloadTask>(url, destination);
    
    // Initialize HTTP-specific task parameters
    task->setProtocolType("HTTP");
    
    return task;
}

bool HttpProtocolHandler::getFileInfo(const std::string& url, DownloadFileInfo& fileInfo) {
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Getting file info for: " + url);
        
        // Send HEAD request to get file info
        HttpResponse response = m_httpClient->sendHead(url);
        
        if (response.statusCode >= 200 && response.statusCode < 300) {
            // Extract content length
            if (response.headers.find("Content-Length") != response.headers.end()) {
                fileInfo.size = std::stoll(response.headers["Content-Length"]);
            } else {
                fileInfo.size = -1; // Unknown size
            }
            
            // Extract last modified time
            if (response.headers.find("Last-Modified") != response.headers.end()) {
                fileInfo.lastModified = response.headers["Last-Modified"];
            }
            
            // Extract content type
            if (response.headers.find("Content-Type") != response.headers.end()) {
                fileInfo.contentType = response.headers["Content-Type"];
            }
            
            // Extract filename from Content-Disposition or from URL
            if (response.headers.find("Content-Disposition") != response.headers.end()) {
                std::string disposition = response.headers["Content-Disposition"];
                size_t filenamePos = disposition.find("filename=");
                
                if (filenamePos != std::string::npos) {
                    filenamePos += 9; // Length of "filename="
                    
                    // Extract filename (handle quoted filenames)
                    if (disposition[filenamePos] == '"') {
                        filenamePos++; // Skip opening quote
                        size_t endQuote = disposition.find('"', filenamePos);
                        if (endQuote != std::string::npos) {
                            fileInfo.filename = disposition.substr(filenamePos, endQuote - filenamePos);
                        }
                    } else {
                        // Non-quoted filename
                        size_t endPos = disposition.find(';', filenamePos);
                        if (endPos == std::string::npos) {
                            endPos = disposition.length();
                        }
                        fileInfo.filename = disposition.substr(filenamePos, endPos - filenamePos);
                    }
                }
            }
            
            // If filename wasn't extracted from Content-Disposition, get it from URL
            if (fileInfo.filename.empty()) {
                Utils::UrlParser urlParser;
                auto parsedUrl = urlParser.parse(url);
                
                // Use the last part of the path as filename
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
            }
            
            // Set resumable flag based on Accept-Ranges header
            fileInfo.resumable = (response.headers.find("Accept-Ranges") != response.headers.end() && 
                                 response.headers["Accept-Ranges"] == "bytes");
            
            return true;
        } else {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to get file info, status code: " + std::to_string(response.statusCode));
            return false;
        }
    } catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in getFileInfo: " + std::string(e.what()));
        return false;
    }
}

bool HttpProtocolHandler::download(std::shared_ptr<DownloadTask> task, const ProgressCallback& progressCallback) {
    if (!task) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, "Invalid download task");
        return false;
    }
    
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Starting HTTP download for: " + task->getUrl());
        
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
        
        // Set up request headers
        HttpRequestOptions options;
        
        // Add range header for resumable downloads
        if (task->isResumable() && task->getDownloadedSize() > 0) {
            options.headers["Range"] = "bytes=" + std::to_string(task->getDownloadedSize()) + "-";
        }
        
        // Set up progress callback
        options.progressCallback = [task, progressCallback](int64_t bytesReceived, int64_t totalBytes) {
            int64_t downloadedSize = task->getDownloadedSize() + bytesReceived;
            task->setDownloadedBytes(downloadedSize);
            
            if (progressCallback) {
                progressCallback(downloadedSize, totalBytes > 0 ? totalBytes : task->getFileSize());
            }
            
            return true; // Continue download
        };
        
        // Set up data callback to write to file
        options.dataCallback = [task](const char* data, size_t size) {
            // Write data to file at current position
            // This would typically use FileManager to handle the file operations
            return true;
        };
        
        // Send GET request
        HttpResponse response = m_httpClient->sendGet(task->getUrl(), options);
        
        // Check if download was successful
        if (response.statusCode >= 200 && response.statusCode < 300) {
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "HTTP download completed successfully: " + task->getUrl());
            return true;
        } else {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "HTTP download failed, status code: " + std::to_string(response.statusCode));
            return false;
        }
    } catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in download: " + std::string(e.what()));
        return false;
    }
}

bool HttpProtocolHandler::cancelDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    // Cancel the ongoing download through HTTP client
    m_httpClient->cancelRequest(task->getUrl());
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "HTTP download canceled: " + task->getUrl());
    
    return true;
}

bool HttpProtocolHandler::pauseDownload(std::shared_ptr<DownloadTask> task) {
    if (!task || !task->isResumable()) {
        return false;
    }
    
    // Pause the download by canceling current request
    // The download can be resumed later because we've already marked it as resumable
    m_httpClient->cancelRequest(task->getUrl());
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "HTTP download paused: " + task->getUrl());
    
    return true;
}

} // namespace Core
} // namespace DownloadManager