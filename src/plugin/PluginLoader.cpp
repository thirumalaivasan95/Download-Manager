#include "../../include/plugin/PluginLoader.h"
#include "../../include/utils/Logger.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace DownloadManager {
namespace Plugin {

DynamicPluginLoader::DynamicPluginLoader() {
}

DynamicPluginLoader::~DynamicPluginLoader() {
    // Clean up any remaining handles
    for (auto& pair : m_libraryHandles) {
        closeLibrary(pair.second);
    }
    m_libraryHandles.clear();
}

std::shared_ptr<IPlugin> DynamicPluginLoader::loadPlugin(const std::string& pluginPath) {
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Loading dynamic plugin from: " + pluginPath);
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(pluginPath)) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Plugin file does not exist: " + pluginPath);
            return nullptr;
        }
        
        // Load the dynamic library
        LibraryHandle handle = loadLibrary(pluginPath);
        if (!handle) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to load library: " + pluginPath);
            return nullptr;
        }
        
        // Get the create function
        CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(
            getSymbol(handle, "createPlugin"));
        
        if (!createFunc) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to find createPlugin function in library: " + pluginPath);
            closeLibrary(handle);
            return nullptr;
        }
        
        // Get the destroy function
        DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(
            getSymbol(handle, "destroyPlugin"));
        
        if (!destroyFunc) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to find destroyPlugin function in library: " + pluginPath);
            closeLibrary(handle);
            return nullptr;
        }
        
        // Create plugin instance
        IPlugin* rawPlugin = createFunc();
        if (!rawPlugin) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to create plugin instance from library: " + pluginPath);
            closeLibrary(handle);
            return nullptr;
        }
        
        // Set plugin path
        rawPlugin->setPath(pluginPath);
        
        // Create shared_ptr with custom deleter to call destroy function
        std::shared_ptr<IPlugin> plugin(rawPlugin, [destroyFunc](IPlugin* p) {
            if (p) {
                destroyFunc(p);
            }
        });
        
        // Store library handle
        m_libraryHandles[plugin->getId()] = handle;
        
        return plugin;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception loading plugin from " + pluginPath + ": " + e.what());
        return nullptr;
    }
}

void DynamicPluginLoader::unloadPlugin(std::shared_ptr<IPlugin> plugin) {
    if (!plugin) {
        return;
    }
    
    std::string pluginId = plugin->getId();
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Unloading dynamic plugin: " + pluginId);
    
    // Find library handle
    auto it = m_libraryHandles.find(pluginId);
    if (it == m_libraryHandles.end()) {
        return;
    }
    
    // Reset the shared_ptr to release the plugin instance
    plugin.reset();
    
    // Close the library
    closeLibrary(it->second);
    
    // Remove from map
    m_libraryHandles.erase(it);
}

#ifdef _WIN32
// Windows implementation
DynamicPluginLoader::LibraryHandle DynamicPluginLoader::loadLibrary(const std::string& path) {
    return LoadLibraryA(path.c_str());
}

void* DynamicPluginLoader::getSymbol(LibraryHandle handle, const std::string& name) {
    return reinterpret_cast<void*>(GetProcAddress(handle, name.c_str()));
}

void DynamicPluginLoader::closeLibrary(LibraryHandle handle) {
    if (handle) {
        FreeLibrary(handle);
    }
}
#else
// Unix/Linux implementation
DynamicPluginLoader::LibraryHandle DynamicPluginLoader::loadLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

void* DynamicPluginLoader::getSymbol(LibraryHandle handle, const std::string& name) {
    return dlsym(handle, name.c_str());
}

void DynamicPluginLoader::closeLibrary(LibraryHandle handle) {
    if (handle) {
        dlclose(handle);
    }
}
#endif

// ScriptPluginLoader implementation

ScriptPluginLoader::ScriptPluginLoader() {
}

ScriptPluginLoader::~ScriptPluginLoader() {
}

std::shared_ptr<IPlugin> ScriptPluginLoader::loadPlugin(const std::string& pluginPath) {
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Loading script plugin from: " + pluginPath);
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(pluginPath)) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Plugin file does not exist: " + pluginPath);
            return nullptr;
        }
        
        // Parse plugin metadata from the script file
        auto metadata = parsePluginMetadata(pluginPath);
        
        if (metadata.id.empty() || metadata.name.empty() || metadata.version.empty()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Invalid plugin metadata in script: " + pluginPath);
            return nullptr;
        }
        
        // Create script plugin instance
        auto plugin = std::make_shared<ScriptPlugin>(
            metadata.id, metadata.name, metadata.version, metadata.description,
            metadata.author, getPluginTypeFromString(metadata.type));
        
        // Set plugin path
        plugin->setPath(pluginPath);
        
        // Load script content
        std::string scriptContent = loadScriptContent(pluginPath);
        plugin->setScriptContent(scriptContent);
        
        return plugin;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception loading plugin from " + pluginPath + ": " + e.what());
        return nullptr;
    }
}

void ScriptPluginLoader::unloadPlugin(std::shared_ptr<IPlugin> plugin) {
    // Script plugins don't need special unloading
}

ScriptPluginLoader::PluginMetadata ScriptPluginLoader::parsePluginMetadata(const std::string& scriptPath) {
    PluginMetadata metadata;
    
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open script file: " + scriptPath);
    }
    
    std::string line;
    bool inMetadataBlock = false;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // Look for metadata block
        if (line == "/* PLUGIN_METADATA") {
            inMetadataBlock = true;
            continue;
        }
        
        if (line == "*/") {
            inMetadataBlock = false;
            continue;
        }
        
        if (inMetadataBlock) {
            // Parse metadata key-value pairs
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                // Trim key and value
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                
                if (key == "id") {
                    metadata.id = value;
                } 
                else if (key == "name") {
                    metadata.name = value;
                } 
                else if (key == "version") {
                    metadata.version = value;
                } 
                else if (key == "description") {
                    metadata.description = value;
                } 
                else if (key == "author") {
                    metadata.author = value;
                } 
                else if (key == "type") {
                    metadata.type = value;
                }
            }
        }
    }
    
    return metadata;
}

std::string ScriptPluginLoader::loadScriptContent(const std::string& scriptPath) {
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open script file: " + scriptPath);
    }
    
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

PluginType ScriptPluginLoader::getPluginTypeFromString(const std::string& typeStr) {
    if (typeStr == "protocol") {
        return PluginType::PROTOCOL;
    } 
    else if (typeStr == "extension") {
        return PluginType::EXTENSION;
    } 
    else if (typeStr == "converter") {
        return PluginType::CONVERTER;
    } 
    else if (typeStr == "extractor") {
        return PluginType::EXTRACTOR;
    } 
    else if (typeStr == "organizer") {
        return PluginType::ORGANIZER;
    } 
    else if (typeStr == "notification") {
        return PluginType::NOTIFICATION;
    }
    
    return PluginType::UNKNOWN;
}

// ScriptPlugin implementation

ScriptPlugin::ScriptPlugin(
    const std::string& id,
    const std::string& name,
    const std::string& version,
    const std::string& description,
    const std::string& author,
    PluginType type)
    : m_id(id),
      m_name(name),
      m_version(version),
      m_description(description),
      m_author(author),
      m_type(type),
      m_enabled(false)
{
}

ScriptPlugin::~ScriptPlugin() {
}

std::string ScriptPlugin::getId() const {
    return m_id;
}

std::string ScriptPlugin::getName() const {
    return m_name;
}

std::string ScriptPlugin::getVersion() const {
    return m_version;
}

std::string ScriptPlugin::getDescription() const {
    return m_description;
}

std::string ScriptPlugin::getAuthor() const {
    return m_author;
}

PluginType ScriptPlugin::getType() const {
    return m_type;
}

bool ScriptPlugin::isEnabled() const {
    return m_enabled;
}

void ScriptPlugin::setEnabled(bool enabled) {
    m_enabled = enabled;
}

std::string ScriptPlugin::getPath() const {
    return m_path;
}

void ScriptPlugin::setPath(const std::string& path) {
    m_path = path;
}

bool ScriptPlugin::initialize() {
    m_enabled = true;
    return true;
}

void ScriptPlugin::shutdown() {
    m_enabled = false;
}

std::string ScriptPlugin::getScriptContent() const {
    return m_scriptContent;
}

void ScriptPlugin::setScriptContent(const std::string& content) {
    m_scriptContent = content;
}

} // namespace Plugin
} // namespace DownloadManager