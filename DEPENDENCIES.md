# Dependencies

This document lists the dependencies required to build and run the Download Manager project.

## Build System

*   **CMake** (version 3.14 or later): The build system used for this project. You can download it from [cmake.org](https://cmake.org/download/).

## C++ Compiler

A C++17 compliant compiler is required. The following are recommended:

*   **Visual Studio** (on Windows): The Community edition is available for free.
*   **GCC** (on Linux): A popular open-source compiler.
*   **Clang** (on macOS or Linux): Another excellent open-source compiler.

## Libraries

These libraries are required for the project to function correctly. On Windows, it is highly recommended to use a package manager like [vcpkg](https://vcpkg.io/) or [Chocolatey](https://chocolatey.org/) to install these dependencies. On Linux and macOS, you can use your system's package manager (e.g., apt, yum, brew).

*   **Qt5** (Core, Gui, Widgets, Network, Concurrent): A cross-platform application framework used for the user interface. You can download it from [qt.io](https://www.qt.io/download).
*   **cURL**: A library for transferring data with URLs, used for downloading files. You can download it from [curl.se](https://curl.se/download.html).
*   **OpenSSL**: A library for cryptography, used for secure connections. You can download it from [openssl.org](https://www.openssl.org/source/).
*   **jsoncpp**: A library for working with JSON, used for configuration files. You can find it on [GitHub](https://github.com/open-source-parsers/jsoncpp).
