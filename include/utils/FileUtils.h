#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace dm {
namespace utils {

/**
 * @brief File utilities class
 * 
 * Provides utility functions for file operations
 */
class FileUtils {
public:
    /**
     * @brief Check if a file exists
     * 
     * @param filePath The file path
     * @return true if the file exists, false otherwise
     */
    static bool fileExists(const std::string& filePath);
    
    /**
     * @brief Get the file size
     * 
     * @param filePath The file path
     * @return int64_t The file size in bytes, or -1 on error
     */
    static int64_t getFileSize(const std::string& filePath);
    
    /**
     * @brief Create a directory
     * 
     * @param dirPath The directory path
     * @return true if successful, false otherwise
     */
    static bool createDirectory(const std::string& dirPath);
    
    /**
     * @brief Delete a file
     * 
     * @param filePath The file path
     * @return true if successful, false otherwise
     */
    static bool deleteFile(const std::string& filePath);
    
    /**
     * @brief Rename a file
     * 
     * @param oldPath The old file path
     * @param newPath The new file path
     * @return true if successful, false otherwise
     */
    static bool renameFile(const std::string& oldPath, const std::string& newPath);
    
    /**
     * @brief Copy a file
     * 
     * @param sourcePath The source file path
     * @param destPath The destination file path
     * @return true if successful, false otherwise
     */
    static bool copyFile(const std::string& sourcePath, const std::string& destPath);
    
    /**
     * @brief Get the directory part of a path
     * 
     * @param path The path
     * @return std::string The directory part
     */
    static std::string getDirectory(const std::string& path);
    
    /**
     * @brief Get the filename part of a path
     * 
     * @param path The path
     * @return std::string The filename part
     */
    static std::string getFilename(const std::string& path);
    
    /**
     * @brief Get the extension part of a filename
     * 
     * @param path The path or filename
     * @return std::string The extension part
     */
    static std::string getExtension(const std::string& path);
    
    /**
     * @brief Get the base name (filename without extension)
     * 
     * @param path The path or filename
     * @return std::string The base name
     */
    static std::string getBaseName(const std::string& path);
    
    /**
     * @brief Format a file size in human-readable form
     * 
     * @param size The size in bytes
     * @param precision The number of decimal places
     * @return std::string The formatted size
     */
    static std::string formatFileSize(int64_t size, int precision = 2);
    
    /**
     * @brief Get the application data directory
     * 
     * @return std::string The application data directory
     */
    static std::string getAppDataDirectory();
    
    /**
     * @brief Get the default download directory
     * 
     * @return std::string The default download directory
     */
    static std::string getDefaultDownloadDirectory();
    
    /**
     * @brief Get the temporary directory
     * 
     * @return std::string The temporary directory
     */
    static std::string getTempDirectory();
    
    /**
     * @brief Create a temporary file
     * 
     * @param prefix The file prefix
     * @param suffix The file suffix
     * @return std::string The temporary file path
     */
    static std::string createTempFile(const std::string& prefix, const std::string& suffix);
    
    /**
     * @brief Combine paths
     * 
     * @param path1 First path component
     * @param path2 Second path component
     * @return std::string The combined path
     */
    static std::string combinePaths(const std::string& path1, const std::string& path2);
    
    /**
     * @brief Read a text file
     * 
     * @param filePath The file path
     * @return std::string The file contents
     */
    static std::string readTextFile(const std::string& filePath);
    
    /**
     * @brief Write a text file
     * 
     * @param filePath The file path
     * @param content The content to write
     * @return true if successful, false otherwise
     */
    static bool writeTextFile(const std::string& filePath, const std::string& content);
    
    /**
     * @brief Read a binary file
     * 
     * @param filePath The file path
     * @return std::vector<uint8_t> The file contents
     */
    static std::vector<uint8_t> readBinaryFile(const std::string& filePath);
    
    /**
     * @brief Write a binary file
     * 
     * @param filePath The file path
     * @param data The data to write
     * @return true if successful, false otherwise
     */
    static bool writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data);
    
    /**
     * @brief Calculate file MD5 hash
     * 
     * @param filePath The file path
     * @return std::string The MD5 hash as a hex string
     */
    static std::string calculateMd5(const std::string& filePath);
    
    /**
     * @brief Find files in a directory
     * 
     * @param dirPath The directory path
     * @param extension Optional file extension filter
     * @param recursive Whether to search recursively
     * @return std::vector<std::string> The found files
     */
    static std::vector<std::string> findFiles(const std::string& dirPath, 
                                            const std::string& extension = "", 
                                            bool recursive = false);
};

} // namespace utils
} // namespace dm

#endif // FILE_UTILS_H