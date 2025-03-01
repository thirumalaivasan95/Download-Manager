#include "../../include/core/WebsiteCrawler.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/UrlParser.h"
#include "../../include/core/HttpClient.h"

#include <regex>
#include <queue>
#include <set>
#include <algorithm>
#include <thread>
#include <chrono>
#include <functional>

namespace DownloadManager {
namespace Core {

WebsiteCrawler::WebsiteCrawler() :
    m_isRunning(false),
    m_maxDepth(3),
    m_maxPages(100),
    m_delay(0),
    m_followExternalLinks(false),
    m_respectRobotsTxt(true),
    m_crawlerThread(nullptr)
{
    // Initialize default file extension patterns
    m_fileExtensions = {
        ".zip", ".rar", ".7z", ".tar", ".gz", ".bz2", ".xz",
        ".exe", ".msi", ".dmg", ".pkg", ".deb", ".rpm",
        ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
        ".mp3", ".mp4", ".avi", ".mkv", ".mov", ".flv", ".wmv",
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".svg", ".tif", ".tiff",
        ".iso", ".img"
    };
}

WebsiteCrawler::~WebsiteCrawler() {
    stop();
}

void WebsiteCrawler::start(const std::string& startUrl) {
    if (m_isRunning) {
        Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
            "Crawler already running");
        return;
    }
    
    // Validate starting URL
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(startUrl);
    
    if (!parsedUrl.valid) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Invalid starting URL: " + startUrl);
        return;
    }
    
    // Clear previous results
    {
        std::lock_guard<std::mutex> lock(m_crawlMutex);
        m_foundUrls.clear();
        m_visitedUrls.clear();
        m_downloadableFiles.clear();
        m_baseUrl = parsedUrl.scheme + "://" + parsedUrl.host;
        m_baseDomain = parsedUrl.host;
    }
    
    m_isRunning = true;
    m_crawlerThread = std::make_unique<std::thread>(&WebsiteCrawler::crawl, this, startUrl);
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Website crawler started with URL: " + startUrl);
}

void WebsiteCrawler::stop() {
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    if (m_crawlerThread && m_crawlerThread->joinable()) {
        m_crawlerThread->join();
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Website crawler stopped");
}

bool WebsiteCrawler::isRunning() const {
    return m_isRunning;
}

void WebsiteCrawler::setMaxDepth(int depth) {
    if (depth >= 0) {
        m_maxDepth = depth;
    }
}

int WebsiteCrawler::getMaxDepth() const {
    return m_maxDepth;
}

void WebsiteCrawler::setMaxPages(int pages) {
    if (pages >= 0) {
        m_maxPages = pages;
    }
}

int WebsiteCrawler::getMaxPages() const {
    return m_maxPages;
}

void WebsiteCrawler::setDelay(int milliseconds) {
    if (milliseconds >= 0) {
        m_delay = milliseconds;
    }
}

int WebsiteCrawler::getDelay() const {
    return m_delay;
}

void WebsiteCrawler::setFollowExternalLinks(bool follow) {
    m_followExternalLinks = follow;
}

bool WebsiteCrawler::getFollowExternalLinks() const {
    return m_followExternalLinks;
}

void WebsiteCrawler::setRespectRobotsTxt(bool respect) {
    m_respectRobotsTxt = respect;
}

bool WebsiteCrawler::getRespectRobotsTxt() const {
    return m_respectRobotsTxt;
}

void WebsiteCrawler::setFileExtensions(const std::set<std::string>& extensions) {
    m_fileExtensions = extensions;
}

std::set<std::string> WebsiteCrawler::getFileExtensions() const {
    return m_fileExtensions;
}

void WebsiteCrawler::addFileExtension(const std::string& extension) {
    // Ensure extension starts with a dot
    std::string ext = extension;
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    m_fileExtensions.insert(ext);
}

void WebsiteCrawler::removeFileExtension(const std::string& extension) {
    // Ensure extension starts with a dot
    std::string ext = extension;
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    m_fileExtensions.erase(ext);
}

void WebsiteCrawler::addUrlFilter(const std::string& pattern) {
    try {
        std::regex re(pattern);
        m_urlFilters.push_back(pattern);
    } 
    catch (const std::regex_error& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Invalid regex pattern: " + pattern + " (" + e.what() + ")");
    }
}

void WebsiteCrawler::clearUrlFilters() {
    m_urlFilters.clear();
}

std::vector<std::string> WebsiteCrawler::getUrlFilters() const {
    return m_urlFilters;
}

std::vector<std::string> WebsiteCrawler::getDownloadableFiles() const {
    std::lock_guard<std::mutex> lock(m_crawlMutex);
    return std::vector<std::string>(m_downloadableFiles.begin(), m_downloadableFiles.end());
}

std::vector<std::string> WebsiteCrawler::getVisitedUrls() const {
    std::lock_guard<std::mutex> lock(m_crawlMutex);
    return std::vector<std::string>(m_visitedUrls.begin(), m_visitedUrls.end());
}

void WebsiteCrawler::addCrawlProgressCallback(const CrawlProgressCallback& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_progressCallbacks.push_back(callback);
}

void WebsiteCrawler::addFileFoundCallback(const FileFoundCallback& callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_fileFoundCallbacks.push_back(callback);
}

void WebsiteCrawler::crawl(const std::string& startUrl) {
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Starting crawl from URL: " + startUrl);
        
        // Parse robots.txt if needed
        std::set<std::string> disallowedPaths;
        if (m_respectRobotsTxt) {
            parseRobotsTxt(m_baseUrl, disallowedPaths);
        }
        
        // Create a queue of URLs to visit with their depth
        std::queue<std::pair<std::string, int>> urlQueue;
        
        // Add the starting URL to the queue at depth 0
        urlQueue.push(std::make_pair(startUrl, 0));
        
        // Keep track of URLs we've already queued to avoid duplication
        std::set<std::string> queuedUrls;
        queuedUrls.insert(startUrl);
        
        // Initialize HTTP client
        HttpClient httpClient;
        
        int pagesVisited = 0;
        
        // Process the queue
        while (m_isRunning && !urlQueue.empty()) {
            // Check if we've reached the maximum number of pages
            if (m_maxPages > 0 && pagesVisited >= m_maxPages) {
                Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                    "Reached maximum pages limit: " + std::to_string(m_maxPages));
                break;
            }
            
            // Get the next URL and its depth from the queue
            auto current = urlQueue.front();
            std::string currentUrl = current.first;
            int currentDepth = current.second;
            urlQueue.pop();
            
            // Skip this URL if it's disallowed by robots.txt
            if (m_respectRobotsTxt && isPathDisallowed(currentUrl, disallowedPaths)) {
                Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                    "Skipping URL disallowed by robots.txt: " + currentUrl);
                continue;
            }
            
            // Add to visited list
            {
                std::lock_guard<std::mutex> lock(m_crawlMutex);
                m_visitedUrls.insert(currentUrl);
            }
            
            // Increment pages visited counter
            pagesVisited++;
            
            // Notify progress
            notifyCrawlProgress(pagesVisited, urlQueue.size(), currentUrl);
            
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "Crawling URL (" + std::to_string(pagesVisited) + "): " + currentUrl);
            
            // Check if this is a downloadable file
            if (isDownloadableFile(currentUrl)) {
                Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                    "Found downloadable file: " + currentUrl);
                
                // Add to downloadable files list
                {
                    std::lock_guard<std::mutex> lock(m_crawlMutex);
                    m_downloadableFiles.insert(currentUrl);
                }
                
                // Notify file found
                notifyFileFound(currentUrl);
                
                // Don't extract links from downloadable files
                continue;
            }
            
            // Don't crawl deeper than max depth
            if (currentDepth >= m_maxDepth) {
                continue;
            }
            
            // Add delay if specified
            if (m_delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(m_delay));
            }
            
            // Download the page
            try {
                HttpRequestOptions options;
                options.timeout = 30000; // 30 seconds timeout
                
                HttpResponse response = httpClient.sendGet(currentUrl, options);
                
                if (response.statusCode == 200) {
                    // Extract links from the page content
                    std::vector<std::string> links = extractLinks(response.body, currentUrl);
                    
                    // Process each link
                    for (const auto& link : links) {
                        // Normalize URL
                        std::string normalizedUrl = normalizeUrl(link, currentUrl);
                        
                        // Skip empty or invalid URLs
                        if (normalizedUrl.empty()) {
                            continue;
                        }
                        
                        // Check if URL matches any filters
                        if (!m_urlFilters.empty() && !matchesFilter(normalizedUrl)) {
                            continue;
                        }
                        
                        // Check if it's an external link
                        bool isExternal = isExternalLink(normalizedUrl);
                        
                        // Skip external links if we're not following them
                        if (isExternal && !m_followExternalLinks) {
                            continue;
                        }
                        
                        // Check if we've already queued this URL
                        if (queuedUrls.find(normalizedUrl) != queuedUrls.end()) {
                            continue;
                        }
                        
                        // Add URL to queue
                        urlQueue.push(std::make_pair(normalizedUrl, currentDepth + 1));
                        queuedUrls.insert(normalizedUrl);
                        
                        // Add to found URLs
                        {
                            std::lock_guard<std::mutex> lock(m_crawlMutex);
                            m_foundUrls.insert(normalizedUrl);
                        }
                    }
                } 
                else {
                    Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                        "Failed to download page: " + currentUrl + 
                        " (Status code: " + std::to_string(response.statusCode) + ")");
                }
            } 
            catch (const std::exception& e) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Exception downloading page: " + currentUrl + " (" + e.what() + ")");
            }
        }
        
        if (m_isRunning) {
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "Crawl completed. Visited " + std::to_string(pagesVisited) + " pages, " +
                "found " + std::to_string(m_downloadableFiles.size()) + " downloadable files.");
        } else {
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "Crawl stopped. Visited " + std::to_string(pagesVisited) + " pages, " +
                "found " + std::to_string(m_downloadableFiles.size()) + " downloadable files.");
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in crawl: " + std::string(e.what()));
    }
    
    m_isRunning = false;
}

bool WebsiteCrawler::isDownloadableFile(const std::string& url) {
    // Parse URL
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(url);
    
    if (!parsedUrl.valid) {
        return false;
    }
    
    // Extract file path
    std::string path = parsedUrl.path;
    
    // Check if the URL path ends with a known file extension
    for (const auto& ext : m_fileExtensions) {
        if (path.length() >= ext.length() && 
            std::equal(ext.rbegin(), ext.rend(), path.rbegin(), 
                     [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> WebsiteCrawler::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    
    try {
        // Regular expression to find links in HTML
        // Look for href attributes in anchor tags
        std::regex hrefRegex("<a[^>]*href=[\"']([^\"']+)[\"'][^>]*>");
        
        // Also search for image sources which might be downloadable
        std::regex imgRegex("<img[^>]*src=[\"']([^\"']+)[\"'][^>]*>");
        
        // Search for href links
        std::smatch hrefMatch;
        std::string::const_iterator searchStart(html.cbegin());
        
        while (std::regex_search(searchStart, html.cend(), hrefMatch, hrefRegex)) {
            std::string link = hrefMatch[1];
            links.push_back(link);
            searchStart = hrefMatch.suffix().first;
        }
        
        // Search for image sources
        std::smatch imgMatch;
        searchStart = html.cbegin();
        
        while (std::regex_search(searchStart, html.cend(), imgMatch, imgRegex)) {
            std::string link = imgMatch[1];
            links.push_back(link);
            searchStart = imgMatch.suffix().first;
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception extracting links: " + std::string(e.what()));
    }
    
    return links;
}

std::string WebsiteCrawler::normalizeUrl(const std::string& url, const std::string& baseUrl) {
    // Parse the base URL
    Utils::UrlParser urlParser;
    auto parsedBase = urlParser.parse(baseUrl);
    
    if (!parsedBase.valid) {
        return "";
    }
    
    // Handle different URL formats
    
    // If the URL is empty, return empty string
    if (url.empty()) {
        return "";
    }
    
    // If the URL is just a fragment, ignore it
    if (url[0] == '#') {
        return "";
    }
    
    // If the URL is a javascript: or mailto: link, ignore it
    if (url.substr(0, 11) == "javascript:" || url.substr(0, 7) == "mailto:") {
        return "";
    }
    
    // If the URL is already absolute (starts with http:// or https://), return it as is
    if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://") {
        return url;
    }
    
    // If the URL is protocol-relative (starts with //), add the scheme from the base URL
    if (url.substr(0, 2) == "//") {
        return parsedBase.scheme + ":" + url;
    }
    
    // At this point, the URL is relative
    
    // If the URL is root-relative (starts with /), append it to the base domain
    if (url[0] == '/') {
        return parsedBase.scheme + "://" + parsedBase.host + url;
    }
    
    // If the URL is relative to the current path, resolve it
    std::string basePath = parsedBase.path;
    
    // Remove filename component from base path
    size_t lastSlash = basePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        basePath = basePath.substr(0, lastSlash + 1);
    } else {
        basePath = "/";
    }
    
    return parsedBase.scheme + "://" + parsedBase.host + basePath + url;
}

bool WebsiteCrawler::isExternalLink(const std::string& url) {
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(url);
    
    if (!parsedUrl.valid) {
        return false;
    }
    
    // Check if the hostname matches the base domain
    return parsedUrl.host != m_baseDomain;
}

bool WebsiteCrawler::matchesFilter(const std::string& url) {
    if (m_urlFilters.empty()) {
        return true;
    }
    
    for (const auto& pattern : m_urlFilters) {
        try {
            std::regex re(pattern);
            if (std::regex_search(url, re)) {
                return true;
            }
        } 
        catch (const std::regex_error& e) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Error in regex pattern " + pattern + ": " + e.what());
        }
    }
    
    return false;
}

void WebsiteCrawler::parseRobotsTxt(const std::string& baseUrl, std::set<std::string>& disallowedPaths) {
    try {
        std::string robotsUrl = baseUrl + "/robots.txt";
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Downloading robots.txt: " + robotsUrl);
        
        // Download robots.txt
        HttpClient httpClient;
        HttpRequestOptions options;
        options.timeout = 10000; // 10 seconds timeout
        
        HttpResponse response = httpClient.sendGet(robotsUrl, options);
        
        if (response.statusCode == 200) {
            // Parse the robots.txt content
            std::istringstream stream(response.body);
            std::string line;
            
            bool relevantUserAgent = false;
            
            while (std::getline(stream, line)) {
                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                
                // Skip empty lines and comments
                if (line.empty() || line[0] == '#') {
                    continue;
                }
                
                // Look for user-agent lines
                if (line.substr(0, 11) == "User-agent:") {
                    std::string agent = line.substr(11);
                    agent.erase(0, agent.find_first_not_of(" \t"));
                    
                    // Check if this section applies to our crawler or all crawlers (*)
                    relevantUserAgent = (agent == "*" || agent == "DownloadManager");
                } 
                // Look for disallow lines if we're in a relevant section
                else if (relevantUserAgent && line.substr(0, 9) == "Disallow:") {
                    std::string path = line.substr(9);
                    path.erase(0, path.find_first_not_of(" \t"));
                    
                    if (!path.empty()) {
                        disallowedPaths.insert(path);
                    }
                }
            }
            
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "Parsed robots.txt, found " + std::to_string(disallowedPaths.size()) + " disallowed paths");
        } 
        else if (response.statusCode == 404) {
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "No robots.txt found at: " + robotsUrl);
        } 
        else {
            Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                "Failed to download robots.txt: " + robotsUrl + 
                " (Status code: " + std::to_string(response.statusCode) + ")");
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception downloading robots.txt: " + std::string(e.what()));
    }
}

bool WebsiteCrawler::isPathDisallowed(const std::string& url, const std::set<std::string>& disallowedPaths) {
    // Parse URL to get the path
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(url);
    
    if (!parsedUrl.valid) {
        return false;
    }
    
    std::string path = parsedUrl.path;
    
    // Check against disallowed paths
    for (const auto& disallowedPath : disallowedPaths) {
        // If the disallowed path ends with *, it's a prefix match
        if (disallowedPath.back() == '*') {
            std::string prefix = disallowedPath.substr(0, disallowedPath.length() - 1);
            if (path.substr(0, prefix.length()) == prefix) {
                return true;
            }
        } 
        // Otherwise, it's an exact match or path prefix
        else {
            if (path == disallowedPath || 
                (disallowedPath.back() == '/' && path.substr(0, disallowedPath.length()) == disallowedPath)) {
                return true;
            }
        }
    }
    
    return false;
}

void WebsiteCrawler::notifyCrawlProgress(int pagesVisited, int queueSize, const std::string& currentUrl) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_progressCallbacks) {
        callback(pagesVisited, queueSize, currentUrl, m_downloadableFiles.size());
    }
}

void WebsiteCrawler::notifyFileFound(const std::string& fileUrl) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_fileFoundCallbacks) {
        callback(fileUrl);
    }
}

} // namespace Core
} // namespace DownloadManager