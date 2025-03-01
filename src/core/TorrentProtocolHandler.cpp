#include "../../include/core/TorrentProtocolHandler.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/FileUtils.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <thread>
#include <cstring>

// Include libtorrent headers
#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/announce_entry.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>

namespace DownloadManager {
namespace Core {

TorrentProtocolHandler::TorrentProtocolHandler() 
    : ProtocolHandler("BitTorrent"), 
      m_session(nullptr),
      m_isRunning(false),
      m_alertThread(nullptr)
{
    // Initialize libtorrent session
    m_session = std::make_unique<libtorrent::session>();
    initSession();
}

TorrentProtocolHandler::~TorrentProtocolHandler() {
    // Stop alert handling thread
    stopAlertHandling();
    
    // Save session state and resume data
    saveSessionState();
    
    // Clean up resources
    m_session.reset();
}

std::vector<std::string> TorrentProtocolHandler::getSupportedSchemes() const {
    return {"magnet", "torrent"};
}

bool TorrentProtocolHandler::canHandleUrl(const std::string& url) const {
    // Check if it's a magnet link
    if (url.substr(0, 8) == "magnet:?") {
        return true;
    }
    
    // Check if it's a local .torrent file
    if (url.substr(0, 7) == "file://") {
        std::string path = url.substr(7);
        std::string ext = Utils::FileUtils::getFileExtension(path);
        if (Utils::StringUtils::toLower(ext) == ".torrent") {
            return true;
        }
    }
    
    // Check if URL ends with .torrent
    std::string ext = Utils::FileUtils::getFileExtension(url);
    if (Utils::StringUtils::toLower(ext) == ".torrent") {
        return true;
    }
    
    return false;
}

std::shared_ptr<DownloadTask> TorrentProtocolHandler::createDownloadTask(const std::string& url, const std::string& destination) {
    // Validate URL
    if (!canHandleUrl(url)) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "URL not supported by BitTorrent handler: " + url);
        return nullptr;
    }
    
    // Create download task
    auto task = std::make_shared<DownloadTask>(url, destination);
    
    // Initialize BitTorrent-specific task parameters
    task->setProtocolType("BitTorrent");
    
    return task;
}

bool TorrentProtocolHandler::getFileInfo(const std::string& url, DownloadFileInfo& fileInfo) {
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Getting torrent file info for: " + url);
        
        // Check if it's a magnet link
        if (url.substr(0, 8) == "magnet:?") {
            // For magnet links, we can't get full file info without downloading metadata
            fileInfo.filename = extractNameFromMagnet(url);
            fileInfo.size = -1; // Unknown size
            fileInfo.resumable = true;
            fileInfo.contentType = "application/x-bittorrent";
            return true;
        }
        
        // Get torrent file path (download if remote)
        std::string torrentPath = getTorrentFilePath(url);
        if (torrentPath.empty()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to get torrent file: " + url);
            return false;
        }
        
        // Parse torrent file
        libtorrent::error_code ec;
        libtorrent::torrent_info torrentInfo(torrentPath, ec);
        
        if (ec) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to parse torrent file: " + ec.message());
            return false;
        }
        
        // Extract file info
        fileInfo.size = torrentInfo.total_size();
        
        // Get name
        fileInfo.filename = torrentInfo.name();
        
        // Check if it's a single file or multi-file torrent
        if (torrentInfo.num_files() == 1) {
            // Single file torrent
            auto fileIterator = torrentInfo.begin_files();
            fileInfo.filename = fileIterator.filename().string();
        } else {
            // Multi-file torrent - use torrent name as folder name
            fileInfo.filename = torrentInfo.name();
        }
        
        fileInfo.contentType = "application/x-bittorrent";
        fileInfo.resumable = true;
        
        return true;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in getFileInfo: " + std::string(e.what()));
        return false;
    }
}

bool TorrentProtocolHandler::download(std::shared_ptr<DownloadTask> task, const ProgressCallback& progressCallback) {
    if (!task) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, "Invalid download task");
        return false;
    }
    
    try {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Starting BitTorrent download for: " + task->getUrl());
        
        // Ensure session is running
        if (!m_isRunning) {
            startAlertHandling();
        }
        
        // Add torrent to session
        libtorrent::error_code ec;
        libtorrent::add_torrent_params params;
        
        // Set save path
        params.save_path = task->getDestination();
        
        // Check if it's a magnet link
        if (task->getUrl().substr(0, 8) == "magnet:?") {
            // Parse magnet link
            libtorrent::parse_magnet_uri(task->getUrl(), params, ec);
            
            if (ec) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to parse magnet link: " + ec.message());
                return false;
            }
        } 
        else {
            // Get torrent file path (download if remote)
            std::string torrentPath = getTorrentFilePath(task->getUrl());
            if (torrentPath.empty()) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to get torrent file: " + task->getUrl());
                return false;
            }
            
            // Load torrent file
            params.ti = std::make_shared<libtorrent::torrent_info>(torrentPath, ec);
            
            if (ec) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to parse torrent file: " + ec.message());
                return false;
            }
        }
        
        // Generate unique hash for the task
        std::string taskHash = generateTaskHash(task);
        
        // Check if we have resume data
        std::string resumeFile = getResumeDataPath(taskHash);
        if (Utils::FileUtils::fileExists(resumeFile)) {
            std::vector<char> resumeData = loadResumeData(resumeFile);
            if (!resumeData.empty()) {
                params = libtorrent::read_resume_data(resumeData, ec);
                if (ec) {
                    Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
                        "Failed to load resume data: " + ec.message());
                }
            }
        }
        
        // Add torrent
        libtorrent::torrent_handle handle = m_session->add_torrent(params, ec);
        
        if (ec) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to add torrent: " + ec.message());
            return false;
        }
        
        // Store handle
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            m_torrents[taskHash] = TorrentInfo{handle, task, progressCallback};
        }
        
        // Update task state
        task->setStatus(DownloadStatus::DOWNLOADING);
        
        // Store task hash in task custom data
        task->setCustomData("torrent_hash", taskHash);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Torrent added successfully: " + taskHash);
        
        return true;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in download: " + std::string(e.what()));
        return false;
    }
}

bool TorrentProtocolHandler::cancelDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    try {
        // Get task hash
        std::string taskHash = task->getCustomData("torrent_hash");
        if (taskHash.empty()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Task has no torrent hash");
            return false;
        }
        
        // Find torrent
        libtorrent::torrent_handle handle;
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            auto it = m_torrents.find(taskHash);
            if (it == m_torrents.end()) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Torrent not found: " + taskHash);
                return false;
            }
            
            handle = it->second.handle;
            
            // Remove from map
            m_torrents.erase(it);
        }
        
        // Remove torrent and delete files
        m_session->remove_torrent(handle, libtorrent::session::delete_files);
        
        // Update task state
        task->setStatus(DownloadStatus::CANCELED);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Torrent removed: " + taskHash);
        
        return true;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in cancelDownload: " + std::string(e.what()));
        return false;
    }
}

bool TorrentProtocolHandler::pauseDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    try {
        // Get task hash
        std::string taskHash = task->getCustomData("torrent_hash");
        if (taskHash.empty()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Task has no torrent hash");
            return false;
        }
        
        // Find torrent
        libtorrent::torrent_handle handle;
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            auto it = m_torrents.find(taskHash);
            if (it == m_torrents.end()) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Torrent not found: " + taskHash);
                return false;
            }
            
            handle = it->second.handle;
        }
        
        // Pause torrent
        handle.pause();
        
        // Update task state
        task->setStatus(DownloadStatus::PAUSED);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Torrent paused: " + taskHash);
        
        return true;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in pauseDownload: " + std::string(e.what()));
        return false;
    }
}

bool TorrentProtocolHandler::resumeDownload(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return false;
    }
    
    try {
        // Get task hash
        std::string taskHash = task->getCustomData("torrent_hash");
        if (taskHash.empty()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Task has no torrent hash");
            return false;
        }
        
        // Find torrent
        libtorrent::torrent_handle handle;
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            auto it = m_torrents.find(taskHash);
            if (it == m_torrents.end()) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Torrent not found: " + taskHash);
                return false;
            }
            
            handle = it->second.handle;
        }
        
        // Resume torrent
        handle.resume();
        
        // Update task state
        task->setStatus(DownloadStatus::DOWNLOADING);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Torrent resumed: " + taskHash);
        
        return true;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in resumeDownload: " + std::string(e.what()));
        return false;
    }
}

void TorrentProtocolHandler::setDownloadLimit(int bytesPerSecond) {
    if (m_session) {
        m_session->set_download_rate_limit(bytesPerSecond);
    }
}

void TorrentProtocolHandler::setUploadLimit(int bytesPerSecond) {
    if (m_session) {
        m_session->set_upload_rate_limit(bytesPerSecond);
    }
}

int TorrentProtocolHandler::getDownloadLimit() const {
    if (m_session) {
        return m_session->download_rate_limit();
    }
    return 0;
}

int TorrentProtocolHandler::getUploadLimit() const {
    if (m_session) {
        return m_session->upload_rate_limit();
    }
    return 0;
}

std::vector<TorrentStatus> TorrentProtocolHandler::getAllTorrentsStatus() const {
    std::vector<TorrentStatus> result;
    
    if (!m_session) {
        return result;
    }
    
    std::vector<libtorrent::torrent_handle> torrents = m_session->get_torrents();
    
    for (const auto& handle : torrents) {
        TorrentStatus status;
        
        libtorrent::torrent_status torrentStatus = handle.status();
        
        status.infoHash = torrentStatus.info_hash.to_string();
        status.name = torrentStatus.name;
        status.state = convertState(torrentStatus.state);
        status.progress = torrentStatus.progress * 100.0f;
        status.downloadRate = torrentStatus.download_rate;
        status.uploadRate = torrentStatus.upload_rate;
        status.totalDownloaded = torrentStatus.total_download;
        status.totalUploaded = torrentStatus.total_upload;
        status.totalSize = torrentStatus.total_wanted;
        status.numPeers = torrentStatus.num_peers;
        status.numSeeds = torrentStatus.num_seeds;
        status.pieces = torrentStatus.num_pieces;
        status.totalPieces = torrentStatus.num_pieces;
        
        result.push_back(status);
    }
    
    return result;
}

TorrentStatus TorrentProtocolHandler::getTorrentStatus(const std::string& taskHash) const {
    TorrentStatus status;
    
    if (!m_session) {
        return status;
    }
    
    // Find torrent
    libtorrent::torrent_handle handle;
    {
        std::lock_guard<std::mutex> lock(m_torrentsMutex);
        auto it = m_torrents.find(taskHash);
        if (it == m_torrents.end()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Torrent not found: " + taskHash);
            return status;
        }
        
        handle = it->second.handle;
    }
    
    if (!handle.is_valid()) {
        return status;
    }
    
    // Get status
    libtorrent::torrent_status torrentStatus = handle.status();
    
    status.infoHash = torrentStatus.info_hash.to_string();
    status.name = torrentStatus.name;
    status.state = convertState(torrentStatus.state);
    status.progress = torrentStatus.progress * 100.0f;
    status.downloadRate = torrentStatus.download_rate;
    status.uploadRate = torrentStatus.upload_rate;
    status.totalDownloaded = torrentStatus.total_download;
    status.totalUploaded = torrentStatus.total_upload;
    status.totalSize = torrentStatus.total_wanted;
    status.numPeers = torrentStatus.num_peers;
    status.numSeeds = torrentStatus.num_seeds;
    status.pieces = torrentStatus.num_pieces;
    status.totalPieces = torrentStatus.num_pieces;
    
    return status;
}

void TorrentProtocolHandler::addTracker(const std::string& taskHash, const std::string& trackerUrl) {
    if (!m_session) {
        return;
    }
    
    // Find torrent
    libtorrent::torrent_handle handle;
    {
        std::lock_guard<std::mutex> lock(m_torrentsMutex);
        auto it = m_torrents.find(taskHash);
        if (it == m_torrents.end()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Torrent not found: " + taskHash);
            return;
        }
        
        handle = it->second.handle;
    }
    
    if (!handle.is_valid()) {
        return;
    }
    
    handle.add_tracker({"url", trackerUrl});
}

void TorrentProtocolHandler::setPriority(const std::string& taskHash, int priority) {
    if (!m_session) {
        return;
    }
    
    // Find torrent
    libtorrent::torrent_handle handle;
    {
        std::lock_guard<std::mutex> lock(m_torrentsMutex);
        auto it = m_torrents.find(taskHash);
        if (it == m_torrents.end()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Torrent not found: " + taskHash);
            return;
        }
        
        handle = it->second.handle;
    }
    
    if (!handle.is_valid()) {
        return;
    }
    
    // Convert priority
    // libtorrent priorities range from 1 (lowest) to 7 (highest)
    int ltPriority = 4; // Normal priority
    
    switch (priority) {
        case 1: // Low
            ltPriority = 2;
            break;
        case 2: // Normal
            ltPriority = 4;
            break;
        case 3: // High
            ltPriority = 6;
            break;
        default:
            ltPriority = 4;
    }
    
    // Set priority
    handle.set_priority(ltPriority);
}

void TorrentProtocolHandler::setFileDownloadPriority(const std::string& taskHash, int fileIndex, int priority) {
    if (!m_session) {
        return;
    }
    
    // Find torrent
    libtorrent::torrent_handle handle;
    {
        std::lock_guard<std::mutex> lock(m_torrentsMutex);
        auto it = m_torrents.find(taskHash);
        if (it == m_torrents.end()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Torrent not found: " + taskHash);
            return;
        }
        
        handle = it->second.handle;
    }
    
    if (!handle.is_valid()) {
        return;
    }
    
    // Convert priority
    // libtorrent priorities range from 0 (don't download) to 7 (highest)
    int ltPriority = 4; // Normal priority
    
    switch (priority) {
        case 0: // Don't download
            ltPriority = 0;
            break;
        case 1: // Low
            ltPriority = 2;
            break;
        case 2: // Normal
            ltPriority = 4;
            break;
        case 3: // High
            ltPriority = 6;
            break;
        default:
            ltPriority = 4;
    }
    
    // Set file priority
    if (handle.has_metadata()) {
        std::shared_ptr<const libtorrent::torrent_info> info = handle.torrent_file();
        if (info && fileIndex >= 0 && fileIndex < info->num_files()) {
            handle.file_priority(fileIndex, ltPriority);
        }
    }
}

std::vector<TorrentFileInfo> TorrentProtocolHandler::getTorrentFiles(const std::string& taskHash) const {
    std::vector<TorrentFileInfo> result;
    
    if (!m_session) {
        return result;
    }
    
    // Find torrent
    libtorrent::torrent_handle handle;
    {
        std::lock_guard<std::mutex> lock(m_torrentsMutex);
        auto it = m_torrents.find(taskHash);
        if (it == m_torrents.end()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Torrent not found: " + taskHash);
            return result;
        }
        
        handle = it->second.handle;
    }
    
    if (!handle.is_valid() || !handle.has_metadata()) {
        return result;
    }
    
    // Get file info
    std::shared_ptr<const libtorrent::torrent_info> info = handle.torrent_file();
    if (!info) {
        return result;
    }
    
    // Get file priorities
    std::vector<int> priorities = handle.file_priorities();
    
    // Get file progress
    std::vector<int64_t> progress;
    handle.file_progress(progress);
    
    // Create file info objects
    int numFiles = info->num_files();
    for (int i = 0; i < numFiles; i++) {
        const libtorrent::file_entry& entry = info->files().at(i);
        
        TorrentFileInfo fileInfo;
        fileInfo.index = i;
        fileInfo.name = entry.path.string();
        fileInfo.size = entry.size;
        
        // Get priority
        if (i < static_cast<int>(priorities.size())) {
            fileInfo.priority = convertPriority(priorities[i]);
        } else {
            fileInfo.priority = 2; // Normal
        }
        
        // Get progress
        if (i < static_cast<int>(progress.size())) {
            fileInfo.progress = static_cast<float>(progress[i]) / entry.size;
        } else {
            fileInfo.progress = 0.0f;
        }
        
        result.push_back(fileInfo);
    }
    
    return result;
}

void TorrentProtocolHandler::initSession() {
    try {
        // Set session settings
        libtorrent::settings_pack settings;
        
        // Enable DHT
        settings.set_bool(libtorrent::settings_pack::enable_dht, true);
        
        // Enable Local Peer Discovery
        settings.set_bool(libtorrent::settings_pack::enable_lsd, true);
        
        // Enable UPnP
        settings.set_bool(libtorrent::settings_pack::enable_upnp, true);
        
        // Enable NAT-PMP
        settings.set_bool(libtorrent::settings_pack::enable_natpmp, true);
        
        // Set connection limits
        settings.set_int(libtorrent::settings_pack::connections_limit, 200);
        
        // Set username and client info
        settings.set_str(libtorrent::settings_pack::user_agent, "DownloadManager/1.0");
        
        // Apply settings
        m_session->apply_settings(settings);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Initialized BitTorrent session");
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in initSession: " + std::string(e.what()));
    }
}

void TorrentProtocolHandler::startAlertHandling() {
    if (m_isRunning) {
        return;
    }
    
    m_isRunning = true;
    m_alertThread = std::make_unique<std::thread>(&TorrentProtocolHandler::alertThreadFunc, this);
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Started BitTorrent alert handling");
}

void TorrentProtocolHandler::stopAlertHandling() {
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    
    if (m_alertThread && m_alertThread->joinable()) {
        m_alertThread->join();
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Stopped BitTorrent alert handling");
}

void TorrentProtocolHandler::alertThreadFunc() {
    using namespace libtorrent;
    
    // Set up alert mask
    m_session->set_alert_mask(
        alert::status_notification |
        alert::error_notification |
        alert::storage_notification |
        alert::progress_notification
    );
    
    while (m_isRunning) {
        std::vector<alert*> alerts;
        m_session->pop_alerts(&alerts);
        
        for (alert* a : alerts) {
            switch (a->type()) {
                case torrent_finished_alert::alert_type:
                {
                    // Torrent finished
                    torrent_finished_alert* finished = alert_cast<torrent_finished_alert>(a);
                    if (finished) {
                        handleTorrentFinished(finished);
                    }
                    break;
                }
                case save_resume_data_alert::alert_type:
                {
                    // Resume data generated
                    save_resume_data_alert* resume = alert_cast<save_resume_data_alert>(a);
                    if (resume) {
                        handleSaveResumeData(resume);
                    }
                    break;
                }
                case torrent_error_alert::alert_type:
                {
                    // Torrent error
                    torrent_error_alert* error = alert_cast<torrent_error_alert>(a);
                    if (error) {
                        handleTorrentError(error);
                    }
                    break;
                }
                case state_changed_alert::alert_type:
                {
                    // State changed
                    state_changed_alert* state = alert_cast<state_changed_alert>(a);
                    if (state) {
                        handleStateChanged(state);
                    }
                    break;
                }
                case metadata_received_alert::alert_type:
                {
                    // Metadata received (for magnet links)
                    metadata_received_alert* metadata = alert_cast<metadata_received_alert>(a);
                    if (metadata) {
                        handleMetadataReceived(metadata);
                    }
                    break;
                }
                case stats_alert::alert_type:
                {
                    // Stats updated
                    stats_alert* stats = alert_cast<stats_alert>(a);
                    if (stats) {
                        handleStatsUpdated(stats);
                    }
                    break;
                }
            }
        }
        
        // Update all torrents
        updateAllTorrents();
        
        // Sleep for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TorrentProtocolHandler::handleTorrentFinished(libtorrent::torrent_finished_alert* alert) {
    try {
        libtorrent::torrent_handle handle = alert->handle;
        std::string hash = handle.info_hash().to_string();
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Torrent finished: " + hash);
        
        // Find task
        std::shared_ptr<DownloadTask> task;
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            for (const auto& pair : m_torrents) {
                if (pair.second.handle == handle) {
                    task = pair.second.task;
                    break;
                }
            }
        }
        
        if (task) {
            // Update task status
            task->setStatus(DownloadStatus::COMPLETED);
            task->setDownloadedBytes(task->getFileSize());
            
            // Save resume data
            handle.save_resume_data();
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in handleTorrentFinished: " + std::string(e.what()));
    }
}

void TorrentProtocolHandler::handleSaveResumeData(libtorrent::save_resume_data_alert* alert) {
    try {
        libtorrent::torrent_handle handle = alert->handle;
        std::string hash = handle.info_hash().to_string();
        
        // Generate resume data
        std::vector<char> resumeData = libtorrent::write_resume_data_buf(alert->params);
        
        // Save to file
        std::string resumePath = getResumeDataPath(hash);
        saveResumeData(resumePath, resumeData);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Saved resume data for torrent: " + hash);
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in handleSaveResumeData: " + std::string(e.what()));
    }
}

void TorrentProtocolHandler::handleTorrentError(libtorrent::torrent_error_alert* alert) {
    try {
        libtorrent::torrent_handle handle = alert->handle;
        std::string hash = handle.info_hash().to_string();
        std::string error = alert->error.message();
        
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Torrent error: " + hash + " - " + error);
        
        // Find task
        std::shared_ptr<DownloadTask> task;
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            for (const auto& pair : m_torrents) {
                if (pair.second.handle == handle) {
                    task = pair.second.task;
                    break;
                }
            }
        }
        
        if (task) {
            // Update task status
            task->setStatus(DownloadStatus::FAILED);
            task->setErrorMessage(error);
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in handleTorrentError: " + std::string(e.what()));
    }
}

void TorrentProtocolHandler::handleStateChanged(libtorrent::state_changed_alert* alert) {
    try {
        libtorrent::torrent_handle handle = alert->handle;
        std::string hash = handle.info_hash().to_string();
        libtorrent::torrent_status::state_t state = alert->state;
        
        Utils::Logger::instance().log(Utils::LogLevel::DEBUG, 
            "Torrent state changed: " + hash + " - " + std::to_string(static_cast<int>(state)));
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in handleStateChanged: " + std::string(e.what()));
    }
}

void TorrentProtocolHandler::handleMetadataReceived(libtorrent::metadata_received_alert* alert) {
    try {
        libtorrent::torrent_handle handle = alert->handle;
        std::string hash = handle.info_hash().to_string();
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Torrent metadata received: " + hash);
        
        // Find task
        std::shared_ptr<DownloadTask> task;
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            for (const auto& pair : m_torrents) {
                if (pair.second.handle == handle) {
                    task = pair.second.task;
                    break;
                }
            }
        }
        
        if (task) {
            // Get torrent info
            auto info = handle.torrent_file();
            if (info) {
                // Update task info
                task->setFileSize(info->total_size());
                
                // Set file name if it's a single file torrent
                if (info->num_files() == 1) {
                    auto fileIterator = info->begin_files();
                    task->setFilename(fileIterator.filename().string());
                } else {
                    task->setFilename(info->name());
                }
            }
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in handleMetadataReceived: " + std::string(e.what()));
    }
}

void TorrentProtocolHandler::handleStatsUpdated(libtorrent::stats_alert* alert) {
    // This is just for monitoring, we don't need to do anything with it
}

void TorrentProtocolHandler::updateAllTorrents() {
    std::vector<std::string> toRemove;
    
    {
        std::lock_guard<std::mutex> lock(m_torrentsMutex);
        
        for (auto& pair : m_torrents) {
            std::string taskHash = pair.first;
            TorrentInfo& info = pair.second;
            
            // Skip invalid handles
            if (!info.handle.is_valid()) {
                continue;
            }
            
            // Get status
            libtorrent::torrent_status status;
            try {
                status = info.handle.status();
            } 
            catch (const std::exception& e) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Exception getting torrent status: " + std::string(e.what()));
                continue;
            }
            
            // Update task
            if (info.task) {
                // Update download progress
                if (status.total_wanted > 0) {
                    int64_t downloadedBytes = static_cast<int64_t>(status.total_wanted * status.progress);
                    info.task->setDownloadedBytes(downloadedBytes);
                    info.task->setFileSize(status.total_wanted);
                }
                
                // Update download speed
                info.task->setCurrentSpeed(status.download_rate);
                
                // Update task status based on torrent state
                switch (status.state) {
                    case libtorrent::torrent_status::checking_files:
                    case libtorrent::torrent_status::downloading_metadata:
                    case libtorrent::torrent_status::downloading:
                    case libtorrent::torrent_status::finished:
                        info.task->setStatus(DownloadStatus::DOWNLOADING);
                        break;
                    case libtorrent::torrent_status::seeding:
                        info.task->setStatus(DownloadStatus::COMPLETED);
                        break;
                    case libtorrent::torrent_status::checking_resume_data:
                        // Keep current status
                        break;
                    case libtorrent::torrent_status::paused:
                        info.task->setStatus(DownloadStatus::PAUSED);
                        break;
                }
                
                // Call progress callback
                if (info.progressCallback && status.total_wanted > 0) {
                    info.progressCallback(
                        static_cast<int64_t>(status.total_wanted * status.progress),
                        status.total_wanted
                    );
                }
            }
            
            // Check if torrent is complete and seeding is disabled
            if (status.is_finished && !isSeedingEnabled()) {
                toRemove.push_back(taskHash);
                
                if (info.task) {
                    info.task->setStatus(DownloadStatus::COMPLETED);
                }
            }
        }
    }
    
    // Remove completed torrents
    for (const std::string& hash : toRemove) {
        removeTorrent(hash, false);
    }
}

void TorrentProtocolHandler::removeTorrent(const std::string& hash, bool deleteFiles) {
    try {
        libtorrent::torrent_handle handle;
        std::shared_ptr<DownloadTask> task;
        
        {
            std::lock_guard<std::mutex> lock(m_torrentsMutex);
            auto it = m_torrents.find(hash);
            if (it == m_torrents.end()) {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Torrent not found: " + hash);
                return;
            }
            
            handle = it->second.handle;
            task = it->second.task;
            
            // Remove from map
            m_torrents.erase(it);
        }
        
        // Save resume data before removing
        if (handle.is_valid() && handle.has_metadata() && !deleteFiles) {
            handle.save_resume_data();
        }
        
        // Remove torrent
        int removeFlags = deleteFiles ? libtorrent::session::delete_files : 0;
        m_session->remove_torrent(handle, removeFlags);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Torrent removed: " + hash);
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in removeTorrent: " + std::string(e.what()));
    }
}

void TorrentProtocolHandler::saveSessionState() {
    try {
        // Save resume data for all torrents
        std::vector<libtorrent::torrent_handle> torrents = m_session->get_torrents();
        
        for (const auto& handle : torrents) {
            if (handle.is_valid() && handle.has_metadata()) {
                handle.save_resume_data();
            }
        }
        
        // Wait for resume data to be processed
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Saved session state");
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in saveSessionState: " + std::string(e.what()));
    }
}

std::string TorrentProtocolHandler::getTorrentFilePath(const std::string& url) {
    // If it's a local file, return the path
    if (url.substr(0, 7) == "file://") {
        return url.substr(7);
    }
    
    // If it's a remote .torrent file, download it
    if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://") {
        // Generate temporary file path
        std::string tempDir = Utils::FileUtils::getTempDirectory();
        std::string tempFile = tempDir + "/download_" + generateRandomString(8) + ".torrent";
        
        // Download file
        HttpClient httpClient;
        HttpRequestOptions options;
        options.timeout = 30000; // 30 seconds
        
        try {
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "Downloading torrent file: " + url);
            
            HttpResponse response = httpClient.sendGet(url, options);
            
            if (response.statusCode == 200) {
                // Save to temp file
                std::ofstream outFile(tempFile, std::ios::binary);
                outFile.write(response.body.c_str(), response.body.size());
                outFile.close();
                
                Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                    "Torrent file downloaded to: " + tempFile);
                
                return tempFile;
            } else {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to download torrent file: HTTP " + std::to_string(response.statusCode));
                return "";
            }
        } 
        catch (const std::exception& e) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Exception downloading torrent file: " + std::string(e.what()));
            return "";
        }
    }
    
    // If it's already a local path, return it
    if (Utils::FileUtils::fileExists(url)) {
        return url;
    }
    
    // Invalid URL
    Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
        "Invalid torrent URL: " + url);
    return "";
}

std::string TorrentProtocolHandler::extractNameFromMagnet(const std::string& url) {
    try {
        // Look for the dn (display name) parameter
        std::regex dnRegex("dn=([^&]+)");
        std::smatch match;
        
        if (std::regex_search(url, match, dnRegex) && match.size() > 1) {
            // URL decode the name
            std::string encodedName = match.str(1);
            return Utils::StringUtils::urldecode(encodedName);
        }
        
        // If no name found, use hash
        std::regex hashRegex("xt=urn:btih:([^&]+)");
        
        if (std::regex_search(url, match, hashRegex) && match.size() > 1) {
            return "Torrent_" + match.str(1);
        }
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in extractNameFromMagnet: " + std::string(e.what()));
    }
    
    // Default name
    return "Unknown_Torrent";
}

std::string TorrentProtocolHandler::generateTaskHash(std::shared_ptr<DownloadTask> task) {
    // Try to extract info hash from magnet link
    if (task->getUrl().substr(0, 8) == "magnet:?") {
        std::regex hashRegex("xt=urn:btih:([^&]+)");
        std::smatch match;
        
        if (std::regex_search(task->getUrl(), match, hashRegex) && match.size() > 1) {
            return match.str(1);
        }
    }
    
    // Generate random hash
    return Utils::StringUtils::generateRandomString(40);
}

std::string TorrentProtocolHandler::getResumeDataPath(const std::string& hash) {
    std::string resumeDir = Utils::FileUtils::getAppDataDirectory() + "/torrents/resume";
    
    // Create directory if it doesn't exist
    Utils::FileUtils::createDirectory(resumeDir);
    
    return resumeDir + "/" + hash + ".resume";
}

std::vector<char> TorrentProtocolHandler::loadResumeData(const std::string& path) {
    std::vector<char> result;
    
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return result;
        }
        
        file.seekg(0, std::ios::end);
        int size = static_cast<int>(file.tellg());
        file.seekg(0, std::ios::beg);
        
        result.resize(size);
        file.read(result.data(), size);
        
        file.close();
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in loadResumeData: " + std::string(e.what()));
        return std::vector<char>();
    }
    
    return result;
}

bool TorrentProtocolHandler::saveResumeData(const std::string& path, const std::vector<char>& data) {
    try {
        // Create directory if it doesn't exist
        std::string dir = Utils::FileUtils::getDirectory(path);
        Utils::FileUtils::createDirectory(dir);
        
        // Save data
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Failed to open resume data file for writing: " + path);
            return false;
        }
        
        file.write(data.data(), data.size());
        file.close();
        
        return true;
    } 
    catch (const std::exception& e) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Exception in saveResumeData: " + std::string(e.what()));
        return false;
    }
}

std::string TorrentProtocolHandler::convertState(libtorrent::torrent_status::state_t state) {
    switch (state) {
        case libtorrent::torrent_status::checking_files:
            return "Checking files";
        case libtorrent::torrent_status::downloading_metadata:
            return "Downloading metadata";
        case libtorrent::torrent_status::downloading:
            return "Downloading";
        case libtorrent::torrent_status::finished:
            return "Finished";
        case libtorrent::torrent_status::seeding:
            return "Seeding";
        case libtorrent::torrent_status::checking_resume_data:
            return "Checking resume data";
        case libtorrent::torrent_status::paused:
            return "Paused";
        default:
            return "Unknown";
    }
}

int TorrentProtocolHandler::convertPriority(int ltPriority) {
    if (ltPriority == 0) {
        return 0; // Don't download
    } else if (ltPriority <= 3) {
        return 1; // Low
    } else if (ltPriority <= 5) {
        return 2; // Normal
    } else {
        return 3; // High
    }
}

bool TorrentProtocolHandler::isSeedingEnabled() const {
    return true; // Default is to allow seeding
}

std::string TorrentProtocolHandler::generateRandomString(int length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size() - 1));
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += chars[dist(generator)];
    }
    
    return result;
}

} // namespace Core
} // namespace DownloadManager