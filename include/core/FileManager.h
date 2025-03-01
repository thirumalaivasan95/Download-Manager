#ifndef DOWNLOADMANAGER_FILEMANAGER_H
#define DOWNLOADMANAGER_FILEMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <filesystem>
#include "../utils/FileUtils.h"

namespace fs = std::filesystem;

namespace DownloadManager {
namespace Core {

/**
 * @brief The FileManager class handles file operations for the download manager.
 * 
 * This class is responsible for creating, writing, reading, and managing files
 * associated with downloads. It also handles file integrity checks, resumable
 * downloads, and temporary file management.
 */
class FileManager {
public:
    /**
     * @brief Constructor for FileManager
     */
    FileManager();
    
    /**
     * @brief Destructor for FileManager
     */
    ~FileManager();
    
    /**
     * @brief Creates a file for a new download
     * @param filePath The path where the file will be created
     * @param fileSize The expected size of the file in bytes
     * @return True if file creation was successful, false otherwise
     */
    bool createFile(const std::string& filePath, int64_t fileSize);
    
    /**
     * @brief Writes data to a file at a specific offset
     * @param filePath The path of the file to write to
     * @param data The data to write
     * @param offset The offset in bytes where the data should be written
     * @param size The size of the data in bytes
     * @return True if write was successful, false otherwise
     */
    bool writeToFile(const std::string& filePath, const char* data, int64_t offset, size_t size);
    
    /**
     * @brief Checks if a file exists and is valid for resuming a download
     * @param filePath The path of the file to check
     * @return True if the file exists and is valid, false otherwise
     */
    bool isFileResumeValid(const std::string& filePath);
    
    /**
     * @brief Gets the size of an existing file
     * @param filePath The path of the file
     * @return The size of the file in bytes, or -1 if the file doesn't exist
     */
    int64_t getFileSize(const std::string& filePath);
    
    /**
     * @brief Creates a temporary file for a download
     * @param originalFilePath The path of the original file
     * @return The path of the created temporary file
     */
    std::string createTempFile(const std::string& originalFilePath);
    
    /**
     * @brief Finalizes a download by renaming the temporary file to the target file
     * @param tempFilePath The path of the temporary file
     * @param targetFilePath The path of the target file
     * @return True if finalization was successful, false otherwise
     */
    bool finalizeDownload(const std::string& tempFilePath, const std::string& targetFilePath);
    
    /**
     * @brief Performs integrity check on a downloaded file
     * @param filePath The path of the file to check
     * @param expectedHash The expected hash value
     * @param hashType The type of hash (MD5, SHA-1, SHA-256, etc.)
     * @return True if the file passes the integrity check, false otherwise
     */
    bool checkFileIntegrity(const std::string& filePath, const std::string& expectedHash, const std::string& hashType);
    
    /**
     * @brief Gets the free space available in the directory
     * @param directoryPath The path of the directory
     * @return The available free space in bytes
     */
    int64_t getAvailableDiskSpace(const std::string& directoryPath);
    
    /**
     * @brief Splits a file into segments for multi-part downloads
     * @param filePath The path of the file to split
     * @param segments The number of segments to create
     * @return A vector of paths to the created segments
     */
    std::vector<std::string> splitFileIntoSegments(const std::string& filePath, int segments);
    
    /**
     * @brief Merges file segments back into a single file
     * @param segmentPaths The paths of the segments to merge
     * @param outputFilePath The path of the output file
     * @return True if merge was successful, false otherwise
     */
    bool mergeFileSegments(const std::vector<std::string>& segmentPaths, const std::string& outputFilePath);

private:
    /**
     * @brief Creates directories for a file path if they don't exist
     * @param filePath The full path of the file
     * @return True if directories exist or were created successfully, false otherwise
     */
    bool ensureDirectoryExists(const std::string& filePath);
    
    /**
     * @brief Generates a unique temporary file name
     * @param basePath The base path for the temporary file
     * @return A unique temporary file path
     */
    std::string generateTempFileName(const std::string& basePath);
};

} // namespace Core
} // namespace DownloadManager

#endif // DOWNLOADMANAGER_FILEMANAGER_H