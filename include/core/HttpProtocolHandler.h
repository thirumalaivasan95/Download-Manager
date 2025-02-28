#ifndef HTTP_PROTOCOL_HANDLER_H
#define HTTP_PROTOCOL_HANDLER_H

#include "include/core/ProtocolHandler.h"
#include "include/core/HttpClient.h"

#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>

namespace dm {
namespace core {

/**
 * @brief HTTP protocol handler class
 * 
 * Implements the ProtocolHandler interface for HTTP/HTTPS
 */
class HttpProtocolHandler : public ProtocolHandler {
public:
    /**
     * @brief Construct a new HttpProtocolHandler
     */
    HttpProtocolHandler();
    
    /**
     * @brief Destroy the HttpProtocolHandler
     */
    ~HttpProtocolHandler() override;
    
    /**
     * @brief Initialize the protocol handler
     * 
     * @return true if successful, false otherwise
     */
    bool initialize() override;
    
    /**
     * @brief Cleanup the protocol handler
     */
    void cleanup() override;
    
    /**
     * @brief Get the protocol name
     * 
     * @return std::string The protocol name
     */
    std::string getProtocolName() const override;
    
    /**
     * @brief Get the protocol schemes
     * 
     * @return std::vector<std::string> The protocol schemes (e.g., "http", "https")
     */
    std::vector<std::string> getProtocolSchemes() const override;
    
    /**
     * @brief Check if the protocol handler supports a capability
     * 
     * @param capability The capability to check
     * @return true if supported, false otherwise
     */
    bool supportsCapability(ProtocolCapability capability) const override;
    
    /**
     * @brief Start a download
     * 
     * @param url The URL to download
     * @param outputFile The output file path
     * @param task The download task
     * @param options The protocol options
     * @param progressCallback Optional progress callback
     * @param errorCallback Optional error callback
     * @param statusCallback Optional status callback
     * @return true if download started successfully, false otherwise
     */
    bool startDownload(const std::string& url, 
                      const std::string& outputFile,
                      std::shared_ptr<DownloadTask> task,
                      const ProtocolOptions& options,
                      ProtocolProgressCallback progressCallback = nullptr,
                      ProtocolErrorCallback errorCallback = nullptr,
                      ProtocolStatusCallback statusCallback = nullptr) override;
    
    /**
     * @brief Pause a download
     * 
     * @param task The download task
     * @return true if paused successfully, false otherwise
     */
    bool pauseDownload(std::shared_ptr<DownloadTask> task) override;
    
    /**
     * @brief Resume a download
     * 
     * @param task The download task
     * @return true if resumed successfully, false otherwise
     */
    bool resumeDownload(std::shared_ptr<DownloadTask> task) override;
    
    /**
     * @brief Cancel a download
     * 
     * @param task The download task
     * @return true if canceled successfully, false otherwise
     */
    bool cancelDownload(std::shared_ptr<DownloadTask> task) override;
    
    /**
     * @brief Get file size
     * 
     * @param url The URL
     * @param options The protocol options
     * @return int64_t The file size in bytes, or -1 if unknown
     */
    int64_t getFileSize(const std::string& url, const ProtocolOptions& options) override;
    
    /**
     * @brief Get file metadata
     * 
     * @param url The URL
     * @param options The protocol options
     * @return std::map<std::string, std::string> The metadata
     */
    std::map<std::string, std::string> getMetadata(const std::string& url, 
                                                 const ProtocolOptions& options) override;
    
    /**
     * @brief List directory contents
     * 
     * @param url The directory URL
     * @param options The protocol options
     * @return std::vector<DirectoryEntry> The directory entries
     */
    std::vector<DirectoryEntry> listDirectory(const std::string& url, 
                                            const ProtocolOptions& options) override;
    
    /**
     * @brief Authenticate with the server
     * 
     * @param url The URL
     * @param authInfo The authentication information
     * @param options The protocol options
     * @return true if authenticated successfully, false otherwise
     */
    bool authenticate(const std::string& url, 
                     const AuthInfo& authInfo, 
                     const ProtocolOptions& options) override;
    
    /**
     * @brief Check if a URL is supported by this protocol handler
     * 
     * @param url The URL to check
     * @return true if supported, false otherwise
     */
    bool supportsUrl(const std::string& url) const override;
    
    /**
     * @brief Check if a server supports range requests
     * 
     * @param url The URL to check
     * @param options The protocol options
     * @return true if supported, false otherwise
     */
    bool supportsRangeRequests(const std::string& url, const ProtocolOptions& options);
    
    /**
     * @brief Get the last modified time of a file
     * 
     * @param url The URL
     * @param options The protocol options
     * @return std::string The last modified time as a string, or empty if unknown
     */
    std::string getLastModifiedTime(const std::string& url, const ProtocolOptions& options);
    
    /**
     * @brief Get the content type of a file
     * 
     * @param url The URL
     * @param options The protocol options
     * @return std::string The content type, or empty if unknown
     */
    std::string getContentType(const std::string& url, const ProtocolOptions& options);
    
    /**
     * @brief Get the ETag of a file
     * 
     * @param url The URL
     * @param options The protocol options
     * @return std::string The ETag, or empty if unknown
     */
    std::string getETag(const std::string& url, const ProtocolOptions& options);
    
private:
    /**
     * @brief Configure HTTP client based on options
     * 
     * @param client The HTTP client to configure
     * @param options The protocol options
     */
    void configureHttpClient(HttpClient& client, const ProtocolOptions& options);
    
    /**
     * @brief Handle HTTP response
     * 
     * @param response The HTTP response
     * @param url The URL
     * @param errorCallback Optional error callback
     * @return true if response is valid, false otherwise
     */
    bool handleHttpResponse(const HttpResponse& response, 
                           const std::string& url,
                           ProtocolErrorCallback errorCallback = nullptr);
    
    /**
     * @brief Parse directory listing from HTML
     * 
     * @param html The HTML content
     * @param baseUrl The base URL
     * @return std::vector<DirectoryEntry> The directory entries
     */
    std::vector<DirectoryEntry> parseDirectoryListing(const std::string& html, 
                                                    const std::string& baseUrl);
    
    /**
     * @brief Extract links from HTML
     * 
     * @param html The HTML content
     * @param baseUrl The base URL
     * @return std::vector<std::string> The extracted links
     */
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl);
    
    /**
     * @brief Convert relative URL to absolute
     * 
     * @param relativeUrl The relative URL
     * @param baseUrl The base URL
     * @return std::string The absolute URL
     */
    std::string relativeToAbsoluteUrl(const std::string& relativeUrl, const std::string& baseUrl);
    
    // Member variables
    std::map<std::string, std::shared_ptr<HttpClient>> activeClients_;
    mutable std::mutex clientsMutex_;
    std::chrono::seconds cookieUpdateInterval_;
    std::chrono::system_clock::time_point lastCookieUpdate_;
};

} // namespace core
} // namespace dm

#endif // HTTP_PROTOCOL_HANDLER_H