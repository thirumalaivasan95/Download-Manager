#ifndef WEBSITE_CRAWLER_H
#define WEBSITE_CRAWLER_H

#include <string>
#include <vector>
#include <set>
#include <queue>
#include <map>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

namespace dm {
namespace core {

// Forward declarations
class DownloadManager;
class BatchDownloader;

/**
 * @brief Crawl mode enumeration
 */
enum class CrawlMode {
    SAME_DOMAIN,          // Crawl only URLs in the same domain
    SAME_HOST,            // Crawl only URLs on the same host
    SPECIFIED_DOMAINS,    // Crawl only URLs in specified domains
    FOLLOW_ALL            // Crawl all linked URLs
};

/**
 * @brief Resource type enumeration
 */
enum class ResourceType {
    HTML_PAGE,            // HTML page
    IMAGE,                // Image file
    VIDEO,                // Video file
    AUDIO,                // Audio file
    DOCUMENT,             // Document file (PDF, DOC, etc.)
    ARCHIVE,              // Archive file (ZIP, RAR, etc.)
    EXECUTABLE,           // Executable file
    OTHER                 // Other file type
};

/**
 * @brief URL filter type
 */
using UrlFilter = std::function<bool(const std::string& url)>;

/**
 * @brief Content filter type
 */
using ContentFilter = std::function<bool(const std::string& url, const std::string& contentType, int64_t contentLength)>;

/**
 * @brief Resource handler type
 */
using ResourceHandler = std::function<void(const std::string& url, ResourceType type)>;

/**
 * @brief Progress callback type
 */
using CrawlProgressCallback = std::function<void(int pagesVisited, int totalUrls, 
                                               int resourcesFound, int downloadQueued)>;

/**
 * @brief Crawl options structure
 */
struct CrawlOptions {
    CrawlMode mode = CrawlMode::SAME_DOMAIN;        // Crawling mode
    int maxDepth = 3;                               // Maximum crawl depth
    int maxPages = 100;                             // Maximum pages to visit
    int maxConcurrent = 5;                          // Maximum concurrent connections
    bool respectRobotsTxt = true;                   // Respect robots.txt rules
    bool downloadResources = true;                  // Download resources (images, etc.)
    std::vector<std::string> allowedDomains;        // Domains to crawl (for SPECIFIED_DOMAINS mode)
    std::vector<std::string> fileTypesToDownload;   // File types to download
    std::string userAgent = "DownloadManager/1.0";  // User agent string
    std::chrono::milliseconds requestDelay{500};    // Delay between requests
    std::string downloadDirectory;                  // Directory to save downloads
    UrlFilter urlFilter = nullptr;                  // Custom URL filter
    ContentFilter contentFilter = nullptr;          // Custom content filter
    ResourceHandler resourceHandler = nullptr;      // Custom resource handler
};

/**
 * @brief Website crawler class
 * 
 * Crawls websites and downloads resources
 */
class WebsiteCrawler {
public:
    /**
     * @brief Construct a new WebsiteCrawler
     * 
     * @param downloadManager Reference to download manager
     */
    explicit WebsiteCrawler(DownloadManager& downloadManager);
    
    /**
     * @brief Destroy the WebsiteCrawler
     */
    ~WebsiteCrawler();
    
    /**
     * @brief Start crawling a website
     * 
     * @param startUrl The URL to start crawling from
     * @param options Crawl options
     * @param progressCallback Optional progress callback
     * @return true if crawling started successfully, false otherwise
     */
    bool startCrawling(const std::string& startUrl, 
                      const CrawlOptions& options,
                      CrawlProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Pause crawling
     * 
     * @return true if paused successfully, false otherwise
     */
    bool pauseCrawling();
    
    /**
     * @brief Resume crawling
     * 
     * @return true if resumed successfully, false otherwise
     */
    bool resumeCrawling();
    
    /**
     * @brief Stop crawling
     * 
     * @return true if stopped successfully, false otherwise
     */
    bool stopCrawling();
    
    /**
     * @brief Check if crawler is running
     * 
     * @return true if running, false otherwise
     */
    bool isRunning() const;
    
    /**
     * @brief Check if crawler is paused
     * 
     * @return true if paused, false otherwise
     */
    bool isPaused() const;
    
    /**
     * @brief Get crawl statistics
     * 
     * @param pagesVisited Output parameter for pages visited
     * @param totalUrls Output parameter for total URLs found
     * @param resourcesFound Output parameter for resources found
     * @param downloadQueued Output parameter for downloads queued
     */
    void getStatistics(int& pagesVisited, int& totalUrls, 
                      int& resourcesFound, int& downloadQueued) const;
    
    /**
     * @brief Get all found URLs
     * 
     * @return std::vector<std::string> All URLs found during crawling
     */
    std::vector<std::string> getFoundUrls() const;
    
    /**
     * @brief Get all downloaded resources
     * 
     * @return std::vector<std::string> All downloaded resource URLs
     */
    std::vector<std::string> getDownloadedResources() const;
    
    /**
     * @brief Get errors that occurred during crawling
     * 
     * @return std::map<std::string, std::string> Map of URL to error message
     */
    std::map<std::string, std::string> getErrors() const;
    
    /**
     * @brief Check if a URL matches robots.txt rules
     * 
     * @param url The URL to check
     * @param userAgent The user agent to check against
     * @return true if allowed by robots.txt, false otherwise
     */
    static bool isAllowedByRobotsTxt(const std::string& url, const std::string& userAgent);
    
    /**
     * @brief Detect resource type from URL and content type
     * 
     * @param url The URL
     * @param contentType The content type (MIME type)
     * @return ResourceType The detected resource type
     */
    static ResourceType detectResourceType(const std::string& url, const std::string& contentType);
    
    /**
     * @brief Check if a URL has a specific file extension
     * 
     * @param url The URL to check
     * @param extension The extension to check for
     * @return true if the URL has the extension, false otherwise
     */
    static bool hasFileExtension(const std::string& url, const std::string& extension);
    
private:
    /**
     * @brief Crawl worker thread function
     */
    void crawlWorker();
    
    /**
     * @brief Process a single URL
     * 
     * @param url The URL to process
     * @param depth The current depth
     * @return true if processed successfully, false otherwise
     */
    bool processUrl(const std::string& url, int depth);
    
    /**
     * @brief Extract links from HTML content
     * 
     * @param baseUrl The base URL for resolving relative links
     * @param htmlContent The HTML content
     * @return std::vector<std::string> The extracted links
     */
    std::vector<std::string> extractLinks(const std::string& baseUrl, const std::string& htmlContent);
    
    /**
     * @brief Extract resources from HTML content
     * 
     * @param baseUrl The base URL for resolving relative links
     * @param htmlContent The HTML content
     * @return std::vector<std::string> The extracted resource URLs
     */
    std::vector<std::string> extractResources(const std::string& baseUrl, const std::string& htmlContent);
    
    /**
     * @brief Normalize a URL
     * 
     * @param url The URL to normalize
     * @param baseUrl The base URL for resolving relative links
     * @return std::string The normalized URL
     */
    std::string normalizeUrl(const std::string& url, const std::string& baseUrl);
    
    /**
     * @brief Check if a URL should be crawled
     * 
     * @param url The URL to check
     * @return true if the URL should be crawled, false otherwise
     */
    bool shouldCrawl(const std::string& url);
    
    /**
     * @brief Check if a resource should be downloaded
     * 
     * @param url The URL to check
     * @param contentType The content type
     * @param contentLength The content length
     * @return true if the resource should be downloaded, false otherwise
     */
    bool shouldDownloadResource(const std::string& url, const std::string& contentType, int64_t contentLength);
    
    /**
     * @brief Handle a discovered resource
     * 
     * @param url The resource URL
     * @param contentType The content type
     * @param contentLength The content length
     */
    void handleResource(const std::string& url, const std::string& contentType, int64_t contentLength);
    
    /**
     * @brief Get robots.txt rules for a domain
     * 
     * @param domain The domain
     * @return std::string The robots.txt content
     */
    std::string getRobotsTxt(const std::string& domain);
    
    /**
     * @brief Update crawl statistics
     */
    void updateStatistics();
    
    // Member variables
    DownloadManager& downloadManager_;
    std::shared_ptr<BatchDownloader> batchDownloader_;
    
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<bool> stopRequested_;
    
    CrawlOptions options_;
    std::string startUrl_;
    std::string baseDomain_;
    
    std::mutex statsMutex_;
    int pagesVisited_;
    int totalUrls_;
    int resourcesFound_;
    int downloadQueued_;
    
    std::mutex urlsMutex_;
    std::set<std::string> visitedUrls_;
    std::set<std::string> downloadedResources_;
    std::queue<std::pair<std::string, int>> urlQueue_; // URL and depth
    std::map<std::string, std::string> errors_;
    
    std::mutex robotsMutex_;
    std::map<std::string, std::string> robotsTxtCache_;
    
    std::vector<std::thread> workerThreads_;
    CrawlProgressCallback progressCallback_;
};

} // namespace core
} // namespace dm

#endif // WEBSITE_CRAWLER_H