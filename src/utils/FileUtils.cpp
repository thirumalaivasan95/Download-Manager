#include "include/utils/FileUtils.h"
#include "include/utils/Logger.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <openssl/md5.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <shlobj.h>
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#define PATH_SEPARATOR "/"
#endif

namespace dm {
namespace utils {

bool FileUtils::fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}

int64_t FileUtils::getFileSize(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.good()) {
        return -1;
    }
    
    return static_cast<int64_t>(file.tellg());
}

bool FileUtils::createDirectory(const std::string& dirPath) {
    // If directory already exists, return true
    if (fileExists(dirPath)) {
        return true;
    }
    
    // Create the directory
#ifdef _WIN32
    int result = _mkdir(dirPath.c_str());
#else
    int result = mkdir(dirPath.c_str(), 0755);
#endif
    
    if (result == 0) {
        return true;
    }
    
    // If parent directory doesn't exist, create it
    if (errno == ENOENT) {
        std::string parentPath = getDirectory(dirPath);
        if (!parentPath.empty() && parentPath != dirPath) {
            if (createDirectory(parentPath)) {
                // Try again
#ifdef _WIN32
                return _mkdir(dirPath.c_str()) == 0;
#else
                return mkdir(dirPath.c_str(), 0755) == 0;
#endif
            }
        }
    }
    
    return false;
}

bool FileUtils::deleteFile(const std::string& filePath) {
    return std::remove(filePath.c_str()) == 0;
}

bool FileUtils::renameFile(const std::string& oldPath, const std::string& newPath) {
    return std::rename(oldPath.c_str(), newPath.c_str()) == 0;
}

bool FileUtils::copyFile(const std::string& sourcePath, const std::string& destPath) {
    // Open source file
    std::ifstream source(sourcePath, std::ios::binary);
    if (!source.is_open()) {
        return false;
    }
    
    // Open destination file
    std::ofstream dest(destPath, std::ios::binary);
    if (!dest.is_open()) {
        return false;
    }
    
    // Copy file
    dest << source.rdbuf();
    
    // Check for errors
    return source.good() && dest.good();
}

std::string FileUtils::getDirectory(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return "";
    }
    
    return path.substr(0, pos);
}

std::string FileUtils::getFilename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return path;
    }
    
    return path.substr(pos + 1);
}

std::string FileUtils::getExtension(const std::string& path) {
    std::string filename = getFilename(path);
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return "";
    }
    
    return filename.substr(pos + 1);
}

std::string FileUtils::getBaseName(const std::string& path) {
    std::string filename = getFilename(path);
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return filename;
    }
    
    return filename.substr(0, pos);
}

std::string FileUtils::formatFileSize(int64_t size, int precision) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    static const int numUnits = sizeof(units) / sizeof(units[0]);
    
    if (size == 0) {
        return "0 B";
    }
    
    // Calculate unit
    int unitIndex = std::min(static_cast<int>(std::log10(std::abs(size)) / 3), numUnits - 1);
    
    // Calculate size in the selected unit
    double normalizedSize = size / std::pow(1000.0, unitIndex);
    
    // Format the result
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << normalizedSize << " " << units[unitIndex];
    
    return oss.str();
}

std::string FileUtils::getAppDataDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        return std::string(path) + PATH_SEPARATOR + "DownloadManager";
    }
    return "";
#else
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) {
            homeDir = pwd->pw_dir;
        }
    }
    
    if (homeDir) {
        return std::string(homeDir) + "/.downloadmanager";
    }
    
    return "";
#endif
}

std::string FileUtils::getDefaultDownloadDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, path))) {
        return std::string(path) + PATH_SEPARATOR + "Downloads";
    }
    return "";
#else
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) {
            homeDir = pwd->pw_dir;
        }
    }
    
    if (homeDir) {
        return std::string(homeDir) + "/Downloads";
    }
    
    return "";
#endif
}

std::string FileUtils::getTempDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD length = GetTempPathA(MAX_PATH, path);
    if (length > 0) {
        return std::string(path);
    }
    return "";
#else
    const char* tempDir = getenv("TMPDIR");
    if (!tempDir) {
        tempDir = "/tmp";
    }
    return std::string(tempDir);
#endif
}

std::string FileUtils::createTempFile(const std::string& prefix, const std::string& suffix) {
    std::string tempDir = getTempDirectory();
    if (tempDir.empty()) {
        return "";
    }
    
    // Generate a unique filename
    std::string tempFile;
    
#ifdef _WIN32
    char tempPath[MAX_PATH];
    if (GetTempFileNameA(tempDir.c_str(), prefix.c_str(), 0, tempPath) != 0) {
        tempFile = tempPath;
        
        // Rename if suffix is provided
        if (!suffix.empty()) {
            std::string newPath = tempFile + suffix;
            if (renameFile(tempFile, newPath)) {
                tempFile = newPath;
            }
        }
    }
#else
    std::string templatePath = tempDir + "/" + prefix + "XXXXXX";
    char* templateCopy = strdup(templatePath.c_str());
    
    int fd = mkstemp(templateCopy);
    if (fd != -1) {
        close(fd);
        tempFile = templateCopy;
        
        // Rename if suffix is provided
        if (!suffix.empty()) {
            std::string newPath = tempFile + suffix;
            if (renameFile(tempFile, newPath)) {
                tempFile = newPath;
            }
        }
    }
    
    free(templateCopy);
#endif
    
    return tempFile;
}

std::string FileUtils::combinePaths(const std::string& path1, const std::string& path2) {
    if (path1.empty()) {
        return path2;
    }
    
    if (path2.empty()) {
        return path1;
    }
    
    char lastChar = path1[path1.length() - 1];
    if (lastChar == '/' || lastChar == '\\') {
        return path1 + path2;
    } else {
        return path1 + PATH_SEPARATOR + path2;
    }
}

std::string FileUtils::readTextFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return buffer.str();
}

bool FileUtils::writeTextFile(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    
    return file.good();
}

std::vector<uint8_t> FileUtils::readBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    
    return {};
}

bool FileUtils::writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    return file.good();
}

std::string FileUtils::calculateMd5(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    
    char buffer[4096];
    while (file.good()) {
        file.read(buffer, sizeof(buffer));
        MD5_Update(&md5Context, buffer, file.gcount());
    }
    
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5_Final(result, &md5Context);
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        oss << std::setw(2) << static_cast<int>(result[i]);
    }
    
    return oss.str();
}

std::vector<std::string> FileUtils::findFiles(const std::string& dirPath, 
                                            const std::string& extension, 
                                            bool recursive) {
    std::vector<std::string> files;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    
    // Find all files
    std::string searchPath = dirPath + "\\*";
    hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Skip "." and ".."
            if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
                continue;
            }
            
            std::string filePath = dirPath + "\\" + findData.cFileName;
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Recurse into subdirectory if requested
                if (recursive) {
                    std::vector<std::string> subFiles = findFiles(filePath, extension, true);
                    files.insert(files.end(), subFiles.begin(), subFiles.end());
                }
            } else {
                // Check extension if provided
                if (extension.empty() || getExtension(filePath) == extension) {
                    files.push_back(filePath);
                }
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(dirPath.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string filePath = dirPath + "/" + entry->d_name;
            
            struct stat statBuf;
            if (stat(filePath.c_str(), &statBuf) == 0) {
                if (S_ISDIR(statBuf.st_mode)) {
                    // Recurse into subdirectory if requested
                    if (recursive) {
                        std::vector<std::string> subFiles = findFiles(filePath, extension, true);
                        files.insert(files.end(), subFiles.begin(), subFiles.end());
                    }
                } else if (S_ISREG(statBuf.st_mode)) {
                    // Check extension if provided
                    if (extension.empty() || getExtension(filePath) == extension) {
                        files.push_back(filePath);
                    }
                }
            }
        }
        
        closedir(dir);
    }
#endif
    
    return files;
}

} // namespace utils
} // namespace dm