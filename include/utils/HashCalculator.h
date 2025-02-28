#ifndef HASH_CALCULATOR_H
#define HASH_CALCULATOR_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

namespace dm {
namespace utils {

/**
 * @brief Hash algorithm enumeration
 */
enum class HashAlgorithm {
    MD5,
    SHA1,
    SHA256,
    SHA512,
    CRC32
};

/**
 * @brief Hash calculation progress callback function type
 */
using HashProgressCallback = std::function<void(double progress)>;

/**
 * @brief Hash calculation completion callback function type
 */
using HashCompletionCallback = std::function<void(const std::string& hash)>;

/**
 * @brief Hash calculation error callback function type
 */
using HashErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Hash calculator class
 * 
 * Provides file hash calculation with multiple algorithms
 */
class HashCalculator {
public:
    /**
     * @brief Construct a new HashCalculator
     */
    HashCalculator();
    
    /**
     * @brief Destroy the HashCalculator
     */
    ~HashCalculator();
    
    /**
     * @brief Calculate file hash synchronously
     * 
     * @param filePath Path to the file
     * @param algorithm Hash algorithm to use
     * @return std::string The calculated hash
     * @throws std::runtime_error if hash calculation fails
     */
    std::string calculateHash(const std::string& filePath, HashAlgorithm algorithm);
    
    /**
     * @brief Calculate file hash asynchronously
     * 
     * @param filePath Path to the file
     * @param algorithm Hash algorithm to use
     * @param progressCallback Optional callback for progress updates
     * @param completionCallback Optional callback for completion
     * @param errorCallback Optional callback for errors
     */
    void calculateHashAsync(const std::string& filePath, 
                          HashAlgorithm algorithm,
                          HashProgressCallback progressCallback = nullptr,
                          HashCompletionCallback completionCallback = nullptr,
                          HashErrorCallback errorCallback = nullptr);
    
    /**
     * @brief Cancel hash calculation
     */
    void cancel();
    
    /**
     * @brief Check if hash calculation is in progress
     * 
     * @return true if calculation is in progress, false otherwise
     */
    bool isCalculating() const;
    
    /**
     * @brief Get algorithm name
     * 
     * @param algorithm Hash algorithm
     * @return std::string The algorithm name
     */
    static std::string getAlgorithmName(HashAlgorithm algorithm);
    
    /**
     * @brief Verify file hash
     * 
     * @param filePath Path to the file
     * @param expectedHash Expected hash
     * @param algorithm Hash algorithm to use
     * @return true if hash matches, false otherwise
     */
    bool verifyHash(const std::string& filePath, 
                   const std::string& expectedHash, 
                   HashAlgorithm algorithm);
    
    /**
     * @brief Verify file hash asynchronously
     * 
     * @param filePath Path to the file
     * @param expectedHash Expected hash
     * @param algorithm Hash algorithm to use
     * @param progressCallback Optional callback for progress updates
     * @param completionCallback Optional callback for completion
     * @param errorCallback Optional callback for errors
     */
    void verifyHashAsync(const std::string& filePath, 
                        const std::string& expectedHash, 
                        HashAlgorithm algorithm,
                        HashProgressCallback progressCallback = nullptr,
                        std::function<void(bool matches)> completionCallback = nullptr,
                        HashErrorCallback errorCallback = nullptr);
    
private:
    /**
     * @brief Calculate MD5 hash
     * 
     * @param filePath Path to the file
     * @param progressCallback Optional callback for progress updates
     * @return std::string The calculated hash
     */
    std::string calculateMD5(const std::string& filePath, 
                            HashProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Calculate SHA1 hash
     * 
     * @param filePath Path to the file
     * @param progressCallback Optional callback for progress updates
     * @return std::string The calculated hash
     */
    std::string calculateSHA1(const std::string& filePath, 
                             HashProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Calculate SHA256 hash
     * 
     * @param filePath Path to the file
     * @param progressCallback Optional callback for progress updates
     * @return std::string The calculated hash
     */
    std::string calculateSHA256(const std::string& filePath, 
                               HashProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Calculate SHA512 hash
     * 
     * @param filePath Path to the file
     * @param progressCallback Optional callback for progress updates
     * @return std::string The calculated hash
     */
    std::string calculateSHA512(const std::string& filePath, 
                               HashProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Calculate CRC32 hash
     * 
     * @param filePath Path to the file
     * @param progressCallback Optional callback for progress updates
     * @return std::string The calculated hash
     */
    std::string calculateCRC32(const std::string& filePath, 
                              HashProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Process the file with a hash function
     * 
     * @param filePath Path to the file
     * @param processFunction Function to process file data
     * @param progressCallback Optional callback for progress updates
     * @return true if successful, false otherwise
     */
    bool processFile(const std::string& filePath, 
                    std::function<void(const char* data, size_t size)> processFunction,
                    HashProgressCallback progressCallback);
    
    // Member variables
    std::atomic<bool> cancelled_;
    std::atomic<bool> calculating_;
    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
};

} // namespace utils
} // namespace dm

#endif // HASH_CALCULATOR_H