#include "core/DownloadManager.h"
#include "utils/Logger.h"
#include "utils/FileUtils.h"
#include "utils/UrlParser.h"

#include <fstream>
#include <json/json.h> // Using jsoncpp library for task serialization

namespace dm {
namespace core {

DownloadManager& DownloadManager::getInstance() {
    static DownloadManager instance;
    return instance;
}

DownloadManager::DownloadManager()
    : running_(false) {
    settings_ = std::make_shared<Settings>();
    queue_ = std::make_shared<DownloadQueue>();
}

DownloadManager::~DownloadManager() {
    shutdown();
}

bool DownloadManager::initialize() {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    // Already initialized
    if (running_) {
        return true;
    }
    
    // Create directories
    std::string appDataDir = dm::utils::FileUtils::getAppDataDirectory();
    if (!dm::utils::FileUtils::createDirectory(appDataDir)) {
        dm::utils::Logger::error("Failed to create application data directory: " + appDataDir);
        return false;
    }
    
    // Load settings
    settings_->load();
    
    // Set queue settings
    queue_->setMaxConcurrentDownloads(settings_->getMaxConcurrentDownloads());
    
    // Set queue processor callback
    queue_->setQueueProcessorCallback([this]() {
        // This is called when the queue is processed
        // Here we can add additional logic if needed
    });
    
    // Load tasks
    if (!loadTasks()) {
        dm::utils::Logger::warning("Failed to load saved tasks");
        // Continue anyway, not critical
    }
    
    // Start queue processor thread
    running_ = true;
    queueProcessorThread_ = std::make_unique<std::thread>(&DownloadManager::queueProcessorThread, this);
    
    dm::utils::Logger::info("Download manager initialized");
    return true;
}

void DownloadManager::shutdown() {
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        
        if (!running_) {
            return;
        }
        
        // Set running flag to false
        running_ = false;
    }
    
    // Save tasks
    saveTasks();
    
    // Wait for queue processor thread to finish
    if (queueProcessorThread_ && queueProcessorThread_->joinable()) {
        queueProcessorThread_->join();
    }
    
    // Save settings
    settings_->save();
    
    dm::utils::Logger::info("Download manager shutdown");
}

std::shared_ptr<DownloadTask> DownloadManager::addDownload(const std::string& url, 
                                                         const std::string& destinationPath,
                                                         const std::string& filename,
                                                         bool start) {
    // Validate URL
    if (url.empty()) {
        dm::utils::Logger::error("Empty URL provided");
        return nullptr;
    }
    
    // Parse URL
    dm::utils::UrlInfo urlInfo = dm::utils::UrlParser::parse(url);
    if (!urlInfo.isValid()) {
        dm::utils::Logger::error("Invalid URL: " + url);
        return nullptr;
    }
    
    // Determine destination path
    std::string finalDestinationPath;
    if (destinationPath.empty()) {
        finalDestinationPath = settings_->getDownloadDirectory();
    } else {
        finalDestinationPath = destinationPath;
    }
    
    // Ensure destination directory exists
    if (!dm::utils::FileUtils::createDirectory(finalDestinationPath)) {
        dm::utils::Logger::error("Failed to create destination directory: " + finalDestinationPath);
        return nullptr;
    }
    
    // Determine filename
    std::string finalFilename;
    if (filename.empty()) {
        finalFilename = urlInfo.filename;
        if (finalFilename.empty()) {
            finalFilename = "download"; // Fallback name
        }
    } else {
        finalFilename = filename;
    }
    
    // Create download task
    auto task = std::make_shared<DownloadTask>(url, finalDestinationPath, finalFilename);
    
    // Set segment count from settings
    task->setSegmentCount(settings_->getSegmentCount());
    
    // Initialize task
    if (!task->initialize()) {
        dm::utils::Logger::error("Failed to initialize download task: " + url);
        return nullptr;
    }
    
    // Add to queue
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        tasks_[task->getId()] = task;
    }
    
    // Add to download queue
    queue_->addTask(task);
    
    // Call task added callback
    if (taskAddedCallback_) {
        taskAddedCallback_(task);
    }
    
    // Start download if requested
    if (start) {
        startDownload(task->getId());
    }
    
    dm::utils::Logger::info("Added download: " + url + " -> " + finalDestinationPath + "/" + finalFilename);
    return task;
}

std::vector<std::shared_ptr<DownloadTask>> DownloadManager::addBatchDownload(
    const std::vector<std::string>& urls,
    const std::string& destinationPath,
    bool start) {
    
    std::vector<std::shared_ptr<DownloadTask>> addedTasks;
    
    for (const auto& url : urls) {
        auto task = addDownload(url, destinationPath, "", false);
        if (task) {
            addedTasks.push_back(task);
        }
    }
    
    // Start tasks if requested
    if (start) {
        for (const auto& task : addedTasks) {
            startDownload(task->getId());
        }
    }
    
    return addedTasks;
}

bool DownloadManager::startDownload(const std::string& taskId) {
    return queue_->startTask(taskId);
}

bool DownloadManager::pauseDownload(const std::string& taskId) {
    return queue_->pauseTask(taskId);
}

bool DownloadManager::resumeDownload(const std::string& taskId) {
    return queue_->resumeTask(taskId);
}

bool DownloadManager::cancelDownload(const std::string& taskId) {
    return queue_->cancelTask(taskId);
}

bool DownloadManager::removeDownload(const std::string& taskId, bool deleteFile) {
    // Get task
    auto task = getDownloadTask(taskId);
    if (!task) {
        return false;
    }
    
    // Delete file if requested
    if (deleteFile) {
        std::string filePath = task->getDestinationPath() + "/" + task->getFilename();
        if (dm::utils::FileUtils::fileExists(filePath)) {
            if (!dm::utils::FileUtils::deleteFile(filePath)) {
                dm::utils::Logger::warning("Failed to delete file: " + filePath);
                // Continue anyway
            }
        }
    }
    
    // Remove from queue
    bool removed = queue_->removeTask(taskId);
    
    // Remove from tasks map
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        tasks_.erase(taskId);
    }
    
    // Call task removed callback
    if (removed && taskRemovedCallback_) {
        taskRemovedCallback_(task);
    }
    
    return removed;
}

void DownloadManager::startAllDownloads() {
    queue_->startAllTasks();
}

void DownloadManager::pauseAllDownloads() {
    queue_->pauseAllTasks();
}

void DownloadManager::resumeAllDownloads() {
    queue_->resumeAllTasks();
}

void DownloadManager::cancelAllDownloads() {
    queue_->cancelAllTasks();
}

std::shared_ptr<DownloadTask> DownloadManager::getDownloadTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<DownloadTask>> DownloadManager::getAllDownloadTasks() {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    std::vector<std::shared_ptr<DownloadTask>> result;
    result.reserve(tasks_.size());
    
    for (const auto& pair : tasks_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<DownloadTask>> DownloadManager::getDownloadTasksByStatus(DownloadStatus status) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    std::vector<std::shared_ptr<DownloadTask>> result;
    
    for (const auto& pair : tasks_) {
        if (pair.second->getStatus() == status) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

void DownloadManager::setTaskAddedCallback(TaskAddedCallback callback) {
    taskAddedCallback_ = callback;
}

void DownloadManager::setTaskRemovedCallback(TaskRemovedCallback callback) {
    taskRemovedCallback_ = callback;
}

void DownloadManager::setTaskStatusChangedCallback(TaskStatusChangedCallback callback) {
    taskStatusChangedCallback_ = callback;
    
    // Set the callback for existing tasks
    std::lock_guard<std::mutex> lock(tasksMutex_);
    for (const auto& pair : tasks_) {
        pair.second->setStatusChangeCallback([this, callback](DownloadStatus status) {
            auto task = std::static_pointer_cast<DownloadTask>(
                std::shared_ptr<void>(nullptr, [](void*) {})); // Create a fake shared_ptr to this
            this->onTaskStatusChanged(task, status);
        });
    }
}

std::shared_ptr<Settings> DownloadManager::getSettings() {
    return settings_;
}

bool DownloadManager::loadTasks() {
    std::string tasksFile = dm::utils::FileUtils::getAppDataDirectory() + "/tasks.json";
    
    // Check if file exists
    if (!dm::utils::FileUtils::fileExists(tasksFile)) {
        return true; // No tasks to load
    }
    
    try {
        // Read JSON file
        std::string jsonStr = dm::utils::FileUtils::readTextFile(tasksFile);
        if (jsonStr.empty()) {
            return false;
        }
        
        // Parse JSON
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(jsonStr, root)) {
            dm::utils::Logger::error("Failed to parse tasks JSON: " + reader.getFormattedErrorMessages());
            return false;
        }
        
        // Load tasks
        Json::Value tasksArray = root["tasks"];
        for (unsigned int i = 0; i < tasksArray.size(); i++) {
            Json::Value taskJson = tasksArray[i];
            
            // Extract task data
            std::string url = taskJson["url"].asString();
            std::string destinationPath = taskJson["destinationPath"].asString();
            std::string filename = taskJson["filename"].asString();
            std::string statusStr = taskJson["status"].asString();
            
            // Skip completed tasks
            if (statusStr == "COMPLETED") {
                continue;
            }
            
            // Create task
            auto task = std::make_shared<DownloadTask>(url, destinationPath, filename);
            
            // Add to tasks map
            std::string taskId = task->getId();
            tasks_[taskId] = task;
            
            // Add to queue
            queue_->addTask(task);
        }
        
        dm::utils::Logger::info("Loaded tasks from " + tasksFile);
        return true;
    } catch (const std::exception& e) {
        dm::utils::Logger::error("Exception while loading tasks: " + std::string(e.what()));
        return false;
    }
}

bool DownloadManager::saveTasks() {
    std::string tasksFile = dm::utils::FileUtils::getAppDataDirectory() + "/tasks.json";
    
    try {
        // Create JSON root
        Json::Value root;
        Json::Value tasksArray(Json::arrayValue);
        
        // Get all tasks
        std::vector<std::shared_ptr<DownloadTask>> tasks = getAllDownloadTasks();
        
        // Add tasks to JSON array
        for (const auto& task : tasks) {
            // Skip if task is null
            if (!task) {
                continue;
            }
            
            // Create task JSON
            Json::Value taskJson;
            taskJson["id"] = task->getId();
            taskJson["url"] = task->getUrl();
            taskJson["destinationPath"] = task->getDestinationPath();
            taskJson["filename"] = task->getFilename();
            
            // Convert status to string
            std::string statusStr;
            switch (task->getStatus()) {
                case DownloadStatus::NONE:
                    statusStr = "NONE";
                    break;
                case DownloadStatus::QUEUED:
                    statusStr = "QUEUED";
                    break;
                case DownloadStatus::CONNECTING:
                    statusStr = "CONNECTING";
                    break;
                case DownloadStatus::DOWNLOADING:
                    statusStr = "DOWNLOADING";
                    break;
                case DownloadStatus::PAUSED:
                    statusStr = "PAUSED";
                    break;
                case DownloadStatus::COMPLETED:
                    statusStr = "COMPLETED";
                    break;
                case DownloadStatus::ERROR:
                    statusStr = "ERROR";
                    break;
                case DownloadStatus::CANCELED:
                    statusStr = "CANCELED";
                    break;
                default:
                    statusStr = "UNKNOWN";
                    break;
            }
            taskJson["status"] = statusStr;
            
            // Add task to array
            tasksArray.append(taskJson);
        }
        
        // Set tasks array in root
        root["tasks"] = tasksArray;
        
        // Write JSON to file
        Json::StyledWriter writer;
        std::string jsonStr = writer.write(root);
        
        if (!dm::utils::FileUtils::writeTextFile(tasksFile, jsonStr)) {
            dm::utils::Logger::error("Failed to write tasks JSON to file: " + tasksFile);
            return false;
        }
        
        dm::utils::Logger::info("Saved tasks to " + tasksFile);
        return true;
    } catch (const std::exception& e) {
        dm::utils::Logger::error("Exception while saving tasks: " + std::string(e.what()));
        return false;
    }
}

std::string DownloadManager::getDefaultDownloadDirectory() const {
    return settings_->getDownloadDirectory();
}

void DownloadManager::setDefaultDownloadDirectory(const std::string& directory) {
    settings_->setDownloadDirectory(directory);
    settings_->save();
}

int DownloadManager::getMaxConcurrentDownloads() const {
    return settings_->getMaxConcurrentDownloads();
}

void DownloadManager::setMaxConcurrentDownloads(int max) {
    settings_->setMaxConcurrentDownloads(max);
    queue_->setMaxConcurrentDownloads(max);
    settings_->save();
}

void DownloadManager::onTaskStatusChanged(std::shared_ptr<DownloadTask> task, DownloadStatus status) {
    // Call the status change callback if provided
    if (taskStatusChangedCallback_) {
        taskStatusChangedCallback_(task, status);
    }
}

void DownloadManager::queueProcessorThread() {
    while (running_) {
        // Process the queue
        queue_->processQueue();
        
        // Update progress for all tasks
        std::vector<std::shared_ptr<DownloadTask>> tasks = getAllDownloadTasks();
        for (const auto& task : tasks) {
            if (task) {
                task->updateProgress();
            }
        }
        
        // Sleep for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace core
} // namespace dm