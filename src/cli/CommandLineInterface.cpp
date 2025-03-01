#include "../../include/cli/CommandLineInterface.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/StringUtils.h"
#include "../../include/core/DownloadManager.h"
#include "../../include/core/BatchDownloader.h"
#include "../../include/core/WebsiteCrawler.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cstring>

namespace DownloadManager {
namespace CLI {

CommandLineInterface::CommandLineInterface() :
    m_isRunning(false),
    m_showProgress(true),
    m_progressUpdateInterval(1000), // 1 second default
    m_activeDownloads(0),
    m_progressThread(nullptr)
{
    registerCommands();
}

CommandLineInterface::~CommandLineInterface() {
    stop();
}

void CommandLineInterface::start() {
    if (m_isRunning) {
        return;
    }
    
    m_isRunning = true;
    
    // Start progress thread
    m_progressThread = std::make_unique<std::thread>(&CommandLineInterface::progressLoop, this);
    
    // Show welcome message
    std::cout << "Download Manager CLI - Type 'help' for available commands" << std::endl;
    
    std::string input;
    
    while (m_isRunning) {
        // Show prompt
        std::cout << "> ";
        
        // Get input
        std::getline(std::cin, input);
        
        // Process command
        processCommand(input);
    }
}

void CommandLineInterface::stop() {
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    
    if (m_progressThread && m_progressThread->joinable()) {
        m_progressThread->join();
    }
}

void CommandLineInterface::processCommand(const std::string& cmdLine) {
    // Skip empty commands
    if (cmdLine.empty()) {
        return;
    }
    
    // Parse command and arguments
    std::string cmd;
    std::vector<std::string> args;
    
    if (!parseCommandLine(cmdLine, cmd, args)) {
        std::cout << "Error parsing command" << std::endl;
        return;
    }
    
    // Convert command to lowercase
    cmd = Utils::StringUtils::toLower(cmd);
    
    // Check if command exists
    if (m_commands.find(cmd) != m_commands.end()) {
        try {
            // Execute command
            m_commands[cmd](args);
        } 
        catch (const std::exception& e) {
            std::cout << "Error executing command: " << e.what() << std::endl;
        }
    } 
    else {
        std::cout << "Unknown command: " << cmd << std::endl;
        std::cout << "Type 'help' for available commands" << std::endl;
    }
}

bool CommandLineInterface::parseCommandLine(const std::string& cmdLine, std::string& cmd, std::vector<std::string>& args) {
    std::istringstream iss(cmdLine);
    
    // Get command
    iss >> cmd;
    
    // Get arguments
    std::string arg;
    bool inQuotes = false;
    std::string quotedArg;
    
    while (iss >> arg) {
        // Check for quoted arguments
        if (!inQuotes && arg.front() == '"' && arg.back() != '"') {
            // Start of quoted argument
            inQuotes = true;
            quotedArg = arg.substr(1); // Remove opening quote
        } 
        else if (inQuotes && arg.back() == '"') {
            // End of quoted argument
            inQuotes = false;
            quotedArg += ' ' + arg.substr(0, arg.length() - 1); // Remove closing quote
            args.push_back(quotedArg);
            quotedArg.clear();
        } 
        else if (inQuotes) {
            // Middle of quoted argument
            quotedArg += ' ' + arg;
        } 
        else {
            // Regular argument
            if (arg.front() == '"' && arg.back() == '"' && arg.length() > 1) {
                // Quoted argument in a single word
                args.push_back(arg.substr(1, arg.length() - 2));
            } else {
                args.push_back(arg);
            }
        }
    }
    
    // Check if we have an unfinished quoted argument
    if (inQuotes) {
        std::cout << "Error: Unclosed quote in command line" << std::endl;
        return false;
    }
    
    return true;
}

void CommandLineInterface::registerCommands() {
    // Help command
    m_commands["help"] = [this](const std::vector<std::string>& args) {
        cmdHelp(args);
    };
    
    // Download command
    m_commands["download"] = [this](const std::vector<std::string>& args) {
        cmdDownload(args);
    };
    
    // List command
    m_commands["list"] = [this](const std::vector<std::string>& args) {
        cmdList(args);
    };
    
    // Pause command
    m_commands["pause"] = [this](const std::vector<std::string>& args) {
        cmdPause(args);
    };
    
    // Resume command
    m_commands["resume"] = [this](const std::vector<std::string>& args) {
        cmdResume(args);
    };
    
    // Cancel command
    m_commands["cancel"] = [this](const std::vector<std::string>& args) {
        cmdCancel(args);
    };
    
    // Remove command
    m_commands["remove"] = [this](const std::vector<std::string>& args) {
        cmdRemove(args);
    };
    
    // Info command
    m_commands["info"] = [this](const std::vector<std::string>& args) {
        cmdInfo(args);
    };
    
    // Batch command
    m_commands["batch"] = [this](const std::vector<std::string>& args) {
        cmdBatch(args);
    };
    
    // Crawl command
    m_commands["crawl"] = [this](const std::vector<std::string>& args) {
        cmdCrawl(args);
    };
    
    // Settings command
    m_commands["settings"] = [this](const std::vector<std::string>& args) {
        cmdSettings(args);
    };
    
    // Progress command
    m_commands["progress"] = [this](const std::vector<std::string>& args) {
        cmdProgress(args);
    };
    
    // Quit command
    m_commands["quit"] = [this](const std::vector<std::string>& args) {
        cmdQuit(args);
    };
    
    // Exit command (alias for quit)
    m_commands["exit"] = [this](const std::vector<std::string>& args) {
        cmdQuit(args);
    };
}

void CommandLineInterface::progressLoop() {
    auto& downloadManager = Core::DownloadManager::instance();
    
    while (m_isRunning) {
        if (m_showProgress && m_activeDownloads > 0) {
            showProgressBars();
        }
        
        // Update active downloads count
        m_activeDownloads = downloadManager.getActiveDownloads();
        
        // Sleep for the progress update interval
        std::this_thread::sleep_for(std::chrono::milliseconds(m_progressUpdateInterval));
    }
}

void CommandLineInterface::showProgressBars() {
    auto& downloadManager = Core::DownloadManager::instance();
    
    // Get active downloads
    auto downloads = downloadManager.getDownloads();
    
    // Clear previous progress bars
    if (!downloads.empty()) {
        for (size_t i = 0; i < m_lastProgressLines; i++) {
            std::cout << "\033[1A" << "\033[2K"; // Move up and clear line
        }
    }
    
    m_lastProgressLines = 0;
    
    // Build progress bars for active downloads
    for (const auto& download : downloads) {
        // Only show active downloads
        if (download->getStatus() != Core::DownloadStatus::DOWNLOADING &&
            download->getStatus() != Core::DownloadStatus::PAUSED) {
            continue;
        }
        
        // Get download information
        std::string filename = download->getFilename();
        int64_t downloadedSize = download->getDownloadedBytes();
        int64_t totalSize = download->getFileSize();
        int speed = download->getCurrentSpeed();
        
        // Calculate progress percentage
        double progress = 0.0;
        if (totalSize > 0) {
            progress = static_cast<double>(downloadedSize) / totalSize;
        }
        
        // Calculate ETA
        std::string eta = "Unknown";
        if (speed > 0 && totalSize > 0) {
            int64_t remainingBytes = totalSize - downloadedSize;
            int64_t remainingSeconds = remainingBytes / speed;
            eta = formatTime(remainingSeconds);
        }
        
        // Format sizes
        std::string downloaded = formatSize(downloadedSize);
        std::string total = formatSize(totalSize);
        std::string speedStr = formatSpeed(speed);
        
        // Build progress bar
        const int barWidth = 40;
        int pos = static_cast<int>(barWidth * progress);
        
        // Status indicator
        std::string status;
        if (download->getStatus() == Core::DownloadStatus::DOWNLOADING) {
            status = "Downloading";
        } else if (download->getStatus() == Core::DownloadStatus::PAUSED) {
            status = "Paused";
        }
        
        // Print filename, status, and progress info
        std::cout << filename << " [" << status << "] - " 
                << downloaded << " / " << total << " (" 
                << static_cast<int>(progress * 100.0) << "%) at " 
                << speedStr << " - ETA: " << eta << std::endl;
        
        // Print progress bar
        std::cout << "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "]" << std::endl;
        
        m_lastProgressLines += 2;
    }
}

std::string CommandLineInterface::formatSize(int64_t bytes) {
    return Utils::StringUtils::formatFileSize(bytes);
}

std::string CommandLineInterface::formatSpeed(int bytesPerSecond) {
    return Utils::StringUtils::formatBitrate(bytesPerSecond);
}

std::string CommandLineInterface::formatTime(int64_t seconds) {
    return Utils::StringUtils::formatTime(seconds);
}

void CommandLineInterface::cmdHelp(const std::vector<std::string>& args) {
    if (args.empty()) {
        // General help
        std::cout << "Available commands:" << std::endl;
        std::cout << "  help [command]              - Show help for a specific command" << std::endl;
        std::cout << "  download <url> [options]    - Download a file" << std::endl;
        std::cout << "  list [status]               - List downloads with optional status filter" << std::endl;
        std::cout << "  pause <id|all>              - Pause download(s)" << std::endl;
        std::cout << "  resume <id|all>             - Resume download(s)" << std::endl;
        std::cout << "  cancel <id|all>             - Cancel download(s)" << std::endl;
        std::cout << "  remove <id|all>             - Remove download(s) from the list" << std::endl;
        std::cout << "  info <id>                   - Show detailed information about a download" << std::endl;
        std::cout << "  batch <file|url> [options]  - Process batch downloads" << std::endl;
        std::cout << "  crawl <url> [options]       - Crawl a website for downloads" << std::endl;
        std::cout << "  settings [key] [value]      - View or change settings" << std::endl;
        std::cout << "  progress <on|off>           - Turn progress display on or off" << std::endl;
        std::cout << "  quit                        - Exit the program" << std::endl;
    } 
    else {
        // Help for specific command
        std::string command = Utils::StringUtils::toLower(args[0]);
        
        if (command == "download") {
            std::cout << "Usage: download <url> [options]" << std::endl;
            std::cout << "Download a file from the specified URL" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -o, --output <file>      - Specify output filename" << std::endl;
            std::cout << "  -d, --dir <directory>    - Specify destination directory" << std::endl;
            std::cout << "  -s, --segments <num>     - Number of download segments (1-16)" << std::endl;
            std::cout << "  -l, --limit <speed>      - Limit download speed in KB/s" << std::endl;
            std::cout << "  -u, --username <user>    - Username for authentication" << std::endl;
            std::cout << "  -p, --password <pass>    - Password for authentication" << std::endl;
            std::cout << "  -r, --resume             - Resume interrupted download" << std::endl;
            std::cout << "  -w, --wait               - Wait for download to complete before returning" << std::endl;
        } 
        else if (command == "list") {
            std::cout << "Usage: list [status]" << std::endl;
            std::cout << "List all downloads or filter by status" << std::endl;
            std::cout << "Status options:" << std::endl;
            std::cout << "  all       - All downloads (default)" << std::endl;
            std::cout << "  active    - Active downloads" << std::endl;
            std::cout << "  waiting   - Waiting downloads" << std::endl;
            std::cout << "  paused    - Paused downloads" << std::endl;
            std::cout << "  completed - Completed downloads" << std::endl;
            std::cout << "  failed    - Failed downloads" << std::endl;
        } 
        else if (command == "pause") {
            std::cout << "Usage: pause <id|all>" << std::endl;
            std::cout << "Pause a specific download by ID or all active downloads" << std::endl;
        } 
        else if (command == "resume") {
            std::cout << "Usage: resume <id|all>" << std::endl;
            std::cout << "Resume a paused download by ID or all paused downloads" << std::endl;
        } 
        else if (command == "cancel") {
            std::cout << "Usage: cancel <id|all>" << std::endl;
            std::cout << "Cancel a download by ID or all active downloads" << std::endl;
        } 
        else if (command == "remove") {
            std::cout << "Usage: remove <id|all>" << std::endl;
            std::cout << "Remove a download from the list by ID or all downloads" << std::endl;
            std::cout << "Note: This will not delete the downloaded file" << std::endl;
        } 
        else if (command == "info") {
            std::cout << "Usage: info <id>" << std::endl;
            std::cout << "Show detailed information about a download" << std::endl;
        } 
        else if (command == "batch") {
            std::cout << "Usage: batch <command> [options]" << std::endl;
            std::cout << "Process batch downloads" << std::endl;
            std::cout << "Commands:" << std::endl;
            std::cout << "  add <file>               - Add URLs from a text file" << std::endl;
            std::cout << "  pattern <url> <start> <end> [step] [padding]" << std::endl;
            std::cout << "                           - Generate URLs from a pattern" << std::endl;
            std::cout << "                             Use {$PATTERN} as placeholder in URL" << std::endl;
            std::cout << "  list                     - List all batch downloads" << std::endl;
            std::cout << "  start                    - Start batch processing" << std::endl;
            std::cout << "  stop                     - Stop batch processing" << std::endl;
            std::cout << "  clear                    - Clear batch queue" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -d, --dir <directory>    - Specify destination directory" << std::endl;
            std::cout << "  -c, --concurrent <num>   - Max concurrent downloads (default: 3)" << std::endl;
        } 
        else if (command == "crawl") {
            std::cout << "Usage: crawl <url> [options]" << std::endl;
            std::cout << "Crawl a website to find downloadable files" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -d, --dir <directory>    - Specify destination directory" << std::endl;
            std::cout << "  -e, --ext <extensions>   - File extensions to download (comma-separated)" << std::endl;
            std::cout << "  -f, --filter <pattern>   - URL filter pattern (regex)" << std::endl;
            std::cout << "  --depth <num>            - Maximum crawl depth (default: 3)" << std::endl;
            std::cout << "  --max-pages <num>        - Maximum pages to crawl (default: 100)" << std::endl;
            std::cout << "  --delay <ms>             - Delay between requests in milliseconds" << std::endl;
            std::cout << "  --external               - Follow external links" << std::endl;
            std::cout << "  --no-robots              - Ignore robots.txt" << std::endl;
            std::cout << "  --download               - Automatically download found files" << std::endl;
        } 
        else if (command == "settings") {
            std::cout << "Usage: settings [key] [value]" << std::endl;
            std::cout << "View or change settings" << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  settings                 - Show all settings" << std::endl;
            std::cout << "  settings download_dir    - Show download directory setting" << std::endl;
            std::cout << "  settings download_dir /path/to/dir" << std::endl;
            std::cout << "                           - Change download directory" << std::endl;
            std::cout << "Available settings:" << std::endl;
            std::cout << "  download_dir            - Default download directory" << std::endl;
            std::cout << "  max_downloads           - Maximum concurrent downloads" << std::endl;
            std::cout << "  max_speed               - Global speed limit in KB/s (0 = unlimited)" << std::endl;
            std::cout << "  segments                - Default number of segments per download" << std::endl;
            std::cout << "  auto_resume             - Auto-resume interrupted downloads (0/1)" << std::endl;
        } 
        else if (command == "progress") {
            std::cout << "Usage: progress <on|off>" << std::endl;
            std::cout << "Turn progress display on or off" << std::endl;
        } 
        else if (command == "quit" || command == "exit") {
            std::cout << "Usage: quit" << std::endl;
            std::cout << "Exit the program" << std::endl;
        } 
        else {
            std::cout << "No help available for command: " << command << std::endl;
        }
    }
}

void CommandLineInterface::cmdDownload(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: URL required" << std::endl;
        std::cout << "Usage: download <url> [options]" << std::endl;
        std::cout << "Type 'help download' for more information" << std::endl;
        return;
    }
    
    std::string url = args[0];
    
    // Parse options
    std::string output;
    std::string directory;
    int segments = 0;
    int speedLimit = 0;
    std::string username;
    std::string password;
    bool resume = false;
    bool wait = false;
    
    for (size_t i = 1; i < args.size(); i++) {
        if ((args[i] == "-o" || args[i] == "--output") && i + 1 < args.size()) {
            output = args[++i];
        } 
        else if ((args[i] == "-d" || args[i] == "--dir") && i + 1 < args.size()) {
            directory = args[++i];
        } 
        else if ((args[i] == "-s" || args[i] == "--segments") && i + 1 < args.size()) {
            try {
                segments = std::stoi(args[++i]);
                if (segments < 1 || segments > 16) {
                    std::cout << "Warning: Segments must be between 1 and 16, using default" << std::endl;
                    segments = 0;
                }
            } catch (...) {
                std::cout << "Warning: Invalid segments value, using default" << std::endl;
            }
        } 
        else if ((args[i] == "-l" || args[i] == "--limit") && i + 1 < args.size()) {
            try {
                speedLimit = std::stoi(args[++i]);
                if (speedLimit < 0) {
                    std::cout << "Warning: Speed limit must be positive, using default" << std::endl;
                    speedLimit = 0;
                }
            } catch (...) {
                std::cout << "Warning: Invalid speed limit value, using default" << std::endl;
            }
        } 
        else if ((args[i] == "-u" || args[i] == "--username") && i + 1 < args.size()) {
            username = args[++i];
        } 
        else if ((args[i] == "-p" || args[i] == "--password") && i + 1 < args.size()) {
            password = args[++i];
        } 
        else if (args[i] == "-r" || args[i] == "--resume") {
            resume = true;
        } 
        else if (args[i] == "-w" || args[i] == "--wait") {
            wait = true;
        }
    }
    
    // Set up download options
    Core::DownloadOptions options;
    options.url = url;
    
    // If directory is specified, ensure it ends with a separator
    if (!directory.empty()) {
        if (directory.back() != '/' && directory.back() != '\\') {
            directory += '/';
        }
    }
    
    // Set destination
    if (!output.empty()) {
        if (!directory.empty()) {
            options.destination = directory + output;
        } else {
            options.destination = output;
        }
    } else if (!directory.empty()) {
        options.destination = directory;
    }
    
    // Set other options
    if (segments > 0) {
        options.segments = segments;
    }
    
    if (speedLimit > 0) {
        options.maxSpeed = speedLimit * 1024; // Convert KB/s to bytes/s
    }
    
    if (!username.empty()) {
        options.username = username;
        options.password = password;
    }
    
    options.resumable = resume;
    
    // Start the download
    try {
        auto& downloadManager = Core::DownloadManager::instance();
        std::shared_ptr<Core::DownloadTask> task = downloadManager.addDownload(options);
        
        if (task) {
            std::cout << "Download started: ID=" << task->getId() << ", URL=" << url << std::endl;
            
            if (wait) {
                std::cout << "Waiting for download to complete..." << std::endl;
                downloadManager.waitForDownload(task->getId());
                
                Core::DownloadStatus status = task->getStatus();
                if (status == Core::DownloadStatus::COMPLETED) {
                    std::cout << "Download completed successfully" << std::endl;
                } else {
                    std::cout << "Download failed: " << task->getErrorMessage() << std::endl;
                }
            }
        } else {
            std::cout << "Failed to start download" << std::endl;
        }
    } 
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void CommandLineInterface::cmdList(const std::vector<std::string>& args) {
    auto& downloadManager = Core::DownloadManager::instance();
    
    // Parse status filter
    std::string filter = "all";
    if (!args.empty()) {
        filter = Utils::StringUtils::toLower(args[0]);
    }
    
    // Get downloads
    auto downloads = downloadManager.getDownloads();
    
    // Filter downloads
    std::vector<std::shared_ptr<Core::DownloadTask>> filteredDownloads;
    
    if (filter == "all") {
        filteredDownloads = downloads;
    } 
    else if (filter == "active") {
        std::copy_if(downloads.begin(), downloads.end(), std::back_inserter(filteredDownloads),
                    [](const std::shared_ptr<Core::DownloadTask>& task) {
                        return task->getStatus() == Core::DownloadStatus::DOWNLOADING;
                    });
    } 
    else if (filter == "waiting") {
        std::copy_if(downloads.begin(), downloads.end(), std::back_inserter(filteredDownloads),
                    [](const std::shared_ptr<Core::DownloadTask>& task) {
                        return task->getStatus() == Core::DownloadStatus::QUEUED;
                    });
    } 
    else if (filter == "paused") {
        std::copy_if(downloads.begin(), downloads.end(), std::back_inserter(filteredDownloads),
                    [](const std::shared_ptr<Core::DownloadTask>& task) {
                        return task->getStatus() == Core::DownloadStatus::PAUSED;
                    });
    } 
    else if (filter == "completed") {
        std::copy_if(downloads.begin(), downloads.end(), std::back_inserter(filteredDownloads),
                    [](const std::shared_ptr<Core::DownloadTask>& task) {
                        return task->getStatus() == Core::DownloadStatus::COMPLETED;
                    });
    } 
    else if (filter == "failed") {
        std::copy_if(downloads.begin(), downloads.end(), std::back_inserter(filteredDownloads),
                    [](const std::shared_ptr<Core::DownloadTask>& task) {
                        return task->getStatus() == Core::DownloadStatus::FAILED;
                    });
    } 
    else {
        std::cout << "Invalid status filter: " << filter << std::endl;
        std::cout << "Valid filters: all, active, waiting, paused, completed, failed" << std::endl;
        return;
    }
    
    // Display downloads
    if (filteredDownloads.empty()) {
        std::cout << "No downloads" << (filter == "all" ? "" : " with status: " + filter) << std::endl;
        return;
    }
    
    // Calculate column widths
    size_t idWidth = 4; // "ID" header width
    size_t filenameWidth = 16; // Minimum width for filename
    size_t sizeWidth = 12; // Width for size column
    size_t progressWidth = 8; // Width for progress column
    size_t statusWidth = 10; // Width for status column
    
    for (const auto& task : filteredDownloads) {
        std::string id = std::to_string(task->getId());
        std::string filename = task->getFilename();
        
        idWidth = std::max(idWidth, id.length());
        filenameWidth = std::max(filenameWidth, filename.length());
    }
    
    // Cap filename width to avoid very wide columns
    filenameWidth = std::min(filenameWidth, static_cast<size_t>(50));
    
    // Print header
    std::cout << std::left
              << std::setw(idWidth) << "ID" << "  "
              << std::setw(filenameWidth) << "Filename" << "  "
              << std::setw(sizeWidth) << "Size" << "  "
              << std::setw(progressWidth) << "Progress" << "  "
              << std::setw(statusWidth) << "Status" << "  "
              << "Speed" << std::endl;
    
    // Print separator
    std::cout << std::string(idWidth + filenameWidth + sizeWidth + progressWidth + statusWidth + 10, '-') << std::endl;
    
    // Print downloads
    for (const auto& task : filteredDownloads) {
        int id = task->getId();
        std::string filename = task->getFilename();
        
        // Truncate filename if too long
        if (filename.length() > filenameWidth) {
            filename = filename.substr(0, filenameWidth - 3) + "...";
        }
        
        int64_t downloadedSize = task->getDownloadedBytes();
        int64_t totalSize = task->getFileSize();
        Core::DownloadStatus status = task->getStatus();
        int speed = task->getCurrentSpeed();
        
        // Calculate progress percentage
        int progress = 0;
        if (totalSize > 0) {
            progress = static_cast<int>((static_cast<double>(downloadedSize) / totalSize) * 100);
        }
        
        // Format status string
        std::string statusStr;
        switch (status) {
            case Core::DownloadStatus::QUEUED:
                statusStr = "Waiting";
                break;
            case Core::DownloadStatus::DOWNLOADING:
                statusStr = "Active";
                break;
            case Core::DownloadStatus::PAUSED:
                statusStr = "Paused";
                break;
            case Core::DownloadStatus::COMPLETED:
                statusStr = "Completed";
                break;
            case Core::DownloadStatus::FAILED:
                statusStr = "Failed";
                break;
            default:
                statusStr = "Unknown";
        }
        
        // Format speed string
        std::string speedStr = (status == Core::DownloadStatus::DOWNLOADING) ? 
                             formatSpeed(speed) : "";
        
        // Format size string
        std::string sizeStr;
        if (totalSize > 0) {
            sizeStr = formatSize(totalSize);
        } else {
            sizeStr = "Unknown";
        }
        
        // Print download info
        std::cout << std::left
                << std::setw(idWidth) << id << "  "
                << std::setw(filenameWidth) << filename << "  "
                << std::setw(sizeWidth) << sizeStr << "  "
                << std::setw(progressWidth) << (std::to_string(progress) + "%") << "  "
                << std::setw(statusWidth) << statusStr << "  "
                << speedStr << std::endl;
    }
}

void CommandLineInterface::cmdPause(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Download ID or 'all' required" << std::endl;
        std::cout << "Usage: pause <id|all>" << std::endl;
        return;
    }
    
    auto& downloadManager = Core::DownloadManager::instance();
    
    if (args[0] == "all") {
        // Pause all active downloads
        int paused = downloadManager.pauseAllDownloads();
        std::cout << "Paused " << paused << " downloads" << std::endl;
    } 
    else {
        try {
            // Parse download ID
            int id = std::stoi(args[0]);
            
            // Pause specific download
            if (downloadManager.pauseDownload(id)) {
                std::cout << "Download ID=" << id << " paused" << std::endl;
            } else {
                std::cout << "Failed to pause download ID=" << id << std::endl;
            }
        } 
        catch (const std::exception& e) {
            std::cout << "Error: Invalid download ID" << std::endl;
        }
    }
}

void CommandLineInterface::cmdResume(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Download ID or 'all' required" << std::endl;
        std::cout << "Usage: resume <id|all>" << std::endl;
        return;
    }
    
    auto& downloadManager = Core::DownloadManager::instance();
    
    if (args[0] == "all") {
        // Resume all paused downloads
        int resumed = downloadManager.resumeAllDownloads();
        std::cout << "Resumed " << resumed << " downloads" << std::endl;
    } 
    else {
        try {
            // Parse download ID
            int id = std::stoi(args[0]);
            
            // Resume specific download
            if (downloadManager.resumeDownload(id)) {
                std::cout << "Download ID=" << id << " resumed" << std::endl;
            } else {
                std::cout << "Failed to resume download ID=" << id << std::endl;
            }
        } 
        catch (const std::exception& e) {
            std::cout << "Error: Invalid download ID" << std::endl;
        }
    }
}

void CommandLineInterface::cmdCancel(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Download ID or 'all' required" << std::endl;
        std::cout << "Usage: cancel <id|all>" << std::endl;
        return;
    }
    
    auto& downloadManager = Core::DownloadManager::instance();
    
    if (args[0] == "all") {
        // Cancel all active downloads
        int canceled = downloadManager.cancelAllDownloads();
        std::cout << "Canceled " << canceled << " downloads" << std::endl;
    } 
    else {
        try {
            // Parse download ID
            int id = std::stoi(args[0]);
            
            // Cancel specific download
            if (downloadManager.cancelDownload(id)) {
                std::cout << "Download ID=" << id << " canceled" << std::endl;
            } else {
                std::cout << "Failed to cancel download ID=" << id << std::endl;
            }
        } 
        catch (const std::exception& e) {
            std::cout << "Error: Invalid download ID" << std::endl;
        }
    }
}

void CommandLineInterface::cmdRemove(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Download ID or 'all' required" << std::endl;
        std::cout << "Usage: remove <id|all>" << std::endl;
        return;
    }
    
    auto& downloadManager = Core::DownloadManager::instance();
    
    if (args[0] == "all") {
        // Remove all downloads
        int removed = downloadManager.removeAllDownloads();
        std::cout << "Removed " << removed << " downloads" << std::endl;
    } 
    else {
        try {
            // Parse download ID
            int id = std::stoi(args[0]);
            
            // Remove specific download
            if (downloadManager.removeDownload(id)) {
                std::cout << "Download ID=" << id << " removed" << std::endl;
            } else {
                std::cout << "Failed to remove download ID=" << id << std::endl;
            }
        } 
        catch (const std::exception& e) {
            std::cout << "Error: Invalid download ID" << std::endl;
        }
    }
}

void CommandLineInterface::cmdInfo(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Download ID required" << std::endl;
        std::cout << "Usage: info <id>" << std::endl;
        return;
    }
    
    auto& downloadManager = Core::DownloadManager::instance();
    
    try {
        // Parse download ID
        int id = std::stoi(args[0]);
        
        // Get download
        auto task = downloadManager.getDownload(id);
        
        if (!task) {
            std::cout << "Download ID=" << id << " not found" << std::endl;
            return;
        }
        
        // Display detailed information
        std::cout << "Download Information:" << std::endl;
        std::cout << "  ID: " << task->getId() << std::endl;
        std::cout << "  URL: " << task->getUrl() << std::endl;
        std::cout << "  Destination: " << task->getDestination() << std::endl;
        std::cout << "  Filename: " << task->getFilename() << std::endl;
        
        // Status
        std::string statusStr;
        switch (task->getStatus()) {
            case Core::DownloadStatus::QUEUED:
                statusStr = "Waiting";
                break;
            case Core::DownloadStatus::DOWNLOADING:
                statusStr = "Downloading";
                break;
            case Core::DownloadStatus::PAUSED:
                statusStr = "Paused";
                break;
            case Core::DownloadStatus::COMPLETED:
                statusStr = "Completed";
                break;
            case Core::DownloadStatus::FAILED:
                statusStr = "Failed";
                break;
            default:
                statusStr = "Unknown";
        }
        
        std::cout << "  Status: " << statusStr << std::endl;
        
        // Size and progress
        int64_t downloadedSize = task->getDownloadedBytes();
        int64_t totalSize = task->getFileSize();
        
        std::cout << "  Size: " << formatSize(totalSize) << std::endl;
        std::cout << "  Downloaded: " << formatSize(downloadedSize) << std::endl;
        
        // Calculate progress percentage
        double progress = 0.0;
        if (totalSize > 0) {
            progress = static_cast<double>(downloadedSize) / totalSize * 100.0;
        }
        
        std::cout << "  Progress: " << std::fixed << std::setprecision(2) << progress << "%" << std::endl;
        
        // Speed
        int speed = task->getCurrentSpeed();
        if (task->getStatus() == Core::DownloadStatus::DOWNLOADING) {
            std::cout << "  Current Speed: " << formatSpeed(speed) << std::endl;
        }
        
        // Average speed
        int avgSpeed = task->getAverageSpeed();
        std::cout << "  Average Speed: " << formatSpeed(avgSpeed) << std::endl;
        
        // ETA
        if (speed > 0 && totalSize > downloadedSize) {
            int64_t remainingBytes = totalSize - downloadedSize;
            int64_t eta = remainingBytes / speed;
            std::cout << "  Estimated Time Remaining: " << formatTime(eta) << std::endl;
        }
        
        // Start time and elapsed time
        time_t startTime = task->getStartTime();
        if (startTime > 0) {
            char timeStr[100];
            struct tm* timeinfo = localtime(&startTime);
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            std::cout << "  Start Time: " << timeStr << std::endl;
            
            // Calculate elapsed time
            time_t now = time(nullptr);
            time_t elapsed = now - startTime;
            std::cout << "  Elapsed Time: " << formatTime(elapsed) << std::endl;
        }
        
        // Additional info
        std::cout << "  Resumable: " << (task->isResumable() ? "Yes" : "No") << std::endl;
        std::cout << "  Segments: " << task->getSegmentCount() << std::endl;
        
        // Error message
        if (task->getStatus() == Core::DownloadStatus::FAILED) {
            std::cout << "  Error: " << task->getErrorMessage() << std::endl;
        }
    } 
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void CommandLineInterface::cmdBatch(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: Batch command required" << std::endl;
        std::cout << "Usage: batch <command> [options]" << std::endl;
        std::cout << "Type 'help batch' for more information" << std::endl;
        return;
    }
    
    std::string command = Utils::StringUtils::toLower(args[0]);
    auto& batchDownloader = Core::BatchDownloader::instance();
    
    if (command == "add") {
        if (args.size() < 2) {
            std::cout << "Error: File path required" << std::endl;
            std::cout << "Usage: batch add <file> [options]" << std::endl;
            return;
        }
        
        std::string filePath = args[1];
        std::string directory;
        
        // Parse options
        for (size_t i = 2; i < args.size(); i++) {
            if ((args[i] == "-d" || args[i] == "--dir") && i + 1 < args.size()) {
                directory = args[++i];
            }
        }
        
        // Add batch from file
        if (batchDownloader.addBatchFromFile(filePath, directory)) {
            std::cout << "Batch URLs added from file: " << filePath << std::endl;
            std::cout << "Total URLs in queue: " << batchDownloader.getTotalItems() << std::endl;
        } else {
            std::cout << "Failed to add batch URLs from file" << std::endl;
        }
    } 
    else if (command == "pattern") {
        if (args.size() < 4) {
            std::cout << "Error: Pattern URL, start, and end values required" << std::endl;
            std::cout << "Usage: batch pattern <url> <start> <end> [step] [padding] [options]" << std::endl;
            return;
        }
        
        std::string patternUrl = args[1];
        int start, end, step = 1, padding = 0;
        std::string directory;
        
        try {
            start = std::stoi(args[2]);
            end = std::stoi(args[3]);
            
            if (args.size() > 4 && args[4][0] != '-') {
                step = std::stoi(args[4]);
            }
            
            if (args.size() > 5 && args[5][0] != '-') {
                padding = std::stoi(args[5]);
            }
        } 
        catch (const std::exception& e) {
            std::cout << "Error: Invalid numeric values" << std::endl;
            return;
        }
        
        // Parse options
        for (size_t i = 4; i < args.size(); i++) {
            if ((args[i] == "-d" || args[i] == "--dir") && i + 1 < args.size()) {
                directory = args[++i];
            }
        }
        
        // Add batch from pattern
        if (batchDownloader.addBatchFromPattern(patternUrl, start, end, step, padding, directory)) {
            std::cout << "Batch URLs added from pattern" << std::endl;
            std::cout << "Total URLs in queue: " << batchDownloader.getTotalItems() << std::endl;
        } else {
            std::cout << "Failed to add batch URLs from pattern" << std::endl;
        }
    } 
    else if (command == "list") {
        // Get batch items
        auto batchItems = batchDownloader.getBatchItems();
        
        if (batchItems.empty()) {
            std::cout << "Batch queue is empty" << std::endl;
            return;
        }
        
        // Display batch items
        std::cout << "Batch Queue:" << std::endl;
        std::cout << "  Total: " << batchItems.size() << std::endl;
        std::cout << "  Pending: " << batchDownloader.getPendingItems() << std::endl;
        std::cout << "  Active: " << batchDownloader.getActiveItems() << std::endl;
        std::cout << "  Completed: " << batchDownloader.getCompletedItems() << std::endl;
        std::cout << "  Failed: " << batchDownloader.getFailedItems() << std::endl;
        std::cout << std::endl;
        
        // Calculate column widths
        size_t indexWidth = 5;
        size_t urlWidth = 60;
        size_t statusWidth = 10;
        
        // Print header
        std::cout << std::left
                << std::setw(indexWidth) << "Index" << "  "
                << std::setw(urlWidth) << "URL" << "  "
                << std::setw(statusWidth) << "Status" << std::endl;
        
        // Print separator
        std::cout << std::string(indexWidth + urlWidth + statusWidth + 4, '-') << std::endl;
        
        // Print items
        int index = 0;
        for (const auto& item : batchItems) {
            std::string url = item.url;
            
            // Truncate URL if too long
            if (url.length() > urlWidth) {
                url = url.substr(0, urlWidth - 3) + "...";
            }
            
            // Format status string
            std::string statusStr;
            switch (item.status) {
                case Core::BatchItemStatus::PENDING:
                    statusStr = "Pending";
                    break;
                case Core::BatchItemStatus::ACTIVE:
                    statusStr = "Active";
                    break;
                case Core::BatchItemStatus::COMPLETED:
                    statusStr = "Completed";
                    break;
                case Core::BatchItemStatus::FAILED:
                    statusStr = "Failed";
                    break;
                default:
                    statusStr = "Unknown";
            }
            
            // Print item info
            std::cout << std::left
                    << std::setw(indexWidth) << index << "  "
                    << std::setw(urlWidth) << url << "  "
                    << std::setw(statusWidth) << statusStr << std::endl;
            
            index++;
        }
    } 
    else if (command == "start") {
        // Parse options
        int concurrentDownloads = 0;
        
        for (size_t i = 1; i < args.size(); i++) {
            if ((args[i] == "-c" || args[i] == "--concurrent") && i + 1 < args.size()) {
                try {
                    concurrentDownloads = std::stoi(args[++i]);
                    if (concurrentDownloads > 0) {
                        batchDownloader.setMaxConcurrentDownloads(concurrentDownloads);
                    }
                } catch (...) {
                    std::cout << "Warning: Invalid concurrent downloads value" << std::endl;
                }
            }
        }
        
        // Start batch processing
        batchDownloader.start();
        std::cout << "Batch processing started" << std::endl;
    } 
    else if (command == "stop") {
        // Stop batch processing
        batchDownloader.stop();
        std::cout << "Batch processing stopped" << std::endl;
    } 
    else if (command == "clear") {
        // Clear batch queue
        batchDownloader.clearQueue();
        std::cout << "Batch queue cleared" << std::endl;
    } 
    else {
        std::cout << "Unknown batch command: " << command << std::endl;
        std::cout << "Valid commands: add, pattern, list, start, stop, clear" << std::endl;
    }
}

void CommandLineInterface::cmdCrawl(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Error: URL required" << std::endl;
        std::cout << "Usage: crawl <url> [options]" << std::endl;
        std::cout << "Type 'help crawl' for more information" << std::endl;
        return;
    }
    
    std::string url = args[0];
    auto& websiteCrawler = Core::WebsiteCrawler::instance();
    
    // Reset crawler settings to defaults
    websiteCrawler.setMaxDepth(3);
    websiteCrawler.setMaxPages(100);
    websiteCrawler.setDelay(0);
    websiteCrawler.setFollowExternalLinks(false);
    websiteCrawler.setRespectRobotsTxt(true);
    websiteCrawler.clearUrlFilters();
    
    // Parse options
    std::string directory;
    std::vector<std::string> extensions;
    bool downloadFiles = false;
    
    for (size_t i = 1; i < args.size(); i++) {
        if ((args[i] == "-d" || args[i] == "--dir") && i + 1 < args.size()) {
            directory = args[++i];
        } 
        else if ((args[i] == "-e" || args[i] == "--ext") && i + 1 < args.size()) {
            // Parse comma-separated extensions
            std::string extList = args[++i];
            std::istringstream iss(extList);
            std::string ext;
            
            while (std::getline(iss, ext, ',')) {
                // Ensure extension starts with a dot
                if (!ext.empty() && ext[0] != '.') {
                    ext = "." + ext;
                }
                
                extensions.push_back(ext);
            }
        } 
        else if ((args[i] == "-f" || args[i] == "--filter") && i + 1 < args.size()) {
            websiteCrawler.addUrlFilter(args[++i]);
        } 
        else if (args[i] == "--depth" && i + 1 < args.size()) {
            try {
                int depth = std::stoi(args[++i]);
                if (depth >= 0) {
                    websiteCrawler.setMaxDepth(depth);
                }
            } catch (...) {
                std::cout << "Warning: Invalid depth value" << std::endl;
            }
        } 
        else if (args[i] == "--max-pages" && i + 1 < args.size()) {
            try {
                int maxPages = std::stoi(args[++i]);
                if (maxPages >= 0) {
                    websiteCrawler.setMaxPages(maxPages);
                }
            } catch (...) {
                std::cout << "Warning: Invalid max-pages value" << std::endl;
            }
        } 
        else if (args[i] == "--delay" && i + 1 < args.size()) {
            try {
                int delay = std::stoi(args[++i]);
                if (delay >= 0) {
                    websiteCrawler.setDelay(delay);
                }
            } catch (...) {
                std::cout << "Warning: Invalid delay value" << std::endl;
            }
        } 
        else if (args[i] == "--external") {
            websiteCrawler.setFollowExternalLinks(true);
        } 
        else if (args[i] == "--no-robots") {
            websiteCrawler.setRespectRobotsTxt(false);
        } 
        else if (args[i] == "--download") {
            downloadFiles = true;
        }
    }
    
    // Set file extensions if specified
    if (!extensions.empty()) {
        std::set<std::string> extSet(extensions.begin(), extensions.end());
        websiteCrawler.setFileExtensions(extSet);
    }
    
    // Add progress callback
    websiteCrawler.addCrawlProgressCallback([](int pagesVisited, int queueSize, const std::string& currentUrl, int filesFound) {
        std::cout << "Crawling page " << pagesVisited << ", " << queueSize << " in queue, " 
                << filesFound << " files found. URL: " << currentUrl << std::endl;
    });
    
    // Add file found callback
    if (downloadFiles) {
        auto& downloadManager = Core::DownloadManager::instance();
        
        websiteCrawler.addFileFoundCallback([&downloadManager, &directory](const std::string& fileUrl) {
            std::cout << "Found file: " << fileUrl << ", starting download..." << std::endl;
            
            // Start download
            Core::DownloadOptions options;
            options.url = fileUrl;
            
            if (!directory.empty()) {
                options.destination = directory;
            }
            
            try {
                auto task = downloadManager.addDownload(options);
                if (task) {
                    std::cout << "Download started: ID=" << task->getId() << ", URL=" << fileUrl << std::endl;
                } else {
                    std::cout << "Failed to start download for: " << fileUrl << std::endl;
                }
            } 
            catch (const std::exception& e) {
                std::cout << "Error starting download: " << e.what() << std::endl;
            }
        });
    } 
    else {
        websiteCrawler.addFileFoundCallback([](const std::string& fileUrl) {
            std::cout << "Found file: " << fileUrl << std::endl;
        });
    }
    
    // Start crawler
    std::cout << "Starting crawler with URL: " << url << std::endl;
    std::cout << "Depth: " << websiteCrawler.getMaxDepth() 
             << ", Max Pages: " << websiteCrawler.getMaxPages() 
             << ", Delay: " << websiteCrawler.getDelay() << "ms" << std::endl;
    
    websiteCrawler.start(url);
    
    // Wait for crawler to complete
    std::cout << "Crawler started. Press Enter to stop..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (websiteCrawler.isRunning()) {
        websiteCrawler.stop();
        std::cout << "Crawler stopped" << std::endl;
    }
    
    // Display results
    auto downloadableFiles = websiteCrawler.getDownloadableFiles();
    auto visitedUrls = websiteCrawler.getVisitedUrls();
    
    std::cout << "Crawl Results:" << std::endl;
    std::cout << "  Pages Visited: " << visitedUrls.size() << std::endl;
    std::cout << "  Files Found: " << downloadableFiles.size() << std::endl;
    
    if (!downloadableFiles.empty() && !downloadFiles) {
        std::cout << "Downloadable Files:" << std::endl;
        
        for (const auto& file : downloadableFiles) {
            std::cout << "  " << file << std::endl;
        }
        
        std::cout << "To download these files, use --download option or copy URLs to batch file" << std::endl;
    }
}

void CommandLineInterface::cmdSettings(const std::vector<std::string>& args) {
    auto& downloadManager = Core::DownloadManager::instance();
    
    if (args.empty()) {
        // Show all settings
        std::cout << "Current Settings:" << std::endl;
        std::cout << "  download_dir: " << downloadManager.getDefaultDownloadDirectory() << std::endl;
        std::cout << "  max_downloads: " << downloadManager.getMaxConcurrentDownloads() << std::endl;
        std::cout << "  max_speed: " << (downloadManager.getGlobalSpeedLimit() / 1024) << " KB/s" << std::endl;
        std::cout << "  segments: " << downloadManager.getDefaultSegmentCount() << std::endl;
        std::cout << "  auto_resume: " << (downloadManager.getAutoResumeDownloads() ? "1" : "0") << std::endl;
    } 
    else if (args.size() == 1) {
        // Show specific setting
        std::string key = Utils::StringUtils::toLower(args[0]);
        
        if (key == "download_dir") {
            std::cout << "download_dir: " << downloadManager.getDefaultDownloadDirectory() << std::endl;
        } 
        else if (key == "max_downloads") {
            std::cout << "max_downloads: " << downloadManager.getMaxConcurrentDownloads() << std::endl;
        } 
        else if (key == "max_speed") {
            std::cout << "max_speed: " << (downloadManager.getGlobalSpeedLimit() / 1024) << " KB/s" << std::endl;
        } 
        else if (key == "segments") {
            std::cout << "segments: " << downloadManager.getDefaultSegmentCount() << std::endl;
        } 
        else if (key == "auto_resume") {
            std::cout << "auto_resume: " << (downloadManager.getAutoResumeDownloads() ? "1" : "0") << std::endl;
        } 
        else {
            std::cout << "Unknown setting: " << key << std::endl;
        }
    } 
    else {
        // Change setting
        std::string key = Utils::StringUtils::toLower(args[0]);
        std::string value = args[1];
        
        if (key == "download_dir") {
            downloadManager.setDefaultDownloadDirectory(value);
            std::cout << "download_dir set to: " << value << std::endl;
        } 
        else if (key == "max_downloads") {
            try {
                int maxDownloads = std::stoi(value);
                if (maxDownloads > 0) {
                    downloadManager.setMaxConcurrentDownloads(maxDownloads);
                    std::cout << "max_downloads set to: " << maxDownloads << std::endl;
                } else {
                    std::cout << "Error: max_downloads must be greater than 0" << std::endl;
                }
            } 
            catch (const std::exception& e) {
                std::cout << "Error: Invalid value for max_downloads" << std::endl;
            }
        } 
        else if (key == "max_speed") {
            try {
                int maxSpeed = std::stoi(value);
                if (maxSpeed >= 0) {
                    downloadManager.setGlobalSpeedLimit(maxSpeed * 1024); // Convert KB/s to bytes/s
                    std::cout << "max_speed set to: " << maxSpeed << " KB/s" << std::endl;
                } else {
                    std::cout << "Error: max_speed must be non-negative" << std::endl;
                }
            } 
            catch (const std::exception& e) {
                std::cout << "Error: Invalid value for max_speed" << std::endl;
            }
        } 
        else if (key == "segments") {
            try {
                int segments = std::stoi(value);
                if (segments > 0 && segments <= 16) {
                    downloadManager.setDefaultSegmentCount(segments);
                    std::cout << "segments set to: " << segments << std::endl;
                } else {
                    std::cout << "Error: segments must be between 1 and 16" << std::endl;
                }
            } 
            catch (const std::exception& e) {
                std::cout << "Error: Invalid value for segments" << std::endl;
            }
        } 
        else if (key == "auto_resume") {
            try {
                int autoResume = std::stoi(value);
                downloadManager.setAutoResumeDownloads(autoResume != 0);
                std::cout << "auto_resume set to: " << (autoResume != 0 ? "1" : "0") << std::endl;
            } 
            catch (const std::exception& e) {
                std::cout << "Error: Invalid value for auto_resume" << std::endl;
            }
        } 
        else {
            std::cout << "Unknown setting: " << key << std::endl;
        }
    }
}

void CommandLineInterface::cmdProgress(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Progress display is currently " << (m_showProgress ? "ON" : "OFF") << std::endl;
        std::cout << "Usage: progress <on|off>" << std::endl;
        return;
    }
    
    std::string option = Utils::StringUtils::toLower(args[0]);
    
    if (option == "on") {
        m_showProgress = true;
        std::cout << "Progress display turned ON" << std::endl;
    } 
    else if (option == "off") {
        m_showProgress = false;
        std::cout << "Progress display turned OFF" << std::endl;
    } 
    else {
        std::cout << "Invalid option: " << option << std::endl;
        std::cout << "Usage: progress <on|off>" << std::endl;
    }
}

void CommandLineInterface::cmdQuit(const std::vector<std::string>& args) {
    std::cout << "Exiting Download Manager..." << std::endl;
    m_isRunning = false;
}

} // namespace CLI
} // namespace DownloadManager