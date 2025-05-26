#include "include/core/HttpProtocolHandler.h"
#include "core/DownloadTask.h"
#include "include/utils/Logger.h"
#include "include/utils/UrlParser.h"
#include "include/utils/FileUtils.h"

#include <chrono>
#include <thread>
#include <future>
#include <regex>

namespace dm {
namespace core {

HttpProtocolHandler::HttpProtocolHandler() 
    : cookieUpdateInterval_(std::chrono::hours(24)),
      lastCookieUpdate_(std::chrono::system_clock::now()) {
}

HttpProtocolHandler::~HttpProtocolHandler() {
    cleanup();
}

bool HttpProtocolHandler::initialize() {
    utils::Logger::info("Initializing HTTP protocol handler");
    return true;
}

void HttpProtocolHandler::cleanup() {
    utils::Logger::info("Cleaning up HTTP protocol handler");
    std::lock_guard<std::mutex> lock(clientsMutex_);
    activeClients_.clear();
}

std::string HttpProtocolHandler::getProtocolName() const {
    return "HTTP/HTTPS";
}

std::vector<std::string> HttpProtocolHandler::getProtocolSchemes() const {
    return {"http", "https"};
}

bool HttpProtocolHandler::supportsCapability(ProtocolCapability capability) const {
    switch (capability) {
        case ProtocolCapability::RESUME:
        case ProtocolCapability::MULTI_SEGMENT:
        case ProtocolCapability::METADATA:
        case ProtocolCapability::RATE_LIMITING:
        case ProtocolCapability::AUTHENTICATION:
        case ProtocolCapability::PROXYING:
            return true;
        case ProtocolCapability::DIRECTORY_LISTING:
            return true; // Limited support via HTML parsing
        case ProtocolCapability::STREAMING:
        case ProtocolCapability::ENCRYPTION:
        case ProtocolCapability::COMPRESSION:
            return false;
        default:
            return false;
    }
}

bool HttpProtocolHandler::startDownload(const std::string& url, 
                                      const std::string& outputFile,
                                      std::shared_ptr<DownloadTask> task,
                                      const ProtocolOptions& options,
                                      ProtocolProgressCallback progressCallback,
                                      ProtocolErrorCallback errorCallback,
                                      ProtocolStatusCallback statusCallback) {
    if (!task) {
        if (errorCallback) {
            errorCallback("Invalid task");
        }
        utils::Logger::error("[HTTP] Download initiation failed: Invalid task");
        return false;
    }
    
    utils::Logger::info("[HTTP] Download started for URL: " + url);
    logMemoryUsage("[HTTP] Before download: " + url);
    
    std::shared_ptr<HttpClient> client = std::make_shared<HttpClient>();
    configureHttpClient(*client, options);
    
    auto progressFunc = [task, progressCallback, this](int64_t totalBytes, int64_t downloadedBytes, 
                                                    int64_t uploadTotal, int64_t uploadNow) -> bool {
        double speed = 0.0;
        if (downloadedBytes > 0) {
            speed = static_cast<double>(downloadedBytes) / 
                   (std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - task->getStartTime()).count() / 1000.0);
        }
        if (progressCallback) {
            progressCallback(totalBytes, downloadedBytes, speed);
        }
        return task->getStatus() != DownloadStatus::CANCELED && 
               task->getStatus() != DownloadStatus::PAUSED;
    };
    client->setProgressCallback(progressFunc);
    
    auto dataFunc = [outputFile, task](const char* data, size_t size) -> bool {
        FILE* file = fopen(outputFile.c_str(), "ab");
        if (!file) {
            dm::utils::Logger::error("[HTTP] File open failed: " + outputFile);
            return false;
        }
        size_t written = fwrite(data, 1, size, file);
        fclose(file);
        if (written != size) {
            dm::utils::Logger::error("[HTTP] File write failed: " + outputFile);
        }
        return (written == size) && 
               (task->getStatus() != DownloadStatus::CANCELED) && 
               (task->getStatus() != DownloadStatus::PAUSED);
    };
    client->setDataCallback(dataFunc);
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        activeClients_[task->getId()] = client;
    }
    // Retry logic
    int maxRetries = 3;
    std::thread downloadThread([=, this]() {
        int attempt = 0;
        bool success = false;
        std::string lastError;
        while (attempt < maxRetries && !success) {
            attempt++;
            try {
                if (statusCallback) statusCallback("Attempt " + std::to_string(attempt) + " of " + std::to_string(maxRetries));
                utils::Logger::info("[HTTP] Download attempt " + std::to_string(attempt) + " for URL: " + url);
                bool resumeSupport = supportsRangeRequests(url, options);
                int64_t startByte = 0;
                if (resumeSupport && utils::FileUtils::fileExists(outputFile)) {
                    startByte = utils::FileUtils::getFileSize(outputFile);
                    if (statusCallback) statusCallback("Resuming download from byte " + std::to_string(startByte));
                } else {
                    FILE* file = fopen(outputFile.c_str(), "wb");
                    if (file) fclose(file);
                    else {
                        lastError = "Failed to create output file: " + outputFile;
                        utils::Logger::error("[HTTP] " + lastError);
                        if (errorCallback) errorCallback(lastError);
                        break;
                    }
                }
                HttpResponse response;
                if (startByte > 0) response = client->getRange(url, startByte, -1);
                else response = client->get(url);
                if (!handleHttpResponse(response, url, errorCallback)) {
                    lastError = "HTTP response error for: " + url;
                    utils::Logger::error("[HTTP] " + lastError);
                    if (attempt < maxRetries) {
                        utils::Logger::warning("[HTTP] Retrying download for: " + url);
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    }
                    continue;
                }
                success = true;
            } catch (const std::exception& ex) {
                lastError = std::string("Exception: ") + ex.what();
                utils::Logger::error("[HTTP] Exception during download: " + lastError);
                if (attempt < maxRetries) {
                    utils::Logger::warning("[HTTP] Retrying after exception for: " + url);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
            } catch (...) {
                lastError = "Unknown exception during download.";
                utils::Logger::error("[HTTP] " + lastError);
                if (attempt < maxRetries) {
                    utils::Logger::warning("[HTTP] Retrying after unknown exception for: " + url);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
            }
        }
        logMemoryUsage("[HTTP] After download: " + url);
        std::lock_guard<std::mutex> lock(clientsMutex_);
        activeClients_.erase(task->getId());
        if (success) {
            utils::Logger::info("[HTTP] Download complete for URL: " + url);
            if (statusCallback) statusCallback("Download complete");
        } else {
            utils::Logger::error("[HTTP] Download failed for URL: " + url + ". Error: " + lastError);
            if (errorCallback) errorCallback("Download failed: " + lastError);
            if (statusCallback) statusCallback("Download failed");
        }
    });
    downloadThread.detach();
    return true;
}

bool HttpProtocolHandler::pauseDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    utils::Logger::info("Pausing HTTP download: " + task->getUrl());
    
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = activeClients_.find(task->getId());
    if (it != activeClients_.end()) {
        it->second->abort();
        return true;
    }
    
    return false;
}

bool HttpProtocolHandler::resumeDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    utils::Logger::info("Resuming HTTP download: " + task->getUrl());
    
    // Check if we can resume
    if (!supportsRangeRequests(task->getUrl(), ProtocolOptions())) {
        utils::Logger::warning("Server does not support resume for: " + task->getUrl());
        return false;
    }
    
    // Start a new download
    return startDownload(task->getUrl(), task->getDestinationPath() + "/" + task->getFilename(), 
                       task, ProtocolOptions(), nullptr, nullptr, nullptr);
}

bool HttpProtocolHandler::cancelDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    utils::Logger::info("Canceling HTTP download: " + task->getUrl());
    
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = activeClients_.find(task->getId());
    if (it != activeClients_.end()) {
        it->second->abort();
        activeClients_.erase(it);
        return true;
    }
    
    return false;
}

int64_t HttpProtocolHandler::getFileSize(const std::string& url, const ProtocolOptions& options) {
    utils::Logger::debug("Getting file size for: " + url);
    
    HttpClient client;
    configureHttpClient(client, options);
    
    return client.getContentLength(url);
}

std::map<std::string, std::string> HttpProtocolHandler::getMetadata(const std::string& url, 
                                                                 const ProtocolOptions& options) {
    utils::Logger::debug("Getting metadata for: " + url);
    
    std::map<std::string, std::string> metadata;
    
    HttpClient client;
    configureHttpClient(client, options);
    
    // Perform a HEAD request to get headers
    HttpResponse response = client.head(url);
    
    if (response.success) {
        // Extract interesting headers
        for (const auto& header : response.headers) {
            if (header.first == "Content-Type") {
                metadata["content-type"] = header.second;
            } else if (header.first == "Content-Length") {
                metadata["content-length"] = header.second;
            } else if (header.first == "Last-Modified") {
                metadata["last-modified"] = header.second;
            } else if (header.first == "ETag") {
                metadata["etag"] = header.second;
            } else if (header.first == "Content-Disposition") {
                metadata["content-disposition"] = header.second;
                
                // Try to extract filename from Content-Disposition
                std::regex fileNameRegex("filename[^;=\\n]*=((['\"]).*?\\2|[^;\\n]*)");
                std::smatch matches;
                if (std::regex_search(header.second, matches, fileNameRegex) && matches.size() > 1) {
                    std::string fileName = matches[1].str();
                    // Remove quotes if present
                    if (fileName.size() >= 2 && 
                        (fileName[0] == '"' || fileName[0] == '\'') && 
                        fileName[0] == fileName[fileName.size() - 1]) {
                        fileName = fileName.substr(1, fileName.size() - 2);
                    }
                    metadata["filename"] = fileName;
                }
            }
        }
    }
    
    return metadata;
}

std::vector<DirectoryEntry> HttpProtocolHandler::listDirectory(const std::string& url, 
                                                            const ProtocolOptions& options) {
    utils::Logger::debug("Listing directory: " + url);
    
    HttpClient client;
    configureHttpClient(client, options);
    
    HttpResponse response = client.get(url);
    
    std::vector<DirectoryEntry> entries;
    
    if (response.success) {
        std::string html(response.body.begin(), response.body.end());
        entries = parseDirectoryListing(html, url);
    }
    
    return entries;
}

bool HttpProtocolHandler::authenticate(const std::string& url, 
                                     const AuthInfo& authInfo, 
                                     const ProtocolOptions& options) {
    utils::Logger::debug("Authenticating with: " + url);
    
    HttpClient client;
    configureHttpClient(client, options);
    
    if (authInfo.method == AuthMethod::BASIC || 
        authInfo.method == AuthMethod::DIGEST) {
        // Set authentication headers
        client.setHeader("Authorization", "Basic " + 
                       utils::UrlParser::encode(authInfo.username + ":" + authInfo.password));
    }
    
    // Try a HEAD request to check authentication
    HttpResponse response = client.head(url);
    
    return response.success && response.statusCode != 401 && response.statusCode != 403;
}

bool HttpProtocolHandler::supportsUrl(const std::string& url) const {
    utils::UrlInfo urlInfo = utils::UrlParser::parse(url);
    
    for (const auto& scheme : getProtocolSchemes()) {
        if (urlInfo.protocol == scheme) {
            return true;
        }
    }
    
    return false;
}

bool HttpProtocolHandler::supportsRangeRequests(const std::string& url, const ProtocolOptions& options) {
    utils::Logger::debug("Checking range support for: " + url);
    
    HttpClient client;
    configureHttpClient(client, options);
    
    return client.supportsRangeRequests(url);
}

std::string HttpProtocolHandler::getLastModifiedTime(const std::string& url, const ProtocolOptions& options) {
    utils::Logger::debug("Getting last modified time for: " + url);
    
    HttpClient client;
    configureHttpClient(client, options);
    
    return client.getLastModified(url);
}

std::string HttpProtocolHandler::getContentType(const std::string& url, const ProtocolOptions& options) {
    utils::Logger::debug("Getting content type for: " + url);
    
    std::map<std::string, std::string> metadata = getMetadata(url, options);
    
    auto it = metadata.find("content-type");
    if (it != metadata.end()) {
        return it->second;
    }
    
    return "";
}

std::string HttpProtocolHandler::getETag(const std::string& url, const ProtocolOptions& options) {
    utils::Logger::debug("Getting ETag for: " + url);
    
    std::map<std::string, std::string> metadata = getMetadata(url, options);
    
    auto it = metadata.find("etag");
    if (it != metadata.end()) {
        return it->second;
    }
    
    return "";
}

void HttpProtocolHandler::configureHttpClient(HttpClient& client, const ProtocolOptions& options) {
    // Set User-Agent
    if (!options.userAgent.empty()) {
        client.setUserAgent(options.userAgent);
    }
    
    // Set timeout
    if (options.timeout > 0) {
        client.setTimeout(options.timeout);
    }
    
    // Set whether to follow redirects
    client.followRedirects(options.followRedirects);
    
    // Set custom headers
    for (const auto& header : options.headers) {
        client.setHeader(header.first, header.second);
    }
}

bool HttpProtocolHandler::handleHttpResponse(const HttpResponse& response, 
                                          const std::string& url,
                                          ProtocolErrorCallback errorCallback) {
    if (!response.success) {
        std::string errorMsg = "HTTP request failed: " + response.error;
        utils::Logger::error(errorMsg);
        
        if (errorCallback) {
            errorCallback(errorMsg);
        }
        
        return false;
    }
    
    // Check status code
    if (response.statusCode < 200 || response.statusCode >= 300) {
        std::string errorMsg = "HTTP error: " + std::to_string(response.statusCode);
        utils::Logger::error(errorMsg + " for URL: " + url);
        
        if (errorCallback) {
            errorCallback(errorMsg);
        }
        
        return false;
    }
    
    return true;
}

std::vector<DirectoryEntry> HttpProtocolHandler::parseDirectoryListing(const std::string& html, 
                                                                    const std::string& baseUrl) {
    std::vector<DirectoryEntry> entries;
    std::vector<std::string> links = extractLinks(html, baseUrl);
    
    for (const auto& link : links) {
        DirectoryEntry entry;
        entry.name = utils::UrlParser::extractFilename(link);
        entry.url = link;
        
        // Try to determine if it's a directory based on URL ending with '/'
        entry.isDirectory = (link.back() == '/');
        
        entries.push_back(entry);
    }
    
    return entries;
}

std::vector<std::string> HttpProtocolHandler::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    std::set<std::string> uniqueLinks; // To avoid duplicates
    
    // Regular expression to extract href attributes
    std::regex linkRegex("<a[^>]*href=[\"']([^\"']*)[\"'][^>]*>", std::regex::icase);
    
    auto linksBegin = std::sregex_iterator(html.begin(), html.end(), linkRegex);
    auto linksEnd = std::sregex_iterator();
    
    for (std::sregex_iterator i = linksBegin; i != linksEnd; ++i) {
        std::smatch match = *i;
        std::string link = match[1].str();
        
        // Skip JavaScript links, anchors, etc.
        if (link.empty() || 
            link.find("javascript:") == 0 || 
            link.find("#") == 0 || 
            link.find("mailto:") == 0) {
            continue;
        }
        
        // Convert relative URLs to absolute
        std::string absoluteUrl = relativeToAbsoluteUrl(link, baseUrl);
        
        // Add unique links only
        if (uniqueLinks.find(absoluteUrl) == uniqueLinks.end()) {
            uniqueLinks.insert(absoluteUrl);
            links.push_back(absoluteUrl);
        }
    }
    
    return links;
}

std::string HttpProtocolHandler::relativeToAbsoluteUrl(const std::string& relativeUrl, const std::string& baseUrl) {
    // If the URL is already absolute, return it
    if (relativeUrl.find("http://") == 0 || relativeUrl.find("https://") == 0) {
        return relativeUrl;
    }
    
    // Parse base URL
    utils::UrlInfo baseUrlInfo = utils::UrlParser::parse(baseUrl);
    
    // Handle absolute path (starting with /)
    if (!relativeUrl.empty() && relativeUrl[0] == '/') {
        return baseUrlInfo.protocol + "://" + baseUrlInfo.host + 
               (baseUrlInfo.port.empty() ? "" : ":" + baseUrlInfo.port) + 
               relativeUrl;
    }
    
    // Handle relative path
    // Get directory part of the base URL path
    std::string basePath = baseUrlInfo.path;
    
    // If the base URL doesn't end with a slash and it's not the root path,
    // we need to remove the file part
    if (!basePath.empty() && basePath.back() != '/' && basePath != "/") {
        size_t lastSlash = basePath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            basePath = basePath.substr(0, lastSlash + 1);
        } else {
            basePath = "/";
        }
    }
    
    // If base path doesn't end with a slash, add it
    if (!basePath.empty() && basePath.back() != '/') {
        basePath += '/';
    }
    
    return baseUrlInfo.protocol + "://" + baseUrlInfo.host + 
           (baseUrlInfo.port.empty() ? "" : ":" + baseUrlInfo.port) + 
           basePath + relativeUrl;
}

} // namespace core
} // namespace dm