#ifndef STATISTICS_H
#define STATISTICS_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace dm {
namespace core {

// Forward declarations
class DownloadManager;
class DownloadTask;

/**
 * @brief Time period enumeration
 */
enum class TimePeriod {
    HOUR,
    DAY,
    WEEK,
    MONTH,
    YEAR,
    ALL_TIME
};

/**
 * @brief Data granularity enumeration
 */
enum class DataGranularity {
    MINUTE,
    HOUR,
    DAY,
    WEEK,
    MONTH
};

/**
 * @brief Statistic type enumeration
 */
enum class StatisticType {
    DOWNLOADED_BYTES,
    DOWNLOAD_COUNT,
    AVERAGE_SPEED,
    PEAK_SPEED,
    DOWNLOAD_TIME,
    SUCCESS_RATE,
    BANDWIDTH_USAGE,
    FILE_TYPES,
    DOMAINS,
    ERROR_COUNT,
    CONCURRENT_DOWNLOADS
};

/**
 * @brief Data point structure
 */
struct DataPoint {
    std::chrono::system_clock::time_point timestamp;  // Timestamp
    double value;                                    // Value
};

/**
 * @brief Download statistics structure
 */
struct DownloadStatistics {
    std::string taskId;                              // Task ID
    std::string url;                                 // URL
    std::string fileName;                            // File name
    int64_t fileSize;                                // File size in bytes
    int64_t downloadedBytes;                         // Downloaded bytes
    double averageSpeed;                             // Average speed in bytes/second
    double peakSpeed;                                // Peak speed in bytes/second
    std::chrono::milliseconds downloadTime;          // Download time
    bool successful;                                 // Whether download was successful
    std::string error;                               // Error message (if any)
    std::chrono::system_clock::time_point startTime; // Start time
    std::chrono::system_clock::time_point endTime;   // End time
    std::string fileType;                            // File type
    std::string domain;                              // Domain
    int segmentCount;                                // Number of segments
    int retryCount;                                  // Number of retries
    bool resumed;                                    // Whether download was resumed
    std::string protocol;                            // Protocol used
    std::string sourceName;                          // Source name (e.g., batch job name)
};

/**
 * @brief Session statistics structure
 */
struct SessionStatistics {
    std::chrono::system_clock::time_point startTime; // Session start time
    std::chrono::milliseconds duration;              // Session duration
    int64_t totalDownloadedBytes;                    // Total downloaded bytes
    int downloadCount;                               // Number of downloads
    int successfulDownloads;                         // Number of successful downloads
    int failedDownloads;                             // Number of failed downloads
    double averageSpeed;                             // Average speed in bytes/second
    double peakSpeed;                                // Peak speed in bytes/second
    int peakConcurrentDownloads;                     // Peak concurrent downloads
    std::map<std::string, int> fileTypeCounts;       // File type counts
    std::map<std::string, int> domainCounts;         // Domain counts
    std::map<std::string, int> errorCounts;          // Error counts
};

/**
 * @brief Aggregate statistics structure
 */
struct AggregateStatistics {
    int64_t totalDownloadedBytes;                    // Total downloaded bytes
    int totalDownloadCount;                          // Total download count
    int successfulDownloads;                         // Number of successful downloads
    int failedDownloads;                             // Number of failed downloads
    double overallSuccessRate;                       // Overall success rate
    double averageSpeed;                             // Average speed in bytes/second
    double peakSpeed;                                // Peak speed in bytes/second
    std::chrono::milliseconds totalDownloadTime;     // Total download time
    std::chrono::milliseconds averageDownloadTime;   // Average download time
    std::map<std::string, int64_t> fileTypeBytes;    // Bytes by file type
    std::map<std::string, int> fileTypeCounts;       // File type counts
    std::map<std::string, int64_t> domainBytes;      // Bytes by domain
    std::map<std::string, int> domainCounts;         // Domain counts
    std::map<std::string, int> errorCounts;          // Error counts
    int peakConcurrentDownloads;                     // Peak concurrent downloads
};

/**
 * @brief Statistics export format enumeration
 */
enum class ExportFormat {
    CSV,
    JSON,
    XML,
    HTML,
    PDF
};

/**
 * @brief Statistics query parameters structure
 */
struct StatisticsQuery {
    TimePeriod period = TimePeriod::DAY;             // Time period
    std::chrono::system_clock::time_point startTime; // Custom start time
    std::chrono::system_clock::time_point endTime;   // Custom end time
    std::vector<StatisticType> types;                // Statistic types to include
    DataGranularity granularity = DataGranularity::HOUR; // Data granularity
    std::string fileType;                            // Filter by file type
    std::string domain;                              // Filter by domain
    std::string protocol;                            // Filter by protocol
    bool includeFailedDownloads = true;              // Include failed downloads
    int limit = 0;                                   // Limit number of results (0 for no limit)
    std::string sortBy;                              // Sort field
    bool sortAscending = false;                      // Sort direction
};

/**
 * @brief Statistics manager class
 * 
 * Collects and manages download statistics
 */
class StatisticsManager {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return StatisticsManager& The singleton instance
     */
    static StatisticsManager& getInstance();
    
    /**
     * @brief Initialize the statistics manager
     * 
     * @param downloadManager Reference to download manager
     * @return true if successful, false otherwise
     */
    bool initialize(DownloadManager& downloadManager);
    
    /**
     * @brief Shutdown the statistics manager
     */
    void shutdown();
    
    /**
     * @brief Record download started
     * 
     * @param task The download task
     */
    void recordDownloadStarted(std::shared_ptr<DownloadTask> task);
    
    /**
     * @brief Record download completed
     * 
     * @param task The download task
     * @param successful Whether the download was successful
     * @param error Error message (if any)
     */
    void recordDownloadCompleted(std::shared_ptr<DownloadTask> task, 
                               bool successful, 
                               const std::string& error = "");
    
    /**
     * @brief Record download progress
     * 
     * @param task The download task
     * @param downloadedBytes Downloaded bytes
     * @param speed Current speed in bytes/second
     */
    void recordDownloadProgress(std::shared_ptr<DownloadTask> task, 
                              int64_t downloadedBytes, 
                              double speed);
    
    /**
     * @brief Get download statistics
     * 
     * @param taskId The task ID
     * @return DownloadStatistics The download statistics
     */
    DownloadStatistics getDownloadStatistics(const std::string& taskId) const;
    
    /**
     * @brief Get current session statistics
     * 
     * @return SessionStatistics The session statistics
     */
    SessionStatistics getCurrentSessionStatistics() const;
    
    /**
     * @brief Get aggregate statistics
     * 
     * @param query The statistics query
     * @return AggregateStatistics The aggregate statistics
     */
    AggregateStatistics getAggregateStatistics(const StatisticsQuery& query) const;
    
    /**
     * @brief Get time series data
     * 
     * @param type The statistic type
     * @param query The statistics query
     * @return std::vector<DataPoint> The time series data
     */
    std::vector<DataPoint> getTimeSeriesData(StatisticType type, 
                                           const StatisticsQuery& query) const;
    
    /**
     * @brief Export statistics
     * 
     * @param filePath Path to output file
     * @param format Export format
     * @param query The statistics query
     * @return true if successful, false otherwise
     */
    bool exportStatistics(const std::string& filePath, 
                         ExportFormat format, 
                         const StatisticsQuery& query) const;
    
    /**
     * @brief Generate report
     * 
     * @param filePath Path to output file
     * @param format Export format
     * @param query The statistics query
     * @param includeCharts Whether to include charts
     * @return true if successful, false otherwise
     */
    bool generateReport(const std::string& filePath, 
                       ExportFormat format, 
                       const StatisticsQuery& query, 
                       bool includeCharts = true) const;
    
    /**
     * @brief Clear statistics
     * 
     * @param period The time period to clear (ALL_TIME to clear all)
     * @return true if successful, false otherwise
     */
    bool clearStatistics(TimePeriod period = TimePeriod::ALL_TIME);
    
    /**
     * @brief Get file type distribution
     * 
     * @param query The statistics query
     * @return std::map<std::string, double> Map of file type to percentage
     */
    std::map<std::string, double> getFileTypeDistribution(const StatisticsQuery& query) const;
    
    /**
     * @brief Get domain distribution
     * 
     * @param query The statistics query
     * @return std::map<std::string, double> Map of domain to percentage
     */
    std::map<std::string, double> getDomainDistribution(const StatisticsQuery& query) const;
    
    /**
     * @brief Get error distribution
     * 
     * @param query The statistics query
     * @return std::map<std::string, double> Map of error to percentage
     */
    std::map<std::string, double> getErrorDistribution(const StatisticsQuery& query) const;
    
    /**
     * @brief Get hourly download pattern
     * 
     * @param query The statistics query
     * @return std::vector<double> Download percentage for each hour (0-23)
     */
    std::vector<double> getHourlyDownloadPattern(const StatisticsQuery& query) const;
    
    /**
     * @brief Get daily download pattern
     * 
     * @param query The statistics query
     * @return std::vector<double> Download percentage for each day (0-6, Sunday-Saturday)
     */
    std::vector<double> getDailyDownloadPattern(const StatisticsQuery& query) const;
    
    /**
     * @brief Get top domains
     * 
     * @param query The statistics query
     * @param limit Maximum number of domains to return
     * @return std::vector<std::pair<std::string, int64_t>> Top domains with download counts
     */
    std::vector<std::pair<std::string, int64_t>> getTopDomains(
        const StatisticsQuery& query, int limit = 10) const;
    
    /**
     * @brief Get top file types
     * 
     * @param query The statistics query
     * @param limit Maximum number of file types to return
     * @return std::vector<std::pair<std::string, int64_t>> Top file types with download counts
     */
    std::vector<std::pair<std::string, int64_t>> getTopFileTypes(
        const StatisticsQuery& query, int limit = 10) const;
    
    /**
     * @brief Get database file path
     * 
     * @return std::string The database file path
     */
    std::string getDatabaseFilePath() const;
    
    /**
     * @brief Set database file path
     * 
     * @param filePath The database file path
     */
    void setDatabaseFilePath(const std::string& filePath);
    
    /**
     * @brief Enable or disable statistics collection
     * 
     * @param enable True to enable, false to disable
     */
    void setStatisticsEnabled(bool enable);
    
    /**
     * @brief Check if statistics collection is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isStatisticsEnabled() const;
    
private:
    /**
     * @brief Construct a new StatisticsManager
     */
    StatisticsManager();
    
    /**
     * @brief Destroy the StatisticsManager
     */
    ~StatisticsManager();
    
    // Prevent copying
    StatisticsManager(const StatisticsManager&) = delete;
    StatisticsManager& operator=(const StatisticsManager&) = delete;
    
    /**
     * @brief Initialize database
     * 
     * @return true if successful, false otherwise
     */
    bool initializeDatabase();
    
    /**
     * @brief Save statistics to database
     * 
     * @return true if successful, false otherwise
     */
    bool saveStatistics();
    
    /**
     * @brief Load statistics from database
     * 
     * @return true if successful, false otherwise
     */
    bool loadStatistics();
    
    /**
     * @brief Extract domain from URL
     * 
     * @param url The URL
     * @return std::string The domain
     */
    std::string extractDomain(const std::string& url) const;
    
    /**
     * @brief Extract file type from file name
     * 
     * @param fileName The file name
     * @return std::string The file type
     */
    std::string extractFileType(const std::string& fileName) const;
    
    /**
     * @brief Convert time period to time range
     * 
     * @param period The time period
     * @param startTime Output parameter for start time
     * @param endTime Output parameter for end time
     */
    void timeRangeFromPeriod(TimePeriod period, 
                            std::chrono::system_clock::time_point& startTime,
                            std::chrono::system_clock::time_point& endTime) const;
    
    /**
     * @brief Apply data granularity
     * 
     * @param timestamp The timestamp
     * @param granularity The data granularity
     * @return std::chrono::system_clock::time_point The adjusted timestamp
     */
    std::chrono::system_clock::time_point applyGranularity(
        const std::chrono::system_clock::time_point& timestamp, 
        DataGranularity granularity) const;
    
    // Member variables
    DownloadManager* downloadManager_ = nullptr;
    std::map<std::string, DownloadStatistics> downloadStats_;
    SessionStatistics sessionStats_;
    std::string databaseFilePath_;
    std::chrono::system_clock::time_point sessionStartTime_;
    std::atomic<bool> enabled_;
    mutable std::mutex mutex_;
};

} // namespace core
} // namespace dm

#endif // STATISTICS_H