#include "../../include/utils/StringUtils.h"
#include <iomanip>
#include <random>
#include <ctime>

namespace DownloadManager {
namespace Utils {

std::string StringUtils::trimLeft(const std::string& str) {
    size_t start = 0;
    while (start < str.length() && std::isspace(str[start])) {
        ++start;
    }
    return (start == 0) ? str : str.substr(start);
}

std::string StringUtils::trimRight(const std::string& str) {
    size_t end = str.length();
    while (end > 0 && std::isspace(str[end - 1])) {
        --end;
    }
    return (end == str.length()) ? str : str.substr(0, end);
}

std::string StringUtils::trim(const std::string& str) {
    return trimLeft(trimRight(str));
}

std::string StringUtils::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string StringUtils::toUpperCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::vector<std::string> StringUtils::split(const std::string& str, 
                                         const std::string& delimiter,
                                         bool skipEmpty) {
    std::vector<std::string> tokens;
    
    if (str.empty()) {
        return tokens;
    }
    
    if (delimiter.empty()) {
        tokens.push_back(str);
        return tokens;
    }
    
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        std::string token = str.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.push_back(token);
        }
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    std::string token = str.substr(start);
    if (!skipEmpty || !token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::vector<std::string> StringUtils::splitByWhitespace(const std::string& str,
                                                     bool skipEmpty) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    
    while (iss >> token) {
        if (!skipEmpty || !token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string StringUtils::join(const std::vector<std::string>& tokens,
                            const std::string& delimiter) {
    if (tokens.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    auto it = tokens.begin();
    oss << *it;
    ++it;
    
    for (; it != tokens.end(); ++it) {
        oss << delimiter << *it;
    }
    
    return oss.str();
}

std::string StringUtils::replace(const std::string& str,
                               const std::string& from,
                               const std::string& to) {
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

bool StringUtils::startsWith(const std::string& str,
                            const std::string& prefix,
                            bool caseSensitive) {
    if (str.length() < prefix.length()) {
        return false;
    }
    
    if (caseSensitive) {
        return str.substr(0, prefix.length()) == prefix;
    } else {
        return toLowerCase(str.substr(0, prefix.length())) == toLowerCase(prefix);
    }
}

bool StringUtils::endsWith(const std::string& str,
                          const std::string& suffix,
                          bool caseSensitive) {
    if (str.length() < suffix.length()) {
        return false;
    }
    
    if (caseSensitive) {
        return str.substr(str.length() - suffix.length()) == suffix;
    } else {
        return toLowerCase(str.substr(str.length() - suffix.length())) == toLowerCase(suffix);
    }
}

bool StringUtils::contains(const std::string& str,
                          const std::string& substring,
                          bool caseSensitive) {
    if (caseSensitive) {
        return str.find(substring) != std::string::npos;
    } else {
        return toLowerCase(str).find(toLowerCase(substring)) != std::string::npos;
    }
}

int StringUtils::toInt(const std::string& str, int defaultValue) {
    try {
        return std::stoi(str);
    } catch (...) {
        return defaultValue;
    }
}

double StringUtils::toDouble(const std::string& str, double defaultValue) {
    try {
        return std::stod(str);
    } catch (...) {
        return defaultValue;
    }
}

bool StringUtils::toBool(const std::string& str, bool defaultValue) {
    std::string lower = toLowerCase(trim(str));
    
    if (lower == "true" || lower == "yes" || lower == "1" || lower == "on") {
        return true;
    } else if (lower == "false" || lower == "no" || lower == "0" || lower == "off") {
        return false;
    }
    
    return defaultValue;
}

std::string StringUtils::toString(int value) {
    return std::to_string(value);
}

std::string StringUtils::toString(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

std::string StringUtils::toString(bool value) {
    return value ? "true" : "false";
}

std::string StringUtils::formatFileSize(int64_t bytes, int precision) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 6) {
        size /= 1024.0;
        ++unitIndex;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << size << " " << units[unitIndex];
    return oss.str();
}

std::string StringUtils::formatBitrate(int64_t bytesPerSecond, int precision) {
    static const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s", "TB/s"};
    int unitIndex = 0;
    double bitrate = static_cast<double>(bytesPerSecond);
    
    while (bitrate >= 1024.0 && unitIndex < 4) {
        bitrate /= 1024.0;
        ++unitIndex;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << bitrate << " " << units[unitIndex];
    return oss.str();
}

std::string StringUtils::formatTime(int64_t seconds) {
    if (seconds < 0) {
        return "--:--:--";
    }
    
    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds % 3600) / 60);
    int secs = static_cast<int>(seconds % 60);
    
    std::ostringstream oss;
    
    if (hours > 0) {
        oss << hours << ":" 
            << std::setw(2) << std::setfill('0') << minutes << ":" 
            << std::setw(2) << std::setfill('0') << secs;
    } else {
        oss << minutes << ":" 
            << std::setw(2) << std::setfill('0') << secs;
    }
    
    return oss.str();
}

std::string StringUtils::formatPercentage(double value, bool includeSymbol) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value;
    
    if (includeSymbol) {
        oss << "%";
    }
    
    return oss.str();
}

std::string StringUtils::extractDomain(const std::string& url) {
    // Simple domain extraction - more robust implementation would use proper URL parsing
    std::string domain;
    
    // Remove protocol
    size_t protocolEnd = url.find("://");
    size_t start = (protocolEnd != std::string::npos) ? protocolEnd + 3 : 0;
    
    // Find end of domain (first slash after protocol)
    size_t end = url.find('/', start);
    
    if (end != std::string::npos) {
        domain = url.substr(start, end - start);
    } else {
        domain = url.substr(start);
    }
    
    // Remove port if present
    size_t portPos = domain.find(':');
    if (portPos != std::string::npos) {
        domain = domain.substr(0, portPos);
    }
    
    return domain;
}

std::string StringUtils::extractFileName(const std::string& urlOrPath) {
    // Find the last slash or backslash
    size_t lastSlash = urlOrPath.find_last_of("/\\");
    
    // Extract everything after the last slash, or the whole string if no slash found
    std::string filename = (lastSlash != std::string::npos) ? 
                          urlOrPath.substr(lastSlash + 1) : 
                          urlOrPath;
    
    // Remove URL parameters if present
    size_t paramPos = filename.find('?');
    if (paramPos != std::string::npos) {
        filename = filename.substr(0, paramPos);
    }
    
    // Remove fragment if present
    size_t fragPos = filename.find('#');
    if (fragPos != std::string::npos) {
        filename = filename.substr(0, fragPos);
    }
    
    return filename;
}

std::string StringUtils::extractFileExtension(const std::string& urlOrPath) {
    std::string filename = extractFileName(urlOrPath);
    
    // Find the last dot
    size_t lastDot = filename.find_last_of('.');
    
    // Extract everything after the last dot, or empty string if no dot found
    return (lastDot != std::string::npos && lastDot < filename.length() - 1) ? 
           filename.substr(lastDot + 1) : 
           "";
}

bool StringUtils::isValidUrl(const std::string& str) {
    // Basic URL validation using regex
    std::regex urlRegex(
        R"(^(https?|ftp)://)"  // protocol
        R"(([a-zA-Z0-9_\-\.]+))"  // domain name
        R"((\.[a-zA-Z]{2,}))"  // top level domain
        R"((:[0-9]+)?)"  // port (optional)
        R"((\/[^\/][a-zA-Z0-9\/_\.:-]*)?$)"  // path (optional)
    );
    
    return std::regex_match(str, urlRegex);
}

bool StringUtils::isValidEmail(const std::string& str) {
    // Basic email validation using regex
    std::regex emailRegex(
        R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
    );
    
    return std::regex_match(str, emailRegex);
}

bool StringUtils::isNumeric(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isdigit(c) || c == '.' || c == '-' || c == '+';
    });
}

std::string StringUtils::generateRandomString(int length, 
                                          bool includeDigits,
                                          bool includeUppercase,
                                          bool includeLowercase,
                                          bool includeSpecial) {
    static const char digits[] = "0123456789";
    static const char uppercase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const char lowercase[] = "abcdefghijklmnopqrstuvwxyz";
    static const char special[] = "!@#$%^&*()-_=+[]{}|;:,.<>?";
    
    std::string chars;
    
    if (includeDigits) chars += digits;
    if (includeUppercase) chars += uppercase;
    if (includeLowercase) chars += lowercase;
    if (includeSpecial) chars += special;
    
    if (chars.empty()) {
        chars = digits; // Default to digits if nothing selected
    }
    
    // Set up random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, static_cast<int>(chars.length() - 1));
    
    std::string result;
    result.reserve(length);
    
    for (int i = 0; i < length; ++i) {
        result += chars[distrib(gen)];
    }
    
    return result;
}

std::string StringUtils::escapeString(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 2); // Reserve space for potential escaping
    
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '\"': result += "\\\""; break;
            case '\'': result += "\\\'"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            default: result += c; break;
        }
    }
    
    return result;
}

std::string StringUtils::unescapeString(const std::string& str) {
    std::string result;
    result.reserve(str.length()); // Reserve space
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '\\': result += '\\'; break;
                case '\"': result += '\"'; break;
                case '\'': result += '\''; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                default: result += str[i + 1]; break;
            }
            ++i; // Skip the escaped character
        } else {
            result += str[i];
        }
    }
    
    return result;
}

std::string StringUtils::urlEncode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    
    for (char c : str) {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        
        // Any other characters are percent-encoded
        escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
    }
    
    return escaped.str();
}

std::string StringUtils::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // Percent-encoded character
            std::string hex = str.substr(i + 1, 2);
            char decoded = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += decoded;
            i += 2; // Skip the hex digits
        } else if (str[i] == '+') {
            // + is decoded as space
            result += ' ';
        } else {
            // Regular character
            result += str[i];
        }
    }
    
    return result;
}

std::string StringUtils::htmlEncode(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 2); // Reserve space for potential encoding
    
    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '\"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c; break;
        }
    }
    
    return result;
}

std::string StringUtils::htmlDecode(const std::string& str) {
    std::string result = str;
    
    // Replace common HTML entities
    result = replace(result, "&amp;", "&");
    result = replace(result, "&lt;", "<");
    result = replace(result, "&gt;", ">");
    result = replace(result, "&quot;", "\"");
    result = replace(result, "&#39;", "'");
    
    return result;
}

// Base64 encoding table
static const char base64Chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string StringUtils::base64Encode(const std::string& str) {
    std::string result;
    int val = 0;
    int valb = -6;
    
    for (unsigned char c : str) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        result.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    // Add padding
    while (result.size() % 4) {
        result.push_back('=');
    }
    
    return result;
}

std::string StringUtils::base64Decode(const std::string& str) {
    static const std::string base64Chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    std::vector<int> T(256, -1);
    
    for (int i = 0; i < 64; i++) {
        T[base64Chars[i]] = i;
    }
    
    int val = 0;
    int valb = -8;
    
    for (unsigned char c : str) {
        if (T[c] == -1) continue; // Skip non-base64 chars (like padding)
        
        val = (val << 6) + T[c];
        valb += 6;
        
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return result;
}