#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

namespace dm {
namespace core {

// Forward declarations
class DownloadManager;
class Settings;

/**
 * @brief Network type enumeration
 */
enum class NetworkType {
    UNKNOWN,        // Unknown network type
    ETHERNET,       // Wired Ethernet
    WIFI,           // WiFi connection
    MOBILE_4G,      // 4G mobile network
    MOBILE_3G,      // 3G mobile network
    MOBILE_2G,      // 2G mobile network
    MOBILE_OTHER,   // Other mobile network
    DIAL_UP,        // Dial-up connection
    OFFLINE         // No network connection
};

/**
 * @brief Network status enumeration
 */
enum class NetworkStatus {
    UNKNOWN,        // Unknown status
    CONNECTED,      // Connected to network
    CONNECTING,     // Connecting to network
    DISCONNECTED,   // Disconnected from network
    LIMITED         // Limited connectivity
};

/**
 * @brief Network speed structure
 */
struct NetworkSpeed {
    double downloadSpeed = 0.0;  // Download speed in bytes/second
    double uploadSpeed = 0.0;    // Upload speed in bytes/second
    double latency = 0.0;        // Latency in milliseconds
    double packetLoss = 0.0;     // Packet loss percentage
    double jitter = 0.0;         // Jitter in milliseconds
};

/**
 * @brief Network information structure
 */
struct NetworkInfo {
    NetworkType type = NetworkType::UNKNOWN;           // Network type
    NetworkStatus status = NetworkStatus::UNKNOWN;     // Network status
    NetworkSpeed speed;                                // Network speed
    std::string connectionName;                        // Connection name
    std::string interfaceName;                         // Network interface name
    std::string ipAddress;                             // IP address
    std::string gatewayAddress;                        // Gateway address
    std::string dnsServers;                            // DNS servers
    bool isMetered = false;                            // Whether the connection is metered
    bool isCaptivePortal = false;                      // Whether a captive portal is present
    std::chrono::system_clock::time_point connectedTime; // Time when connected
};

/**
 * @brief Profile type enumeration
 */
enum class ProfileType {
    DEFAULT,        // Default profile
    LOW_BANDWIDTH,  // Low bandwidth profile
    HIGH_BANDWIDTH, // High bandwidth profile
    METERED,        // Metered connection profile
    CUSTOM          // Custom profile
};

/**
 * @brief Network profile structure
 */
struct NetworkProfile {
    std::string name;                // Profile name
    ProfileType type;                // Profile type
    int maxConcurrentDownloads;      // Maximum concurrent downloads
    int maxSegmentsPerDownload;      // Maximum segments per download
    int maxBandwidth;                // Maximum bandwidth in KB/s (0 for unlimited)
    bool pauseOnMetered;             // Pause downloads on metered connections
    bool resumeOnUnmetered;          // Resume downloads when connection is no longer metered
    bool enableAdaptiveBandwidth;    // Enable adaptive bandwidth adjustment
    int retryCount;                  // Number of retries on failure
    int retryDelay;                  // Delay between retries in seconds
    bool enableQos;                  // Enable Quality of Service
    int qosPriority;                 // QoS priority (1-7, higher is more important)
};

/**
 * @brief Network status change callback function type
 */
using NetworkStatusCallback = std::function<void(const NetworkInfo& info)>;

/**
 * @brief Network speed change callback function type
 */
using NetworkSpeedCallback = std::function<void(const NetworkSpeed& speed)>;

/**
 * @brief Network profile change callback function type
 */
using NetworkProfileCallback = std::function<void(const NetworkProfile& profile)>;

/**
 * @brief Network monitor class
 * 
 * Monitors network conditions and adjusts download behavior
 */
class NetworkMonitor {
public:
    /**
     * @brief Construct a new NetworkMonitor
     * 
     * @param downloadManager Reference to download manager
     * @param settings Reference to settings
     */
    explicit NetworkMonitor(DownloadManager& downloadManager, Settings& settings);
    
    /**
     * @brief Destroy the NetworkMonitor
     */
    ~NetworkMonitor();
    
    /**
     * @brief Initialize the network monitor
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Start monitoring
     * 
     * @return true if started successfully, false otherwise
     */
    bool start();
    
    /**
     * @brief Stop monitoring
     */
    void stop();
    
    /**
     * @brief Get current network information
     * 
     * @return NetworkInfo The current network information
     */
    NetworkInfo getCurrentNetworkInfo() const;
    
    /**
     * @brief Get current network speed
     * 
     * @return NetworkSpeed The current network speed
     */
    NetworkSpeed getCurrentNetworkSpeed() const;
    
    /**
     * @brief Get current network profile
     * 
     * @return NetworkProfile The current network profile
     */
    NetworkProfile getCurrentNetworkProfile() const;
    
    /**
     * @brief Test network speed
     * 
     * @param serverUrl Optional speed test server URL
     * @return NetworkSpeed The measured network speed
     */
    NetworkSpeed testNetworkSpeed(const std::string& serverUrl = "");
    
    /**
     * @brief Check if network is available
     * 
     * @return true if available, false otherwise
     */
    bool isNetworkAvailable() const;
    
    /**
     * @brief Check if connection is metered
     * 
     * @return true if metered, false otherwise
     */
    bool isConnectionMetered() const;
    
    /**
     * @brief Get all available network profiles
     * 
     * @return std::vector<NetworkProfile> The available profiles
     */
    std::vector<NetworkProfile> getNetworkProfiles() const;
    
    /**
     * @brief Add a network profile
     * 
     * @param profile The profile to add
     * @return true if added successfully, false otherwise
     */
    bool addNetworkProfile(const NetworkProfile& profile);
    
    /**
     * @brief Update a network profile
     * 
     * @param profile The profile to update
     * @return true if updated successfully, false otherwise
     */
    bool updateNetworkProfile(const NetworkProfile& profile);
    
    /**
     * @brief Remove a network profile
     * 
     * @param profileName The name of the profile to remove
     * @return true if removed successfully, false otherwise
     */
    bool removeNetworkProfile(const std::string& profileName);
    
    /**
     * @brief Apply a network profile
     * 
     * @param profileName The name of the profile to apply
     * @return true if applied successfully, false otherwise
     */
    bool applyNetworkProfile(const std::string& profileName);
    
    /**
     * @brief Enable or disable adaptive bandwidth adjustment
     * 
     * @param enable True to enable, false to disable
     */
    void setAdaptiveBandwidth(bool enable);
    
    /**
     * @brief Check if adaptive bandwidth adjustment is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isAdaptiveBandwidthEnabled() const;
    
    /**
     * @brief Set the network status callback
     * 
     * @param callback The callback function
     */
    void setNetworkStatusCallback(NetworkStatusCallback callback);
    
    /**
     * @brief Set the network speed callback
     * 
     * @param callback The callback function
     */
    void setNetworkSpeedCallback(NetworkSpeedCallback callback);
    
    /**
     * @brief Set the network profile callback
     * 
     * @param callback The callback function
     */
    void setNetworkProfileCallback(NetworkProfileCallback callback);
    
    /**
     * @brief Enable or disable automatic profile switching
     * 
     * @param enable True to enable, false to disable
     */
    void setAutoProfileSwitching(bool enable);
    
    /**
     * @brief Check if automatic profile switching is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isAutoProfileSwitchingEnabled() const;
    
private:
    /**
     * @brief Update network information
     */
    void updateNetworkInfo();
    
    /**
     * @brief Measure network speed
     */
    void measureNetworkSpeed();
    
    /**
     * @brief Select appropriate network profile based on conditions
     */
    void selectNetworkProfile();
    
    /**
     * @brief Apply bandwidth limits based on current profile
     */
    void applyBandwidthLimits();
    
    /**
     * @brief Handle network status changes
     */
    void handleNetworkStatusChange();
    
    /**
     * @brief Monitor thread function
     */
    void monitorThread();
    
    /**
     * @brief Create default network profiles
     */
    void createDefaultProfiles();
    
    /**
     * @brief Load network profiles from settings
     */
    void loadNetworkProfiles();
    
    /**
     * @brief Save network profiles to settings
     */
    void saveNetworkProfiles();
    
    /**
     * @brief Detect network type
     * 
     * @return NetworkType The detected network type
     */
    NetworkType detectNetworkType();
    
    /**
     * @brief Check if a captive portal is present
     * 
     * @return true if present, false otherwise
     */
    bool detectCaptivePortal();
    
    /**
     * @brief Get network interface information
     * 
     * @param interfaceName The network interface name
     * @return NetworkInfo Information about the interface
     */
    NetworkInfo getInterfaceInfo(const std::string& interfaceName);
    
    // Member variables
    DownloadManager& downloadManager_;
    Settings& settings_;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> monitorThread_;
    mutable std::mutex mutex_;
    
    NetworkInfo currentNetworkInfo_;
    NetworkSpeed currentNetworkSpeed_;
    NetworkProfile currentProfile_;
    
    std::map<std::string, NetworkProfile> profiles_;
    
    std::atomic<bool> adaptiveBandwidthEnabled_;
    std::atomic<bool> autoProfileSwitchingEnabled_;
    
    std::chrono::seconds monitorInterval_;
    std::chrono::steady_clock::time_point lastSpeedMeasurement_;
    
    NetworkStatusCallback networkStatusCallback_ = nullptr;
    NetworkSpeedCallback networkSpeedCallback_ = nullptr;
    NetworkProfileCallback networkProfileCallback_ = nullptr;
};

} // namespace core
} // namespace dm

#endif // NETWORK_MONITOR_H