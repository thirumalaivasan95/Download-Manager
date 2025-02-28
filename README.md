# Internet Download Manager

A comprehensive, feature-rich download manager with multi-segment downloading, batch processing, website crawling, media handling, and extensive protocol support.

## Features

### Core Functionality
- **Multi-segment downloading** - Download files in parallel segments for maximum speed
- **Resume capability** - Continue interrupted downloads from where they left off
- **Queue management** - Prioritize and schedule your downloads
- **Bandwidth control** - Limit download speeds to avoid network congestion

### Advanced Features
- **Protocol support** - HTTP, HTTPS, FTP, SFTP, and more via plugin system
- **Batch downloads** - Process multiple URLs at once from various sources
- **Website crawling** - Automatically discover and download files from websites
- **Download scheduling** - Schedule downloads for specific times
- **Media processing** - Generate previews, extract metadata, and convert formats
- **Network awareness** - Adapt to network conditions with intelligent profiles

### Extensibility
- **Plugin system** - Extend functionality with third-party plugins
- **Protocol handlers** - Add support for additional download protocols
- **Command-line interface** - Automate operations with scripting support
- **Statistics and reporting** - Track download patterns and performance

### User Experience
- **Modern GUI** - Clean, intuitive interface with Qt
- **Progress visualization** - Detailed progress tracking with animated indicators
- **Tray integration** - Run in the background with system tray support
- **Notifications** - Get notified about completed downloads

## Architecture

The project follows a modular architecture with clear separation of concerns:

- **Core** - Essential download management functionality
- **UI** - User interface components
- **Utils** - Utility classes and helper functions
- **Plugins** - Extensibility framework
- **CLI** - Command-line interface

### Key Components

- `DownloadManager` - Central coordinator for all download operations
- `DownloadTask` - Represents a single download
- `SegmentDownloader` - Handles individual download segments
- `ProtocolHandler` - Abstract interface for different download protocols
- `WebsiteCrawler` - Discovers and downloads files from websites
- `MediaHandler` - Specialized handling for different media types
- `BatchDownloader` - Manages multiple downloads from various sources
- `DownloadScheduler` - Schedules downloads for specific times
- `NetworkMonitor` - Adjusts download behavior based on network conditions
- `PluginSystem` - Manages third-party extensions
- `StatisticsManager` - Collects and analyzes download statistics

## Installation

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14 or later
- Qt 5.12 or later
- libcurl 7.58 or later
- OpenSSL 1.1.0 or later

### Building from Source

#### Linux

```bash
# Install dependencies
sudo apt-get install build-essential cmake libcurl4-openssl-dev libssl-dev qtbase5-dev

# Clone the repository
git clone https://github.com/yourusername/download-manager.git
cd download-manager

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Install (optional)
sudo make install
```

#### Windows

```bash
# Clone the repository
git clone https://github.com/yourusername/download-manager.git
cd download-manager

# Create build directory
mkdir build
cd build

# Configure and build (adjust paths as needed)
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
cmake --build . --config Release

# Install (optional)
cmake --install .
```

#### macOS

```bash
# Install dependencies with Homebrew
brew install cmake qt curl openssl

# Clone the repository
git clone https://github.com/yourusername/download-manager.git
cd download-manager

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
make -j$(sysctl -n hw.ncpu)

# Install (optional)
make install
```

## Usage

### Graphical User Interface

1. Launch the application
2. Click "Add Download" to add a new download
3. Enter the URL and destination folder
4. Configure optional settings
5. Click "OK" to start the download

### Command Line Interface

```bash
# Add a download
downloadmanager add https://example.com/file.zip

# List downloads
downloadmanager list

# Start a download
downloadmanager start <id>

# Pause a download
downloadmanager pause <id>

# Resume a download
downloadmanager resume <id>

# Cancel a download
downloadmanager cancel <id>

# Remove a download
downloadmanager remove <id>

# Batch download from a file
downloadmanager batch urls.txt
```

### Website Crawling

```bash
# Crawl a website and download all PDF files
downloadmanager crawl https://example.com --file-types=pdf --max-depth=3
```

### Scheduling

```bash
# Schedule a download for 3:00 AM
downloadmanager schedule add https://example.com/file.zip --time="03:00"
```

## Configuration

Configuration is stored in the application settings directory:

- **Linux**: `~/.config/downloadmanager/`
- **Windows**: `%APPDATA%\DownloadManager\`
- **macOS**: `~/Library/Application Support/DownloadManager/`

Main configuration files:

- `settings.ini` - General application settings
- `downloads.json` - Download queue and history
- `plugins.json` - Plugin configuration
- `statistics.db` - Download statistics database

## Extending the Application

### Creating Plugins

Plugins must implement the `Plugin` interface defined in `include/plugin/PluginSystem.h`:

```cpp
class MyPlugin : public dm::plugin::Plugin {
public:
    dm::plugin::PluginInfo getPluginInfo() const override {
        dm::plugin::PluginInfo info;
        info.id = "com.example.myplugin";
        info.name = "My Plugin";
        info.version = "1.0.0";
        info.author = "Your Name";
        info.description = "Example plugin";
        info.type = dm::plugin::PluginType::UTILITY;
        return info;
    }
    
    bool initialize(dm::core::DownloadManager& downloadManager) override {
        // Initialization code
        return true;
    }
    
    void shutdown() override {
        // Cleanup code
    }
    
    // Implement other interface methods...
};

// Plugin factory
extern "C" {
    PLUGIN_EXPORT dm::plugin::PluginFactory* createPluginFactory() {
        return new MyPluginFactory();
    }
}
```

### Adding Protocol Handlers

Protocol handlers must implement the `ProtocolHandler` interface defined in `include/core/ProtocolHandler.h`:

```cpp
class MyProtocolHandler : public dm::core::ProtocolHandler {
public:
    std::string getProtocolName() const override {
        return "MyProtocol";
    }
    
    std::vector<std::string> getProtocolSchemes() const override {
        return {"myp", "myps"};
    }
    
    bool supportsCapability(dm::core::ProtocolCapability capability) const override {
        // Report capabilities
    }
    
    // Implement other interface methods...
};
```

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please follow the coding style guidelines in the [CONTRIBUTING.md](CONTRIBUTING.md) file.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Credits

This project uses the following third-party libraries:

- [Qt](https://www.qt.io/) - Cross-platform GUI framework
- [libcurl](https://curl.se/libcurl/) - Client-side URL transfer library
- [OpenSSL](https://www.openssl.org/) - Cryptography library

## Roadmap

- Additional protocol handlers (BitTorrent, SFTP)
- Mobile application
- Cloud synchronization
- Integration with browser extensions
- Machine learning for download optimization