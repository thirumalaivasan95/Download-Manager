#include "include/plugin/PluginSystem.h"
#include "core/DownloadManager.h"
#include "include/utils/Logger.h"
#include "include/utils/FileUtils.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <json/json.h> // Using jsoncpp for configuration
#include <dlfcn.h>     // For dynamic library loading on UNIX
#include <filesystem>

namespace dm {
namespace plugin {

// Function pointer type for plugin factory creation
using CreatePluginFactoryFunc = PluginFactory* (*)();

// Singleton instance
PluginManager& PluginManager::getInstance() {
    static PluginManager instance;
    return instance;
}

bool PluginManager::initialize(core::DownloadManager& downloadManager) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Already initialized
    if (initialized_) {
        return true;
    }
    
    // Store download manager reference
    downloadManager_ = &downloadManager;
    
    // Set default plugin directory if not set
    if (pluginDirectory_.empty()) {
        pluginDirectory_ = dm::utils::FileUtils::getAppDataDirectory() + "/plugins";
    }
    
    // Create plugin directory if it doesn't exist
    if (!dm::utils::FileUtils::createDirectory(pluginDirectory_)) {
        dm::utils::Logger::error("Failed to create plugin directory: " + pluginDirectory_);
        return false;
    }
    
    // Load plugin configurations
    if (!loadPluginConfigurations()) {
        dm::utils::Logger::warning("Failed to load plugin configurations, using defaults");
        // Continue anyway, not critical
    }
    
    // Set initialized flag
    initialized_ = true;
    
    dm::utils::Logger::info("Plugin manager initialized");
    return true;
}

void PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }
    
    // Save plugin configurations
    savePluginConfigurations();
    
    // Shutdown all plugins
    for (auto& pair : plugins_) {
        // Get plugin
        auto& plugin = pair.second;
        
        // Skip disabled plugins or already shutdown plugins
        if (!plugin->isInitialized()) {
            continue;
        }
        
        try {
            // Log plugin shutdown
            dm::utils::Logger::info("Shutting down plugin: " + plugin->getPluginInfo().name);
            
            // Shutdown plugin
            plugin->shutdown();
        } catch (const std::exception& e) {
            dm::utils::Logger::error("Error shutting down plugin " + plugin->getPluginInfo().name + 
                                   ": " + e.what());
        } catch (...) {
            dm::utils::Logger::error("Unknown error shutting down plugin " + plugin->getPluginInfo().name);
        }
    }
    
    // Clear plugins
    plugins_.clear();
    
    // Reset initialized flag
    initialized_ = false;
    
    dm::utils::Logger::info("Plugin manager shutdown");
}

std::shared_ptr<Plugin> PluginManager::loadPlugin(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        dm::utils::Logger::error("Plugin manager not initialized");
        return nullptr;
    }
    
    // Check if file exists
    if (!dm::utils::FileUtils::fileExists(filePath)) {
        dm::utils::Logger::error("Plugin file not found: " + filePath);
        return nullptr;
    }
    
    try {
        // Load dynamic library
        void* handle = dlopen(filePath.c_str(), RTLD_LAZY);
        if (!handle) {
            dm::utils::Logger::error("Failed to load plugin library: " + std::string(dlerror()));
            return nullptr;
        }
        
        // Reset errors
        dlerror();
        
        // Get plugin factory creation function
        CreatePluginFactoryFunc createFactoryFunc = 
            reinterpret_cast<CreatePluginFactoryFunc>(dlsym(handle, "createPluginFactory"));
        
        // Check for errors
        const char* dlsymError = dlerror();
        if (dlsymError) {
            dm::utils::Logger::error("Failed to get plugin factory: " + std::string(dlsymError));
            dlclose(handle);
            return nullptr;
        }
        
        // Create plugin factory
        PluginFactory* factory = createFactoryFunc();
        if (!factory) {
            dm::utils::Logger::error("Failed to create plugin factory");
            dlclose(handle);
            return nullptr;
        }
        
        // Get plugin info
        PluginInfo info = factory->getPluginInfo();
        
        // Check if plugin with same ID already exists
        if (plugins_.find(info.id) != plugins_.end()) {
            dm::utils::Logger::error("Plugin with ID " + info.id + " already exists");
            dlclose(handle);
            delete factory;
            return nullptr;
        }
        
        // Create plugin instance
        std::shared_ptr<Plugin> plugin = factory->createPlugin();
        if (!plugin) {
            dm::utils::Logger::error("Failed to create plugin instance");
            dlclose(handle);
            delete factory;
            return nullptr;
        }
        
        // Clean up factory
        delete factory;
        
        // Check minimum app version if specified
        if (!info.minAppVersion.empty()) {
            // TODO: Implement version checking
            // For now, we'll just log a warning
            dm::utils::Logger::warning("Plugin " + info.name + " requires app version " + 
                                     info.minAppVersion + " or higher");
        }
        
        // Resolve dependencies
        if (!resolveDependencies(plugin)) {
            dm::utils::Logger::error("Failed to resolve dependencies for plugin " + info.name);
            return nullptr;
        }
        
        // Initialize plugin
        if (!plugin->initialize(*downloadManager_)) {
            dm::utils::Logger::error("Failed to initialize plugin " + info.name);
            return nullptr;
        }
        
        // Register plugin
        plugins_[info.id] = plugin;
        
        // Log plugin loaded
        dm::utils::Logger::info("Loaded plugin: " + info.name + " (ID: " + info.id + ")");
        
        // Call plugin callback if set
        if (pluginCallback_) {
            pluginCallback_(plugin, "loaded");
        }
        
        return plugin;
    } catch (const std::exception& e) {
        dm::utils::Logger::error("Exception loading plugin: " + std::string(e.what()));
        return nullptr;
    } catch (...) {
        dm::utils::Logger::error("Unknown exception loading plugin");
        return nullptr;
    }
}

std::vector<std::shared_ptr<Plugin>> PluginManager::loadPluginsFromDirectory(const std::string& dirPath) {
    std::vector<std::shared_ptr<Plugin>> loadedPlugins;
    
    if (!initialized_) {
        dm::utils::Logger::error("Plugin manager not initialized");
        return loadedPlugins;
    }
    
    // Check if directory exists
    if (!dm::utils::FileUtils::fileExists(dirPath)) {
        dm::utils::Logger::error("Plugin directory not found: " + dirPath);
        return loadedPlugins;
    }
    
    try {
        // Get all shared library files in directory
        std::vector<std::string> files;
        
        // Use std::filesystem to iterate through directory
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                
                // Check if file is a shared library
                if (extension == ".so" || extension == ".dll" || extension == ".dylib") {
                    files.push_back(entry.path().string());
                }
            }
        }
        
        // Sort files to ensure deterministic loading order
        std::sort(files.begin(), files.end());
        
        // Load each plugin
        for (const auto& file : files) {
            auto plugin = loadPlugin(file);
            if (plugin) {
                loadedPlugins.push_back(plugin);
            }
        }
        
        dm::utils::Logger::info("Loaded " + std::to_string(loadedPlugins.size()) + 
                              " plugins from directory: " + dirPath);
        
        return loadedPlugins;
    } catch (const std::exception& e) {
        dm::utils::Logger::error("Exception loading plugins from directory: " + std::string(e.what()));
        return loadedPlugins;
    } catch (...) {
        dm::utils::Logger::error("Unknown exception loading plugins from directory");
        return loadedPlugins;
    }
}

bool PluginManager::registerPlugin(std::shared_ptr<Plugin> plugin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        dm::utils::Logger::error("Plugin manager not initialized");
        return false;
    }
    
    if (!plugin) {
        dm::utils::Logger::error("Null plugin provided");
        return false;
    }
    
    // Get plugin info
    PluginInfo info = plugin->getPluginInfo();
    
    // Check if plugin with same ID already exists
    if (plugins_.find(info.id) != plugins_.end()) {
        dm::utils::Logger::error("Plugin with ID " + info.id + " already exists");
        return false;
    }
    
    // Resolve dependencies
    if (!resolveDependencies(plugin)) {
        dm::utils::Logger::error("Failed to resolve dependencies for plugin " + info.name);
        return false;
    }
    
    // Initialize plugin if not already initialized
    if (!plugin->isInitialized() && !plugin->initialize(*downloadManager_)) {
        dm::utils::Logger::error("Failed to initialize plugin " + info.name);
        return false;
    }
    
    // Register plugin
    plugins_[info.id] = plugin;
    
    // Log plugin registered
    dm::utils::Logger::info("Registered plugin: " + info.name + " (ID: " + info.id + ")");
    
    // Call plugin callback if set
    if (pluginCallback_) {
        pluginCallback_(plugin, "registered");
    }
    
    return true;
}

bool PluginManager::unregisterPlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        dm::utils::Logger::error("Plugin manager not initialized");
        return false;
    }
    
    // Find plugin
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        dm::utils::Logger::error("Plugin not found: " + pluginId);
        return false;
    }
    
    // Get plugin
    auto plugin = it->second;
    
    // Check for dependencies
    for (const auto& pair : plugins_) {
        if (pair.first == pluginId) {
            continue;
        }
        
        // Get plugin info
        PluginInfo info = pair.second->getPluginInfo();
        
        // Check dependencies
        for (const auto& dep : info.dependencies) {
            if (dep == pluginId) {
                dm::utils::Logger::error("Cannot unregister plugin " + pluginId + 
                                       ", it is a dependency of " + info.id);
                return false;
            }
        }
    }
    
    // Log plugin unregistering
    dm::utils::Logger::info("Unregistering plugin: " + plugin->getPluginInfo().name);
    
    // Shutdown plugin if initialized
    if (plugin->isInitialized()) {
        try {
            plugin->shutdown();
        } catch (const std::exception& e) {
            dm::utils::Logger::error("Error shutting down plugin " + plugin->getPluginInfo().name + 
                                   ": " + e.what());
        } catch (...) {
            dm::utils::Logger::error("Unknown error shutting down plugin " + plugin->getPluginInfo().name);
        }
    }
    
    // Call plugin callback if set
    if (pluginCallback_) {
        pluginCallback_(plugin, "unregistered");
    }
    
    // Remove plugin
    plugins_.erase(it);
    
    return true;
}

std::shared_ptr<Plugin> PluginManager::getPlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return nullptr;
    }
    
    return it->second;
}

std::vector<std::shared_ptr<Plugin>> PluginManager::getAllPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Plugin>> result;
    result.reserve(plugins_.size());
    
    for (const auto& pair : plugins_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<Plugin>> PluginManager::getPluginsByType(PluginType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Plugin>> result;
    
    for (const auto& pair : plugins_) {
        if (pair.second->getPluginInfo().type == type) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool PluginManager::enablePlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        dm::utils::Logger::error("Plugin not found: " + pluginId);
        return false;
    }
    
    // Get plugin
    auto plugin = it->second;
    
    // Get plugin info
    PluginInfo info = plugin->getPluginInfo();
    
    // Already enabled
    if (info.enabled) {
        return true;
    }
    
    // Resolve dependencies
    if (!resolveDependencies(plugin)) {
        dm::utils::Logger::error("Failed to resolve dependencies for plugin " + info.name);
        return false;
    }
    
    // Initialize plugin if not already initialized
    if (!plugin->isInitialized() && !plugin->initialize(*downloadManager_)) {
        dm::utils::Logger::error("Failed to initialize plugin " + info.name);
        return false;
    }
    
    // Update enabled status
    info.enabled = true;
    
    // Call plugin callback if set
    if (pluginCallback_) {
        pluginCallback_(plugin, "enabled");
    }
    
    // Log plugin enabled
    dm::utils::Logger::info("Enabled plugin: " + info.name);
    
    return true;
}

bool PluginManager::disablePlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        dm::utils::Logger::error("Plugin not found: " + pluginId);
        return false;
    }
    
    // Get plugin
    auto plugin = it->second;
    
    // Get plugin info
    PluginInfo info = plugin->getPluginInfo();
    
    // Already disabled
    if (!info.enabled) {
        return true;
    }
    
    // Check for dependencies
    for (const auto& pair : plugins_) {
        if (pair.first == pluginId) {
            continue;
        }
        
        // Get plugin info
        PluginInfo depInfo = pair.second->getPluginInfo();
        
        // Skip disabled plugins
        if (!depInfo.enabled) {
            continue;
        }
        
        // Check dependencies
        for (const auto& dep : depInfo.dependencies) {
            if (dep == pluginId) {
                dm::utils::Logger::error("Cannot disable plugin " + pluginId + 
                                       ", it is a dependency of enabled plugin " + depInfo.id);
                return false;
            }
        }
    }
    
    // Shutdown plugin if initialized
    if (plugin->isInitialized()) {
        try {
            plugin->shutdown();
        } catch (const std::exception& e) {
            dm::utils::Logger::error("Error shutting down plugin " + info.name + 
                                   ": " + e.what());
        } catch (...) {
            dm::utils::Logger::error("Unknown error shutting down plugin " + info.name);
        }
    }
    
    // Update enabled status
    info.enabled = false;
    
    // Call plugin callback if set
    if (pluginCallback_) {
        pluginCallback_(plugin, "disabled");
    }
    
    // Log plugin disabled
    dm::utils::Logger::info("Disabled plugin: " + info.name);
    
    return true;
}

bool PluginManager::isPluginEnabled(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return false;
    }
    
    return it->second->getPluginInfo().enabled;
}

void PluginManager::setPluginCallback(PluginCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    pluginCallback_ = callback;
}

std::map<std::string, std::string> PluginManager::executeCommandOnAllPlugins(
    const std::string& command, 
    const std::vector<std::string>& args) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, std::string> results;
    
    for (const auto& pair : plugins_) {
        // Get plugin
        auto plugin = pair.second;
        
        // Skip disabled plugins
        if (!plugin->getPluginInfo().enabled) {
            continue;
        }
        
        // Check if plugin supports command
        auto commands = plugin->getCommands();
        if (std::find(commands.begin(), commands.end(), command) == commands.end()) {
            continue;
        }
        
        // Execute command
        std::string result;
        try {
            if (plugin->executeCommand(command, args, result)) {
                results[pair.first] = result;
            }
        } catch (const std::exception& e) {
            dm::utils::Logger::error("Error executing command " + command + " on plugin " + 
                                   plugin->getPluginInfo().name + ": " + e.what());
        } catch (...) {
            dm::utils::Logger::error("Unknown error executing command " + command + " on plugin " + 
                                   plugin->getPluginInfo().name);
        }
    }
    
    return results;
}

std::string PluginManager::getPluginDirectory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pluginDirectory_;
}

void PluginManager::setPluginDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(mutex_);
    pluginDirectory_ = directory;
}

bool PluginManager::savePluginConfigurations() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Create JSON root
        Json::Value root;
        
        // Add plugins array
        Json::Value pluginsArray(Json::arrayValue);
        
        // Add each plugin's configuration
        for (const auto& pair : plugins_) {
            // Get plugin
            auto plugin = pair.second;
            
            // Get plugin info
            PluginInfo info = plugin->getPluginInfo();
            
            // Create plugin JSON object
            Json::Value pluginJson;
            pluginJson["id"] = info.id;
            pluginJson["enabled"] = info.enabled;
            
            // Add configuration
            Json::Value configJson(Json::objectValue);
            auto config = plugin->getConfiguration();
            for (const auto& configPair : config) {
                configJson[configPair.first] = configPair.second;
            }
            pluginJson["config"] = configJson;
            
            // Add to array
            pluginsArray.append(pluginJson);
        }
        
        // Set plugins array in root
        root["plugins"] = pluginsArray;
        
        // Add plugin directory
        root["pluginDirectory"] = pluginDirectory_;
        
        // Write JSON to file
        Json::StyledWriter writer;
        std::string jsonStr = writer.write(root);
        
        std::string configFile = getConfigurationFilePath();
        
        // Create directory if needed
        std::string configDir = dm::utils::FileUtils::getDirectory(configFile);
        if (!dm::utils::FileUtils::createDirectory(configDir)) {
            dm::utils::Logger::error("Failed to create configuration directory: " + configDir);
            return false;
        }
        
        // Write to file
        std::ofstream file(configFile);
        if (!file.is_open()) {
            dm::utils::Logger::error("Failed to open configuration file for writing: " + configFile);
            return false;
        }
        
        file << jsonStr;
        file.close();
        
        dm::utils::Logger::info("Saved plugin configurations to " + configFile);
        return true;
    } catch (const std::exception& e) {
        dm::utils::Logger::error("Exception saving plugin configurations: " + std::string(e.what()));
        return false;
    } catch (...) {
        dm::utils::Logger::error("Unknown exception saving plugin configurations");
        return false;
    }
}

bool PluginManager::loadPluginConfigurations() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string configFile = getConfigurationFilePath();
    
    // Check if file exists
    if (!dm::utils::FileUtils::fileExists(configFile)) {
        dm::utils::Logger::info("Plugin configuration file not found: " + configFile);
        return false;
    }
    
    try {
        // Read file
        std::ifstream file(configFile);
        if (!file.is_open()) {
            dm::utils::Logger::error("Failed to open configuration file: " + configFile);
            return false;
        }
        
        // Parse JSON
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root)) {
            dm::utils::Logger::error("Failed to parse configuration file: " + 
                                   reader.getFormattedErrorMessages());
            return false;
        }
        
        // Get plugin directory
        if (root.isMember("pluginDirectory") && root["pluginDirectory"].isString()) {
            pluginDirectory_ = root["pluginDirectory"].asString();
        }
        
        // Get plugins array
        if (!root.isMember("plugins") || !root["plugins"].isArray()) {
            dm::utils::Logger::error("Invalid configuration file format: missing plugins array");
            return false;
        }
        
        // Process each plugin
        const Json::Value& pluginsArray = root["plugins"];
        for (Json::ArrayIndex i = 0; i < pluginsArray.size(); i++) {
            const Json::Value& pluginJson = pluginsArray[i];
            
            // Get plugin ID
            if (!pluginJson.isMember("id") || !pluginJson["id"].isString()) {
                dm::utils::Logger::warning("Invalid plugin configuration: missing ID");
                continue;
            }
            
            std::string pluginId = pluginJson["id"].asString();
            
            // Find plugin
            auto it = plugins_.find(pluginId);
            if (it == plugins_.end()) {
                // Plugin not loaded yet, skip
                continue;
            }
            
            // Get plugin
            auto plugin = it->second;
            
            // Set enabled status
            if (pluginJson.isMember("enabled") && pluginJson["enabled"].isBool()) {
                bool enabled = pluginJson["enabled"].asBool();
                
                // Get current status
                bool currentlyEnabled = plugin->getPluginInfo().enabled;
                
                // Update if different
                if (enabled != currentlyEnabled) {
                    if (enabled) {
                        enablePlugin(pluginId);
                    } else {
                        disablePlugin(pluginId);
                    }
                }
            }
            
            // Set configuration
            if (pluginJson.isMember("config") && pluginJson["config"].isObject()) {
                const Json::Value& configJson = pluginJson["config"];
                
                // Convert to map
                std::map<std::string, std::string> config;
                for (const auto& key : configJson.getMemberNames()) {
                    if (configJson[key].isString()) {
                        config[key] = configJson[key].asString();
                    } else {
                        // Convert to string
                        Json::FastWriter writer;
                        config[key] = writer.write(configJson[key]);
                    }
                }
                
                // Set configuration
                plugin->setConfiguration(config);
            }
        }
        
        dm::utils::Logger::info("Loaded plugin configurations from " + configFile);
        return true;
    } catch (const std::exception& e) {
        dm::utils::Logger::error("Exception loading plugin configurations: " + std::string(e.what()));
        return false;
    } catch (...) {
        dm::utils::Logger::error("Unknown exception loading plugin configurations");
        return false;
    }
}

bool PluginManager::resolveDependencies(std::shared_ptr<Plugin> plugin) {
    // Get plugin info
    PluginInfo info = plugin->getPluginInfo();
    
    // Check dependencies
    for (const auto& depId : info.dependencies) {
        // Find dependency
        auto it = plugins_.find(depId);
        if (it == plugins_.end()) {
            dm::utils::Logger::error("Missing dependency " + depId + " for plugin " + info.id);
            return false;
        }
        
        // Check if dependency is enabled
        if (!it->second->getPluginInfo().enabled) {
            dm::utils::Logger::error("Dependency " + depId + " for plugin " + info.id + 
                                   " is disabled");
            return false;
        }
    }
    
    return true;
}

std::string PluginManager::getConfigurationFilePath() const {
    return dm::utils::FileUtils::getAppDataDirectory() + "/plugins.json";
}

} // namespace plugin
} // namespace dm