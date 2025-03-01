#include "../../include/utils/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <random>
#include <functional>

namespace DownloadManager {
namespace Utils {

std::string StringUtils::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) {
        return "";
    }
    
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, last - first + 1);
}

std::string StringUtils::trimLeft(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) {
        return "";
    }
    
    return str.substr(first);
}

std::string StringUtils::trimRight(const std::string& str) {
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    if (last == std::string::npos) {
        return "";
    }
    
    return str.substr(0, last + 1);
}

std::string StringUtils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string StringUtils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool StringUtils::startsWith(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    
    return str.substr(0, prefix.length()) == prefix;
}

bool StringUtils::endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    
    return str.substr(str.length() - suffix.length()) == suffix;
}

std::vector<std::string> StringUtils::split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

std::vector<std::string> StringUtils::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t pos = 0;
    size_t prev = 0;
    
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        result.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.length();
    }
    
    // Add the last part
    result.push_back(str.substr(prev));
    
    return result;
}

std::string StringUtils::join(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) {
        return "";
    }
    
    std::stringstream ss;
    ss << strings[0];
    
    for (size_t i = 1; i < strings.size(); ++i) {
        ss << delimiter << strings[i];
    }
    
    return ss.str();
}

std::string StringUtils::replace(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return str;
    }
    
    std::string result = str;
    size_t pos = 0;
    
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    
    return result;
}

std::string StringUtils::replaceAll(const std::string& str, const std::map<std::string, std::string>& replacements) {
    std::string result = str;
    
    for (const auto& pair : replacements) {
        result = replace(result, pair.first, pair.second);
    }
    
    return result;
}

bool StringUtils::contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

std::string StringUtils::urlencode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : str) {
        // Keep alphanumeric and other accepted characters intact
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other character is escaped
        escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
    }

    return escaped.str();
}

std::string StringUtils::urldecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.size()) {
                try {
                    std::string hex = str.substr(i + 1, 2);
                    int value = std::stoi(hex, nullptr, 16);
                    result += static_cast<char>(value);
                    i += 2;
                } 
                catch (const std::exception&) {
                    // If not a valid hex code, just add the '%' character
                    result += '%';
                }
            } else {
                // If we don't have 2 chars after '%', just add the '%' character
                result += '%';
            }
        } 
        else if (str[i] == '+') {
            result += ' ';
        } 
        else {
            result += str[i];
        }
    }
    
    return result;
}

std::string StringUtils::formatFileSize(int64_t size) {
    const int64_t KB = 1024;
    const int64_t MB = 1024 * KB;
    const int64_t GB = 1024 * MB;
    const int64_t TB = 1024 * GB;
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    if (size < KB) {
        ss << size << " bytes";
    } else if (size < MB) {
        ss << static_cast<double>(size) / KB << " KB";
    } else if (size < GB) {
        ss << static_cast<double>(size) / MB << " MB";
    } else if (size < TB) {
        ss << static_cast<double>(size) / GB << " GB";
    } else {
        ss << static_cast<double>(size) / TB << " TB";
    }
    
    return ss.str();
}

std::string StringUtils::formatBitrate(int64_t bytesPerSecond) {
    const int64_t KBPS = 1024;
    const int64_t MBPS = 1024 * KBPS;
    const int64_t GBPS = 1024 * MBPS;
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    if (bytesPerSecond < KBPS) {
        ss << bytesPerSecond << " B/s";
    } else if (bytesPerSecond < MBPS) {
        ss << static_cast<double>(bytesPerSecond) / KBPS << " KB/s";
    } else if (bytesPerSecond < GBPS) {
        ss << static_cast<double>(bytesPerSecond) / MBPS << " MB/s";
    } else {
        ss << static_cast<double>(bytesPerSecond) / GBPS << " GB/s";
    }
    
    return ss.str();
}

std::string StringUtils::formatTime(int64_t seconds) {
    if (seconds < 0) {
        return "Unknown";
    }
    
    int64_t days = seconds / (24 * 3600);
    seconds %= (24 * 3600);
    int64_t hours = seconds / 3600;
    seconds %= 3600;
    int64_t minutes = seconds / 60;
    seconds %= 60;
    
    std::stringstream ss;
    
    if (days > 0) {
        ss << days << "d ";
    }
    
    if (hours > 0 || days > 0) {
        ss << hours << "h ";
    }
    
    if (minutes > 0 || hours > 0 || days > 0) {
        ss << minutes << "m ";
    }
    
    ss << seconds << "s";
    
    return ss.str();
}

std::string StringUtils::generateRandomString(size_t length) {
    static const std::string chars =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += chars[distribution(generator)];
    }
    
    return result;
}

std::string StringUtils::extractFilenameFromUrl(const std::string& url) {
    // Find the last slash in the URL
    size_t lastSlash = url.find_last_of('/');
    
    if (lastSlash == std::string::npos || lastSlash == url.length() - 1) {
        return "";
    }
    
    // Extract everything after the last slash
    std::string filename = url.substr(lastSlash + 1);
    
    // Remove query parameters if present
    size_t questionMark = filename.find('?');
    if (questionMark != std::string::npos) {
        filename = filename.substr(0, questionMark);
    }
    
    // Remove fragment identifier if present
    size_t hash = filename.find('#');
    if (hash != std::string::npos) {
        filename = filename.substr(0, hash);
    }
    
    // URL decode the filename
    return urldecode(filename);
}

std::string StringUtils::extractFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    
    if (dotPos == std::string::npos || dotPos == 0 || dotPos == filename.length() - 1) {
        return "";
    }
    
    return filename.substr(dotPos);
}

} // namespace Utils
} // namespace DownloadManager