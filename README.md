# Internet Download Manager

A comprehensive, professional, and extensible download manager for modern platforms (Linux, Windows, macOS). Supports multi-segment downloads, batch processing, website crawling, media handling, and a plugin system for protocol and feature extensions.

---

## Features

- **Multi-segment downloading** for maximum speed
- **Resume capability** for interrupted downloads
- **Queue management** and scheduling
- **Bandwidth control**
- **Protocol support:** HTTP, HTTPS, FTP, SFTP, BitTorrent (via plugins)
- **Batch downloads** and website crawling
- **Media processing** (previews, metadata extraction)
- **Modern Qt GUI** and command-line interface
- **Plugin system** for extensibility
- **Statistics and reporting**

---

## Project Structure

- `core/` - Download management, protocol handlers, scheduling, statistics
- `ui/` - Qt-based graphical interface
- `cli/` - Command-line interface
- `plugin/` - Plugin system and sample plugins
- `utils/` - Utility classes (logging, file, network, etc.)
- `resources/` - Icons, translations, and other assets

---

## Installation

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14 or later
- Qt 5.12+ (Core, Gui, Widgets, Network, Concurrent)
- libcurl 7.58+
- OpenSSL 1.1.0+
- jsoncpp

#### Linux (Recommended)
```bash
sudo apt-get install build-essential cmake libcurl4-openssl-dev libssl-dev qtbase5-dev libjsoncpp-dev
# Clone the repository
git clone https://github.com/yourusername/download-manager.git
cd download-manager
mkdir build && cd build
cmake ..
make -j$(nproc)
# Optional: sudo make install
```
- The application is fully supported and tested on major Linux distributions.
- For Arch/Manjaro: use `pacman -S cmake qt5-base curl openssl jsoncpp`.
- For Fedora: use `dnf install @development-tools cmake qt5-qtbase-devel libcurl-devel openssl-devel jsoncpp-devel`.

#### Windows
- Install Qt, CMake, and dependencies (see documentation)
- Use CMake GUI or command line:
```bash
git clone https://github.com/yourusername/download-manager.git
cd download-manager
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
cmake --build . --config Release
# Optional: cmake --install .
```

#### macOS
```bash
brew install cmake qt curl openssl jsoncpp
git clone https://github.com/yourusername/download-manager.git
cd download-manager
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
make -j$(sysctl -n hw.ncpu)
# Optional: sudo make install
```

---

## Usage

### Graphical User Interface
- Launch the application (on Linux: `./DownloadManager` from the build directory or use your desktop environment's launcher)
- Click "Add Download" to add a new download
- Enter the URL and destination
- Configure options and start

### Command Line Interface (Linux, Windows, macOS)
```bash
downloadmanager add https://example.com/file.zip
downloadmanager list
downloadmanager start <id>
downloadmanager pause <id>
downloadmanager resume <id>
downloadmanager cancel <id>
downloadmanager remove <id>
downloadmanager batch urls.txt
```

### Website Crawling
```bash
downloadmanager crawl https://example.com --file-types=pdf --max-depth=3
```

### Scheduling
```bash
downloadmanager schedule add https://example.com/file.zip --time="03:00"
```

---

## Configuration

- **Linux:** `~/.config/downloadmanager/`
- **Windows:** `%APPDATA%\DownloadManager\`
- **macOS:** `~/Library/Application Support/DownloadManager/`

Files:
- `settings.ini` - Application settings
- `downloads.json` - Download queue/history
- `plugins.json` - Plugin configuration
- `statistics.db` - Download statistics

---

## Extending the Application

### Plugins
Implement the `Plugin` interface in `include/plugin/PluginSystem.h`.

### Protocol Handlers
Implement the `ProtocolHandler` interface in `include/core/ProtocolHandler.h`.

---

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines, coding standards, and development setup. Linux-specific improvements and testing are especially appreciated.

---

## License

MIT License. See [LICENSE](LICENSE).

---

## Credits

- [Qt](https://www.qt.io/) - GUI framework
- [libcurl](https://curl.se/libcurl/) - URL transfer
- [OpenSSL](https://www.openssl.org/) - Cryptography
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp) - JSON parsing

---

## Roadmap
- Additional protocol handlers (BitTorrent, SFTP)
- Mobile and cloud sync
- Browser integration
- Machine learning for optimization

---

For implementation details, see [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md).