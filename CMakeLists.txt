cmake_minimum_required(VERSION 3.14)
project(DownloadManager VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enable Qt's AUTOMOC for meta-object code generation
set(CMAKE_AUTOMOC ON)

# Find required packages
find_package(Qt5 COMPONENTS Core Gui Widgets Network Concurrent REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Find jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

if (JSONCPP_FOUND)
    include_directories(${JSONCPP_INCLUDE_DIRS})
    link_directories(${JSONCPP_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "jsoncpp not found")
endif()

# If JSONCPP_INCLUDE_DIRS is empty, add the default path for Ubuntu
if (NOT JSONCPP_INCLUDE_DIRS)
    include_directories(/usr/include/jsoncpp)
endif()

# Set files to be compiled
set(SOURCES
    main.cpp
    src/core/DownloadManager.cpp
    src/core/DownloadTask.cpp
    src/core/DownloadQueue.cpp
    src/core/HttpClient.cpp
    src/core/FileManager.cpp
    src/core/SegmentDownloader.cpp
    src/core/Throttler.cpp
    src/core/Settings.cpp
    src/ui/MainWindow.cpp
    src/ui/DownloadItemWidget.cpp
    src/ui/AddDownloadDialog.cpp
    src/ui/SettingsDialog.cpp
    src/ui/ProgressBar.cpp
    src/ui/SpeedLabel.cpp
    src/utils/UrlParser.cpp
    src/utils/HashCalculator.cpp
    src/utils/Logger.cpp
    src/utils/FileUtils.cpp
)

# Set headers
set(HEADERS
    include/core/DownloadManager.h
    include/core/DownloadTask.h
    include/core/DownloadQueue.h
    include/core/HttpClient.h
    include/core/FileManager.h
    include/core/SegmentDownloader.h
    include/core/Throttler.h
    include/core/Settings.h
    include/ui/MainWindow.h
    include/ui/DownloadItemWidget.h
    include/ui/AddDownloadDialog.h
    include/ui/SettingsDialog.h
    include/ui/ProgressBar.h
    include/ui/SpeedLabel.h
    include/utils/UrlParser.h
    include/utils/HashCalculator.h
    include/utils/Logger.h
    include/utils/FileUtils.h
)

# Resources file
set(RESOURCES resources/resources.qrc)
qt5_add_resources(QT_RESOURCES ${RESOURCES})

# Set up target
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${QT_RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Concurrent
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    Threads::Threads
    ${JSONCPP_LIBRARIES}
)

# Check for GCC version and link stdc++fs if needed for filesystem support
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        target_link_libraries(${PROJECT_NAME} stdc++fs)
    endif()
endif()

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)