#ifndef PLUGIN_SYSTEM_H
#define PLUGIN_SYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

namespace dm {
namespace plugin {

// Forward declarations
namespace core {
    class DownloadManager;
}

/**
 * @brief Plugin type enumeration
 */
enum class PluginType {
    PROTOCOL_HANDLER,     // Protocol handler plugin
    UI_EXTENSION,         // UI extension plugin
    MEDIA_PROCESSOR,      // Media processing plugin
    DOWNLOAD_FILTER,      // Download filtering plugin
    POST_PROCESSOR,       // Post-processing plugin
    SCHEDULER_EXTENSION,  // Scheduler extension plugin
    INTEGRATION,          // System integration plugin
    UTILITY,              // Utility plugin
    OTHER                 // Other plugin type
};

/**
 * @brief Plugin info structure
 */
struct PluginInfo {
    std::string id;                // Unique plugin identifier
    std::string name;              // Plugin name
    std::string version;           // Plugin version
    std::string author;            // Plugin author
    std::string description;       // Plugin description
    std::string website;           // Plugin website
    std::string license;           // Plugin license
    PluginType type;               // Plugin type
    std::vector<std::string> tags; // Plugin tags
    std::string iconPath;          // Path to plugin icon
    std::vector<std::string> dependencies; // Plugin dependencies
    std::string minAppVersion;     // Minimum app version required
    bool enabled = true;           // Whether the plugin is enabled
};

/**
 * @brief Plugin interface
 * 
 * Base class for all plugins
 */
class Plugin {
public:
    /**
     * @brief Construct a new Plugin
     */
    Plugin() = default;
    
    /**
     * @brief Destroy the Plugin
     */
    virtual ~Plugin() = default;
    
    /**
     * @brief Get plugin information
     * 
     * @return PluginInfo The plugin information
     */
    virtual PluginInfo getPluginInfo() const = 0;
    
    /**
     * @brief Initialize the plugin
     * 
     * @param downloadManager Reference to download manager
     * @return true if successful, false otherwise
     */
    virtual bool initialize(core::DownloadManager& downloadManager) = 0;
    
    /**
     * @brief Shutdown the plugin
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if the plugin is initialized
     * 
     * @return true if initialized, false otherwise
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * @brief Execute a plugin command
     * 
     * @param command The command to execute
     * @param args Command arguments
     * @param result Output parameter for command result
     * @return true if command executed successfully, false otherwise
     */
    virtual bool executeCommand(const std::string& command, 
                              const std::vector<std::string>& args,
                              std::string& result) = 0;
    
    /**
     * @brief Get available plugin commands
     * 
     * @return std::vector<std::string> The available commands
     */
    virtual std::vector<std::string> getCommands() const = 0;
    
    /**
     * @brief Get command help
     * 
     * @param command The command
     * @return std::string The help text
     */
    virtual std::string getCommandHelp(const std::string& command) const = 0;
    
    /**
     * @brief Get plugin configuration
     * 
     * @return std::map<std::string, std::string> The configuration
     */
    virtual std::map<std::string, std::string> getConfiguration() const = 0;
    
    /**
     * @brief Set plugin configuration
     * 
     * @param config The configuration
     * @return true if successful, false otherwise
     */
    virtual bool setConfiguration(const std::map<std::string, std::string>& config) = 0;
    
    /**
     * @brief Get plugin UI files
     * 
     * @return std::map<std::string, std::string> Map of file name to file path
     */
    virtual std::map<std::string, std::string> getUiFiles() const = 0;
};

/**
 * @brief Plugin factory interface
 * 
 * Factory class for creating plugin instances
 */
class PluginFactory {
public:
    /**
     * @brief Construct a new PluginFactory
     */
    PluginFactory() = default;
    
    /**
     * @brief Destroy the PluginFactory
     */
    virtual ~PluginFactory() = default;
    
    /**
     * @brief Create a plugin instance
     * 
     * @return std::shared_ptr<Plugin> The created plugin instance
     */
    virtual std::shared_ptr<Plugin> createPlugin() = 0;
    
    /**
     * @brief Get plugin information
     * 
     * @return PluginInfo The plugin information
     */
    virtual PluginInfo getPluginInfo() const = 0;
};

/**
 * @brief Plugin callback function type
 */
using PluginCallback = std::function<void(std::shared_ptr<Plugin> plugin, const std::string& event)>;

/**
 * @brief Plugin manager class
 * 
 * Manages plugin loading, initialization, and lifecycle
 */
class PluginManager {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return PluginManager& The singleton instance
     */
    static PluginManager& getInstance();
    
    /**
     * @brief Initialize the plugin manager
     * 
     * @param downloadManager Reference to download manager
     * @return true if successful, false otherwise
     */
    bool initialize(core::DownloadManager& downloadManager);
    
    /**
     * @brief Shutdown the plugin manager
     */
    void shutdown();
    
    /**
     * @brief Load a plugin from file
     * 
     * @param filePath Path to the plugin file
     * @return std::shared_ptr<Plugin> The loaded plugin, or nullptr if failed
     */
    std::shared_ptr<Plugin> loadPlugin(const std::string& filePath);
    
    /**
     * @brief Load plugins from directory
     * 
     * @param dirPath Path to the plugins directory
     * @return std::vector<std::shared_ptr<Plugin>> The loaded plugins
     */
    std::vector<std::shared_ptr<Plugin>> loadPluginsFromDirectory(const std::string& dirPath);
    
    /**
     * @brief Register a plugin
     * 
     * @param plugin The plugin to register
     * @return true if registered successfully, false otherwise
     */
    bool registerPlugin(std::shared_ptr<Plugin> plugin);
    
    /**
     * @brief Unregister a plugin
     * 
     * @param pluginId The plugin ID
     * @return true if unregistered successfully, false otherwise
     */
    bool unregisterPlugin(const std::string& pluginId);
    
    /**
     * @brief Get a plugin by ID
     * 
     * @param pluginId The plugin ID
     * @return std::shared_ptr<Plugin> The plugin, or nullptr if not found
     */
    std::shared_ptr<Plugin> getPlugin(const std::string& pluginId);
    
    /**
     * @brief Get all plugins
     * 
     * @return std::vector<std::shared_ptr<Plugin>> All registered plugins
     */
    std::vector<std::shared_ptr<Plugin>> getAllPlugins() const;
    
    /**
     * @brief Get plugins by type
     * 
     * @param type The plugin type
     * @return std::vector<std::shared_ptr<Plugin>> Plugins of the specified type
     */
    std::vector<std::shared_ptr<Plugin>> getPluginsByType(PluginType type) const;
    
    /**
     * @brief Enable a plugin
     * 
     * @param pluginId The plugin ID
     * @return true if enabled successfully, false otherwise
     */
    bool enablePlugin(const std::string& pluginId);
    
    /**
     * @brief Disable a plugin
     * 
     * @param pluginId The plugin ID
     * @return true if disabled successfully, false otherwise
     */
    bool disablePlugin(const std::string& pluginId);
    
    /**
     * @brief Check if a plugin is enabled
     * 
     * @param pluginId The plugin ID
     * @return true if enabled, false otherwise
     */
    bool isPluginEnabled(const std::string& pluginId) const;
    
    /**
     * @brief Set plugin callback
     * 
     * @param callback The callback function
     */
    void setPluginCallback(PluginCallback callback);
    
    /**
     * @brief Execute a command on all plugins
     * 
     * @param command The command to execute
     * @param args Command arguments
     * @return std::map<std::string, std::string> Map of plugin ID to command result
     */
    std::map<std::string, std::string> executeCommandOnAllPlugins(
        const std::string& command, 
        const std::vector<std::string>& args);
    
    /**
     * @brief Get plugin directory
     * 
     * @return std::string The plugin directory
     */
    std::string getPluginDirectory() const;
    
    /**
     * @brief Set plugin directory
     * 
     * @param directory The plugin directory
     */
    void setPluginDirectory(const std::string& directory);
    
    /**
     * @brief Save plugin configurations
     * 
     * @return true if successful, false otherwise
     */
    bool savePluginConfigurations();
    
    /**
     * @brief Load plugin configurations
     * 
     * @return true if successful, false otherwise
     */
    bool loadPluginConfigurations();
    
private:
    /**
     * @brief Construct a new PluginManager
     */
    PluginManager() = default;
    
    /**
     * @brief Destroy the PluginManager
     */
    ~PluginManager() = default;
    
    // Prevent copying
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    /**
     * @brief Resolve plugin dependencies
     * 
     * @param plugin The plugin to check
     * @return true if dependencies are satisfied, false otherwise
     */
    bool resolveDependencies(std::shared_ptr<Plugin> plugin);
    
    /**
     * @brief Get configuration file path
     * 
     * @return std::string The configuration file path
     */
    std::string getConfigurationFilePath() const;
    
    // Member variables
    std::map<std::string, std::shared_ptr<Plugin>> plugins_;
    core::DownloadManager* downloadManager_ = nullptr;
    std::string pluginDirectory_;
    PluginCallback pluginCallback_ = nullptr;
    mutable std::mutex mutex_;
    bool initialized_ = false;
};

} // namespace plugin
} // namespace dm

#endif // PLUGIN_SYSTEM_H