#include "../../include/core/Statistics.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/DatabaseManager.h"
#include "../../include/utils/TimeUtils.h"
#include "../../include/utils/StringUtils.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <ctime>
#include <chrono>

namespace DownloadManager {
namespace Core {

// Initialize static members
std::shared_ptr<Statistics> Statistics::s_instance = nullptr;
std::mutex Statistics::s_instanceMutex;

Statistics::Statistics() 
    : m_completedDownloads(0),
      m_failedDownloads(0),
      m_canceledDownloads(0),
      m_totalBytesDownloaded(0),
      m_totalDownloadTime(0),
      m_maxSpeed(0),
      m_minSpeed(std::numeric_limits<int64_t>::max()),
      m_lastStatsUpdateTime(0),
      m_startTime(Utils::TimeUtils::getCurrentTimeSeconds()),
      m_isInitialized(false)
{
    initialize();
}

Statistics::~Statistics() {
    saveStatistics();
}

std::shared_ptr<Statistics> Statistics::instance() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        s_instance = std::shared_ptr<Statistics>(new Statistics());
    }
    return s_instance;
}

void Statistics::initialize() {
    if (m_isInitialized) {
        return;
    }
    
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, "Initializing statistics");
        
        // Initialize data
        resetDailyStatistics();
        
        // Load statistics from database
        loadStatistics();
        
        m_isInitialized = true;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Failed to initialize statistics: " + std::string(e.what()));
    }
}

void Statistics::loadStatistics() {
    try {
        auto& db = Utils::DatabaseManager::instance();
        
        // Query for all-time statistics
        std::string sql = 
            "SELECT SUM(total_downloads) AS total_downloads, "
            "SUM(successful_downloads) AS successful_downloads, "
            "SUM(failed_downloads) AS failed_downloads, "
            "SUM(total_bytes_downloaded) AS total_bytes_downloaded "
            "FROM statistics";
        
        struct AllTimeStatsResult {
            int64_t totalDownloads = 0;
            int64_t successfulDownloads = 0;
            int64_t failedDownloads = 0;
            int64_t totalBytesDownloaded = 0;
        };
        
        AllTimeStatsResult allTimeStats;
        
        auto allTimeCallback = [](void* data, int argc, char** argv, char** colNames) -> int {
            AllTimeStatsResult* result = static_cast<AllTimeStatsResult*>(data);
            
            for (int i = 0; i < argc; i++) {
                if (std::string(colNames[i]) == "total_downloads" && argv[i]) {
                    result->totalDownloads = std::stoll(argv[i]);
                } 
                else if (std::string(colNames[i]) == "successful_downloads" && argv[i]) {
                    result->successfulDownloads = std::stoll(argv[i]);
                } 
                else if (std::string(colNames[i]) == "failed_downloads" && argv[i]) {
                    result->failedDownloads = std::stoll(argv[i]);
                } 
                else if (std::string(colNames[i]) == "total_bytes_downloaded" && argv[i]) {
                    result->totalBytesDownloaded = std::stoll(argv[i]);
                }
            }
            
            return 0;
        };
        
        db.executeWithCallback(sql, allTimeCallback, &allTimeStats);
        
        // Update statistics
        m_completedDownloads = allTimeStats.successfulDownloads;
        m_failedDownloads = allTimeStats.failedDownloads;
        m_totalBytesDownloaded = allTimeStats.totalBytesDownloaded;
        
        // Load today's statistics
        time_t today = Utils::TimeUtils::getCurrentTimeSeconds();
        struct tm* tm = std::localtime(&today);
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        time_t todayStart = std::mktime(tm);
        
        std::string todaySql = 
            "SELECT * FROM statistics WHERE date = ?";
        
        auto stmt = db.prepare(todaySql);
        stmt->bindInt64(1, todayStart);
        
        if (stmt->step() == SQLITE_ROW) {
            m_dailyStats.totalDownloads = stmt->getColumnInt(2);
            m_dailyStats.successfulDownloads = stmt->getColumnInt(3);
            m_dailyStats.failedDownloads = stmt->getColumnInt(4);
            m_dailyStats.totalBytesDownloaded = stmt->getColumnInt64(5);
            m_dailyStats.averageSpeed = stmt->getColumnInt(6);
        } else {
            // No data for today, initialize a new entry
            std::string insertSql = 
                "INSERT INTO statistics (date, total_downloads, successful_downloads, "
                "failed_downloads, total_bytes_downloaded, average_speed) "
                "VALUES (?, 0, 0, 0, 0, 0)";
            
            auto insertStmt = db.prepare(insertSql);
            insertStmt->bindInt64(1, todayStart);
            insertStmt->step();
        }
        
        // Load speed statistics
        std::string speedSql = 
            "SELECT MAX(average_speed) AS max_speed FROM statistics";
        
        int maxSpeed = 0;
        
        auto speedCallback = [](void* data, int argc, char** argv, char** colNames) -> int {
            int* maxSpeed = static_cast<int*>(data);
            
            for (int i = 0; i < argc; i++) {
                if (std::string(colNames[i]) == "max_speed" && argv[i]) {
                    *maxSpeed = std::stoi(argv[i]);
                }
            }
            
            return 0;
        };
        
        db.executeWithCallback(speedSql, speedCallback, &maxSpeed);
        
        if (maxSpeed > m_maxSpeed) {
            m_maxSpeed = maxSpeed;
        }
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, "Statistics loaded from database");
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Failed to load statistics: " + std::string(e.what()));
    }
}

void Statistics::saveStatistics() {
    try {
        auto& db = Utils::DatabaseManager::instance();
        
        // Update today's statistics
        time_t today = Utils::TimeUtils::getCurrentTimeSeconds();
        struct tm* tm = std::localtime(&today);
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        time_t todayStart = std::mktime(tm);
        
        // Calculate average speed if we have data
        int avgSpeed = 0;
        if (m_dailyStats.totalDownloadTime > 0) {
            avgSpeed = static_cast<int>(m_dailyStats.totalBytesDownloaded / m_dailyStats.totalDownloadTime);
        }
        
        std::string updateSql = 
            "UPDATE statistics SET "
            "total_downloads = ?, "
            "successful_downloads = ?, "
            "failed_downloads = ?, "
            "total_bytes_downloaded = ?, "
            "average_speed = ? "
            "WHERE date = ?";
        
        auto stmt = db.prepare(updateSql);
        stmt->bindInt(1, m_dailyStats.totalDownloads);
        stmt->bindInt(2, m_dailyStats.successfulDownloads);
        stmt->bindInt(3, m_dailyStats.failedDownloads);
        stmt->bindInt64(4, m_dailyStats.totalBytesDownloaded);
        stmt->bindInt(5, avgSpeed);
        stmt->bindInt64(6, todayStart);
        
        stmt->step();
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, "Statistics saved to database");
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Failed to save statistics: " + std::string(e.what()));
    }
}

void Statistics::resetDailyStatistics() {
    m_dailyStats.totalDownloads = 0;
    m_dailyStats.successfulDownloads = 0;
    m_dailyStats.failedDownloads = 0;
    m_dailyStats.totalBytesDownloaded = 0;
    m_dailyStats.totalDownloadTime = 0;
    m_dailyStats.averageSpeed = 0;
    m_dailyStats.peakSpeed = 0;
}

void Statistics::resetAllStatistics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Reset daily statistics
    resetDailyStatistics();
    
    // Reset all-time statistics
    m_completedDownloads = 0;
    m_failedDownloads = 0;
    m_canceledDownloads = 0;
    m_totalBytesDownloaded = 0;
    m_totalDownloadTime = 0;
    m_maxSpeed = 0;
    m_minSpeed = std::numeric_limits<int64_t>::max();
    m_startTime = Utils::TimeUtils::getCurrentTimeSeconds();
    m_lastStatsUpdateTime = 0;
    
    // Clear history
    m_downloadHistory.clear();
    m_speedHistory.clear();
    m_dailyDownloadHistory.clear();
    
    try {
        // Clear database statistics
        auto& db = Utils::DatabaseManager::instance();
        db.execute("DELETE FROM statistics");
        
        // Create new entry for today
        time_t today = Utils::TimeUtils::getCurrentTimeSeconds();
        struct tm* tm = std::localtime(&today);
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        time_t todayStart = std::mktime(tm);
        
        std::string insertSql = 
            "INSERT INTO statistics (date, total_downloads, successful_downloads, "
            "failed_downloads, total_bytes_downloaded, average_speed) "
            "VALUES (?, 0, 0, 0, 0, 0)";
        
        auto stmt = db.prepare(insertSql);
        stmt->bindInt64(1, todayStart);
        stmt->step();
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Failed to reset statistics database: " + std::string(e.what()));
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "All statistics reset");
}

void Statistics::addDownloadComplete(int64_t bytesDownloaded, int64_t downloadTime, int averageSpeed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update all-time statistics
    m_completedDownloads++;
    m_totalBytesDownloaded += bytesDownloaded;
    m_totalDownloadTime += downloadTime;
    
    // Update daily statistics
    m_dailyStats.totalDownloads++;
    m_dailyStats.successfulDownloads++;
    m_dailyStats.totalBytesDownloaded += bytesDownloaded;
    m_dailyStats.totalDownloadTime += downloadTime;
    
    // Update speed statistics
    if (averageSpeed > m_maxSpeed) {
        m_maxSpeed = averageSpeed;
    }
    if (averageSpeed > 0 && averageSpeed < m_minSpeed) {
        m_minSpeed = averageSpeed;
    }
    if (averageSpeed > m_dailyStats.peakSpeed) {
        m_dailyStats.peakSpeed = averageSpeed;
    }
    
    // Add to download history
    time_t now = Utils::TimeUtils::getCurrentTimeSeconds();
    
    DownloadHistoryItem historyItem;
    historyItem.timestamp = now;
    historyItem.bytesDownloaded = bytesDownloaded;
    historyItem.downloadTime = downloadTime;
    historyItem.averageSpeed = averageSpeed;
    historyItem.status = DownloadStatus::COMPLETED;
    
    m_downloadHistory.push_back(historyItem);
    
    // Maintain history size limit (keep last 1000 entries)
    if (m_downloadHistory.size() > 1000) {
        m_downloadHistory.erase(m_downloadHistory.begin());
    }
    
    // Update daily download history
    updateDailyDownloadHistory(bytesDownloaded, now);
    
    // Save statistics to database
    saveStatistics();
    
    Utils::Logger::instance().log(Utils::LogLevel::DEBUG, 
        "Added completed download to statistics: " + Utils::StringUtils::formatFileSize(bytesDownloaded) + 
        " in " + Utils::StringUtils::formatTime(downloadTime) + 
        " at " + Utils::StringUtils::formatBitrate(averageSpeed));
}

void Statistics::addDownloadFailed(int64_t bytesDownloaded, int64_t downloadTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update all-time statistics
    m_failedDownloads++;
    m_totalBytesDownloaded += bytesDownloaded;
    m_totalDownloadTime += downloadTime;
    
    // Update daily statistics
    m_dailyStats.totalDownloads++;
    m_dailyStats.failedDownloads++;
    m_dailyStats.totalBytesDownloaded += bytesDownloaded;
    m_dailyStats.totalDownloadTime += downloadTime;
    
    // Add to download history
    time_t now = Utils::TimeUtils::getCurrentTimeSeconds();
    
    DownloadHistoryItem historyItem;
    historyItem.timestamp = now;
    historyItem.bytesDownloaded = bytesDownloaded;
    historyItem.downloadTime = downloadTime;
    historyItem.averageSpeed = (downloadTime > 0) ? static_cast<int>(bytesDownloaded / downloadTime) : 0;
    historyItem.status = DownloadStatus::FAILED;
    
    m_downloadHistory.push_back(historyItem);
    
    // Maintain history size limit
    if (m_downloadHistory.size() > 1000) {
        m_downloadHistory.erase(m_downloadHistory.begin());
    }
    
    // Update daily download history
    updateDailyDownloadHistory(bytesDownloaded, now);
    
    // Save statistics to database
    saveStatistics();
    
    Utils::Logger::instance().log(Utils::LogLevel::DEBUG, 
        "Added failed download to statistics: " + Utils::StringUtils::formatFileSize(bytesDownloaded) + 
        " in " + Utils::StringUtils::formatTime(downloadTime));
}

void Statistics::addDownloadCanceled(int64_t bytesDownloaded, int64_t downloadTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update all-time statistics
    m_canceledDownloads++;
    m_totalBytesDownloaded += bytesDownloaded;
    m_totalDownloadTime += downloadTime;
    
    // Update daily statistics
    m_dailyStats.totalDownloads++;
    m_dailyStats.totalBytesDownloaded += bytesDownloaded;
    m_dailyStats.totalDownloadTime += downloadTime;
    
    // Add to download history
    time_t now = Utils::TimeUtils::getCurrentTimeSeconds();
    
    DownloadHistoryItem historyItem;
    historyItem.timestamp = now;
    historyItem.bytesDownloaded = bytesDownloaded;
    historyItem.downloadTime = downloadTime;
    historyItem.averageSpeed = (downloadTime > 0) ? static_cast<int>(bytesDownloaded / downloadTime) : 0;
    historyItem.status = DownloadStatus::CANCELED;
    
    m_downloadHistory.push_back(historyItem);
    
    // Maintain history size limit
    if (m_downloadHistory.size() > 1000) {
        m_downloadHistory.erase(m_downloadHistory.begin());
    }
    
    // Update daily download history
    updateDailyDownloadHistory(bytesDownloaded, now);
    
    // Save statistics to database
    saveStatistics();
    
    Utils::Logger::instance().log(Utils::LogLevel::DEBUG, 
        "Added canceled download to statistics: " + Utils::StringUtils::formatFileSize(bytesDownloaded) + 
        " in " + Utils::StringUtils::formatTime(downloadTime));
}

void Statistics::updateDownloadProgress(int64_t bytesDownloaded, int currentSpeed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update speed history
    time_t now = Utils::TimeUtils::getCurrentTimeSeconds();
    
    // Only update every second
    if (now - m_lastStatsUpdateTime >= 1) {
        SpeedHistoryItem speedItem;
        speedItem.timestamp = now;
        speedItem.speed = currentSpeed;
        
        m_speedHistory.push_back(speedItem);
        
        // Maintain history size limit (keep last hour)
        while (m_speedHistory.size() > 3600) {
            m_speedHistory.erase(m_speedHistory.begin());
        }
        
        // Update peak speeds
        if (currentSpeed > m_maxSpeed) {
            m_maxSpeed = currentSpeed;
        }
        if (currentSpeed > m_dailyStats.peakSpeed) {
            m_dailyStats.peakSpeed = currentSpeed;
        }
        
        m_lastStatsUpdateTime = now;
    }
}

int Statistics::getCompletedDownloads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_completedDownloads;
}

int Statistics::getFailedDownloads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_failedDownloads;
}

int Statistics::getCanceledDownloads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_canceledDownloads;
}

int Statistics::getTotalDownloads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_completedDownloads + m_failedDownloads + m_canceledDownloads;
}

int64_t Statistics::getTotalBytesDownloaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalBytesDownloaded;
}

int64_t Statistics::getTotalDownloadTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalDownloadTime;
}

int Statistics::getAverageSpeed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_totalDownloadTime > 0) {
        return static_cast<int>(m_totalBytesDownloaded / m_totalDownloadTime);
    }
    
    return 0;
}

int Statistics::getMaxSpeed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxSpeed;
}

int Statistics::getMinSpeed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_minSpeed == std::numeric_limits<int64_t>::max()) {
        return 0;
    }
    
    return m_minSpeed;
}

time_t Statistics::getUptime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    time_t now = Utils::TimeUtils::getCurrentTimeSeconds();
    return now - m_startTime;
}

std::vector<DownloadHistoryItem> Statistics::getDownloadHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_downloadHistory;
}

std::vector<DownloadHistoryItem> Statistics::getDownloadHistory(time_t startTime, time_t endTime) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<DownloadHistoryItem> result;
    
    for (const auto& item : m_downloadHistory) {
        if (item.timestamp >= startTime && item.timestamp <= endTime) {
            result.push_back(item);
        }
    }
    
    return result;
}

std::vector<SpeedHistoryItem> Statistics::getSpeedHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_speedHistory;
}

std::vector<SpeedHistoryItem> Statistics::getSpeedHistory(time_t startTime, time_t endTime) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<SpeedHistoryItem> result;
    
    for (const auto& item : m_speedHistory) {
        if (item.timestamp >= startTime && item.timestamp <= endTime) {
            result.push_back(item);
        }
    }
    
    return result;
}

std::vector<DailyStatsItem> Statistics::getDailyDownloadStats(int days) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<DailyStatsItem> result;
    
    try {
        auto& db = Utils::DatabaseManager::instance();
        
        // Calculate start date
        time_t now = Utils::TimeUtils::getCurrentTimeSeconds();
        struct tm* tm = std::localtime(&now);
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        time_t today = std::mktime(tm);
        
        time_t startDate = today - (days - 1) * 86400; // days * seconds in a day
        
        std::string sql = 
            "SELECT date, total_downloads, successful_downloads, "
            "failed_downloads, total_bytes_downloaded, average_speed "
            "FROM statistics "
            "WHERE date >= ? "
            "ORDER BY date ASC";
        
        auto stmt = db.prepare(sql);
        stmt->bindInt64(1, startDate);
        
        while (stmt->step() == SQLITE_ROW) {
            DailyStatsItem item;
            item.date = stmt->getColumnInt64(0);
            item.totalDownloads = stmt->getColumnInt(1);
            item.successfulDownloads = stmt->getColumnInt(2);
            item.failedDownloads = stmt->getColumnInt(3);
            item.totalBytesDownloaded = stmt->getColumnInt64(4);
            item.averageSpeed = stmt->getColumnInt(5);
            
            result.push_back(item);
        }
        
        // Fill in missing days with zero values
        std::vector<DailyStatsItem> completeResult;
        
        for (int i = 0; i < days; i++) {
            time_t date = startDate + i * 86400;
            
            // Find if we have data for this date
            auto it = std::find_if(result.begin(), result.end(), 
                               [date](const DailyStatsItem& item) {
                                   return item.date == date;
                               });
            
            if (it != result.end()) {
                completeResult.push_back(*it);
            } else {
                DailyStatsItem emptyItem;
                emptyItem.date = date;
                emptyItem.totalDownloads = 0;
                emptyItem.successfulDownloads = 0;
                emptyItem.failedDownloads = 0;
                emptyItem.totalBytesDownloaded = 0;
                emptyItem.averageSpeed = 0;
                
                completeResult.push_back(emptyItem);
            }
        }
        
        return completeResult;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Failed to get daily stats: " + std::string(e.what()));
        return result;
    }
}

DailyStats Statistics::getDailyStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dailyStats;
}

std::map<std::string, int64_t> Statistics::getDownloadsByFileType() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_fileTypeStats;
}

void Statistics::updateFileTypeStats(const std::string& fileType, int64_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string type = fileType;
    if (type.empty()) {
        type = "unknown";
    }
    
    // Convert to lowercase
    std::transform(type.begin(), type.end(), type.begin(), 
                 [](unsigned char c) { return std::tolower(c); });
    
    m_fileTypeStats[type] += bytes;
}

void Statistics::updateDailyDownloadHistory(int64_t bytes, time_t timestamp) {
    // Get the date part only
    struct tm* tm = std::localtime(&timestamp);
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    time_t date = std::mktime(tm);
    
    // Add to daily history
    m_dailyDownloadHistory[date] += bytes;
}

std::string Statistics::getStatsReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::stringstream ss;
    
    ss << "Download Manager Statistics Report" << std::endl;
    ss << "=================================" << std::endl;
    ss << std::endl;
    
    ss << "All-Time Statistics:" << std::endl;
    ss << "-----------------" << std::endl;
    ss << "Total Downloads: " << getTotalDownloads() << std::endl;
    ss << "Completed Downloads: " << m_completedDownloads << std::endl;
    ss << "Failed Downloads: " << m_failedDownloads << std::endl;
    ss << "Canceled Downloads: " << m_canceledDownloads << std::endl;
    ss << "Total Bytes Downloaded: " << Utils::StringUtils::formatFileSize(m_totalBytesDownloaded) << std::endl;
    ss << "Total Download Time: " << Utils::StringUtils::formatTime(m_totalDownloadTime) << std::endl;
    
    int avgSpeed = getAverageSpeed();
    ss << "Average Speed: " << Utils::StringUtils::formatBitrate(avgSpeed) << std::endl;
    ss << "Maximum Speed: " << Utils::StringUtils::formatBitrate(m_maxSpeed) << std::endl;
    
    if (m_minSpeed != std::numeric_limits<int64_t>::max()) {
        ss << "Minimum Speed: " << Utils::StringUtils::formatBitrate(m_minSpeed) << std::endl;
    }
    
    ss << "Uptime: " << Utils::StringUtils::formatTime(getUptime()) << std::endl;
    ss << std::endl;
    
    ss << "Today's Statistics:" << std::endl;
    ss << "------------------" << std::endl;
    ss << "Total Downloads: " << m_dailyStats.totalDownloads << std::endl;
    ss << "Completed Downloads: " << m_dailyStats.successfulDownloads << std::endl;
    ss << "Failed Downloads: " << m_dailyStats.failedDownloads << std::endl;
    ss << "Total Bytes Downloaded: " << Utils::StringUtils::formatFileSize(m_dailyStats.totalBytesDownloaded) << std::endl;
    
    int dailyAvgSpeed = 0;
    if (m_dailyStats.totalDownloadTime > 0) {
        dailyAvgSpeed = static_cast<int>(m_dailyStats.totalBytesDownloaded / m_dailyStats.totalDownloadTime);
    }
    
    ss << "Average Speed: " << Utils::StringUtils::formatBitrate(dailyAvgSpeed) << std::endl;
    ss << "Peak Speed: " << Utils::StringUtils::formatBitrate(m_dailyStats.peakSpeed) << std::endl;
    ss << std::endl;
    
    ss << "File Type Statistics:" << std::endl;
    ss << "-------------------" << std::endl;
    
    for (const auto& pair : m_fileTypeStats) {
        ss << pair.first << ": " << Utils::StringUtils::formatFileSize(pair.second) << std::endl;
    }
    
    return ss.str();
}

} // namespace Core
} // namespace DownloadManager