#include "utils/Logger.h"
#include "utils/FileUtils.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>

namespace dm {
namespace utils {

// Initialize static member variables
std::string Logger::logFile_;
std::ofstream Logger::logStream_;
std::mutex Logger::mutex_;
LogLevel Logger::logLevel_ = LogLevel::INFO;
size_t Logger::maxFileSize_ = 5 * 1024 * 1024; // 5 MB default
int Logger::maxFiles_ = 5;
bool Logger::initialized_ = false;
bool Logger::consoleLoggingEnabled_ = true;
bool Logger::fileLoggingEnabled_ = true;
LogCallback Logger::logCallback_ = nullptr;

bool Logger::initialize(const std::string& logFile, size_t maxFileSize, int maxFiles) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already initialized
    if (initialized_) {
        // Close existing log file
        if (logStream_.is_open()) {
            logStream_.close();
        }
    }
    
    // Set parameters
    logFile_ = logFile;
    maxFileSize_ = maxFileSize;
    maxFiles_ = maxFiles;
    
    // Create directory if needed
    std::string directory = FileUtils::getDirectory(logFile_);
    if (!directory.empty() && !FileUtils::createDirectory(directory)) {
        std::cerr << "Failed to create log directory: " << directory << std::endl;
        return false;
    }
    
    // Open log file
    logStream_.open(logFile_, std::ios::app);
    if (!logStream_.is_open()) {
        std::cerr << "Failed to open log file: " << logFile_ << std::endl;
        return false;
    }
    
    // Set initialized flag
    initialized_ = true;
    
    // Log initialization
    info("Logger initialized");
    
    return true;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        // Log shutdown
        info("Logger shutdown");
        
        // Close log file
        if (logStream_.is_open()) {
            logStream_.close();
        }
        
        // Reset initialized flag
        initialized_ = false;
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::LOG_ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    // Check if level is enabled
    if (level < logLevel_) {
        return;
    }
    
    // Format log message
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = logLevelToString(level);
    std::string formattedMessage = timestamp + " [" + levelStr + "] " + message;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Write to console if enabled
        if (consoleLoggingEnabled_) {
            // Set output color based on log level
            if (level == LogLevel::LOG_ERROR || level == LogLevel::CRITICAL) {
                std::cerr << formattedMessage << std::endl;
            } else {
                std::cout << formattedMessage << std::endl;
            }
        }
        
        // Write to file if enabled and initialized
        if (fileLoggingEnabled_ && initialized_) {
            writeToFile(formattedMessage);
        }
    }
    
    // Call log callback if set
    if (logCallback_) {
        logCallback_(level, message);
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    logLevel_ = level;
}

LogLevel Logger::getLogLevel() {
    std::lock_guard<std::mutex> lock(mutex_);
    return logLevel_;
}

void Logger::setLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    logCallback_ = callback;
}

void Logger::enableConsoleLogging(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    consoleLoggingEnabled_ = enable;
}

void Logger::enableFileLogging(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    fileLoggingEnabled_ = enable;
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_time_t);
    
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S.");
    ss << std::setfill('0') << std::setw(3) << now_ms.count();
    
    return ss.str();
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::LOG_ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

void Logger::writeToFile(const std::string& message) {
    // Check if file rotation is needed
    if (FileUtils::getFileSize(logFile_) > static_cast<int64_t>(maxFileSize_)) {
        rotateLogFiles();
    }
    
    // Write to log file
    logStream_ << message << std::endl;
    logStream_.flush();
}

void Logger::rotateLogFiles() {
    // Close current log file
    if (logStream_.is_open()) {
        logStream_.close();
    }
    
    // Rotate log files
    for (int i = maxFiles_ - 1; i >= 1; --i) {
        std::string oldFile = logFile_ + "." + std::to_string(i);
        std::string newFile = logFile_ + "." + std::to_string(i + 1);
        
        if (FileUtils::fileExists(oldFile)) {
            if (i == maxFiles_ - 1) {
                // Delete the oldest log file
                FileUtils::deleteFile(oldFile);
            } else {
                // Rename log file
                FileUtils::renameFile(oldFile, newFile);
            }
        }
    }
    
    // Rename current log file
    if (FileUtils::fileExists(logFile_)) {
        FileUtils::renameFile(logFile_, logFile_ + ".1");
    }
    
    // Open new log file
    logStream_.open(logFile_, std::ios::app);
    if (!logStream_.is_open()) {
        std::cerr << "Failed to open rotated log file: " << logFile_ << std::endl;
    }
}

} // namespace utils
} // namespace dm