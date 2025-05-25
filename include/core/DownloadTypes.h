#pragma once
#include <string>
#include <cstdint>

namespace dm {
namespace core {

struct DownloadFileInfo {
    std::string filename;
    int64_t size = 0;
    std::string contentType;
    bool resumable = false;
};

struct DownloadOptions {
    std::string url;
    std::string destination;
    int segments = 1;
    int64_t maxSpeed = 0;
    std::string username;
    std::string password;
    int64_t scheduledTime = 0;
};

} // namespace core
} // namespace dm
