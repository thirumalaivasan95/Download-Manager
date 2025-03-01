#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace dm {
namespace utils {

/**
 * @brief Time utilities class
 * 
 * Provides utility functions for time operations
 */
class TimeUtils {
public:
    /**
     * @brief Format a time duration in human-readable form
     * 
     * @param seconds The duration in seconds
     * @param includeSeconds Whether to include seconds in the output
     * @return std::string The formatted duration (e.g., "2h 15m 30s")
     */
    static std::string formatDuration(int64_t seconds, bool includeSeconds = true);
    
    /**
     * @brief Format a time duration in compact form
     * 
     * @param seconds The duration in seconds
     * @return std::string The formatted duration (e.g., "02:15:30")
     */
    static std::string formatDurationCompact(int64_t seconds);
    
    /**
     * @brief Format a time point as a date string
     * 
     * @param timePoint The time point
     * @param format The date format string (strftime format)
     * @return std::string The formatted date
     */
    static std::string formatDate(const std::chrono::system_clock::time_point& timePoint, 
                                const std::string& format = "%Y-%m-%d");
    
    /**
     * @brief Format a time point as a time string
     * 
     * @param timePoint The time point
     * @param format The time format string (strftime format)
     * @return std::string The formatted time
     */
    static std::string formatTime(const std::chrono::system_clock::time_point& timePoint, 
                                const std::string& format = "%H:%M:%S");
    
    /**
     * @brief Format a time point as a datetime string
     * 
     * @param timePoint The time point
     * @param format The datetime format string (strftime format)
     * @return std::string The formatted datetime
     */
    static std::string formatDateTime(const std::chrono::system_clock::time_point& timePoint, 
                                    const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    /**
     * @brief Get the current time point
     * 
     * @return std::chrono::system_clock::time_point The current time point
     */
    static std::chrono::system_clock::time_point now();
    
    /**
     * @brief Get the current time as a timestamp
     * 
     * @return int64_t The current timestamp (seconds since epoch)
     */
    static int64_t currentTimestamp();
    
    /**
     * @brief Get the current time as a millisecond timestamp
     * 
     * @return int64_t The current timestamp (milliseconds since epoch)
     */
    static int64_t currentTimestampMs();
    
    /**
     * @brief Convert a time point to a timestamp
     * 
     * @param timePoint The time point
     * @return int64_t The timestamp (seconds since epoch)
     */
    static int64_t toTimestamp(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Convert a time point to a millisecond timestamp
     * 
     * @param timePoint The time point
     * @return int64_t The timestamp (milliseconds since epoch)
     */
    static int64_t toTimestampMs(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Convert a timestamp to a time point
     * 
     * @param timestamp The timestamp (seconds since epoch)
     * @return std::chrono::system_clock::time_point The time point
     */
    static std::chrono::system_clock::time_point fromTimestamp(int64_t timestamp);
    
    /**
     * @brief Convert a millisecond timestamp to a time point
     * 
     * @param timestampMs The timestamp (milliseconds since epoch)
     * @return std::chrono::system_clock::time_point The time point
     */
    static std::chrono::system_clock::time_point fromTimestampMs(int64_t timestampMs);
    
    /**
     * @brief Parse a date string
     * 
     * @param dateStr The date string
     * @param format The date format string (strftime format)
     * @return std::chrono::system_clock::time_point The parsed time point
     */
    static std::chrono::system_clock::time_point parseDate(const std::string& dateStr, 
                                                        const std::string& format = "%Y-%m-%d");
    
    /**
     * @brief Parse a time string
     * 
     * @param timeStr The time string
     * @param format The time format string (strftime format)
     * @return std::chrono::system_clock::time_point The parsed time point
     */
    static std::chrono::system_clock::time_point parseTime(const std::string& timeStr, 
                                                        const std::string& format = "%H:%M:%S");
    
    /**
     * @brief Parse a datetime string
     * 
     * @param dateTimeStr The datetime string
     * @param format The datetime format string (strftime format)
     * @return std::chrono::system_clock::time_point The parsed time point
     */
    static std::chrono::system_clock::time_point parseDateTime(const std::string& dateTimeStr, 
                                                            const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    /**
     * @brief Calculate the time difference in seconds
     * 
     * @param t1 First time point
     * @param t2 Second time point
     * @return int64_t The time difference in seconds
     */
    static int64_t diffSeconds(const std::chrono::system_clock::time_point& t1, 
                             const std::chrono::system_clock::time_point& t2);
    
    /**
     * @brief Calculate the time difference in milliseconds
     * 
     * @param t1 First time point
     * @param t2 Second time point
     * @return int64_t The time difference in milliseconds
     */
    static int64_t diffMilliseconds(const std::chrono::system_clock::time_point& t1, 
                                  const std::chrono::system_clock::time_point& t2);
    
    /**
     * @brief Add seconds to a time point
     * 
     * @param timePoint The time point
     * @param seconds The seconds to add
     * @return std::chrono::system_clock::time_point The resulting time point
     */
    static std::chrono::system_clock::time_point addSeconds(const std::chrono::system_clock::time_point& timePoint, 
                                                         int64_t seconds);
    
    /**
     * @brief Add minutes to a time point
     * 
     * @param timePoint The time point
     * @param minutes The minutes to add
     * @return std::chrono::system_clock::time_point The resulting time point
     */
    static std::chrono::system_clock::time_point addMinutes(const std::chrono::system_clock::time_point& timePoint, 
                                                         int64_t minutes);
    
    /**
     * @brief Add hours to a time point
     * 
     * @param timePoint The time point
     * @param hours The hours to add
     * @return std::chrono::system_clock::time_point The resulting time point
     */
    static std::chrono::system_clock::time_point addHours(const std::chrono::system_clock::time_point& timePoint, 
                                                       int64_t hours);
    
    /**
     * @brief Add days to a time point
     * 
     * @param timePoint The time point
     * @param days The days to add
     * @return std::chrono::system_clock::time_point The resulting time point
     */
    static std::chrono::system_clock::time_point addDays(const std::chrono::system_clock::time_point& timePoint, 
                                                      int64_t days);
    
    /**
     * @brief Get the start of day for a time point
     * 
     * @param timePoint The time point
     * @return std::chrono::system_clock::time_point The start of day
     */
    static std::chrono::system_clock::time_point startOfDay(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the end of day for a time point
     * 
     * @param timePoint The time point
     * @return std::chrono::system_clock::time_point The end of day
     */
    static std::chrono::system_clock::time_point endOfDay(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the start of week for a time point
     * 
     * @param timePoint The time point
     * @param startOfWeek The start day of week (0 = Sunday, 1 = Monday, etc.)
     * @return std::chrono::system_clock::time_point The start of week
     */
    static std::chrono::system_clock::time_point startOfWeek(const std::chrono::system_clock::time_point& timePoint, 
                                                          int startOfWeek = 0);
    
    /**
     * @brief Get the end of week for a time point
     * 
     * @param timePoint The time point
     * @param startOfWeek The start day of week (0 = Sunday, 1 = Monday, etc.)
     * @return std::chrono::system_clock::time_point The end of week
     */
    static std::chrono::system_clock::time_point endOfWeek(const std::chrono::system_clock::time_point& timePoint, 
                                                        int startOfWeek = 0);
    
    /**
     * @brief Get the start of month for a time point
     * 
     * @param timePoint The time point
     * @return std::chrono::system_clock::time_point The start of month
     */
    static std::chrono::system_clock::time_point startOfMonth(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the end of month for a time point
     * 
     * @param timePoint The time point
     * @return std::chrono::system_clock::time_point The end of month
     */
    static std::chrono::system_clock::time_point endOfMonth(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the start of year for a time point
     * 
     * @param timePoint The time point
     * @return std::chrono::system_clock::time_point The start of year
     */
    static std::chrono::system_clock::time_point startOfYear(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the end of year for a time point
     * 
     * @param timePoint The time point
     * @return std::chrono::system_clock::time_point The end of year
     */
    static std::chrono::system_clock::time_point endOfYear(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Check if a year is a leap year
     * 
     * @param year The year to check
     * @return bool True if leap year, false otherwise
     */
    static bool isLeapYear(int year);
    
    /**
     * @brief Get the day of week for a time point
     * 
     * @param timePoint The time point
     * @return int The day of week (0 = Sunday, 1 = Monday, etc.)
     */
    static int getDayOfWeek(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the day of month for a time point
     * 
     * @param timePoint The time point
     * @return int The day of month (1-31)
     */
    static int getDayOfMonth(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the day of year for a time point
     * 
     * @param timePoint The time point
     * @return int The day of year (1-366)
     */
    static int getDayOfYear(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the month for a time point
     * 
     * @param timePoint The time point
     * @return int The month (1-12)
     */
    static int getMonth(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the year for a time point
     * 
     * @param timePoint The time point
     * @return int The year
     */
    static int getYear(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the hour for a time point
     * 
     * @param timePoint The time point
     * @return int The hour (0-23)
     */
    static int getHour(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the minute for a time point
     * 
     * @param timePoint The time point
     * @return int The minute (0-59)
     */
    static int getMinute(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the second for a time point
     * 
     * @param timePoint The time point
     * @return int The second (0-59)
     */
    static int getSecond(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get the millisecond for a time point
     * 
     * @param timePoint The time point
     * @return int The millisecond (0-999)
     */
    static int getMillisecond(const std::chrono::system_clock::time_point& timePoint);
    
    /**
     * @brief Get days in month
     * 
     * @param year The year
     * @param month The month (1-12)
     * @return int The number of days in the month
     */
    static int getDaysInMonth(int year, int month);
    
    /**
     * @brief Get days in year
     * 
     * @param year The year
     * @return int The number of days in the year
     */
    static int getDaysInYear(int year);
};

} // namespace utils
} // namespace dm

#endif // TIME_UTILS_H