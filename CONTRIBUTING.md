# Contributing to Internet Download Manager

Thank you for your interest in contributing to Internet Download Manager! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

This project adheres to a Code of Conduct that all contributors are expected to follow. By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

## How Can I Contribute?

### Reporting Bugs

Before submitting a bug report:

1. Check the [issue tracker](https://github.com/yourusername/download-manager/issues) to see if the issue has already been reported
2. Update your software to the latest version to see if the issue persists

When submitting a bug report, include:

- A clear and descriptive title
- Detailed steps to reproduce the issue
- Expected and actual behavior
- Screenshots if applicable
- Environment details (OS, compiler version, etc.)
- Any relevant log files or console output

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion:

1. Use a clear and descriptive title
2. Provide a detailed description of the proposed functionality
3. Explain why this enhancement would be useful
4. If possible, include mockups or examples

### Pull Requests

1. Fork the repository
2. Create a branch from `main` (`git checkout -b feature/amazing-feature`)
3. Make your changes following the [coding standards](#coding-standards)
4. Write or update tests as needed
5. Update documentation as needed
6. Commit your changes with clear commit messages
7. Push to your fork (`git push origin feature/amazing-feature`)
8. Open a Pull Request against the `main` branch

## Coding Standards

### C++ Style Guide

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with these exceptions:
  - Use 4-space indentation instead of 2 spaces
  - Use `camelCase` for method names
  - Use `snake_case` for variable names and member variables with trailing underscore (`member_variable_`)

### File Organization

- Keep header files minimal; implementation details go in source files
- One class per file, unless the classes are very closely related
- Keep files under 500 lines if possible
- Use forward declarations when possible to minimize dependencies

### Naming Conventions

- Class names: `PascalCase` (e.g., `DownloadManager`)
- Functions/methods: `camelCase` (e.g., `addDownload()`)
- Variables: `snake_case` (e.g., `download_count`)
- Member variables: `snake_case_` with trailing underscore (e.g., `download_manager_`)
- Constants: `kConstantName` or `CONSTANT_NAME` (for legacy code)
- Enum members: `ENUM_VALUE`
- File names: Match the primary class name (`DownloadManager.h`, `DownloadManager.cpp`)

### Documentation

- Document all public API functions with Doxygen comments
- Keep comments up-to-date with code changes
- Use simple, clear language
- Include examples for non-trivial functionality

Example:
```cpp
/**
 * @brief Add a new download task
 * 
 * @param url The URL to download
 * @param destinationPath The path to save the file (optional)
 * @param filename The filename to use (optional)
 * @param start Whether to start the download immediately
 * @return std::shared_ptr<DownloadTask> The created task or nullptr on failure
 */
std::shared_ptr<DownloadTask> addDownload(const std::string& url, 
                                        const std::string& destinationPath = "",
                                        const std::string& filename = "",
                                        bool start = true);
```

### Testing

- Write unit tests for all new functionality
- Tests should be in the `tests/` directory
- Name test files after the class being tested (`DownloadManagerTest.cpp`)
- Use [Google Test](https://github.com/google/googletest) framework
- Aim for at least 80% code coverage

### Commit Messages

- Use the present tense ("Add feature" not "Added feature")
- Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
- First line is a summary (50 characters or less)
- Followed by a blank line and detailed description if needed
- Reference issues and pull requests liberally

Example:
```
Add multi-segment downloading capability

- Implement SegmentDownloader class
- Add segment management to DownloadTask
- Create progress tracking for individual segments
- Update UI to show segment progress
- Fixes #42
```

## Development Environment Setup

### Prerequisites

- C++17 compatible compiler
- CMake 3.14 or later
- Qt 5.12 or later
- libcurl 7.58 or later
- OpenSSL 1.1.0 or later

### Build Instructions

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
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
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
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64 ..
cmake --build . --config Debug
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
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$(brew --prefix qt) ..
make -j$(sysctl -n hw.ncpu)
```

### Running Tests

```bash
# In the build directory
ctest -V
```

## Code Review Process

All submissions, including those by project members, require review:

1. A project maintainer will review the pull request
2. Automated checks must pass (CI build, tests, code style)
3. Any issues must be addressed before merging
4. After approval, a maintainer will merge the pull request

## Community

- Join our [Discord server](https://discord.gg/example) for real-time discussion
- Subscribe to the [mailing list](mailto:example@example.com) for project announcements
- Follow [@ExampleProject](https://twitter.com/example) on Twitter

Thank you for contributing to Internet Download Manager!