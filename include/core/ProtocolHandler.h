#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

namespace dm {
namespace core {

// Forward declarations
class DownloadTask;

/**
 * @brief Protocol capability enumeration
 */
enum class ProtocolCapability {
    RESUME,             // Supports resuming downloads
    MULTI_SEGMENT,      // Supports multi-segment downloads
    STREAMING,          // Supports streaming
    DIRECTORY_LISTING,  // Supports directory listing
    AUTHENTICATION,     // Supports authentication
    ENCRYPTION,         // Supports encryption
    PROXYING,           // Supports proxying
    METADATA,           // Supports metadata retrieval
    RATE_LIMITING,      // Supports rate limiting
    COMPRESSION         // Supports compression
};

/**
 * @brief Protocol authentication method enumeration
 */
enum class AuthMethod {
    NONE,               // No authentication
    BASIC,              // Basic authentication
    DIGEST,             // Digest authentication
    NTLM,               // NTLM authentication
    KERBEROS,           // Kerberos authentication
    OAUTH,              // OAuth authentication
    CERTIFICATE,        // Certificate-based authentication
    CUSTOM              // Custom authentication
};

/**
 * @brief Protocol progress callback function type
 */
using ProtocolProgressCallback = std::function<void(int64_t totalBytes, int64_t downloadedBytes, 
                                                  double downloadSpeed)>;

/**
 * @brief Protocol error callback function type
 */
using ProtocolErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Protocol status callback function type
 */
using ProtocolStatusCallback = std::function<void(const std::string& status)>;

/**
 * @brief Protocol authentication callback function type
 */
using ProtocolAuthCallback = std::function<bool(const std::string& realm, 
                                              std::string& username, 
                                              std::string& password)>;

/**
 * @brief Protocol authentication information structure
 */
struct AuthInfo {
    AuthMethod method = AuthMethod::NONE;      // Authentication method
    std::string username;                      // Username
    std::string password;                      // Password
    std::string domain;                        // Domain (for NTLM)
    std::string realm;                         // Realm
    std::string token;                         // Token (for OAuth)
    std::string certificatePath;               // Path to certificate file
    std::string privateKeyPath;                // Path to private key file
    bool saveCredentials = false;              // Whether to save credentials
};

/**
 * @brief Protocol options structure
 */
struct ProtocolOptions {
    std::string userAgent;                     // User agent string
    int timeout = 30;                          // Timeout in seconds
    bool followRedirects = true;               // Whether to follow redirects
    int maxRedirects = 10;                     // Maximum number of redirects to follow
    bool verifyPeer = true;                    // Whether to verify peer certificate
    bool verifyHost = true;                    // Whether to verify host name
    std::string caCertPath;                    // Path to CA certificate file
    std::string proxyUrl;                      // Proxy URL
    AuthInfo proxyAuth;                        // Proxy authentication
    int bufferSize = 8192;                     // Buffer size for data transfer
    int connectTimeout = 10;                   // Connection timeout in seconds
    int lowSpeedLimit = 1;                     // Low speed limit in bytes per second
    int lowSpeedTime = 30;                     // Time in seconds for low speed limit
    bool verbose = false;                      // Enable verbose logging
    std::string cookieFile;                    // Path to cookie file
    bool keepAlive = true;                     // Enable keep-alive connections
    bool noSignal = true;                      // Disable signal handling
    std::map<std::string, std::string> headers; // Custom headers
    std::string interfaceName;                 // Network interface to use
    bool acceptEncoding = true;                // Accept encoding
    std::string outputFile;                    // Output file path
    std::string cacheDir;                      // Cache directory
    bool resumeSupport = true;                 // Whether to enable resume support
    int retryCount = 3;                        // Number of retries on failure
    int retryDelay = 5;                        // Delay between retries in seconds
};

/**
 * @brief Directory entry structure
 */
struct DirectoryEntry {
    std::string name;                          // File/directory name
    bool isDirectory = false;                  // Whether this is a directory
    int64_t size = 0;                          // File size in bytes
    std::string permissions;                   // File permissions
    std::string owner;                         // File owner
    std::string group;                         // File group
    std::string lastModified;                  // Last modified time
    std::string url;                           // Full URL
};

/**
 * @brief Abstract protocol handler interface
 * 
 * Base class for all protocol handlers
 */
class ProtocolHandler {
public:
    /**
     * @brief Construct a new ProtocolHandler
     */
    ProtocolHandler() = default;
    
    /**
     * @brief Destroy the ProtocolHandler
     */
    virtual ~ProtocolHandler() = default;
    
    /**
     * @brief Initialize the protocol handler
     * 
     * @return true if successful, false otherwise
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Cleanup the protocol handler
     */
    virtual void cleanup() = 0;
    
    /**
     * @brief Get the protocol name
     * 
     * @return std::string The protocol name
     */
    virtual std::string getProtocolName() const = 0;
    
    /**
     * @brief Get the protocol schemes
     * 
     * @return std::vector<std::string> The protocol schemes (e.g., "http", "https")
     */
    virtual std::vector<std::string> getProtocolSchemes() const = 0;
    
    /**
     * @brief Check if the protocol handler supports a capability
     * 
     * @param capability The capability to check
     * @return true if supported, false otherwise
     */
    virtual bool supportsCapability(ProtocolCapability capability) const = 0;
    
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
    virtual bool startDownload(const std::string& url, 
                              const std::string& outputFile,
                              std::shared_ptr<DownloadTask> task,
                              const ProtocolOptions& options,
                              ProtocolProgressCallback progressCallback = nullptr,
                              ProtocolErrorCallback errorCallback = nullptr,
                              ProtocolStatusCallback statusCallback = nullptr) = 0;
    
    /**
     * @brief Pause a download
     * 
     * @param task The download task
     * @return true if paused successfully, false otherwise
     */
    virtual bool pauseDownload(std::shared_ptr<DownloadTask> task) = 0;
    
    /**
     * @brief Resume a download
     * 
     * @param task The download task
     * @return true if resumed successfully, false otherwise
     */
    virtual bool resumeDownload(std::shared_ptr<DownloadTask> task) = 0;
    
    /**
     * @brief Cancel a download
     * 
     * @param task The download task
     * @return true if canceled successfully, false otherwise
     */
    virtual bool cancelDownload(std::shared_ptr<DownloadTask> task) = 0;
    
    /**
     * @brief Get file size
     * 
     * @param url The URL
     * @param options The protocol options
     * @return int64_t The file size in bytes, or -1 if unknown
     */
    virtual int64_t getFileSize(const std::string& url, const ProtocolOptions& options) = 0;
    
    /**
     * @brief Get file metadata
     * 
     * @param url The URL
     * @param options The protocol options
     * @return std::map<std::string, std::string> The metadata
     */
    virtual std::map<std::string, std::string> getMetadata(const std::string& url, 
                                                         const ProtocolOptions& options) = 0;
    
    /**
     * @brief List directory contents
     * 
     * @param url The directory URL
     * @param options The protocol options
     * @return std::vector<DirectoryEntry> The directory entries
     */
    virtual std::vector<DirectoryEntry> listDirectory(const std::string& url, 
                                                    const ProtocolOptions& options) = 0;
    
    /**
     * @brief Authenticate with the server
     * 
     * @param url The URL
     * @param authInfo The authentication information
     * @param options The protocol options
     * @return true if authenticated successfully, false otherwise
     */
    virtual bool authenticate(const std::string& url, 
                             const AuthInfo& authInfo, 
                             const ProtocolOptions& options) = 0;
    
    /**
     * @brief Check if a URL is supported by this protocol handler
     * 
     * @param url The URL to check
     * @return true if supported, false otherwise
     */
    virtual bool supportsUrl(const std::string& url) const = 0;
    
    /**
     * @brief Set the authentication callback
     * 
     * @param callback The authentication callback function
     */
    void setAuthCallback(ProtocolAuthCallback callback) {
        authCallback_ = callback;
    }
    
protected:
    /**
     * @brief Call the authentication callback
     * 
     * @param realm The authentication realm
     * @param username Output parameter for username
     * @param password Output parameter for password
     * @return true if authentication information was provided, false otherwise
     */
    bool callAuthCallback(const std::string& realm, std::string& username, std::string& password) {
        if (authCallback_) {
            return authCallback_(realm, username, password);
        }
        return false;
    }
    
private:
    ProtocolAuthCallback authCallback_ = nullptr;
};

/**
 * @brief Protocol handler factory class
 * 
 * Creates and manages protocol handlers
 */
class ProtocolHandlerFactory {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return ProtocolHandlerFactory& The singleton instance
     */
    static ProtocolHandlerFactory& getInstance();
    
    /**
     * @brief Register a protocol handler
     * 
     * @param handler The protocol handler to register
     * @return true if registered successfully, false otherwise
     */
    bool registerHandler(std::shared_ptr<ProtocolHandler> handler);
    
    /**
     * @brief Unregister a protocol handler
     * 
     * @param protocolName The protocol name
     * @return true if unregistered successfully, false otherwise
     */
    bool unregisterHandler(const std::string& protocolName);
    
    /**
     * @brief Get a protocol handler for a URL
     * 
     * @param url The URL
     * @return std::shared_ptr<ProtocolHandler> The protocol handler, or nullptr if none found
     */
    std::shared_ptr<ProtocolHandler> getHandlerForUrl(const std::string& url);
    
    /**
     * @brief Get a protocol handler by name
     * 
     * @param protocolName The protocol name
     * @return std::shared_ptr<ProtocolHandler> The protocol handler, or nullptr if not found
     */
    std::shared_ptr<ProtocolHandler> getHandlerByName(const std::string& protocolName);
    
    /**
     * @brief Get all registered protocol handlers
     * 
     * @return std::vector<std::shared_ptr<ProtocolHandler>> All registered handlers
     */
    std::vector<std::shared_ptr<ProtocolHandler>> getAllHandlers() const;
    
    /**
     * @brief Check if a protocol is supported
     * 
     * @param protocol The protocol scheme (e.g., "http", "https")
     * @return true if supported, false otherwise
     */
    bool isProtocolSupported(const std::string& protocol) const;
    
    /**
     * @brief Get the default protocol options
     * 
     * @return ProtocolOptions The default options
     */
    ProtocolOptions getDefaultOptions() const;
    
    /**
     * @brief Set the default protocol options
     * 
     * @param options The default options
     */
    void setDefaultOptions(const ProtocolOptions& options);
    
    /**
     * @brief Set the global authentication callback
     * 
     * @param callback The authentication callback function
     */
    void setGlobalAuthCallback(ProtocolAuthCallback callback);
    
private:
    /**
     * @brief Construct a new ProtocolHandlerFactory
     */
    ProtocolHandlerFactory() = default;
    
    /**
     * @brief Destroy the ProtocolHandlerFactory
     */
    ~ProtocolHandlerFactory() = default;
    
    // Prevent copying
    ProtocolHandlerFactory(const ProtocolHandlerFactory&) = delete;
    ProtocolHandlerFactory& operator=(const ProtocolHandlerFactory&) = delete;
    
    /**
     * @brief Extract protocol from URL
     * 
     * @param url The URL
     * @return std::string The protocol scheme
     */
    std::string extractProtocol(const std::string& url) const;
    
    // Member variables
    std::map<std::string, std::shared_ptr<ProtocolHandler>> handlersByName_;
    std::map<std::string, std::shared_ptr<ProtocolHandler>> handlersByScheme_;
    mutable std::mutex mutex_;
    ProtocolOptions defaultOptions_;
    ProtocolAuthCallback globalAuthCallback_ = nullptr;
};

} // namespace core
} // namespace dm

#endif // PROTOCOL_HANDLER_H