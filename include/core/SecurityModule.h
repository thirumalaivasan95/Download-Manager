#ifndef SECURITY_MODULE_H
#define SECURITY_MODULE_H

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
class DownloadTask;

/**
 * @brief Security risk level enumeration
 */
enum class RiskLevel {
    SAFE,               // No risk detected
    LOW_RISK,           // Low risk level
    MEDIUM_RISK,        // Medium risk level
    HIGH_RISK,          // High risk level
    CRITICAL_RISK       // Critical risk level
};

/**
 * @brief Scan result structure
 */
struct ScanResult {
    bool scanned = false;                    // Whether the file was scanned
    RiskLevel riskLevel = RiskLevel::SAFE;   // Detected risk level
    std::string threatName;                  // Name of detected threat (if any)
    std::string details;                     // Additional details
    std::chrono::system_clock::time_point scanTime; // Time of scan
    std::string scannerName;                 // Name of scanner used
    std::string filePath;                    // Path to scanned file
    std::string fileHash;                    // Hash of scanned file
    bool quarantined = false;                // Whether the file was quarantined
};

/**
 * @brief Security policy structure
 */
struct SecurityPolicy {
    bool enableFileScanning = true;          // Enable file scanning
    bool scanBeforeDownload = true;          // Scan before download starts
    bool scanAfterDownload = true;           // Scan after download completes
    bool blockRiskyDownloads = true;         // Block downloads with security risks
    bool confirmRiskyDownloads = true;       // Confirm risky downloads with user
    bool enableSafeDownloadsOnly = false;    // Only allow downloads from safe sites
    bool checkFileSignatures = true;         // Check digital signatures
    bool enableSSLVerification = true;       // Enable SSL certificate verification
    bool quarantineRiskyFiles = true;        // Quarantine risky files
    bool enableContentFiltering = true;      // Enable content filtering
    bool enableFileTypeVerification = true;  // Verify file types
    std::vector<std::string> trustedDomains; // Trusted domains
    std::vector<std::string> blockedDomains; // Blocked domains
    std::map<std::string, bool> fileTypeRules; // Rules for file types
    int maxFileSizeMB = 0;                   // Max file size (0 for unlimited)
    bool enablePasswordProtection = false;   // Enable password protection
    std::string password;                    // Password hash
    bool logSecurityEvents = true;           // Log security events
    bool enableRealTimeProtection = true;    // Enable real-time protection
    bool promptForUnknownFileTypes = true;   // Prompt for unknown file types
};

/**
 * @brief Certificate info structure
 */
struct CertificateInfo {
    std::string subject;                     // Certificate subject
    std::string issuer;                      // Certificate issuer
    std::string serialNumber;                // Certificate serial number
    std::chrono::system_clock::time_point validFrom; // Valid from date
    std::chrono::system_clock::time_point validTo;   // Valid to date
    std::string fingerprint;                 // Certificate fingerprint
    std::string publicKey;                   // Public key
    int keySize = 0;                         // Key size in bits
    std::string signatureAlgorithm;          // Signature algorithm
    bool trusted = false;                    // Whether the certificate is trusted
    std::vector<std::string> subjectAltNames; // Subject alternative names
    bool valid = false;                      // Whether the certificate is valid
};

/**
 * @brief File verification result structure
 */
struct FileVerificationResult {
    bool verified = false;                   // Whether the file was verified
    bool matchesExtension = false;           // Whether the file matches its extension
    bool isTruncated = false;                // Whether the file is truncated
    bool hasValidHeader = false;             // Whether the file has a valid header
    bool hasValidFooter = false;             // Whether the file has a valid footer
    std::string detectedFileType;            // Detected file type
    std::string expectedFileType;            // Expected file type based on extension
    std::string details;                     // Additional details
};

/**
 * @brief Security event type enumeration
 */
enum class SecurityEventType {
    MALWARE_DETECTED,                        // Malware detected
    SUSPICIOUS_FILE_DETECTED,                // Suspicious file detected
    SSL_CERTIFICATE_ERROR,                   // SSL certificate error
    POLICY_VIOLATION,                        // Security policy violation
    AUTHENTICATION_FAILURE,                  // Authentication failure
    QUARANTINE_ACTION,                       // File quarantined
    BLOCKED_DOMAIN,                          // Blocked domain
    SIGNATURE_VERIFICATION_FAILURE,          // Signature verification failure
    FILE_TYPE_MISMATCH,                      // File type mismatch
    DOWNLOAD_BLOCKED,                        // Download blocked
    PASSWORD_PROTECTION_TRIGGERED,           // Password protection triggered
    UNSAFE_DOWNLOAD_CONFIRMED,               // Unsafe download confirmed by user
    REAL_TIME_PROTECTION_EVENT               // Real-time protection event
};

/**
 * @brief Security event structure
 */
struct SecurityEvent {
    SecurityEventType type;                  // Event type
    std::string message;                     // Event message
    std::chrono::system_clock::time_point timestamp; // Event timestamp
    std::string url;                         // Related URL
    std::string filePath;                    // Related file path
    std::string taskId;                      // Related task ID
    RiskLevel riskLevel = RiskLevel::SAFE;   // Risk level
    std::string details;                     // Additional details
};

/**
 * @brief Scan callback type
 */
using ScanCallback = std::function<void(const ScanResult& result)>;

/**
 * @brief Security event callback type
 */
using SecurityEventCallback = std::function<void(const SecurityEvent& event)>;

/**
 * @brief Download verification callback type
 */
using DownloadVerificationCallback = 
    std::function<bool(const std::string& url, const std::string& filePath, const ScanResult& result)>;

/**
 * @brief Security module class
 * 
 * Provides security features for the download manager
 */
class SecurityModule {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return SecurityModule& The singleton instance
     */
    static SecurityModule& getInstance();
    
    /**
     * @brief Initialize the security module
     * 
     * @param downloadManager Reference to download manager
     * @return true if successful, false otherwise
     */
    bool initialize(DownloadManager& downloadManager);
    
    /**
     * @brief Shutdown the security module
     */
    void shutdown();
    
    /**
     * @brief Scan a file
     * 
     * @param filePath Path to the file
     * @param callback Optional callback for scan result
     * @return ScanResult The scan result
     */
    ScanResult scanFile(const std::string& filePath, ScanCallback callback = nullptr);
    
    /**
     * @brief Scan a file asynchronously
     * 
     * @param filePath Path to the file
     * @param callback Callback for scan result
     */
    void scanFileAsync(const std::string& filePath, ScanCallback callback);
    
    /**
     * @brief Verify URL before download
     * 
     * @param url The URL to verify
     * @return true if safe to download, false otherwise
     */
    bool verifyUrl(const std::string& url);
    
    /**
     * @brief Get SSL certificate info
     * 
     * @param url The URL
     * @return CertificateInfo The certificate info
     */
    CertificateInfo getCertificateInfo(const std::string& url);
    
    /**
     * @brief Verify file type
     * 
     * @param filePath Path to the file
     * @return FileVerificationResult The verification result
     */
    FileVerificationResult verifyFileType(const std::string& filePath);
    
    /**
     * @brief Check if a domain is trusted
     * 
     * @param domain The domain to check
     * @return true if trusted, false otherwise
     */
    bool isDomainTrusted(const std::string& domain);
    
    /**
     * @brief Check if a domain is blocked
     * 
     * @param domain The domain to check
     * @return true if blocked, false otherwise
     */
    bool isDomainBlocked(const std::string& domain);
    
    /**
     * @brief Add a trusted domain
     * 
     * @param domain The domain to add
     */
    void addTrustedDomain(const std::string& domain);
    
    /**
     * @brief Remove a trusted domain
     * 
     * @param domain The domain to remove
     */
    void removeTrustedDomain(const std::string& domain);
    
    /**
     * @brief Add a blocked domain
     * 
     * @param domain The domain to add
     */
    void addBlockedDomain(const std::string& domain);
    
    /**
     * @brief Remove a blocked domain
     * 
     * @param domain The domain to remove
     */
    void removeBlockedDomain(const std::string& domain);
    
    /**
     * @brief Quarantine a file
     * 
     * @param filePath Path to the file
     * @return true if successful, false otherwise
     */
    bool quarantineFile(const std::string& filePath);
    
    /**
     * @brief Restore a file from quarantine
     * 
     * @param fileId The file ID in quarantine
     * @param destPath Destination path
     * @return true if successful, false otherwise
     */
    bool restoreFromQuarantine(const std::string& fileId, const std::string& destPath);
    
    /**
     * @brief Get quarantined files
     * 
     * @return std::vector<std::string> The quarantined file IDs
     */
    std::vector<std::string> getQuarantinedFiles();
    
    /**
     * @brief Set security policy
     * 
     * @param policy The security policy
     */
    void setSecurityPolicy(const SecurityPolicy& policy);
    
    /**
     * @brief Get security policy
     * 
     * @return SecurityPolicy The security policy
     */
    SecurityPolicy getSecurityPolicy() const;
    
    /**
     * @brief Set security event callback
     * 
     * @param callback The callback function
     */
    void setSecurityEventCallback(SecurityEventCallback callback);
    
    /**
     * @brief Set download verification callback
     * 
     * @param callback The callback function
     */
    void setDownloadVerificationCallback(DownloadVerificationCallback callback);
    
    /**
     * @brief Get security events
     * 
     * @param limit Maximum number of events to return (0 for all)
     * @param startTime Start time for events
     * @param endTime End time for events
     * @return std::vector<SecurityEvent> The security events
     */
    std::vector<SecurityEvent> getSecurityEvents(
        int limit = 0,
        std::chrono::system_clock::time_point startTime = std::chrono::system_clock::time_point(),
        std::chrono::system_clock::time_point endTime = std::chrono::system_clock::time_point());
    
    /**
     * @brief Clear security events
     */
    void clearSecurityEvents();
    
    /**
     * @brief Enable or disable the security module
     * 
     * @param enable True to enable, false to disable
     */
    void setEnabled(bool enable);
    
    /**
     * @brief Check if the security module is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;
    
    /**
     * @brief Set password
     * 
     * @param password The password
     * @return true if successful, false otherwise
     */
    bool setPassword(const std::string& password);
    
    /**
     * @brief Verify password
     * 
     * @param password The password to verify
     * @return true if correct, false otherwise
     */
    bool verifyPassword(const std::string& password);
    
    /**
     * @brief Check if password protection is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isPasswordProtectionEnabled() const;
    
    /**
     * @brief Enable or disable password protection
     * 
     * @param enable True to enable, false to disable
     */
    void setPasswordProtectionEnabled(bool enable);
    
    /**
     * @brief Get quarantine directory
     * 
     * @return std::string The quarantine directory
     */
    std::string getQuarantineDirectory() const;
    
    /**
     * @brief Set quarantine directory
     * 
     * @param directory The quarantine directory
     */
    void setQuarantineDirectory(const std::string& directory);
    
private:
    /**
     * @brief Construct a new SecurityModule
     */
    SecurityModule();
    
    /**
     * @brief Destroy the SecurityModule
     */
    ~SecurityModule();
    
    // Prevent copying
    SecurityModule(const SecurityModule&) = delete;
    SecurityModule& operator=(const SecurityModule&) = delete;
    
    /**
     * @brief Log a security event
     * 
     * @param type The event type
     * @param message The event message
     * @param url The related URL
     * @param filePath The related file path
     * @param taskId The related task ID
     * @param riskLevel The risk level
     * @param details Additional details
     */
    void logSecurityEvent(SecurityEventType type, 
                         const std::string& message,
                         const std::string& url = "",
                         const std::string& filePath = "",
                         const std::string& taskId = "",
                         RiskLevel riskLevel = RiskLevel::SAFE,
                         const std::string& details = "");
    
    /**
     * @brief Extract domain from URL
     * 
     * @param url The URL
     * @return std::string The domain
     */
    std::string extractDomain(const std::string& url) const;
    
    /**
     * @brief Calculate file hash
     * 
     * @param filePath Path to the file
     * @return std::string The file hash
     */
    std::string calculateFileHash(const std::string& filePath) const;
    
    /**
     * @brief Detect file type
     * 
     * @param filePath Path to the file
     * @return std::string The detected file type
     */
    std::string detectFileType(const std::string& filePath) const;
    
    /**
     * @brief Check file headers and footers
     * 
     * @param filePath Path to the file
     * @param fileType Expected file type
     * @return true if valid, false otherwise
     */
    bool checkFileHeadersAndFooters(const std::string& filePath, const std::string& fileType) const;
    
    /**
     * @brief Generate a quarantine file ID
     * 
     * @param filePath Path to the file
     * @return std::string The quarantine file ID
     */
    std::string generateQuarantineFileId(const std::string& filePath) const;
    
    /**
     * @brief Hash password
     * 
     * @param password The password to hash
     * @return std::string The password hash the hash
     */
    std::string hashPassword(const std::string& password) const;
    
    // Member variables
    DownloadManager* downloadManager_ = nullptr;
    SecurityPolicy policy_;
    std::string quarantineDirectory_;
    std::vector<SecurityEvent> securityEvents_;
    std::atomic<bool> enabled_;
    mutable std::mutex mutex_;
    SecurityEventCallback securityEventCallback_ = nullptr;
    DownloadVerificationCallback downloadVerificationCallback_ = nullptr;
};

} // namespace core
} // namespace dm

#endif // SECURITY_MODULE_H