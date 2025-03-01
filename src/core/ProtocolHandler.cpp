#include "../../include/core/ProtocolHandler.h"
#include "../../include/utils/Logger.h"

namespace DownloadManager {
namespace Core {

ProtocolHandler::ProtocolHandler(const std::string& name) : 
    m_name(name), 
    m_isActive(false) {
}

ProtocolHandler::~ProtocolHandler() {
}

std::string ProtocolHandler::getName() const {
    return m_name;
}

bool ProtocolHandler::isActive() const {
    return m_isActive;
}

void ProtocolHandler::setActive(bool active) {
    m_isActive = active;
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        m_name + " protocol handler " + (active ? "activated" : "deactivated"));
}

bool ProtocolHandler::supportsScheme(const std::string& scheme) const {
    auto supportedSchemes = getSupportedSchemes();
    return std::find(supportedSchemes.begin(), supportedSchemes.end(), scheme) != supportedSchemes.end();
}

} // namespace Core
} // namespace DownloadManager