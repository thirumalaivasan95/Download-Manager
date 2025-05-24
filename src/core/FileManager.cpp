#include "../../include/core/FileManager.h"
#include "utils/Logger.h"
#include "utils/HashCalculator.h"

#include <fstream>
#include <random>
#include <chrono>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace DownloadManager {
namespace Core {

FileManager::FileManager() {
    // Constructor implementation
}

FileManager::~FileManager() {
    // Destructor implementation
}

bool FileManager::createFile(const std::string& filePath, int64_t fileSize) {
    try {
        // Ensure the directory exists
        if (!ensureDirectoryExists(filePath)) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to create directory for: " + filePath);
            return false;
        }
        
        // Create the file with specified size
        std::ofstream file(filePath, std::ios::binary | std::ios::out);
        if (!file.is_open()) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to create file: " + filePath);
            return false;
        }
        
        // Pre-allocate the file if size is known
        if (fileSize > 0) {
            // Seek to the end position
            file.seekp(fileSize - 1);
            // Write a single byte to allocate the space
            file.put(0);
        }
        
        file.close();
        return true;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in createFile: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::writeToFile(const std::string& filePath, const char* data, int64_t offset, size_t size) {
    try {
        std::fstream file(filePath, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open()) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to open file for writing: " + filePath);
            return false;
        }
        
        // Seek to the position
        file.seekp(offset);
        
        // Write the data
        file.write(data, size);
        
        // Check if write was successful
        if (file.fail()) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to write data to file: " + filePath);
            return false;
        }
        
        file.close();
        return true;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in writeToFile: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::isFileResumeValid(const std::string& filePath) {
    try {
        // Check if file exists
        if (!fs::exists(filePath)) {
            return false;
        }
        
        // Open the file to verify it's not corrupted
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.close();
        return true;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in isFileResumeValid: " + std::string(e.what()));
        return false;
    }
}

int64_t FileManager::getFileSize(const std::string& filePath) {
    try {
        if (!fs::exists(filePath)) {
            return -1;
        }
        
        return fs::file_size(filePath);
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in getFileSize: " + std::string(e.what()));
        return -1;
    }
}

std::string FileManager::createTempFile(const std::string& originalFilePath) {
    try {
        std::string tempFilePath = generateTempFileName(originalFilePath);
        
        // Create an empty file
        std::ofstream file(tempFilePath, std::ios::binary);
        if (!file.is_open()) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to create temporary file: " + tempFilePath);
            return "";
        }
        
        file.close();
        return tempFilePath;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in createTempFile: " + std::string(e.what()));
        return "";
    }
}

bool FileManager::finalizeDownload(const std::string& tempFilePath, const std::string& targetFilePath) {
    try {
        // Ensure the target directory exists
        if (!ensureDirectoryExists(targetFilePath)) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to create directory for target file: " + targetFilePath);
            return false;
        }
        
        // Remove existing target file if it exists
        if (fs::exists(targetFilePath)) {
            fs::remove(targetFilePath);
        }
        
        // Rename temp file to target file
        fs::rename(tempFilePath, targetFilePath);
        
        return true;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in finalizeDownload: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::checkFileIntegrity(const std::string& filePath, const std::string& expectedHash, const std::string& hashType) {
    try {
        // Calculate file hash
        Utils::HashCalculator hashCalculator;
        std::string calculatedHash;
        
        if (hashType == "MD5") {
            calculatedHash = hashCalculator.calculateMD5(filePath);
        } 
        else if (hashType == "SHA1") {
            calculatedHash = hashCalculator.calculateSHA1(filePath);
        } 
        else if (hashType == "SHA256") {
            calculatedHash = hashCalculator.calculateSHA256(filePath);
        } 
        else {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Unsupported hash type: " + hashType);
            return false;
        }
        
        // Compare hashes (case-insensitive)
        std::string expectedHashLower = expectedHash;
        std::string calculatedHashLower = calculatedHash;
        std::transform(expectedHashLower.begin(), expectedHashLower.end(), expectedHashLower.begin(), ::tolower);
        std::transform(calculatedHashLower.begin(), calculatedHashLower.end(), calculatedHashLower.begin(), ::tolower);
        
        return expectedHashLower == calculatedHashLower;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in checkFileIntegrity: " + std::string(e.what()));
        return false;
    }
}

int64_t FileManager::getAvailableDiskSpace(const std::string& directoryPath) {
    try {
        fs::space_info spaceInfo = fs::space(directoryPath);
        return static_cast<int64_t>(spaceInfo.available);
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in getAvailableDiskSpace: " + std::string(e.what()));
        return -1;
    }
}

std::vector<std::string> FileManager::splitFileIntoSegments(const std::string& filePath, int segments) {
    std::vector<std::string> segmentPaths;
    
    try {
        // Validate file
        if (!fs::exists(filePath)) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "File does not exist: " + filePath);
            return segmentPaths;
        }
        
        // Calculate file size
        size_t fileSize = fs::file_size(filePath);
        
        // Calculate segment size
        size_t segmentSize = fileSize / segments;
        size_t remainder = fileSize % segments;
        
        // Open source file
        std::ifstream srcFile(filePath, std::ios::binary);
        if (!srcFile.is_open()) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to open source file: " + filePath);
            return segmentPaths;
        }
        
        // Create segments
        for (int i = 0; i < segments; i++) {
            // Calculate current segment size
            size_t currentSegmentSize = segmentSize;
            if (i == segments - 1) {
                currentSegmentSize += remainder;
            }
            
            // Create segment file name
            std::string segmentPath = filePath + ".part" + std::to_string(i + 1);
            
            // Open segment file
            std::ofstream segmentFile(segmentPath, std::ios::binary);
            if (!segmentFile.is_open()) {
                dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to create segment file: " + segmentPath);
                continue;
            }
            
            // Read and write data
            std::vector<char> buffer(1024 * 1024); // 1MB buffer
            size_t bytesRemaining = currentSegmentSize;
            
            while (bytesRemaining > 0 && srcFile.good()) {
                size_t bytesToRead = std::min(bytesRemaining, buffer.size());
                srcFile.read(buffer.data(), bytesToRead);
                size_t bytesRead = srcFile.gcount();
                
                if (bytesRead > 0) {
                    segmentFile.write(buffer.data(), bytesRead);
                    bytesRemaining -= bytesRead;
                } else {
                    break;
                }
            }
            
            segmentFile.close();
            segmentPaths.push_back(segmentPath);
        }
        
        srcFile.close();
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in splitFileIntoSegments: " + std::string(e.what()));
    }
    
    return segmentPaths;
}

bool FileManager::mergeFileSegments(const std::vector<std::string>& segmentPaths, const std::string& outputFilePath) {
    try {
        // Ensure the output directory exists
        if (!ensureDirectoryExists(outputFilePath)) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to create directory for output file: " + outputFilePath);
            return false;
        }
        
        // Create output file
        std::ofstream outputFile(outputFilePath, std::ios::binary);
        if (!outputFile.is_open()) {
            dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to create output file: " + outputFilePath);
            return false;
        }
        
        // Merge segments
        std::vector<char> buffer(1024 * 1024); // 1MB buffer
        
        for (const auto& segmentPath : segmentPaths) {
            // Validate segment file
            if (!fs::exists(segmentPath)) {
                dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Segment file does not exist: " + segmentPath);
                outputFile.close();
                fs::remove(outputFilePath);
                return false;
            }
            
            // Open segment file
            std::ifstream segmentFile(segmentPath, std::ios::binary);
            if (!segmentFile.is_open()) {
                dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to open segment file: " + segmentPath);
                outputFile.close();
                fs::remove(outputFilePath);
                return false;
            }
            
            // Read and write data
            while (segmentFile.good()) {
                segmentFile.read(buffer.data(), buffer.size());
                size_t bytesRead = segmentFile.gcount();
                
                if (bytesRead > 0) {
                    outputFile.write(buffer.data(), bytesRead);
                } else {
                    break;
                }
            }
            
            segmentFile.close();
        }
        
        outputFile.close();
        return true;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in mergeFileSegments: " + std::string(e.what()));
        return false;
    }
}

bool FileManager::ensureDirectoryExists(const std::string& filePath) {
    try {
        fs::path path(filePath);
        fs::path dirPath = path.parent_path();
        
        if (!dirPath.empty() && !fs::exists(dirPath)) {
            return fs::create_directories(dirPath);
        }
        
        return true;
    } 
    catch (const std::exception& e) {
        dm::utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in ensureDirectoryExists: " + std::string(e.what()));
        return false;
    }
}

std::string FileManager::generateTempFileName(const std::string& basePath) {
    // Generate a unique temporary file name based on timestamp and random number
    std::stringstream ss;
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    
    // Generate random number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1000, 9999);
    int randomNum = distrib(gen);
    
    // Extract path and filename
    fs::path path(basePath);
    fs::path directory = path.parent_path();
    std::string filename = path.filename().string();
    
    // Create temporary file name
    ss << directory.string() << "/"
       << filename << "."
       << timestamp << "."
       << randomNum << ".tmp";
    
    return ss.str();
}

} // namespace Core
} // namespace DownloadManager