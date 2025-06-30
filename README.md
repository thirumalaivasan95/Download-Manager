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

## Installation & Build Instructions

### Prerequisites
- C++17 compatible compiler
- CMake 3.14+
- Qt 5.12+ (Core, Gui, Widgets, Network, Concurrent)
- libcurl 7.58+
- OpenSSL 1.1.0+
- jsoncpp

#### Linux (Recommended)
```bash
sudo apt-get install build-essential cmake libcurl4-openssl-dev libssl-dev qtbase5-dev libjsoncpp-dev
git clone https://github.com/yourusername/download-manager.git
cd download-manager
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```
- For Arch/Manjaro: use `pacman -S cmake qt5-base curl openssl jsoncpp`.
- For Fedora: use `dnf install @development-tools cmake qt5-qtbase-devel libcurl-devel openssl-devel jsoncpp-devel`.

#### Windows
- Install Qt, CMake, and dependencies
- Use CMake GUI or command line:
```bash
git clone https://github.com/yourusername/download-manager.git
cd download-manager
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64 ..
cmake --build . --config Debug
```

#### macOS
```bash
brew install cmake qt curl openssl jsoncpp
git clone https://github.com/yourusername/download-manager.git
cd download-manager
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$(brew --prefix qt) ..
make -j$(sysctl -n hw.ncpu)
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

We welcome contributions from everyone! Please follow these guidelines:

- **Code of Conduct:** All contributors must adhere to our Code of Conduct. Report unacceptable behavior to the maintainers.
- **Reporting Bugs:** Search the [issue tracker](https://github.com/yourusername/download-manager/issues) before submitting. Include clear steps to reproduce, expected/actual behavior, environment details (especially OS), and logs/screenshots if possible.
- **Suggesting Enhancements:** Open a GitHub issue with a clear title and detailed description. Explain the motivation and, if possible, provide mockups or examples.
- **Pull Requests:**
  1. Fork the repository
  2. Create a feature branch (`git checkout -b feature/your-feature`)
  3. Make your changes following the coding standards
  4. Add or update tests as needed
  5. Update documentation if applicable
  6. Commit with clear messages
  7. Push to your fork and open a Pull Request against `main`
- **Coding Standards:**
  - Google C++ Style Guide (4-space indentation)
  - Classes: PascalCase; Methods: camelCase; Variables: snake_case (member: snake_case_); Constants: kConstantName or CONSTANT_NAME
  - One class per file, keep files under 500 lines if possible
  - Use Doxygen for all public APIs
  - Keep comments up-to-date and clear
- **Development Environment:**
  - See prerequisites and build instructions above
- **Testing:**
  - Write unit tests for all new features (see `tests/` directory)
  - Use Google Test or Catch2
  - Run tests with `ctest -V` in the build directory
  - Aim for at least 80% code coverage
  - Linux users: please test on multiple distributions if possible and report any distro-specific issues.
- **Commit Messages:**
  - Use present tense and imperative mood
  - First line: summary (≤50 chars), then blank line, then details
  - Reference issues/PRs as needed
- **Code Review:**
  - All PRs require review by a maintainer
  - Automated checks (CI, tests, style) must pass
  - Address all review comments before merging

For full details, see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Community & Support

- Join our [Discord](https://discord.gg/example) for discussion
- Subscribe to the [mailing list](mailto:example@example.com)
- Follow [@ExampleProject](https://twitter.com/example)

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