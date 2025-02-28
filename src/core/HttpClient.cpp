#include "include/core/HttpClient.h"
#include "include/utils/Logger.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <string>

namespace dm {
namespace core {

// Structure to pass to CURL callbacks
struct CurlCallbackData {
    HttpResponse* response;
    DataCallback dataCallback;
    ProgressCallback progressCallback;
    bool aborted;
    
    CurlCallbackData(HttpResponse* resp) 
        : response(resp), dataCallback(nullptr), progressCallback(nullptr), aborted(false) {}
};

// Callback for receiving data from CURL
size_t HttpClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    CurlCallbackData* data = static_cast<CurlCallbackData*>(userp);
    
    try {
        // Check if the operation has been aborted
        if (data->aborted) {
            return 0; // Abort the transfer
        }
        
        // If there's a data callback, use it
        if (data->dataCallback) {
            if (!data->dataCallback(static_cast<char*>(contents), realSize)) {
                data->aborted = true;
                return 0; // Abort the transfer
            }
        }
        
        // Store the data in the response body
        const char* bytes = static_cast<char*>(contents);
        data->response->body.insert(data->response->body.end(), bytes, bytes + realSize);
        
        return realSize;
    } catch (const std::exception& e) {
        data->response->error = "Exception in write callback: " + std::string(e.what());
        return 0; // Abort the transfer
    }
}

// Callback for receiving headers from CURL
size_t HttpClient::headerCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    CurlCallbackData* data = static_cast<CurlCallbackData*>(userp);
    
    try {
        // Check if the operation has been aborted
        if (data->aborted) {
            return 0; // Abort the transfer
        }
        
        // Parse the header
        std::string header(static_cast<char*>(contents), realSize);
        
        // Trim trailing CR/LF
        header.erase(std::remove(header.begin(), header.end(), '\r'), header.end());
        header.erase(std::remove(header.begin(), header.end(), '\n'), header.end());
        
        // Skip empty headers
        if (header.empty()) {
            return realSize;
        }
        
        // Split header into name and value
        size_t colonPos = header.find(':');
        if (colonPos != std::string::npos) {
            std::string name = header.substr(0, colonPos);
            std::string value = header.substr(colonPos + 1);
            
            // Trim leading/trailing whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Convert header name to lowercase for case-insensitive lookup
            std::transform(name.begin(), name.end(), name.begin(), 
                          [](unsigned char c) { return std::tolower(c); });
            
            // Store the header
            data->response->headers[name] = value;
        }
        
        return realSize;
    } catch (const std::exception& e) {
        data->response->error = "Exception in header callback: " + std::string(e.what());
        return 0; // Abort the transfer
    }
}

// Callback for progress updates from CURL
int HttpClient::progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    CurlCallbackData* data = static_cast<CurlCallbackData*>(clientp);
    
    try {
        // Check if the operation has been aborted
        if (data->aborted) {
            return 1; // Abort the transfer
        }
        
        // If there's a progress callback, use it
        if (data->progressCallback) {
            if (!data->progressCallback(
                static_cast<int64_t>(dltotal),
                static_cast<int64_t>(dlnow),
                static_cast<int64_t>(ultotal),
                static_cast<int64_t>(ulnow))) {
                data->aborted = true;
                return 1; // Abort the transfer
            }
        }
        
        return 0; // Continue the transfer
    } catch (const std::exception& e) {
        data->response->error = "Exception in progress callback: " + std::string(e.what());
        return 1; // Abort the transfer
    }
}

HttpClient::HttpClient() {
    // Initialize CURL
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

HttpClient::~HttpClient() {
    // Clean up CURL
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
}

HttpClient& HttpClient::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
    return *this;
}

HttpClient& HttpClient::setCookie(const std::string& name, const std::string& value) {
    cookies_[name] = value;
    return *this;
}

HttpClient& HttpClient::setTimeout(int timeoutSeconds) {
    timeoutSeconds_ = timeoutSeconds;
    return *this;
}

HttpClient& HttpClient::setUserAgent(const std::string& userAgent) {
    userAgent_ = userAgent;
    return *this;
}

HttpClient& HttpClient::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
    return *this;
}

HttpClient& HttpClient::setDataCallback(DataCallback callback) {
    dataCallback_ = callback;
    return *this;
}

HttpClient& HttpClient::followRedirects(bool follow) {
    followRedirects_ = follow;
    return *this;
}

void HttpClient::setupCurlOptions(CURL* curl, const std::string& url) {
    // Reset the aborted flag
    aborted_ = false;
    
    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Set the user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent_.c_str());
    
    // Set timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds_));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(timeoutSeconds_));
    
    // Follow redirects if requested
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, followRedirects_ ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    
    // Set up headers
    struct curl_slist* headerList = nullptr;
    for (const auto& header : headers_) {
        std::string headerStr = header.first + ": " + header.second;
        headerList = curl_slist_append(headerList, headerStr.c_str());
    }
    
    if (headerList) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
    
    // Set up cookies
    if (!cookies_.empty()) {
        std::string cookieStr;
        for (const auto& cookie : cookies_) {
            if (!cookieStr.empty()) {
                cookieStr += "; ";
            }
            cookieStr += cookie.first + "=" + cookie.second;
        }
        curl_easy_setopt(curl, CURLOPT_COOKIE, cookieStr.c_str());
    }
    
    // Use TLS verification
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Set TCP keepalive
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
}

HttpResponse HttpClient::performRequest(CURL* curl) {
    HttpResponse response;
    CurlCallbackData callbackData(&response);
    
    // Set up callbacks
    callbackData.dataCallback = dataCallback_;
    callbackData.progressCallback = progressCallback_;
    
    // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callbackData);
    
    // Set header callback
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &callbackData);
    
    // Set progress callback if provided
    if (progressCallback_) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &callbackData);
    } else {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    }
    
    // Perform the request
    CURLcode result = curl_easy_perform(curl);
    
    // Get the status code
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    response.statusCode = static_cast<int>(statusCode);
    
    // Check for errors
    if (result != CURLE_OK && !callbackData.aborted) {
        response.error = curl_easy_strerror(result);
        response.success = false;
    } else if (callbackData.aborted) {
        response.error = "Request aborted";
        response.success = false;
    } else {
        response.success = true;
    }
    
    return response;
}

HttpResponse HttpClient::head(const std::string& url) {
    // Log the request
    dm::utils::Logger::debug("HTTP HEAD: " + url);
    
    // Create a new CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        HttpResponse response;
        response.error = "Failed to initialize CURL";
        response.success = false;
        return response;
    }
    
    // Set up CURL options
    setupCurlOptions(curl, url);
    
    // Set HEAD method
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    
    // Perform the request
    HttpResponse response = performRequest(curl);
    
    // Clean up
    curl_easy_cleanup(curl);
    
    // Log the response
    dm::utils::Logger::debug("HTTP Response: " + std::to_string(response.statusCode) + 
                           (response.success ? " (Success)" : " (Error: " + response.error + ")"));
    
    return response;
}

HttpResponse HttpClient::get(const std::string& url) {
    // Log the request
    dm::utils::Logger::debug("HTTP GET: " + url);
    
    // Create a new CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        HttpResponse response;
        response.error = "Failed to initialize CURL";
        response.success = false;
        return response;
    }
    
    // Set up CURL options
    setupCurlOptions(curl, url);
    
    // Set GET method
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    
    // Perform the request
    HttpResponse response = performRequest(curl);
    
    // Clean up
    curl_easy_cleanup(curl);
    
    // Log the response
    dm::utils::Logger::debug("HTTP Response: " + std::to_string(response.statusCode) + 
                           (response.success ? " (Success)" : " (Error: " + response.error + ")"));
    
    return response;
}

HttpResponse HttpClient::getRange(const std::string& url, int64_t startByte, int64_t endByte) {
    // Log the request
    dm::utils::Logger::debug("HTTP GET Range: " + url + " [" + 
                           std::to_string(startByte) + "-" + std::to_string(endByte) + "]");
    
    // Create a new CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        HttpResponse response;
        response.error = "Failed to initialize CURL";
        response.success = false;
        return response;
    }
    
    // Set up CURL options
    setupCurlOptions(curl, url);
    
    // Set GET method
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    
    // Set range
    std::string range = std::to_string(startByte) + "-";
    if (endByte >= 0) {
        range += std::to_string(endByte);
    }
    curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
    
    // Perform the request
    HttpResponse response = performRequest(curl);
    
    // Clean up
    curl_easy_cleanup(curl);
    
    // Log the response
    dm::utils::Logger::debug("HTTP Response: " + std::to_string(response.statusCode) + 
                           (response.success ? " (Success)" : " (Error: " + response.error + ")"));
    
    return response;
}

bool HttpClient::supportsRangeRequests(const std::string& url) {
    // Send a HEAD request to check for range support
    HttpResponse response = head(url);
    
    if (!response.success) {
        return false;
    }
    
    // Check for Accept-Ranges header
    auto it = response.headers.find("accept-ranges");
    if (it != response.headers.end() && it->second == "bytes") {
        return true;
    }
    
    // Try with a small range request
    response = getRange(url, 0, 1);
    
    // Check if the response is partial content (206)
    return response.success && response.statusCode == 206;
}

int64_t HttpClient::getContentLength(const std::string& url) {
    // Send a HEAD request to get the content length
    HttpResponse response = head(url);
    
    if (!response.success) {
        return -1;
    }
    
    // Look for Content-Length header
    auto it = response.headers.find("content-length");
    if (it != response.headers.end()) {
        try {
            return std::stoll(it->second);
        } catch (...) {
            return -1;
        }
    }
    
    return -1;
}

std::string HttpClient::getLastModified(const std::string& url) {
    // Send a HEAD request to get the last modified time
    HttpResponse response = head(url);
    
    if (!response.success) {
        return "";
    }
    
    // Look for Last-Modified header
    auto it = response.headers.find("last-modified");
    if (it != response.headers.end()) {
        return it->second;
    }
    
    return "";
}

bool HttpClient::downloadFile(const std::string& url, const std::string& filePath,
                             ProgressCallback progressCallback) {
    // Set progress callback if provided
    ProgressCallback oldCallback = progressCallback_;
    if (progressCallback) {
        progressCallback_ = progressCallback;
    }
    
    // Open the file for writing
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        if (progressCallback) {
            progressCallback_ = oldCallback;
        }
        return false;
    }
    
    // Set a data callback to write to the file
    DataCallback oldDataCallback = dataCallback_;
    dataCallback_ = [&file](const char* data, size_t size) -> bool {
        file.write(data, size);
        return file.good();
    };
    
    // Send a GET request
    HttpResponse response = get(url);
    
    // Reset callbacks
    dataCallback_ = oldDataCallback;
    if (progressCallback) {
        progressCallback_ = oldCallback;
    }
    
    // Close the file
    file.close();
    
    // Return success status
    return response.success;
}

bool HttpClient::downloadFileSegment(const std::string& url, const std::string& filePath,
                                    int64_t startByte, int64_t endByte,
                                    ProgressCallback progressCallback) {
    // Set progress callback if provided
    ProgressCallback oldCallback = progressCallback_;
    if (progressCallback) {
        progressCallback_ = progressCallback;
    }
    
    // Open the file for writing at the specified position
    std::ofstream file(filePath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) {
        // If the file doesn't exist, create it
        file.open(filePath, std::ios::binary | std::ios::out);
        if (!file) {
            if (progressCallback) {
                progressCallback_ = oldCallback;
            }
            return false;
        }
    }
    
    // Seek to the start byte
    file.seekp(startByte);
    
    // Set a data callback to write to the file
    DataCallback oldDataCallback = dataCallback_;
    dataCallback_ = [&file](const char* data, size_t size) -> bool {
        file.write(data, size);
        return file.good();
    };
    
    // Send a range GET request
    HttpResponse response = getRange(url, startByte, endByte);
    
    // Reset callbacks
    dataCallback_ = oldDataCallback;
    if (progressCallback) {
        progressCallback_ = oldCallback;
    }
    
    // Close the file
    file.close();
    
    // Return success status
    return response.success;
}

void HttpClient::abort() {
    aborted_ = true;
}

} // namespace core
} // namespace dm