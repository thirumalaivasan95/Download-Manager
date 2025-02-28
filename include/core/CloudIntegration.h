#ifndef CLOUD_INTEGRATION_H
#define CLOUD_INTEGRATION_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>

namespace dm {
namespace core {

// Forward declarations
class DownloadManager;
class Settings;

/**
 * @brief Cloud provider enumeration
 */
enum class CloudProvider {
    NONE,
    DROPBOX,
    GOOGLE_DRIVE,
    ONEDRIVE,
    BOX,
    AMAZON_S3,
    CUSTOM
};

/**
 * @brief Sync direction enumeration
 */
enum class SyncDirection {
    LOCAL_TO_CLOUD,     // Upload local data to cloud
    CLOUD_TO_LOCAL,     // Download cloud data to local
    BIDIRECTIONAL       // Sync in both directions
};

/**
 * @brief Sync frequency enumeration
 */
enum class SyncFrequency {
    MANUAL,             // Manual sync only
    ON_CHANGE,          // Sync on change
    HOURLY,             // Sync every hour
    DAILY,              // Sync every day
    WEEKLY              // Sync every week
};

/**
 * @brief Sync data type enumeration
 */
enum class SyncDataType {
    SETTINGS,           // Application settings
    DOWNLOAD_HISTORY,   // Download history
    STATISTICS,         // Download statistics
    SECURITY_SETTINGS,  // Security settings
    SCHEDULER_SETTINGS, // Scheduler settings
    PLUGINS,            // Plugins
    QUARANTINE,         // Quarantined files
    ALL                 // All data types
};

/**
 * @brief Sync conflict resolution enumeration
 */
enum class ConflictResolution {
    USE_LOCAL,          // Use local version
    USE_CLOUD,          // Use cloud version
    USE_NEWEST,         // Use newest version
    USE_OLDEST,         // Use oldest version
    ALWAYS_ASK          // Always ask the user
};

/**
 * @brief Sync configuration structure
 */
struct SyncConfig {
    CloudProvider provider = CloudProvider::NONE;   // Cloud provider
    std::string accountName;                        // Account name
    std::string accountId;                          // Account ID
    std::string remotePath;                         // Remote path
    std::vector<SyncDataType> dataTypes;            // Data types to sync
    SyncDirection direction = SyncDirection::BIDIRECTIONAL; // Sync direction
    SyncFrequency frequency = SyncFrequency::MANUAL; // Sync frequency
    ConflictResolution conflictResolution = ConflictResolution::ALWAYS_ASK; // Conflict resolution
    bool autoSync = false;                          // Auto sync on startup/exit
    bool syncInBackground = false;                  // Sync in background
    bool useCompression = true;                     // Use compression
    bool useEncryption = true;                      // Use encryption
    std::string encryptionKey;                      // Encryption key (if any)
    int maxSyncSize = 0;                            // Max sync size in bytes (0 for unlimited)
    bool includeDownloadedFiles = false;            // Include downloaded files in sync
    bool syncOnMeteredConnection = false;           // Sync on metered connection
    std::chrono::seconds syncInterval{3600};        // Sync interval for automatic sync
};

/**
 * @brief Sync status enumeration
 */
enum class SyncStatus {
    IDLE,               // No sync in progress
    SYNCING,            // Sync in progress
    ERROR,              // Error occurred
    CONFLICT,           // Conflict detected
    COMPLETED,          // Sync completed
    PAUSED              // Sync paused
};

/**
 * @brief Sync result structure
 */
struct SyncResult {
    SyncStatus status = SyncStatus::IDLE;           // Sync status
    std::string message;                            // Status message
    int uploadedItems = 0;                          // Number of items uploaded
    int downloadedItems = 0;                        // Number of items downloaded
    int conflictItems = 0;                          // Number of conflicts
    int errorItems = 0;                             // Number of errors
    std::vector<std::string> conflictPaths;         // Paths with conflicts
    std::vector<std::string> errorPaths;            // Paths with errors
    std::chrono::milliseconds duration{0};          // Sync duration
    std::chrono::system_clock::time_point timestamp; // Sync timestamp
    int64_t bytesTransferred = 0;                   // Bytes transferred
};

/**
 * @brief Cloud file info structure
 */
struct CloudFileInfo {
    std::string path;                               // File path
    std::string name;                               // File name
    int64_t size = 0;                               // File size in bytes
    std::chrono::system_clock::time_point modifiedTime; // Modified time
    std::string hash;                               // File hash
    std::string downloadUrl;                        // Download URL
    bool isDirectory = false;                       // Whether this is a directory
    std::map<std::string, std::string> metadata;    // Additional metadata
};

/**
 * @brief Authentication status enumeration
 */
enum class AuthStatus {
    NOT_AUTHENTICATED,  // Not authenticated
    AUTHENTICATING,     // Authentication in progress
    AUTHENTICATED,      // Authenticated
    AUTH_ERROR          // Authentication error
};

/**
 * @brief Authentication info structure
 */
struct AuthInfo {
    AuthStatus status = AuthStatus::NOT_AUTHENTICATED; // Authentication status
    std::string accountName;                        // Account name
    std::string accountId;                          // Account ID
    std::string tokenType;                          // Token type
    std::string accessToken;                        // Access token
    std::string refreshToken;                       // Refresh token
    std::chrono::system_clock::time_point expiryTime; // Token expiry time
    std::string error;                              // Error message (if any)
};

/**
 * @brief Sync progress callback type
 */
using SyncProgressCallback = std::function<void(int currentItem, int totalItems, 
                                              const std::string& currentPath,
                                              int64_t bytesTransferred, int64_t totalBytes)>;

/**
 * @brief Sync completion callback type
 */
using SyncCompletionCallback = std::function<void(const SyncResult& result)>;

/**
 * @brief Authentication callback type
 */
using AuthCallback = std::function<void(const AuthInfo& authInfo)>;

/**
 * @brief Conflict resolution callback type
 */
using ConflictResolutionCallback = std::function<ConflictResolution(
    const std::string& path, 
    const std::chrono::system_clock::time_point& localTime,
    const std::chrono::system_clock::time_point& cloudTime)>;

/**
 * @brief Cloud integration class
 * 
 * Provides cloud synchronization functionality
 */
class CloudIntegration {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return CloudIntegration& The singleton instance
     */
    static CloudIntegration& getInstance();
    
    /**
     * @brief Initialize the cloud integration
     * 
     * @param downloadManager Reference to download manager
     * @param settings Reference to settings
     * @return true if successful, false otherwise
     */
    bool initialize(DownloadManager& downloadManager, Settings& settings);
    
    /**
     * @brief Shutdown the cloud integration
     */
    void shutdown();
    
    /**
     * @brief Set sync configuration
     * 
     * @param config The sync configuration
     */
    void setSyncConfig(const SyncConfig& config);
    
    /**
     * @brief Get sync configuration
     * 
     * @return SyncConfig The sync configuration
     */
    SyncConfig getSyncConfig() const;
    
    /**
     * @brief Authenticate with cloud provider
     * 
     * @param provider The cloud provider
     * @param callback Optional callback for authentication result
     * @return true if authentication started successfully, false otherwise
     */
    bool authenticate(CloudProvider provider, AuthCallback callback = nullptr);
    
    /**
     * @brief Get authentication info
     * 
     * @return AuthInfo The authentication info
     */
    AuthInfo getAuthInfo() const;
    
    /**
     * @brief Start sync
     * 
     * @param dataTypes Data types to sync (empty for all configured types)
     * @param direction Sync direction (default from config)
     * @param progressCallback Optional callback for sync progress
     * @param completionCallback Optional callback for sync completion
     * @return true if sync started successfully, false otherwise
     */
    bool startSync(const std::vector<SyncDataType>& dataTypes = {},
                  SyncDirection direction = SyncDirection::BIDIRECTIONAL,
                  SyncProgressCallback progressCallback = nullptr,
                  SyncCompletionCallback completionCallback = nullptr);
    
    /**
     * @brief Cancel sync
     * 
     * @return true if canceled successfully, false otherwise
     */
    bool cancelSync();
    
    /**
     * @brief Get sync status
     * 
     * @return SyncStatus The sync status
     */
    SyncStatus getSyncStatus() const;
    
    /**
     * @brief Get last sync result
     * 
     * @return SyncResult The last sync result
     */
    SyncResult getLastSyncResult() const;
    
    /**
     * @brief Set conflict resolution callback
     * 
     * @param callback The callback function
     */
    void setConflictResolutionCallback(ConflictResolutionCallback callback);
    
    /**
     * @brief List cloud directory
     * 
     * @param path The directory path
     * @return std::vector<CloudFileInfo> The directory contents
     */
    std::vector<CloudFileInfo> listCloudDirectory(const std::string& path = "");
    
    /**
     * @brief Download file from cloud
     * 
     * @param cloudPath The cloud file path
     * @param localPath The local file path
     * @param progressCallback Optional callback for download progress
     * @return true if download started successfully, false otherwise
     */
    bool downloadFromCloud(const std::string& cloudPath, 
                          const std::string& localPath,
                          SyncProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Upload file to cloud
     * 
     * @param localPath The local file path
     * @param cloudPath The cloud file path
     * @param progressCallback Optional callback for upload progress
     * @return true if upload started successfully, false otherwise
     */
    bool uploadToCloud(const std::string& localPath, 
                      const std::string& cloudPath,
                      SyncProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Create cloud directory
     * 
     * @param path The directory path
     * @return true if successful, false otherwise
     */
    bool createCloudDirectory(const std::string& path);
    
    /**
     * @brief Delete cloud file
     * 
     * @param path The file path
     * @return true if successful, false otherwise
     */
    bool deleteCloudFile(const std::string& path);
    
    /**
     * @brief Check if a path exists in cloud
     * 
     * @param path The path to check
     * @return true if exists, false otherwise
     */
    bool cloudPathExists(const std::string& path);
    
    /**
     * @brief Get cloud file info
     * 
     * @param path The file path
     * @return CloudFileInfo The file info
     */
    CloudFileInfo getCloudFileInfo(const std::string& path);
    
    /**
     * @brief Get cloud storage quota
     * 
     * @param usedBytes Output parameter for used bytes
     * @param totalBytes Output parameter for total bytes
     * @return true if successful, false otherwise
     */
    bool getCloudStorageQuota(int64_t& usedBytes, int64_t& totalBytes);
    
    /**
     * @brief Get cloud provider name
     * 
     * @param provider The cloud provider
     * @return std::string The provider name
     */
    static std::string getProviderName(CloudProvider provider);
    
    /**
     * @brief Generate shareable link
     * 
     * @param cloudPath The cloud file path
     * @param expiration Optional expiration time
     * @return std::string The shareable link
     */
    std::string generateShareableLink(const std::string& cloudPath, 
                                    std::chrono::system_clock::time_point expiration = 
                                        std::chrono::system_clock::time_point());
    
    /**
     * @brief Check if cloud integration is available
     * 
     * @return true if available, false otherwise
     */
    bool isAvailable() const;
    
    /**
     * @brief Get available cloud providers
     * 
     * @return std::vector<CloudProvider> The available providers
     */
    std::vector<CloudProvider> getAvailableProviders() const;
    
    /**
     * @brief Get the sync data file path
     * 
     * @param dataType The data type
     * @return std::string The file path
     */
    std::string getSyncDataFilePath(SyncDataType dataType) const;
    
    /**
     * @brief Enable or disable cloud integration
     * 
     * @param enable True to enable, false to disable
     */
    void setEnabled(bool enable);
    
    /**
     * @brief Check if cloud integration is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;
    
private:
    /**
     * @brief Construct a new CloudIntegration
     */
    CloudIntegration();
    
    /**
     * @brief Destroy the CloudIntegration
     */
    ~CloudIntegration();
    
    // Prevent copying
    CloudIntegration(const CloudIntegration&) = delete;
    CloudIntegration& operator=(const CloudIntegration&) = delete;
    
    /**
     * @brief Initialize the configured cloud provider
     * 
     * @return true if successful, false otherwise
     */
    bool initializeProvider();
    
    /**
     * @brief Sync thread function
     */
    void syncThread();
    
    /**
     * @brief Initialize Dropbox provider
     * 
     * @return true if successful, false otherwise
     */
    bool initializeDropbox();
    
    /**
     * @brief Initialize Google Drive provider
     * 
     * @return true if successful, false otherwise
     */
    bool initializeGoogleDrive();
    
    /**
     * @brief Initialize OneDrive provider
     * 
     * @return true if successful, false otherwise
     */
    bool initializeOneDrive();
    
    /**
     * @brief Refresh authentication token
     * 
     * @return true if successful, false otherwise
     */
    bool refreshToken();
    
    /**
     * @brief Save authentication info
     * 
     * @return true if successful, false otherwise
     */
    bool saveAuthInfo();
    
    /**
     * @brief Load authentication info
     * 
     * @return true if successful, false otherwise
     */
    bool loadAuthInfo();
    
    /**
     * @brief Encrypt data
     * 
     * @param data The data to encrypt
     * @return std::vector<uint8_t> The encrypted data
     */
    std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief Decrypt data
     * 
     * @param data The data to decrypt
     * @return std::vector<uint8_t> The decrypted data
     */
    std::vector<uint8_t> decryptData(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief Calculate file hash
     * 
     * @param filePath Path to the file
     * @return std::string The file hash
     */
    std::string calculateFileHash(const std::string& filePath) const;
    
    /**
     * @brief Get the time of last sync
     * 
     * @param dataType The data type
     * @return std::chrono::system_clock::time_point The last sync time
     */
    std::chrono::system_clock::time_point getLastSyncTime(SyncDataType dataType) const;
    
    /**
     * @brief Update last sync time
     * 
     * @param dataType The data type
     * @param time The sync time
     */
    void updateLastSyncTime(SyncDataType dataType, 
                           const std::chrono::system_clock::time_point& time);
    
    /**
     * @brief Check if sync is due
     * 
     * @return true if sync is due, false otherwise
     */
    bool isSyncDue() const;
    
    /**
     * @brief Schedule next automatic sync
     */
    void scheduleNextSync();
    
    // Member variables
    DownloadManager* downloadManager_ = nullptr;
    Settings* settings_ = nullptr;
    SyncConfig config_;
    AuthInfo authInfo_;
    SyncResult lastSyncResult_;
    std::atomic<SyncStatus> syncStatus_;
    std::atomic<bool> enabled_;
    std::unique_ptr<std::thread> syncThread_;
    std::mutex mutex_;
    ConflictResolutionCallback conflictResolutionCallback_ = nullptr;
    SyncProgressCallback syncProgressCallback_ = nullptr;
    SyncCompletionCallback syncCompletionCallback_ = nullptr;
    std::chrono::system_clock::time_point nextScheduledSync_;
    std::map<SyncDataType, std::chrono::system_clock::time_point> lastSyncTimes_;
};

} // namespace core
} // namespace dm

#endif // CLOUD_INTEGRATION_H