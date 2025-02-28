#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <curl/curl.h>

namespace dm {
namespace core {

/**
 * @brief HTTP response data structure
 */
struct HttpResponse {
    int statusCode = 0;
    std::map<std::string, std::string> headers;
    std::vector<char> body;
    std::string error;
    bool success = false;
};

/**
 * @brief HTTP progress callback function type
 * 
 * @param downloadTotal Total bytes to download
 * @param downloadedNow Bytes downloaded so far
 * @param uploadTotal Total bytes to upload
 * @param uploadedNow Bytes uploaded so far
 * @return true to continue, false to abort
 */
using ProgressCallback = std::function<bool(int64_t downloadTotal, int64_t downloadedNow, 
                                          int64_t uploadTotal, int64_t uploadedNow)>;

/**
 * @brief HTTP data callback function type
 * 
 * Called when data is received during download
 * 
 * @param data Pointer to received data
 * @param size Size of received data
 * @return true to continue, false to abort
 */
using DataCallback = std::function<bool(const char* data, size_t size)>;

/**
 * @brief HTTP client class for making HTTP requests
 * 
 * Uses libcurl for handling HTTP/HTTPS communication
 */
class HttpClient {
public:
    /**
     * @brief Construct a new HttpClient object
     */
    HttpClient();
    
    /**
     * @brief Destroy the HttpClient object
     */
    ~HttpClient();
    
    /**
     * @brief Set a request header
     * 
     * @param name Header name
     * @param value Header value
     * @return HttpClient& Reference to this object for method chaining
     */
    HttpClient& setHeader(const std::string& name, const std::string& value);
    
    /**
     * @brief Set a cookie
     * 
     * @param name Cookie name
     * @param value Cookie value
     * @return HttpClient& Reference to this object for method chaining
     */
    HttpClient& setCookie(const std::string& name, const std::string& value);
    
    /**
     * @brief Set the connection timeout
     * 
     * @param timeoutSeconds Timeout in seconds
     * @return HttpClient& Reference to this object for method chaining
     */
    HttpClient& setTimeout(int timeoutSeconds);
    
    /**
     * @brief Set the user agent string
     * 
     * @param userAgent User agent string
     * @return HttpClient& Reference to this object for method chaining
     */
    HttpClient& setUserAgent(const std::string& userAgent);
    
    /**
     * @brief Set the progress callback
     * 
     * @param callback The progress callback function
     * @return HttpClient& Reference to this object for method chaining
     */
    HttpClient& setProgressCallback(ProgressCallback callback);
    
    /**
     * @brief Set the data callback
     * 
     * @param callback The data callback function
     * @return HttpClient& Reference to this object for method chaining
     */
    HttpClient& setDataCallback(DataCallback callback);
    
    /**
     * @brief Set whether to follow redirects
     * 
     * @param follow True to follow redirects, false otherwise
     * @return HttpClient& Reference to this object for method chaining
     */
    HttpClient& followRedirects(bool follow);
    
    /**
     * @brief Perform a HEAD request
     * 
     * @param url The URL to request
     * @return HttpResponse The response
     */
    HttpResponse head(const std::string& url);
    
    /**
     * @brief Perform a GET request
     * 
     * @param url The URL to request
     * @return HttpResponse The response
     */
    HttpResponse get(const std::string& url);
    
    /**
     * @brief Perform a GET request with a byte range
     * 
     * @param url The URL to request
     * @param startByte Starting byte offset
     * @param endByte Ending byte offset
     * @return HttpResponse The response
     */
    HttpResponse getRange(const std::string& url, int64_t startByte, int64_t endByte);
    
    /**
     * @brief Check if a server supports range requests
     * 
     * @param url The URL to check
     * @return true if range requests are supported, false otherwise
     */
    bool supportsRangeRequests(const std::string& url);
    
    /**
     * @brief Get the content length of a resource
     * 
     * @param url The URL to check
     * @return int64_t The content length in bytes, or -1 if unknown
     */
    int64_t getContentLength(const std::string& url);
    
    /**
     * @brief Get the last modification time of a resource
     * 
     * @param url The URL to check
     * @return std::string The last modification time as a string, or empty if unknown
     */
    std::string getLastModified(const std::string& url);
    
    /**
     * @brief Download a file
     * 
     * @param url The URL to download
     * @param filePath The path to save the file
     * @param progressCallback Optional progress callback
     * @return true if successful, false otherwise
     */
    bool downloadFile(const std::string& url, const std::string& filePath,
                     ProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Download a file segment
     * 
     * @param url The URL to download
     * @param filePath The path to save the file
     * @param startByte Starting byte offset
     * @param endByte Ending byte offset
     * @param progressCallback Optional progress callback
     * @return true if successful, false otherwise
     */
    bool downloadFileSegment(const std::string& url, const std::string& filePath,
                            int64_t startByte, int64_t endByte,
                            ProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Abort the current operation
     */
    void abort();
    
private:
    /**
     * @brief Set up common CURL options
     * 
     * @param curl The CURL handle
     * @param url The URL to request
     */
    void setupCurlOptions(CURL* curl, const std::string& url);
    
    /**
     * @brief Perform a CURL request
     * 
     * @param curl The CURL handle
     * @return HttpResponse The response
     */
    HttpResponse performRequest(CURL* curl);
    
    // CURL callback functions
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t headerCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static int progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    
    // Member variables
    std::map<std::string, std::string> headers_;
    std::map<std::string, std::string> cookies_;
    int timeoutSeconds_ = 30;
    std::string userAgent_ = "DownloadManager/1.0";
    bool followRedirects_ = true;
    bool aborted_ = false;
    
    ProgressCallback progressCallback_ = nullptr;
    DataCallback dataCallback_ = nullptr;
    
    // CURL easy handle
    CURL* curl_ = nullptr;
};

} // namespace core
} // namespace dm

#endif // HTTP_CLIENT_H