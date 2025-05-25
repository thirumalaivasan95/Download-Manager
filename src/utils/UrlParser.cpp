#include "utils/UrlParser.h"
#include "utils/Logger.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <regex>
#include <iomanip>

namespace dm {
namespace utils {

UrlInfo UrlParser::parse(const std::string& url) {
    UrlInfo info;
    
    if (url.empty()) {
        return info;
    }
    
    try {
        // Parse protocol
        size_t protocolEnd = url.find("://");
        if (protocolEnd != std::string::npos) {
            info.protocol = url.substr(0, protocolEnd);
            protocolEnd += 3;  // Skip "://"
        } else {
            // Default to HTTP if no protocol specified
            info.protocol = "http";
            protocolEnd = 0;
        }
        
        // Extract authentication info (username:password@)
        size_t authEnd = url.find('@', protocolEnd);
        size_t hostStart;
        
        if (authEnd != std::string::npos) {
            std::string authInfo = url.substr(protocolEnd, authEnd - protocolEnd);
            size_t colonPos = authInfo.find(':');
            
            if (colonPos != std::string::npos) {
                info.username = authInfo.substr(0, colonPos);
                info.password = authInfo.substr(colonPos + 1);
            } else {
                info.username = authInfo;
            }
            
            hostStart = authEnd + 1;
        } else {
            hostStart = protocolEnd;
        }
        
        // Extract host and port
        size_t pathStart = url.find('/', hostStart);
        size_t queryStart = url.find('?', hostStart);
        size_t fragmentStart = url.find('#', hostStart);
        
        size_t hostEnd = std::min({
            pathStart != std::string::npos ? pathStart : std::string::npos,
            queryStart != std::string::npos ? queryStart : std::string::npos,
            fragmentStart != std::string::npos ? fragmentStart : std::string::npos
        });
        
        std::string hostPort = url.substr(hostStart, hostEnd != std::string::npos ? hostEnd - hostStart : std::string::npos);
        size_t portPos = hostPort.find(':');
        
        if (portPos != std::string::npos) {
            info.host = hostPort.substr(0, portPos);
            info.port = hostPort.substr(portPos + 1);
        } else {
            info.host = hostPort;
            
            // Set default port based on protocol
            if (info.protocol == "http") {
                info.port = "80";
            } else if (info.protocol == "https") {
                info.port = "443";
            } else if (info.protocol == "ftp") {
                info.port = "21";
            } else if (info.protocol == "ftps") {
                info.port = "990";
            }
        }
        
        // Extract path
        if (pathStart != std::string::npos) {
            size_t pathEnd = std::min(
                queryStart != std::string::npos ? queryStart : std::string::npos,
                fragmentStart != std::string::npos ? fragmentStart : std::string::npos
            );
            
            info.path = url.substr(pathStart, pathEnd != std::string::npos ? pathEnd - pathStart : std::string::npos);
        } else {
            info.path = "/";
        }
        
        // Extract query
        if (queryStart != std::string::npos) {
            size_t queryEnd = fragmentStart != std::string::npos ? fragmentStart : std::string::npos;
            info.query = url.substr(queryStart, queryEnd != std::string::npos ? queryEnd - queryStart : std::string::npos);
        }
        
        // Extract fragment
        if (fragmentStart != std::string::npos) {
            info.fragment = url.substr(fragmentStart);
        }
        
        // Extract filename from path
        info.filename = extractFilename(url);
        
    } catch (const std::exception& e) {
        Logger::error("Error parsing URL: " + url + " - " + e.what());
        info = UrlInfo(); // Reset info
    }
    
    return info;
}

std::string UrlParser::extractFilename(const std::string& url) {
    try {
        // Parse URL
        UrlInfo info = parse(url);
        
        // Get the path part
        std::string path = info.path;
        
        // Remove query parameters
        size_t queryPos = path.find('?');
        if (queryPos != std::string::npos) {
            path = path.substr(0, queryPos);
        }
        
        // Remove fragment
        size_t fragmentPos = path.find('#');
        if (fragmentPos != std::string::npos) {
            path = path.substr(0, fragmentPos);
        }
        
        // Find the last slash
        size_t lastSlash = path.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash < path.length() - 1) {
            // Extract filename
            std::string filename = path.substr(lastSlash + 1);
            
            // URL decode the filename
            return decode(filename);
        }
        
        // If no filename found, try to use the hostname
        if (!info.host.empty()) {
            return info.host;
        }
    } catch (const std::exception& e) {
        Logger::error("Error extracting filename from URL: " + url + " - " + e.what());
    }
    
    return "";
}

std::string UrlParser::normalize(const std::string& url) {
    try {
        // Parse URL
        UrlInfo info = parse(url);
        
        // Build normalized URL
        std::ostringstream oss;
        
        // Add protocol
        oss << info.protocol << "://";
        
        // Add authentication info if present
        if (!info.username.empty()) {
            oss << info.username;
            
            if (!info.password.empty()) {
                oss << ":" << info.password;
            }
            
            oss << "@";
        }
        
        // Add host
        oss << info.host;
        
        // Add port if not default
        if (!(info.protocol == "http" && info.port == "80") &&
            !(info.protocol == "https" && info.port == "443") &&
            !(info.protocol == "ftp" && info.port == "21") &&
            !(info.protocol == "ftps" && info.port == "990") &&
            !info.port.empty()) {
            oss << ":" << info.port;
        }
        
        // Process path
        std::string path = info.path;
        
        // If no path, use root
        if (path.empty()) {
            path = "/";
        }
        
        // Normalize the path
        std::vector<std::string> segments;
        std::istringstream pathStream(path);
        std::string segment;
        
        while (std::getline(pathStream, segment, '/')) {
            if (segment.empty() || segment == ".") {
                continue;
            } else if (segment == "..") {
                if (!segments.empty()) {
                    segments.pop_back();
                }
            } else {
                segments.push_back(segment);
            }
        }
        
        // Build normalized path
        oss << "/";
        for (size_t i = 0; i < segments.size(); ++i) {
            if (i > 0) {
                oss << "/";
            }
            oss << segments[i];
        }
        
        // Add query string
        if (!info.query.empty()) {
            oss << info.query;
        }
        
        // Add fragment
        if (!info.fragment.empty()) {
            oss << info.fragment;
        }
        
        return oss.str();
    } catch (const std::exception& e) {
        Logger::error("Error normalizing URL: " + url + " - " + e.what());
        return url;
    }
}

std::string UrlParser::encode(const std::string& text) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    
    for (char c : text) {
        // Keep alphanumeric and other allowed characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            // Any other character is encoded
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }
    
    return escaped.str();
}

std::string UrlParser::decode(const std::string& text) {
    std::string result;
    result.reserve(text.length());
    
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '%' && i + 2 < text.length()) {
            // Get the hex value after the %
            std::string hex = text.substr(i + 1, 2);
            
            // Convert hex to int
            int value = 0;
            std::istringstream(hex) >> std::hex >> value;
            
            // Append the character
            result += static_cast<char>(value);
            
            // Skip the two hex digits
            i += 2;
        } else if (text[i] == '+') {
            // '+' is converted to space
            result += ' ';
        } else {
            // Other characters are kept as is
            result += text[i];
        }
    }
    
    return result;
}

std::string UrlParser::combine(const std::string& baseUrl, const std::string& relativeUrl) {
    // If relative URL is actually absolute, just return it
    if (relativeUrl.find("://") != std::string::npos) {
        return relativeUrl;
    }
    
    // If relative URL starts with //, it's protocol-relative
    if (relativeUrl.substr(0, 2) == "//") {
        size_t pos = baseUrl.find("://");
        if (pos != std::string::npos) {
            return baseUrl.substr(0, pos + 1) + relativeUrl;
        } else {
            return "http:" + relativeUrl;
        }
    }
    
    // Parse base URL
    UrlInfo baseInfo = parse(baseUrl);
    
    // Build the base part of the URL
    std::ostringstream result;
    result << baseInfo.protocol << "://";
    
    if (!baseInfo.username.empty()) {
        result << baseInfo.username;
        if (!baseInfo.password.empty()) {
            result << ":" << baseInfo.password;
        }
        result << "@";
    }
    
    result << baseInfo.host;
    
    if (!baseInfo.port.empty() &&
        !(baseInfo.protocol == "http" && baseInfo.port == "80") &&
        !(baseInfo.protocol == "https" && baseInfo.port == "443") &&
        !(baseInfo.protocol == "ftp" && baseInfo.port == "21") &&
        !(baseInfo.protocol == "ftps" && baseInfo.port == "990")) {
        result << ":" << baseInfo.port;
    }
    
    // If relative URL starts with /, it's relative to the root
    if (!relativeUrl.empty() && relativeUrl[0] == '/') {
        result << relativeUrl;
    } else {
        // It's relative to the current path
        std::string basePath = baseInfo.path;
        
        // Remove filename component from base path
        size_t lastSlash = basePath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            basePath = basePath.substr(0, lastSlash + 1);
        } else {
            basePath = "/";
        }
        
        result << basePath;
        
        if (!basePath.empty() && basePath.back() != '/' && !relativeUrl.empty() && relativeUrl[0] != '/') {
            result << "/";
        }
        
        result << relativeUrl;
    }
    
    // Normalize the resulting URL
    return normalize(result.str());
}

} // namespace utils
} // namespace dm