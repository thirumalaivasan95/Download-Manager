#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <regex>
#include <stdexcept>

namespace DownloadManager {
namespace Utils {

/**
 * @brief String utilities class
 * 
 * Provides utility functions for string manipulation
 */
class StringUtils {
public:
    /**
     * @brief Trim whitespace from the beginning of a string
     * 
     * @param str The string to trim
     * @return std::string The trimmed string
     */
    static std::string trimLeft(const std::string& str);
    
    /**
     * @brief Trim whitespace from the end of a string
     * 
     * @param str The string to trim
     * @return std::string The trimmed string
     */
    static std::string trimRight(const std::string& str);
    
    /**
     * @brief Trim whitespace from both ends of a string
     * 
     * @param str The string to trim
     * @return std::string The trimmed string
     */
    static std::string trim(const std::string& str);
    
    /**
     * @brief Convert a string to lowercase
     * 
     * @param str The string to convert
     * @return std::string The lowercase string
     */
    static std::string toLowerCase(const std::string& str);
    
    /**
     * @brief Convert a string to uppercase
     * 
     * @param str The string to convert
     * @return std::string The uppercase string
     */
    static std::string toUpperCase(const std::string& str);
    
    /**
     * @brief Split a string by delimiter
     * 
     * @param str The string to split
     * @param delimiter The delimiter
     * @param skipEmpty Skip empty tokens
     * @return std::vector<std::string> The tokens
     */
    static std::vector<std::string> split(const std::string& str, 
                                       const std::string& delimiter,
                                       bool skipEmpty = true);
    
    /**
     * @brief Split a string by whitespace
     * 
     * @param str The string to split
     * @param skipEmpty Skip empty tokens
     * @return std::vector<std::string> The tokens
     */
    static std::vector<std::string> splitByWhitespace(const std::string& str,
                                                   bool skipEmpty = true);
    
    /**
     * @brief Join strings with a delimiter
     * 
     * @param tokens The strings to join
     * @param delimiter The delimiter
     * @return std::string The joined string
     */
    static std::string join(const std::vector<std::string>& tokens,
                          const std::string& delimiter);
    
    /**
     * @brief Replace all occurrences of a substring
     * 
     * @param str The string to process
     * @param from The substring to replace
     * @param to The replacement
     * @return std::string The resulting string
     */
    static std::string replace(const std::string& str,
                             const std::string& from,
                             const std::string& to);
    
    /**
     * @brief Check if a string starts with a prefix
     * 
     * @param str The string to check
     * @param prefix The prefix
     * @param caseSensitive Whether the check is case-sensitive
     * @return bool True if the string starts with the prefix
     */
    static bool startsWith(const std::string& str,
                          const std::string& prefix,
                          bool caseSensitive = true);
    
    /**
     * @brief Check if a string ends with a suffix
     * 
     * @param str The string to check
     * @param suffix The suffix
     * @param caseSensitive Whether the check is case-sensitive
     * @return bool True if the string ends with the suffix
     */
    static bool endsWith(const std::string& str,
                        const std::string& suffix,
                        bool caseSensitive = true);
    
    /**
     * @brief Check if a string contains a substring
     * 
     * @param str The string to check
     * @param substring The substring
     * @param caseSensitive Whether the check is case-sensitive
     * @return bool True if the string contains the substring
     */
    static bool contains(const std::string& str,
                        const std::string& substring,
                        bool caseSensitive = true);
    
    /**
     * @brief Convert a string to an integer
     * 
     * @param str The string to convert
     * @param defaultValue The default value to use if conversion fails
     * @return int The converted value
     */
    static int toInt(const std::string& str, int defaultValue = 0);
    
    /**
     * @brief Convert a string to a double
     * 
     * @param str The string to convert
     * @param defaultValue The default value to use if conversion fails
     * @return double The converted value
     */
    static double toDouble(const std::string& str, double defaultValue = 0.0);
    
    /**
     * @brief Convert a string to a boolean
     * 
     * @param str The string to convert
     * @param defaultValue The default value to use if conversion fails
     * @return bool The converted value
     */
    static bool toBool(const std::string& str, bool defaultValue = false);
    
    /**
     * @brief Convert an integer to a string
     * 
     * @param value The value to convert
     * @return std::string The converted string
     */
    static std::string toString(int value);
    
    /**
     * @brief Convert a double to a string
     * 
     * @param value The value to convert
     * @param precision The number of decimal places
     * @return std::string The converted string
     */
    static std::string toString(double value, int precision = 2);
    
    /**
     * @brief Convert a boolean to a string
     * 
     * @param value The value to convert
     * @return std::string The converted string
     */
    static std::string toString(bool value);
    
    /**
     * @brief Format a file size in a human-readable form
     * 
     * @param bytes The size in bytes
     * @param precision The number of decimal places
     * @return std::string The formatted size
     */
    static std::string formatFileSize(int64_t bytes, int precision = 2);
    
    /**
     * @brief Format a bitrate in a human-readable form
     * 
     * @param bytesPerSecond The bitrate in bytes per second
     * @param precision The number of decimal places
     * @return std::string The formatted bitrate
     */
    static std::string formatBitrate(int64_t bytesPerSecond, int precision = 2);
    
    /**
     * @brief Format a time duration in a human-readable form
     * 
     * @param seconds The duration in seconds
     * @return std::string The formatted duration
     */
    static std::string formatTime(int64_t seconds);
    
    /**
     * @brief Format a percentage
     * 
     * @param value The percentage value (0-100)
     * @param includeSymbol Whether to include the % symbol
     * @return std::string The formatted percentage
     */
    static std::string formatPercentage(double value, bool includeSymbol = true);
    
    /**
     * @brief Extract the domain from a URL
     * 
     * @param url The URL
     * @return std::string The domain
     */
    static std::string extractDomain(const std::string& url);
    
    /**
     * @brief Extract the file name from a URL or path
     * 
     * @param urlOrPath The URL or path
     * @return std::string The file name
     */
    static std::string extractFileName(const std::string& urlOrPath);
    
    /**
     * @brief Extract the file extension from a URL or path
     * 
     * @param urlOrPath The URL or path
     * @return std::string The file extension
     */
    static std::string extractFileExtension(const std::string& urlOrPath);
    
    /**
     * @brief Check if a string is a valid URL
     * 
     * @param str The string to check
     * @return bool True if the string is a valid URL
     */
    static bool isValidUrl(const std::string& str);
    
    /**
     * @brief Check if a string is a valid email
     * 
     * @param str The string to check
     * @return bool True if the string is a valid email
     */
    static bool isValidEmail(const std::string& str);
    
    /**
     * @brief Check if a string is numeric
     * 
     * @param str The string to check
     * @return bool True if the string is numeric
     */
    static bool isNumeric(const std::string& str);
    
    /**
     * @brief Generate a random string
     * 
     * @param length The length of the string
     * @param includeDigits Whether to include digits
     * @param includeUppercase Whether to include uppercase letters
     * @param includeLowercase Whether to include lowercase letters
     * @param includeSpecial Whether to include special characters
     * @return std::string The random string
     */
    static std::string generateRandomString(int length, 
                                          bool includeDigits = true,
                                          bool includeUppercase = true,
                                          bool includeLowercase = true,
                                          bool includeSpecial = false);
    
    /**
     * @brief Escape special characters in a string
     * 
     * @param str The string to escape
     * @return std::string The escaped string
     */
    static std::string escapeString(const std::string& str);
    
    /**
     * @brief Unescape special characters in a string
     * 
     * @param str The string to unescape
     * @return std::string The unescaped string
     */
    static std::string unescapeString(const std::string& str);
    
    /**
     * @brief Encode a string for URL
     * 
     * @param str The string to encode
     * @return std::string The encoded string
     */
    static std::string urlEncode(const std::string& str);
    
    /**
     * @brief Decode a URL-encoded string
     * 
     * @param str The string to decode
     * @return std::string The decoded string
     */
    static std::string urlDecode(const std::string& str);
    
    /**
     * @brief Encode a string for HTML
     * 
     * @param str The string to encode
     * @return std::string The encoded string
     */
    static std::string htmlEncode(const std::string& str);
    
    /**
     * @brief Decode an HTML-encoded string
     * 
     * @param str The string to decode
     * @return std::string The decoded string
     */
    static std::string htmlDecode(const std::string& str);
    
    /**
     * @brief Base64 encode a string
     * 
     * @param str The string to encode
     * @return std::string The encoded string
     */
    static std::string base64Encode(const std::string& str);
    
    /**
     * @brief Base64 decode a string
     * 
     * @param str The string to decode
     * @return std::string The decoded string
     */
    static std::string base64Decode(const std::string& str);
};

} // namespace Utils
} // namespace DownloadManager

#endif // STRING_UTILS_H