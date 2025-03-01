#include "../../include/utils/TimeUtils.h"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace DownloadManager {
namespace Utils {

int64_t TimeUtils::getCurrentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

int64_t TimeUtils::getCurrentTimeSeconds() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

std::string TimeUtils::formatTimestamp(int64_t timestamp, const std::string& format) {
    std::time_t time = timestamp;
    std::tm* timeInfo = std::localtime(&time);
    
    std::stringstream ss;
    ss << std::put_time(timeInfo, format.c_str());
    
    return ss.str();
}

std::string TimeUtils::getCurrentDateTime(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&time);
    
    std::stringstream ss;
    ss << std::put_time(timeInfo, format.c_str());
    
    return ss.str();
}

std::string TimeUtils::formatDuration(int64_t milliseconds) {
    int64_t seconds = milliseconds / 1000;
    int64_t minutes = seconds / 60;
    seconds %= 60;
    int64_t hours = minutes / 60;
    minutes %= 60;
    
    std::stringstream ss;
    
    if (hours > 0) {
        ss << hours << "h ";
    }
    
    if (minutes > 0 || hours > 0) {
        ss << minutes << "m ";
    }
    
    ss << seconds << "s";
    
    return ss.str();
}

int64_t TimeUtils::parseDateTime(const std::string& dateTime, const std::string& format) {
    std::tm timeInfo = {};
    std::istringstream ss(dateTime);
    ss >> std::get_time(&timeInfo, format.c_str());
    
    if (ss.fail()) {
        return -1;
    }
    
    std::time_t time = std::mktime(&timeInfo);
    return static_cast<int64_t>(time);
}

int64_t TimeUtils::calculateRemainingTime(int64_t totalSize, int64_t downloadedSize, int64_t currentSpeed) {
    if (currentSpeed <= 0) {
        return -1; // Unknown
    }
    
    int64_t remainingSize = totalSize - downloadedSize;
    
    if (remainingSize <= 0) {
        return 0;
    }
    
    return remainingSize / currentSpeed;
}

int64_t TimeUtils::getTimeDifference(int64_t time1, int64_t time2) {
    return std::abs(time1 - time2);
}

std::string TimeUtils::getTimeZone() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&time);
    
    std::stringstream ss;
    ss << std::put_time(timeInfo, "%Z");
    
    return ss.str();
}

int TimeUtils::getTimeZoneOffset() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    std::tm* gmTime = std::gmtime(&time);
    std::time_t gmTimestamp = std::mktime(gmTime);
    
    std::tm* localTime = std::localtime(&time);
    std::time_t localTimestamp = std::mktime(localTime);
    
    // Calculate offset in seconds
    return static_cast<int>(localTimestamp - gmTimestamp);
}

bool TimeUtils::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int TimeUtils::getDaysInMonth(int year, int month) {
    // Array of days in each month (regular year)
    static const int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month < 1 || month > 12) {
        return -1;
    }
    
    // February special case
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    
    return daysPerMonth[month - 1];
}

std::string TimeUtils::humanReadableTimeAgo(int64_t timestamp) {
    int64_t now = getCurrentTimeSeconds();
    int64_t diff = now - timestamp;
    
    if (diff < 0) {
        return "in the future";
    }
    
    if (diff < 60) {
        return "just now";
    }
    
    if (diff < 3600) {
        int minutes = static_cast<int>(diff / 60);
        return std::to_string(minutes) + " minute" + (minutes == 1 ? "" : "s") + " ago";
    }
    
    if (diff < 86400) {
        int hours = static_cast<int>(diff / 3600);
        return std::to_string(hours) + " hour" + (hours == 1 ? "" : "s") + " ago";
    }
    
    if (diff < 2592000) {
        int days = static_cast<int>(diff / 86400);
        return std::to_string(days) + " day" + (days == 1 ? "" : "s") + " ago";
    }
    
    if (diff < 31536000) {
        int months = static_cast<int>(diff / 2592000);
        return std::to_string(months) + " month" + (months == 1 ? "" : "s") + " ago";
    }
    
    int years = static_cast<int>(diff / 31536000);
    return std::to_string(years) + " year" + (years == 1 ? "" : "s") + " ago";
}

} // namespace Utils
} // namespace DownloadManager