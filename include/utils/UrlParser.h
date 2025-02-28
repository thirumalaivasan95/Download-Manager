#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <string>

namespace dm {
namespace utils {

/**
 * @brief URL information structure
 * 
 * Contains the parsed components of a URL
 */
struct UrlInfo {
    std::string protocol;    // http, https, ftp, etc.
    std::string host;        // domain name or IP address
    std::string port;        // port number as a string
    std::string path;        // path component
    std::string query;       // query string
    std::string fragment;    // fragment identifier
    std::string username;    // username for authentication
    std::string password;    // password for authentication
    std::string filename;    // extracted filename
    
    bool isValid() const { return !host.empty(); }
};

/**
 * @brief URL Parser class
 * 
 * Provides methods for parsing and manipulating URLs
 */
class UrlParser {
public:
    /**
     * @brief Parse a URL string into components
     * 
     * @param url The URL to parse
     * @return UrlInfo The parsed URL information
     */
    static UrlInfo parse(const std::string& url);
    
    /**
     * @brief Extract the filename from a URL
     * 
     * @param url The URL
     * @return std::string The extracted filename or empty string if not found
     */
    static std::string extractFilename(const std::string& url);
    
    /**
     * @brief Normalize a URL (resolve relative paths, etc.)
     * 
     * @param url The URL to normalize
     * @return std::string The normalized URL
     */
    static std::string normalize(const std::string& url);
    
    /**
     * @brief Encode a URL component
     * 
     * @param text The text to encode
     * @return std::string The URL-encoded text
     */
    static std::string encode(const std::string& text);
    
    /**
     * @brief Decode a URL component
     * 
     * @param text The URL-encoded text
     * @return std::string The decoded text
     */
    static std::string decode(const std::string& text);
    
    /**
     * @brief Combine a base URL with a relative URL
     * 
     * @param baseUrl The base URL
     * @param relativeUrl The relative URL
     * @return std::string The combined URL
     */
    static std::string combine(const std::string& baseUrl, const std::string& relativeUrl);
};

} // namespace utils
} // namespace dm

#endif // URL_PARSER_H