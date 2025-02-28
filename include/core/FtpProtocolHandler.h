#ifndef FTP_PROTOCOL_HANDLER_H
#define FTP_PROTOCOL_HANDLER_H

#include "include/core/ProtocolHandler.h"
#include <curl/curl.h>
#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>

namespace dm {
namespace core {

/**
 * @brief FTP protocol handler class
 * 
 * Implements the ProtocolHandler interface for FTP/FTPS
 */
class FtpProtocolHandler : public ProtocolHandler {
public:
    /**
     * @brief Construct a new FtpProtocolHandler
     */
    FtpProtocolHandler();
    
    /**
     * @brief Destroy the FtpProtocolHandler
     */
    ~FtpProtocolHandler() override;
    
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
     * @return std::vector<std::string> The protocol schemes (e.g., "ftp", "ftps")
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
    
private:
    /**
     * @brief Configure CURL handle for FTP
     * 
     * @param curl The CURL handle
     * @param url The URL
     * @param options The protocol options
     * @return true if successful, false otherwise
     */
    bool configureCurl(CURL* curl, const std::string& url, const ProtocolOptions& options);
    
    /**
     * @brief Get active CURL handle for a task
     * 
     * @param taskId The task ID
     * @return CURL* The CURL handle, or nullptr if not found
     */
    CURL* getActiveCurl(const std::string& taskId);
    
    /**
     * @brief Parse directory listing
     * 
     * @param listing The directory listing
     * @param baseUrl The base URL
     * @return std::vector<DirectoryEntry> The parsed directory entries
     */
    std::vector<DirectoryEntry> parseDirectoryListing(const std::string& listing, 
                                                    const std::string& baseUrl);
    
    /**
     * @brief Parse file information from listing line
     * 
     * @param line The listing line
     * @param baseUrl The base URL
     * @return DirectoryEntry The parsed directory entry
     */
    DirectoryEntry parseListingLine(const std::string& line, const std::string& baseUrl);
    
    // CURL callback functions
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t headerCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, 
                               curl_off_t ultotal, curl_off_t ulnow);
    
    // Member variables
    std::map<std::string, CURL*> activeHandles_;
    std::map<std::string, std::string> userPassCache_;
    std::map<std::string, std::pair<ProtocolProgressCallback, void*>> progressCallbacks_;
    std::atomic<bool> initialized_;
    mutable std::mutex mutex_;
};

} // namespace core
} // namespace dm

#endif // FTP_PROTOCOL_HANDLER_H