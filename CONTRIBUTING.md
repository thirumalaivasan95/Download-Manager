# Contributing to Internet Download Manager

Thank you for your interest in contributing! This document provides guidelines for effective and professional collaboration. Linux, Windows, and macOS are all first-class supported platforms.

---

## Code of Conduct
All contributors must adhere to our Code of Conduct. Please report unacceptable behavior to the maintainers.

---

## How to Contribute

### Reporting Bugs
- Search the [issue tracker](https://github.com/yourusername/download-manager/issues) before submitting.
- Include clear steps to reproduce, expected/actual behavior, environment details (especially OS: Linux, Windows, or macOS), and logs/screenshots if possible.

### Suggesting Enhancements
- Open a GitHub issue with a clear title and detailed description.
- Explain the motivation and, if possible, provide mockups or examples.

### Pull Requests
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Make your changes following the coding standards
4. Add or update tests as needed
5. Update documentation if applicable
6. Commit with clear messages
7. Push to your fork and open a Pull Request against `main`

---

## Coding Standards

- **Style:** Google C++ Style Guide (with 4-space indentation)
- **Naming:**
  - Classes: `PascalCase`
  - Methods: `camelCase`
  - Variables: `snake_case` (member variables: `snake_case_`)
  - Constants: `kConstantName` or `CONSTANT_NAME`
- **Files:** One class per file, keep files under 500 lines if possible
- **Documentation:** Use Doxygen for all public APIs
- **Comments:** Keep up-to-date and clear

Example:
```cpp
/**
 * @brief Add a new download task
 * @param url The URL to download
 * @param destinationPath The path to save the file
 * @param filename The filename to use
 * @param start Whether to start the download immediately
 * @return std::shared_ptr<DownloadTask> The created task or nullptr on failure
 */
std::shared_ptr<DownloadTask> addDownload(const std::string& url, const std::string& destinationPath = "", const std::string& filename = "", bool start = true);
```

---

## Development Environment Setup

### Prerequisites
- C++17 compatible compiler
- CMake 3.14+
- Qt 5.12+
- libcurl 7.58+
- OpenSSL 1.1.0+
- jsoncpp

### Build Instructions
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

## Testing
- Write unit tests for all new features (see `tests/` directory)
- Use Google Test or Catch2
- Run tests with `ctest -V` in the build directory
- Aim for at least 80% code coverage
- Linux users: please test on multiple distributions if possible and report any distro-specific issues.

---

## Commit Messages
- Use present tense and imperative mood
- First line: summary (≤50 chars), then blank line, then details
- Reference issues/PRs as needed

Example:
```
Add multi-segment downloading capability

- Implement SegmentDownloader
- Add segment management to DownloadTask
- Fixes #42
```

---

## Code Review Process
- All PRs require review by a maintainer
- Automated checks (CI, tests, style) must pass
- Address all review comments before merging

---

## Community & Support
- Join our [Discord](https://discord.gg/example) for discussion
- Subscribe to the [mailing list](mailto:example@example.com)
- Follow [@ExampleProject](https://twitter.com/example)

---

Thank you for helping make Internet Download Manager better! Linux support and improvements are especially welcome.