#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <mutex>
#include <map>

namespace dm {
namespace core {

/**
 * @brief Settings class for application configuration
 */
class Settings {
public:
    /**
     * @brief Construct a new Settings object
     */
    Settings();
    
    /**
     * @brief Destroy the Settings object
     */
    ~Settings();
    
    /**
     * @brief Load settings from disk
     * 
     * @return true if successful, false otherwise
     */
    bool load();
    
    /**
     * @brief Save settings to disk
     * 
     * @return true if successful, false otherwise
     */
    bool save();
    
    /**
     * @brief Reset settings to defaults
     */
    void resetToDefaults();
    
    /**
     * @brief Get the download directory
     * 
     * @return std::string The download directory
     */
    std::string getDownloadDirectory() const;
    
    /**
     * @brief Set the download directory
     * 
     * @param directory The download directory
     */
    void setDownloadDirectory(const std::string& directory);
    
    /**
     * @brief Get the maximum concurrent downloads
     * 
     * @return int The maximum concurrent downloads
     */
    int getMaxConcurrentDownloads() const;
    
    /**
     * @brief Set the maximum concurrent downloads
     * 
     * @param max The maximum concurrent downloads
     */
    void setMaxConcurrentDownloads(int max);
    
    /**
     * @brief Get the number of segments per download
     * 
     * @return int The number of segments
     */
    int getSegmentCount() const;
    
    /**
     * @brief Set the number of segments per download
     * 
     * @param count The number of segments
     */
    void setSegmentCount(int count);
    
    /**
     * @brief Get whether to close to tray
     * 
     * @return bool True if close to tray is enabled
     */
    bool getCloseToTray() const;
    
    /**
     * @brief Set whether to close to tray
     * 
     * @param enabled True to enable close to tray
     */
    void setCloseToTray(bool enabled);
    
    /**
     * @brief Get whether to show notifications
     * 
     * @return bool True if notifications are enabled
     */
    bool getShowNotifications() const;
    
    /**
     * @brief Set whether to show notifications
     * 
     * @param enabled True to enable notifications
     */
    void setShowNotifications(bool enabled);
    
    /**
     * @brief Get whether to start with the system
     * 
     * @return bool True if start with system is enabled
     */
    bool getStartWithSystem() const;
    
    /**
     * @brief Set whether to start with the system
     * 
     * @param enabled True to enable start with system
     */
    void setStartWithSystem(bool enabled);
    
    /**
     * @brief Get whether to start minimized
     * 
     * @return bool True if start minimized is enabled
     */
    bool getStartMinimized() const;
    
    /**
     * @brief Set whether to start minimized
     * 
     * @param enabled True to enable start minimized
     */
    void setStartMinimized(bool enabled);
    
    /**
     * @brief Get whether to auto-start downloads
     * 
     * @return bool True if auto-start is enabled
     */
    bool getAutoStartDownloads() const;
    
    /**
     * @brief Set whether to auto-start downloads
     * 
     * @param enabled True to enable auto-start
     */
    void setAutoStartDownloads(bool enabled);
    
    /**
     * @brief Get the maximum download speed (0 for unlimited)
     * 
     * @return int The maximum download speed in KB/s
     */
    int getMaxDownloadSpeed() const;
    
    /**
     * @brief Set the maximum download speed (0 for unlimited)
     * 
     * @param speed The maximum download speed in KB/s
     */
    void setMaxDownloadSpeed(int speed);
    
    /**
     * @brief Get a string setting value
     * 
     * @param key The setting key
     * @param defaultValue The default value
     * @return std::string The setting value
     */
    std::string getStringSetting(const std::string& key, const std::string& defaultValue = "") const;
    
    /**
     * @brief Set a string setting value
     * 
     * @param key The setting key
     * @param value The setting value
     */
    void setStringSetting(const std::string& key, const std::string& value);
    
    /**
     * @brief Get an integer setting value
     * 
     * @param key The setting key
     * @param defaultValue The default value
     * @return int The setting value
     */
    int getIntSetting(const std::string& key, int defaultValue = 0) const;
    
    /**
     * @brief Set an integer setting value
     * 
     * @param key The setting key
     * @param value The setting value
     */
    void setIntSetting(const std::string& key, int value);
    
    /**
     * @brief Get a boolean setting value
     * 
     * @param key The setting key
     * @param defaultValue The default value
     * @return bool The setting value
     */
    bool getBoolSetting(const std::string& key, bool defaultValue = false) const;
    
    /**
     * @brief Set a boolean setting value
     * 
     * @param key The setting key
     * @param value The setting value
     */
    void setBoolSetting(const std::string& key, bool value);
    
private:
    /**
     * @brief Get the settings file path
     * 
     * @return std::string The settings file path
     */
    std::string getSettingsFilePath() const;
    
    // Member variables
    mutable std::mutex mutex_;
    std::map<std::string, std::string> settings_;
};

} // namespace core
} // namespace dm

#endif // SETTINGS_H