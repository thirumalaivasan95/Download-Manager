#ifndef BATCH_DOWNLOADER_H
#define BATCH_DOWNLOADER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <map>

namespace dm {
namespace core {

// Forward declarations
class DownloadManager;
class DownloadTask;

/**
 * @brief Batch URL source types
 */
enum class BatchUrlSourceType {
    TEXT_FILE,       // Plain text file with one URL per line
    HTML_FILE,       // HTML file with links to extract
    CSV_FILE,        // CSV file with URLs in a column
    SITEMAP_FILE,    // XML sitemap file
    URL_LIST,        // Direct list of URLs
    HTTP_SOURCE      // URL to fetch URLs from
};

/**
 * @brief URL filter function type
 */
using UrlFilterFunction = std::function<bool(const std::string& url)>;

/**
 * @brief Batch job progress callback function type
 */
using BatchProgressCallback = std::function<void(int processedCount, int totalCount, 
                                               double overallProgress, int successCount, int failureCount)>;

/**
 * @brief Batch job completion callback function type
 */
using BatchCompletionCallback = std::function<void(int successCount, int failureCount, 
                                                 const std::vector<std::string>& failedUrls)>;

/**
 * @brief Batch job error callback function type
 */
using BatchErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Batch download configuration structure
 */
struct BatchDownloadConfig {
    std::string sourceUrl;               // URL or file path to get URLs from
    BatchUrlSourceType sourceType;       // Type of URL source
    int maxConcurrentFiles;              // Maximum concurrent files to download
    std::string destinationDirectory;    // Directory to save files to
    bool createSubdirectories;           // Create subdirectories based on URL structure
    bool skipExistingFiles;              // Skip files that already exist
    bool startImmediately;               // Start downloads immediately
    int retryCount;                      // Number of retries for failed downloads
    UrlFilterFunction filterFunction;    // Function to filter URLs
    std::string urlColumnName;           // Column name for URLs in CSV files
    std::string fileNameColumnName;      // Column name for filenames in CSV files
    int urlColumnIndex;                  // Column index for URLs in CSV files (if no header)
};

/**
 * @brief Batch downloader class
 * 
 * Manages batch downloads from various sources
 */
class BatchDownloader {
public:
    /**
     * @brief Construct a new BatchDownloader
     * 
     * @param downloadManager Reference to the download manager
     */
    explicit BatchDownloader(DownloadManager& downloadManager);
    
    /**
     * @brief Destroy the BatchDownloader
     */
    ~BatchDownloader();
    
    /**
     * @brief Start a batch download job
     * 
     * @param config The batch download configuration
     * @param progressCallback Optional callback for progress updates
     * @param completionCallback Optional callback for completion
     * @param errorCallback Optional callback for errors
     * @return true if the job was started successfully, false otherwise
     */
    bool startBatchJob(const BatchDownloadConfig& config,
                      BatchProgressCallback progressCallback = nullptr,
                      BatchCompletionCallback completionCallback = nullptr,
                      BatchErrorCallback errorCallback = nullptr);
    
    /**
     * @brief Cancel the current batch job
     */
    void cancelBatchJob();
    
    /**
     * @brief Check if a batch job is running
     * 
     * @return true if a job is running, false otherwise
     */
    bool isJobRunning() const;
    
    /**
     * @brief Get the current batch job progress
     * 
     * @param processedCount Output parameter for processed count
     * @param totalCount Output parameter for total count
     * @param overallProgress Output parameter for overall progress
     * @param successCount Output parameter for success count
     * @param failureCount Output parameter for failure count
     */
    void getJobProgress(int& processedCount, int& totalCount, 
                       double& overallProgress, int& successCount, int& failureCount) const;
    
    /**
     * @brief Scan a source for URLs without downloading
     * 
     * @param config The batch download configuration
     * @return std::vector<std::string> The list of URLs found
     */
    std::vector<std::string> scanUrlSource(const BatchDownloadConfig& config);
    
    /**
     * @brief Extract URLs from a text file
     * 
     * @param filePath Path to the text file
     * @param filterFunction Optional function to filter URLs
     * @return std::vector<std::string> The list of URLs found
     */
    std::vector<std::string> extractUrlsFromTextFile(const std::string& filePath,
                                                   UrlFilterFunction filterFunction = nullptr);
    
    /**
     * @brief Extract URLs from an HTML file
     * 
     * @param filePath Path to the HTML file
     * @param filterFunction Optional function to filter URLs
     * @return std::vector<std::string> The list of URLs found
     */
    std::vector<std::string> extractUrlsFromHtmlFile(const std::string& filePath,
                                                   UrlFilterFunction filterFunction = nullptr);
    
    /**
     * @brief Extract URLs from a CSV file
     * 
     * @param filePath Path to the CSV file
     * @param columnName Name of the column containing URLs
     * @param columnIndex Index of the column containing URLs (if no header)
     * @param filterFunction Optional function to filter URLs
     * @return std::vector<std::string> The list of URLs found
     */
    std::vector<std::string> extractUrlsFromCsvFile(const std::string& filePath,
                                                  const std::string& columnName = "",
                                                  int columnIndex = 0,
                                                  UrlFilterFunction filterFunction = nullptr);
    
    /**
     * @brief Extract URLs from a sitemap file
     * 
     * @param filePath Path to the sitemap file
     * @param filterFunction Optional function to filter URLs
     * @return std::vector<std::string> The list of URLs found
     */
    std::vector<std::string> extractUrlsFromSitemapFile(const std::string& filePath,
                                                      UrlFilterFunction filterFunction = nullptr);
    
    /**
     * @brief Extract URLs from an HTTP source
     * 
     * @param url URL to fetch
     * @param filterFunction Optional function to filter URLs
     * @return std::vector<std::string> The list of URLs found
     */
    std::vector<std::string> extractUrlsFromHttpSource(const std::string& url,
                                                     UrlFilterFunction filterFunction = nullptr);
    
private:
    /**
     * @brief Process a batch job in a separate thread
     * 
     * @param config The batch download configuration
     */
    void processBatchJob(const BatchDownloadConfig& config);
    
    /**
     * @brief Download a single URL
     * 
     * @param url The URL to download
     * @param destinationPath The destination path
     * @param config The batch download configuration
     * @return true if successful, false otherwise
     */
    bool downloadUrl(const std::string& url, const std::string& destinationPath,
                    const BatchDownloadConfig& config);
    
    /**
     * @brief Extract filename from URL
     * 
     * @param url The URL
     * @return std::string The extracted filename
     */
    std::string extractFilenameFromUrl(const std::string& url);
    
    /**
     * @brief Determine destination path for a URL
     * 
     * @param url The URL
     * @param config The batch download configuration
     * @return std::string The destination path
     */
    std::string determineDestinationPath(const std::string& url,
                                        const BatchDownloadConfig& config);
    
    /**
     * @brief Update job progress
     * 
     * @param processedCount Number of processed URLs
     * @param totalCount Total number of URLs
     * @param successCount Number of successful downloads
     * @param failureCount Number of failed downloads
     */
    void updateJobProgress(int processedCount, int totalCount,
                          int successCount, int failureCount);
    
    // Member variables
    DownloadManager& downloadManager_;
    std::atomic<bool> jobRunning_;
    std::atomic<bool> jobCancelled_;
    
    std::mutex progressMutex_;
    int processedCount_;
    int totalCount_;
    double overallProgress_;
    int successCount_;
    int failureCount_;
    std::vector<std::string> failedUrls_;
    
    BatchProgressCallback progressCallback_;
    BatchCompletionCallback completionCallback_;
    BatchErrorCallback errorCallback_;
    
    std::mutex tasksMutex_;
    std::map<std::string, std::shared_ptr<DownloadTask>> activeTasks_;
};

} // namespace core
} // namespace dm

#endif // BATCH_DOWNLOADER_H