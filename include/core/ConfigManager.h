#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <any>
#include <mutex>
#include <functional>
#include <memory>

namespace dm {
namespace utils {

/**
 * @brief Configuration change callback function type
 */
using ConfigChangeCallback = std::function<void(const std::string& key, const std::any& value)>;

/**
 * @brief Configuration validator function type
 */
using ConfigValidator = std::function<bool(const std::string& key, const std::any& value)>;

/**
 * @brief Configuration category enumeration
 */
enum class ConfigCategory {
    CORE,            // Core settings
    UI,              // UI settings
    NETWORK,         // Network settings
    PROTOCOLS,       // Protocol settings
    PLUGINS,         // Plugin settings
    SCHEDULER,       // Scheduler settings
    STATISTICS,      // Statistics settings
    PERFORMANCE,     // Performance settings
    SECURITY,        // Security settings
    ADVANCED,        // Advanced settings
    USER_DEFINED     // User-defined settings
};

/**
 * @brief Configuration manager class
 * 
 * Centralized configuration system with change notification
 */
class ConfigManager {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return ConfigManager& The singleton instance
     */
    static ConfigManager& getInstance();
    
    /**
     * @brief Initialize the configuration manager
     * 
     * @param configFile Path to the configuration file
     * @return true if successful, false otherwise
     */
    bool initialize(const std::string& configFile = "");
    
    /**
     * @brief Save configuration to file
     * 
     * @param configFile Optional path to configuration file
     * @return true if successful, false otherwise
     */
    bool saveConfig(const std::string& configFile = "");
    
    /**
     * @brief Load configuration from file
     * 
     * @param configFile Path to configuration file
     * @return true if successful, false otherwise
     */
    bool loadConfig(const std::string& configFile);
    
    /**
     * @brief Reset configuration to defaults
     * 
     * @param category Optional category to reset (all categories if not specified)
     */
    void resetToDefaults(ConfigCategory category = ConfigCategory::CORE);
    
    /**
     * @brief Get configuration value
     * 
     * @tparam T Value type
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return T The configuration value
     */
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = config_.find(key);
        if (it != config_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        
        return defaultValue;
    }
    
    /**
     * @brief Set configuration value
     * 
     * @tparam T Value type
     * @param key Configuration key
     * @param value Configuration value
     * @param category Configuration category
     * @return true if successful, false otherwise
     */
    template<typename T>
    bool setValue(const std::string& key, const T& value, ConfigCategory category = ConfigCategory::CORE) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if validator exists for this key
        auto validatorIt = validators_.find(key);
        if (validatorIt != validators_.end()) {
            if (!validatorIt->second(key, value)) {
                return false;
            }
        }
        
        // Store old value for change notification
        std::any oldValue;
        auto it = config_.find(key);
        bool keyExists = (it != config_.end());
        if (keyExists) {
            oldValue = it->second;
        }
        
        // Set new value
        config_[key] = value;
        
        // Update category map
        keyCategories_[key] = category;
        
        // Trigger change callbacks
        if (!keyExists || oldValue.type() != typeid(T) || std::any_cast<T>(oldValue) != value) {
            triggerChangeCallbacks(key, value);
        }
        
        // Mark as modified
        modified_ = true;
        
        return true;
    }
    
    /**
     * @brief Check if a key exists
     * 
     * @param key Configuration key
     * @return true if exists, false otherwise
     */
    bool hasKey(const std::string& key) const;
    
    /**
     * @brief Remove a key
     * 
     * @param key Configuration key
     * @return true if removed, false otherwise
     */
    bool removeKey(const std::string& key);
    
    /**
     * @brief Get all keys
     * 
     * @return std::vector<std::string> All configuration keys
     */
    std::vector<std::string> getAllKeys() const;
    
    /**
     * @brief Get keys by category
     * 
     * @param category Configuration category
     * @return std::vector<std::string> Keys in the specified category
     */
    std::vector<std::string> getKeysByCategory(ConfigCategory category) const;
    
    /**
     * @brief Get category for key
     * 
     * @param key Configuration key
     * @return ConfigCategory Category for the key
     */
    ConfigCategory getCategoryForKey(const std::string& key) const;
    
    /**
     * @brief Register change callback
     * 
     * @param key Configuration key
     * @param callback Callback function
     * @return int Callback ID for unregistering
     */
    int registerChangeCallback(const std::string& key, ConfigChangeCallback callback);
    
    /**
     * @brief Unregister change callback
     * 
     * @param callbackId Callback ID from registration
     * @return true if unregistered, false otherwise
     */
    bool unregisterChangeCallback(int callbackId);
    
    /**
     * @brief Register validator
     * 
     * @param key Configuration key
     * @param validator Validator function
     */
    void registerValidator(const std::string& key, ConfigValidator validator);
    
    /**
     * @brief Export configuration
     * 
     * @param filePath Path to export file
     * @param category Optional category to export
     * @return true if successful, false otherwise
     */
    bool exportConfig(const std::string& filePath, ConfigCategory category = ConfigCategory::CORE);
    
    /**
     * @brief Import configuration
     * 
     * @param filePath Path to import file
     * @param overwrite Whether to overwrite existing values
     * @return true if successful, false otherwise
     */
    bool importConfig(const std::string& filePath, bool overwrite = true);
    
    /**
     * @brief Get configuration file path
     * 
     * @return std::string The configuration file path
     */
    std::string getConfigFilePath() const;
    
    /**
     * @brief Check if configuration has been modified
     * 
     * @return true if modified, false otherwise
     */
    bool isModified() const;
    
private:
    /**
     * @brief Construct a new ConfigManager
     */
    ConfigManager();
    
    /**
     * @brief Destroy the ConfigManager
     */
    ~ConfigManager();
    
    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    /**
     * @brief Trigger change callbacks for a key
     * 
     * @param key Configuration key
     * @param value New value
     */
    void triggerChangeCallbacks(const std::string& key, const std::any& value);
    
    /**
     * @brief Create default configuration
     */
    void createDefaultConfig();
    
    /**
     * @brief Convert string to ConfigCategory
     * 
     * @param category Category string
     * @return ConfigCategory The category enum
     */
    ConfigCategory stringToCategory(const std::string& category) const;
    
    /**
     * @brief Convert ConfigCategory to string
     * 
     * @param category The category enum
     * @return std::string Category string
     */
    std::string categoryToString(ConfigCategory category) const;
    
    // Member variables
    std::map<std::string, std::any> config_;
    std::map<std::string, ConfigCategory> keyCategories_;
    std::map<std::string, ConfigValidator> validators_;
    std::map<int, std::pair<std::string, ConfigChangeCallback>> callbacks_;
    std::string configFilePath_;
    int nextCallbackId_;
    bool modified_;
    mutable std::mutex mutex_;
    bool initialized_;
};

} // namespace utils
} // namespace dm

#endif // CONFIG_MANAGER_H