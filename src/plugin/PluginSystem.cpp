#include "include/plugin/PluginSystem.h"
#include "include/utils/Logger.h"
#include "include/utils/FileUtils.h"
#include "core/DownloadManager.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <random>
#include <iomanip>
#include <json/json.h>  // Using jsoncpp for configuration

#ifdef _WIN32
#include <windows.h>
#define PLUGIN_EXT ".dll"
#define LOAD_LIBRARY(path) LoadLibrary(path)
#define GET_PROC_ADDRESS(handle, name) GetProcAddress((HMODULE)handle, name)
#define FREE_LIBRARY(handle) FreeLibrary((HMODULE)handle)
#define HANDLE_TYPE HMODULE
#else
#include <dlfcn.h>
#define PLUGIN_EXT ".so"
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
#define FREE_LIBRARY(handle) dlclose(handle)
#define HANDLE_TYPE void*
#endif

namespace dm {
namespace plugin {

// Plugin factory creation function type
using CreateFactoryFunc = PluginFactory* (*)();

// Structure to hold plugin library information
struct PluginLibrary {
    HANDLE_TYPE handle = nullptr;
    std::string path;
    std::shared_ptr<PluginFactory> factory;
    std::shared_ptr<Plugin> plugin;
};

// Private implementation class for PluginManager
class PluginManagerImpl {
public:
    std::map<std::string, PluginLibrary> pluginLibraries;
    std::map<std::string, std::shared_ptr<Plugin>> plugins;
    core::DownloadManager* downloadManager = nullptr;
    std::string pluginDirectory;
    PluginCallback pluginCallback = nullptr;
    mutable std::mutex mutex;
    bool initialized = false;
};

// Static instance of PluginManager
PluginManager& PluginManager::getInstance() {
    static PluginManager instance;
    return instance;
}

bool PluginManager::initialize(core::DownloadManager& downloadManager) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        utils::Logger::warning("Plugin manager already initialized");
        return true;
    }
    
    utils::Logger::info("Initializing plugin manager");
    
    // Store download manager reference
    downloadManager_ = &downloadManager;
    
    // Set default plugin directory if not set
    if (pluginDirectory_.empty()) {
        pluginDirectory_ = utils::FileUtils::getAppDataDirectory() + "/plugins";
    }
    
    // Create plugin directory if it doesn't exist
    if (!utils::FileUtils::createDirectory(pluginDirectory_)) {
        utils::Logger::error("Failed to create plugin directory: " + pluginDirectory_);
        return false;
    }
    
    utils::Logger::info("Plugin directory: " + pluginDirectory_);
    
    // Load plugin configurations
    if (!loadPluginConfigurations()) {
        utils::Logger::warning("Failed to load plugin configurations");
        // Continue anyway, not critical
    }
    
    initialized_ = true;
    return true;
}

void PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }
    
    utils::Logger::info("Shutting down plugin manager");
    
    // Save plugin configurations
    savePluginConfigurations();
    
    // Shutdown and unregister all plugins
    for (auto it = plugins_.begin(); it != plugins_.end(); /* no increment */) {
        auto plugin = it->second;
        std::string pluginId = it->first;
        
        // Shutdown plugin
        try {
            plugin->shutdown();
        } catch (const std::exception& e) {
            utils::Logger::error("Error shutting down plugin " + pluginId + ": " + e.what());
        }
        
        // Erase from map
        it = plugins_.erase(it);
    }
    
    initialized_ = false;
    downloadManager_ = nullptr;
}

std::shared_ptr<Plugin> PluginManager::loadPlugin(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        utils::Logger::error("Plugin manager not initialized");
        return nullptr;
    }
    
    // Check if file exists
    if (!utils::FileUtils::fileExists(filePath)) {
        utils::Logger::error("Plugin file not found: " + filePath);
        return nullptr;
    }
    
    // Check file extension
    std::string ext = utils::FileUtils::getExtension(filePath);
    if (ext != PLUGIN_EXT + 1) {  // Skip the dot
        utils::Logger::error("Invalid plugin file extension: " + ext);
        return nullptr;
    }
    
    utils::Logger::info("Loading plugin: " + filePath);
    
    try {
        // Load the library
        HANDLE_TYPE handle = LOAD_LIBRARY(filePath.c_str());
        if (!handle) {
            utils::Logger::error("Failed to load plugin library: " + filePath);
            return nullptr;
        }
        
        // Get factory creation function
        CreateFactoryFunc createFactory = (CreateFactoryFunc)GET_PROC_ADDRESS(handle, "createPluginFactory");
        if (!createFactory) {
            utils::Logger::error("Failed to find createPluginFactory function in plugin: " + filePath);
            FREE_LIBRARY(handle);
            return nullptr;
        }
        
        // Create factory
        PluginFactory* factory = createFactory();
        if (!factory) {
            utils::Logger::error("Failed to create plugin factory: " + filePath);
            FREE_LIBRARY(handle);
            return nullptr;
        }
        
        // Get plugin info
        PluginInfo info = factory->getPluginInfo();
        
        // Check if plugin with same ID already exists
        if (plugins_.find(info.id) != plugins_.end()) {
            utils::Logger::warning("Plugin with ID " + info.id + " already exists");
            delete factory;
            FREE_LIBRARY(handle);
            return plugins_[info.id];
        }
        
        // Create plugin
        std::shared_ptr<Plugin> plugin = factory->createPlugin();
        if (!plugin) {
            utils::Logger::error("Failed to create plugin instance: " + filePath);
            delete factory;
            FREE_LIBRARY(handle);
            return nullptr;
        }
        
        // Create plugin library info
        PluginLibrary libraryInfo;
        libraryInfo.handle = handle;
        libraryInfo.path = filePath;
        libraryInfo.factory.reset(factory);
        libraryInfo.plugin = plugin;
        
        // Check dependencies
        if (!resolveDependencies(plugin)) {
            utils::Logger::error("Failed to resolve plugin dependencies: " + info.id);
            return nullptr;
        }
        
        // Initialize plugin
        if (!plugin->initialize(*downloadManager_)) {
            utils::Logger::error("Failed to initialize plugin: " + info.id);
            return nullptr;
        }
        
        // Register plugin
        plugins_[info.id] = plugin;
        
        // Get plugin info again after initialization (it might have changed)
        info = plugin->getPluginInfo();
        
        // Log success
        utils::Logger::info("Plugin loaded successfully: " + info.name + " (" + info.id + ") version " + info.version);
        
        // Notify callback
        if (pluginCallback_) {
            pluginCallback_(plugin, "loaded");
        }
        
        return plugin;
    } catch (const std::exception& e) {
        utils::Logger::error("Exception loading plugin: " + std::string(e.what()));
        return nullptr;
    }
}

std::vector<std::shared_ptr<Plugin>> PluginManager::loadPluginsFromDirectory(const std::string& dirPath) {
    std::vector<std::shared_ptr<Plugin>> loadedPlugins;
    
    // Get all plugin files in directory
    std::vector<std::string> files = utils::FileUtils::findFiles(dirPath, PLUGIN_EXT + 1);
    
    // Load each plugin
    for (const auto& file : files) {
        std::shared_ptr<Plugin> plugin = loadPlugin(file);
        if (plugin) {
            loadedPlugins.push_back(plugin);
        }
    }
    
    return loadedPlugins;
}

bool PluginManager::registerPlugin(std::shared_ptr<Plugin> plugin) {
    if (!plugin) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get plugin info
    PluginInfo info = plugin->getPluginInfo();
    
    // Check if plugin with same ID already exists
    if (plugins_.find(info.id) != plugins_.end()) {
        utils::Logger::warning("Plugin with ID " + info.id + " already exists");
        return false;
    }
    
    // Check dependencies
    if (!resolveDependencies(plugin)) {
        utils::Logger::error("Failed to resolve plugin dependencies: " + info.id);
        return false;
    }
    
    // Initialize plugin if not already initialized
    if (!plugin->isInitialized()) {
        if (!plugin->initialize(*downloadManager_)) {
            utils::Logger::error("Failed to initialize plugin: " + info.id);
            return false;
        }
    }
    
    // Register plugin
    plugins_[info.id] = plugin;
    
    // Log success
    utils::Logger::info("Plugin registered: " + info.name + " (" + info.id + ") version " + info.version);
    
    // Notify callback
    if (pluginCallback_) {
        pluginCallback_(plugin, "registered");
    }
    
    return true;
}

bool PluginManager::unregisterPlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find plugin
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        utils::Logger::warning("Plugin not found: " + pluginId);
        return false;
    }
    
    auto plugin = it->second;
    
    // Shutdown plugin
    try {
        plugin->shutdown();
    } catch (const std::exception& e) {
        utils::Logger::error("Error shutting down plugin " + pluginId + ": " + e.what());
        // Continue unregistering anyway
    }
    
    // Notify callback
    if (pluginCallback_) {
        pluginCallback_(plugin, "unregistered");
    }
    
    // Remove from map
    plugins_.erase(it);
    
    utils::Logger::info("Plugin unregistered: " + pluginId);
    
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
    for (const auto& pair : plugins_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<Plugin>> PluginManager::getPluginsByType(PluginType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Plugin>> result;
    for (const auto& pair : plugins_) {
        PluginInfo info = pair.second->getPluginInfo();
        if (info.type == type) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool PluginManager::enablePlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        utils::Logger::warning("Plugin not found: " + pluginId);
        return false;
    }
    
    auto plugin = it->second;
    PluginInfo info = plugin->getPluginInfo();
    
    // Already enabled
    if (info.enabled) {
        return true;
    }
    
    // Update enabled state
    info.enabled = true;
    
    // Set new plugin info (this is a bit of a hack since we can't modify the PluginInfo directly)
    // In a real implementation, we would have a setEnabled method on the Plugin interface
    std::map<std::string, std::string> config = plugin->getConfiguration();
    config["enabled"] = "true";
    plugin->setConfiguration(config);
    
    // Notify callback
    if (pluginCallback_) {
        pluginCallback_(plugin, "enabled");
    }
    
    utils::Logger::info("Plugin enabled: " + pluginId);
    
    return true;
}

bool PluginManager::disablePlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        utils::Logger::warning("Plugin not found: " + pluginId);
        return false;
    }
    
    auto plugin = it->second;
    PluginInfo info = plugin->getPluginInfo();
    
    // Already disabled
    if (!info.enabled) {
        return true;
    }
    
    // Update enabled state
    info.enabled = false;
    
    // Set new plugin info (this is a bit of a hack since we can't modify the PluginInfo directly)
    // In a real implementation, we would have a setEnabled method on the Plugin interface
    std::map<std::string, std::string> config = plugin->getConfiguration();
    config["enabled"] = "false";
    plugin->setConfiguration(config);
    
    // Notify callback
    if (pluginCallback_) {
        pluginCallback_(plugin, "disabled");
    }
    
    utils::Logger::info("Plugin disabled: " + pluginId);
    
    return true;
}

bool PluginManager::isPluginEnabled(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return false;
    }
    
    PluginInfo info = it->second->getPluginInfo();
    return info.enabled;
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
        std::string pluginId = pair.first;
        std::shared_ptr<Plugin> plugin = pair.second;
        
        // Skip disabled plugins
        PluginInfo info = plugin->getPluginInfo();
        if (!info.enabled) {
            continue;
        }
        
        // Check if plugin supports this command
        std::vector<std::string> commands = plugin->getCommands();
        if (std::find(commands.begin(), commands.end(), command) == commands.end()) {
            continue;
        }
        
        // Execute command
        std::string result;
        if (plugin->executeCommand(command, args, result)) {
            results[pluginId] = result;
        } else {
            results[pluginId] = "ERROR: Command execution failed";
        }
    }
    
    return results;
}

std::string PluginManager::getPluginDirectory() const {
    return pluginDirectory_;
}

void PluginManager::setPluginDirectory(const std::string& directory) {
    pluginDirectory_ = directory;
}

bool PluginManager::savePluginConfigurations() {
    try {
        // Create JSON root
        Json::Value root;
        
        // Add plugins
        Json::Value pluginsArray(Json::arrayValue);
        
        for (const auto& pair : plugins_) {
            std::string pluginId = pair.first;
            std::shared_ptr<Plugin> plugin = pair.second;
            
            // Get plugin info
            PluginInfo info = plugin->getPluginInfo();
            
            // Get plugin configuration
            std::map<std::string, std::string> config = plugin->getConfiguration();
            
            // Create plugin JSON
            Json::Value pluginJson;
            pluginJson["id"] = info.id;
            pluginJson["enabled"] = info.enabled;
            
            // Add configuration
            Json::Value configJson;
            for (const auto& configPair : config) {
                configJson[configPair.first] = configPair.second;
            }
            pluginJson["config"] = configJson;
            
            // Add to array
            pluginsArray.append(pluginJson);
        }
        
        root["plugins"] = pluginsArray;
        
        // Write to file
        std::string configPath = getConfigurationFilePath();
        
        // Create directory if it doesn't exist
        std::string configDir = utils::FileUtils::getDirectory(configPath);
        if (!utils::FileUtils::createDirectory(configDir)) {
            utils::Logger::error("Failed to create config directory: " + configDir);
            return false;
        }
        
        // Write to file
        std::ofstream file(configPath);
        if (!file.is_open()) {
            utils::Logger::error("Failed to open config file for writing: " + configPath);
            return false;
        }
        
        Json::StyledWriter writer;
        file << writer.write(root);
        
        file.close();
        
        utils::Logger::info("Plugin configurations saved to " + configPath);
        
        return true;
    } catch (const std::exception& e) {
        utils::Logger::error("Exception saving plugin configurations: " + std::string(e.what()));
        return false;
    }
}

bool PluginManager::loadPluginConfigurations() {
    try {
        std::string configPath = getConfigurationFilePath();
        
        // Check if file exists
        if (!utils::FileUtils::fileExists(configPath)) {
            utils::Logger::info("Plugin configuration file not found: " + configPath);
            return false;
        }
        
        // Read file
        std::string json = utils::FileUtils::readTextFile(configPath);
        
        // Parse JSON
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(json, root)) {
            utils::Logger::error("Failed to parse plugin configuration file: " + configPath);
            return false;
        }
        
        // Process plugins
        Json::Value pluginsArray = root["plugins"];
        if (!pluginsArray.isArray()) {
            utils::Logger::error("Invalid plugin configuration format");
            return false;
        }
        
        for (unsigned int i = 0; i < pluginsArray.size(); i++) {
            Json::Value pluginJson = pluginsArray[i];
            
            std::string pluginId = pluginJson["id"].asString();
            bool enabled = pluginJson["enabled"].asBool();
            
            // Find plugin
            auto it = plugins_.find(pluginId);
            if (it == plugins_.end()) {
                // Plugin not loaded yet, skip
                continue;
            }
            
            std::shared_ptr<Plugin> plugin = it->second;
            
            // Enable/disable plugin
            if (enabled) {
                enablePlugin(pluginId);
            } else {
                disablePlugin(pluginId);
            }
            
            // Set configuration
            Json::Value configJson = pluginJson["config"];
            
            std::map<std::string, std::string> config;
            for (const auto& memberName : configJson.getMemberNames()) {
                config[memberName] = configJson[memberName].asString();
            }
            
            plugin->setConfiguration(config);
        }
        
        utils::Logger::info("Plugin configurations loaded from " + configPath);
        
        return true;
    } catch (const std::exception& e) {
        utils::Logger::error("Exception loading plugin configurations: " + std::string(e.what()));
        return false;
    }
}

bool PluginManager::resolveDependencies(std::shared_ptr<Plugin> plugin) {
    if (!plugin) {
        return false;
    }
    
    PluginInfo info = plugin->getPluginInfo();
    
    for (const auto& dependency : info.dependencies) {
        // Find dependency plugin
        auto it = plugins_.find(dependency);
        if (it == plugins_.end()) {
            utils::Logger::error("Plugin dependency not found: " + dependency);
            return false;
        }
        
        // Check if dependency is enabled
        PluginInfo depInfo = it->second->getPluginInfo();
        if (!depInfo.enabled) {
            utils::Logger::error("Plugin dependency is disabled: " + dependency);
            return false;
        }
    }
    
    return true;
}

std::string PluginManager::getConfigurationFilePath() const {
    return utils::FileUtils::getAppDataDirectory() + "/plugin_config.json";
}

} // namespace plugin
} // namespace dm