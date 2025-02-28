#ifndef TORRENT_PROTOCOL_HANDLER_H
#define TORRENT_PROTOCOL_HANDLER_H

#include "include/core/ProtocolHandler.h"
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

// Forward declarations for libtorrent
namespace libtorrent {
    class session;
    class torrent_handle;
    struct torrent_status;
    struct add_torrent_params;
    class alert;
}

namespace dm {
namespace core {

/**
 * @brief Structure to hold torrent download information
 */
struct TorrentDownloadInfo {
    std::string taskId;                           // Associated task ID
    std::string url;                              // Torrent URL or magnet link
    std::string outputPath;                       // Output directory
    std::string torrentFile;                      // Path to .torrent file (if not magnet)
    libtorrent::torrent_handle handle;            // Torrent handle
    ProtocolProgressCallback progressCallback;    // Progress callback
    ProtocolErrorCallback errorCallback;          // Error callback
    ProtocolStatusCallback statusCallback;        // Status callback
    std::atomic<bool> paused{false};              // Whether the torrent is paused
    std::atomic<bool> stopped{false};             // Whether the torrent is stopped
    std::string selectedFile;                     // Selected file path (if single file)
    std::vector<int> fileSelection;               // Selected file indices (if multiple files)
    int64_t totalSize{0};                         // Total size in bytes
    int64_t downloadedBytes{0};                   // Downloaded bytes
    double downloadSpeed{0.0};                    // Download speed in bytes/second
    double uploadSpeed{0.0};                      // Upload speed in bytes/second
    double progress{0.0};                         // Progress (0.0-1.0)
    int seeders{0};                               // Number of seeders
    int leechers{0};                              // Number of leechers
    std::string state;                            // Current state
    std::map<std::string, std::string> metadata;  // Torrent metadata
};

/**
 * @brief BitTorrent protocol handler class
 * 
 * Implements the ProtocolHandler interface for BitTorrent
 */
class TorrentProtocolHandler : public ProtocolHandler {
public:
    /**
     * @brief Construct a new TorrentProtocolHandler
     */
    TorrentProtocolHandler();
    
    /**
     * @brief Destroy the TorrentProtocolHandler
     */
    ~TorrentProtocolHandler() override;
    
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
     * @return std::vector<std::string> The protocol schemes (e.g., "magnet", "torrent")
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
     * @param url The URL to download (torrent file URL or magnet link)
     * @param outputFile The output file path (or directory for multiple files)
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
     * @param url The URL (torrent file URL or magnet link)
     * @param options The protocol options
     * @return int64_t The file size in bytes, or -1 if unknown
     */
    int64_t getFileSize(const std::string& url, const ProtocolOptions& options) override;
    
    /**
     * @brief Get file metadata
     * 
     * @param url The URL (torrent file URL or magnet link)
     * @param options The protocol options
     * @return std::map<std::string, std::string> The metadata
     */
    std::map<std::string, std::string> getMetadata(const std::string& url, 
                                                 const ProtocolOptions& options) override;
    
    /**
     * @brief List directory contents (files in the torrent)
     * 
     * @param url The URL (torrent file URL or magnet link)
     * @param options The protocol options
     * @return std::vector<DirectoryEntry> The directory entries (files in the torrent)
     */
    std::vector<DirectoryEntry> listDirectory(const std::string& url, 
                                            const ProtocolOptions& options) override;
    
    /**
     * @brief Authenticate with the server (not used for BitTorrent)
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
     * @brief Set download limits
     * 
     * @param task The download task
     * @param downloadLimit Download speed limit in bytes per second (0 for unlimited)
     * @param uploadLimit Upload speed limit in bytes per second (0 for unlimited)
     * @return true if successful, false otherwise
     */
    bool setLimits(std::shared_ptr<DownloadTask> task, int downloadLimit, int uploadLimit);
    
    /**
     * @brief Get download limits
     * 
     * @param task The download task
     * @param downloadLimit Output parameter for download speed limit
     * @param uploadLimit Output parameter for upload speed limit
     * @return true if successful, false otherwise
     */
    bool getLimits(std::shared_ptr<DownloadTask> task, int& downloadLimit, int& uploadLimit);
    
    /**
     * @brief Get peers information
     * 
     * @param task The download task
     * @return std::vector<std::map<std::string, std::string>> Peer information
     */
    std::vector<std::map<std::string, std::string>> getPeers(std::shared_ptr<DownloadTask> task);
    
    /**
     * @brief Get detailed torrent status
     * 
     * @param task The download task
     * @return std::map<std::string, std::string> The torrent status
     */
    std::map<std::string, std::string> getTorrentStatus(std::shared_ptr<DownloadTask> task);
    
    /**
     * @brief Get files in the torrent
     * 
     * @param task The download task
     * @return std::vector<std::map<std::string, std::string>> File information
     */
    std::vector<std::map<std::string, std::string>> getFiles(std::shared_ptr<DownloadTask> task);
    
    /**
     * @brief Select specific files to download
     * 
     * @param task The download task
     * @param fileIndices Indices of files to download
     * @return true if successful, false otherwise
     */
    bool selectFiles(std::shared_ptr<DownloadTask> task, const std::vector<int>& fileIndices);
    
    /**
     * @brief Set session settings
     * 
     * @param settings The settings to set
     * @return true if successful, false otherwise
     */
    bool setSessionSettings(const std::map<std::string, std::string>& settings);
    
    /**
     * @brief Get session settings
     * 
     * @return std::map<std::string, std::string> The session settings
     */
    std::map<std::string, std::string> getSessionSettings();
    
    /**
     * @brief Add trackers to a torrent
     * 
     * @param task The download task
     * @param trackers The trackers to add
     * @return true if successful, false otherwise
     */
    bool addTrackers(std::shared_ptr<DownloadTask> task, const std::vector<std::string>& trackers);
    
private:
    /**
     * @brief Add a torrent
     * 
     * @param url The URL (torrent file URL or magnet link)
     * @param outputPath The output directory
     * @param taskId The task ID
     * @param options The protocol options
     * @param info Output parameter for torrent download info
     * @return true if successful, false otherwise
     */
    bool addTorrent(const std::string& url, 
                   const std::string& outputPath, 
                   const std::string& taskId,
                   const ProtocolOptions& options,
                   TorrentDownloadInfo& info);
    
    /**
     * @brief Download torrent file from URL
     * 
     * @param url The URL
     * @param options The protocol options
     * @return std::string The path to the downloaded torrent file
     */
    std::string downloadTorrentFile(const std::string& url, const ProtocolOptions& options);
    
    /**
     * @brief Parse magnet link
     * 
     * @param url The magnet link
     * @param params Output parameter for torrent parameters
     * @return true if successful, false otherwise
     */
    bool parseMagnetLink(const std::string& url, libtorrent::add_torrent_params& params);
    
    /**
     * @brief Update torrent status
     * 
     * @param info The torrent download info
     * @param status The torrent status
     */
    void updateTorrentStatus(TorrentDownloadInfo& info, const libtorrent::torrent_status& status);
    
    /**
     * @brief Alert processing thread function
     */
    void alertThread();
    
    /**
     * @brief Process an alert
     * 
     * @param alert The alert to process
     */
    void processAlert(libtorrent::alert* alert);
    
    /**
     * @brief Create temporary directory for torrent files
     * 
     * @return std::string The temporary directory path
     */
    std::string createTempDirectory();
    
    /**
     * @brief Check if a string is a magnet link
     * 
     * @param url The URL to check
     * @return true if it's a magnet link, false otherwise
     */
    bool isMagnetLink(const std::string& url) const;
    
    /**
     * @brief Get TorrentDownloadInfo for a task
     * 
     * @param taskId The task ID
     * @return TorrentDownloadInfo* Pointer to the info, or nullptr if not found
     */
    TorrentDownloadInfo* getDownloadInfo(const std::string& taskId);
    
    /**
     * @brief Save resume data for all torrents
     */
    void saveResumeData();
    
    /**
     * @brief Load resume data for all torrents
     */
    void loadResumeData();
    
    // Member variables
    std::unique_ptr<libtorrent::session> session_;
    std::map<std::string, TorrentDownloadInfo> downloads_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> alertThread_;
    std::string temporaryDirectory_;
    std::string resumeDataDirectory_;
    ProtocolOptions defaultOptions_;
    mutable std::mutex mutex_;
};

} // namespace core
} // namespace dm

#endif // TORRENT_PROTOCOL_HANDLER_H