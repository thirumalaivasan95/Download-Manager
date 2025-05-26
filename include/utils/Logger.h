#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <functional>

namespace dm {
namespace utils {

/**
 * @brief Log level enumeration
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    LOG_ERROR,
    CRITICAL
};

/**
 * @brief Log callback function type
 */
using LogCallback = std::function<void(LogLevel level, const std::string& message)>;

/**
 * @brief Logger class for application logging
 */
class Logger {
public:
    /**
     * @brief Initialize the logger
     * 
     * @param logFile Path to the log file
     * @param maxFileSize Maximum log file size in bytes before rotation
     * @param maxFiles Maximum number of log files to keep
     * @return true if successful, false otherwise
     */
    static bool initialize(const std::string& logFile, 
                          size_t maxFileSize = 5 * 1024 * 1024, 
                          int maxFiles = 5);
    
    /**
     * @brief Shutdown the logger
     */
    static void shutdown();
    
    /**
     * @brief Log a debug message
     * 
     * @param message The message to log
     */
    static void debug(const std::string& message);
    
    /**
     * @brief Log an info message
     * 
     * @param message The message to log
     */
    static void info(const std::string& message);
    
    /**
     * @brief Log a warning message
     * 
     * @param message The message to log
     */
    static void warning(const std::string& message);
    
    /**
     * @brief Log an error message
     * 
     * @param message The message to log
     */
    static void error(const std::string& message);
    
    /**
     * @brief Log a critical message
     * 
     * @param message The message to log
     */
    static void critical(const std::string& message);
    
    /**
     * @brief Log a message with the specified level
     * 
     * @param level The log level
     * @param message The message to log
     */
    static void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Set the minimum log level
     * 
     * @param level The minimum log level
     */
    static void setLogLevel(LogLevel level);
    
    /**
     * @brief Get the minimum log level
     * 
     * @return LogLevel The minimum log level
     */
    static LogLevel getLogLevel();
    
    /**
     * @brief Set the log callback
     * 
     * @param callback The log callback function
     */
    static void setLogCallback(LogCallback callback);
    
    /**
     * @brief Enable/disable console logging
     * 
     * @param enable True to enable console logging, false to disable
     */
    static void enableConsoleLogging(bool enable);
    
    /**
     * @brief Enable/disable file logging
     * 
     * @param enable True to enable file logging, false to disable
     */
    static void enableFileLogging(bool enable);
    
private:
    /**
     * @brief Get the current timestamp
     * 
     * @return std::string The timestamp
     */
    static std::string getCurrentTimestamp();
    
    /**
     * @brief Convert a log level to a string
     * 
     * @param level The log level
     * @return std::string The log level as a string
     */
    static std::string logLevelToString(LogLevel level);
    
    /**
     * @brief Write a log message to the file
     * 
     * @param message The message to write
     */
    static void writeToFile(const std::string& message);
    
    /**
     * @brief Rotate log files if needed
     */
    static void rotateLogFiles();
    
    // Static member variables
    static std::string logFile_;
    static std::ofstream logStream_;
    static std::mutex mutex_;
    static LogLevel logLevel_;
    static size_t maxFileSize_;
    static int maxFiles_;
    static bool initialized_;
    static bool consoleLoggingEnabled_;
    static bool fileLoggingEnabled_;
    static LogCallback logCallback_;
};

} // namespace utils
} // namespace dm

#endif // LOGGER_H