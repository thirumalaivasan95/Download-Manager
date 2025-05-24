#include "core/DownloadQueue.h"
#include "utils/Logger.h"

namespace dm {
namespace core {

DownloadQueue::DownloadQueue(int maxConcurrentDownloads)
    : maxConcurrentDownloads_(maxConcurrentDownloads), activeDownloads_(0) {
    // Log queue creation
    dm::utils::Logger::info("Download queue created with max concurrent downloads: " + 
                          std::to_string(maxConcurrentDownloads));
}

DownloadQueue::~DownloadQueue() {
    // Cancel all tasks
    cancelAllTasks();
    
    // Log queue destruction
    dm::utils::Logger::info("Download queue destroyed");
}

void DownloadQueue::addTask(std::shared_ptr<DownloadTask> task) {
    if (!task) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string taskId = task->getId();
    if (tasks_.find(taskId) != tasks_.end()) {
        dm::utils::Logger::warning("Task already exists in queue: " + taskId);
        return;
    }
    tasks_[taskId] = task;
    // Set status change callback with new signature
    task->setStatusChangeCallback([this](std::shared_ptr<DownloadTask> t, DownloadStatus oldStatus, DownloadStatus newStatus) {
        this->onTaskStatusChanged(t, oldStatus, newStatus);
    });
    if (task->getStatus() == DownloadStatus::QUEUED) {
        pendingTasks_.push(taskId);
    }
    dm::utils::Logger::info("Task added to queue: " + taskId);
    processQueue();
}

bool DownloadQueue::removeTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find task
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }
    
    // Get task
    auto task = it->second;
    
    // Cancel task if it's active
    if (task->getStatus() == DownloadStatus::DOWNLOADING) {
        task->cancel();
        activeDownloads_--;
    }
    
    // Remove from map
    tasks_.erase(it);
    
    // Log task removal
    dm::utils::Logger::info("Task removed from queue: " + taskId);
    
    // Process queue
    processQueue();
    
    return true;
}

std::shared_ptr<DownloadTask> DownloadQueue::getTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find task
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return nullptr;
    }
    
    return it->second;
}

std::vector<std::shared_ptr<DownloadTask>> DownloadQueue::getAllTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<DownloadTask>> result;
    result.reserve(tasks_.size());
    
    for (const auto& pair : tasks_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<DownloadTask>> DownloadQueue::getTasksByStatus(DownloadStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<DownloadTask>> result;
    
    for (const auto& pair : tasks_) {
        if (pair.second->getStatus() == status) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool DownloadQueue::startTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find task
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }
    
    // Get task
    auto task = it->second;
    
    // Check if we have available slots
    if (activeDownloads_ >= maxConcurrentDownloads_) {
        // Queue the task instead of starting it
        task->initialize();
        
        // Add to pending tasks
        pendingTasks_.push(taskId);
        
        // Log task queuing
        dm::utils::Logger::info("Task queued due to max concurrent downloads: " + taskId);
        
        return true;
    }
    
    // Start task
    if (task->start()) {
        activeDownloads_++;
        return true;
    }
    
    return false;
}

bool DownloadQueue::pauseTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find task
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }
    
    // Get task
    auto task = it->second;
    
    // Pause task
    if (task->pause()) {
        if (task->getStatus() == DownloadStatus::PAUSED && 
            task->getStatus() == DownloadStatus::DOWNLOADING) {
            activeDownloads_--;
        }
        return true;
    }
    
    return false;
}

bool DownloadQueue::resumeTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find task
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }
    
    // Get task
    auto task = it->second;
    
    // Check if we have available slots
    if (activeDownloads_ >= maxConcurrentDownloads_) {
        // Queue the task instead of resuming it
        pendingTasks_.push(taskId);
        
        // Log task queuing
        dm::utils::Logger::info("Task re-queued due to max concurrent downloads: " + taskId);
        
        return true;
    }
    
    // Resume task
    if (task->resume()) {
        activeDownloads_++;
        return true;
    }
    
    return false;
}

bool DownloadQueue::cancelTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find task
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }
    
    // Get task
    auto task = it->second;
    
    // Cancel task
    if (task->cancel()) {
        if (task->getStatus() == DownloadStatus::DOWNLOADING) {
            activeDownloads_--;
        }
        return true;
    }
    
    return false;
}

void DownloadQueue::startAllTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Start all tasks that aren't already downloading or completed
    for (const auto& pair : tasks_) {
        auto task = pair.second;
        DownloadStatus status = task->getStatus();
        
        if (status != DownloadStatus::DOWNLOADING && 
            status != DownloadStatus::COMPLETED && 
            status != DownloadStatus::CANCELED) {
            
            if (activeDownloads_ < maxConcurrentDownloads_) {
                // Start task
                if (task->start()) {
                    activeDownloads_++;
                }
            } else {
                // Queue task
                task->initialize();
                pendingTasks_.push(pair.first);
            }
        }
    }
    
    // Log action
    dm::utils::Logger::info("Started all tasks");
}

void DownloadQueue::pauseAllTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Pause all downloading tasks
    for (const auto& pair : tasks_) {
        auto task = pair.second;
        
        if (task->getStatus() == DownloadStatus::DOWNLOADING) {
            if (task->pause()) {
                activeDownloads_--;
            }
        }
    }
    
    // Log action
    dm::utils::Logger::info("Paused all tasks");
}

void DownloadQueue::resumeAllTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Resume all paused tasks
    for (const auto& pair : tasks_) {
        auto task = pair.second;
        
        if (task->getStatus() == DownloadStatus::PAUSED) {
            if (activeDownloads_ < maxConcurrentDownloads_) {
                // Resume task
                if (task->resume()) {
                    activeDownloads_++;
                }
            } else {
                // Queue task
                pendingTasks_.push(pair.first);
            }
        }
    }
    
    // Log action
    dm::utils::Logger::info("Resumed all tasks");
}

void DownloadQueue::cancelAllTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Cancel all active tasks
    for (const auto& pair : tasks_) {
        auto task = pair.second;
        
        if (task->getStatus() == DownloadStatus::DOWNLOADING || 
            task->getStatus() == DownloadStatus::PAUSED) {
            if (task->cancel() && task->getStatus() == DownloadStatus::DOWNLOADING) {
                activeDownloads_--;
            }
        }
    }
    
    // Clear pending tasks
    while (!pendingTasks_.empty()) {
        pendingTasks_.pop();
    }
    
    // Log action
    dm::utils::Logger::info("Canceled all tasks");
}

void DownloadQueue::processQueue() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update active downloads count
    activeDownloads_ = countActiveDownloads();
    
    // Process pending tasks
    while (!pendingTasks_.empty() && activeDownloads_ < maxConcurrentDownloads_) {
        // Get next task
        std::string taskId = pendingTasks_.front();
        pendingTasks_.pop();
        
        // Find task
        auto it = tasks_.find(taskId);
        if (it == tasks_.end()) {
            continue;
        }
        
        // Get task
        auto task = it->second;
        
        // Check task status
        DownloadStatus status = task->getStatus();
        
        if (status == DownloadStatus::QUEUED) {
            // Start task
            if (task->start()) {
                activeDownloads_++;
            }
        } else if (status == DownloadStatus::PAUSED) {
            // Resume task
            if (task->resume()) {
                activeDownloads_++;
            }
        }
    }
    
    // Call queue processor callback if provided
    if (queueProcessorCallback_) {
        queueProcessorCallback_();
    }
}

void DownloadQueue::setMaxConcurrentDownloads(int maxConcurrentDownloads) {
    maxConcurrentDownloads_ = maxConcurrentDownloads;
    
    // Log change
    dm::utils::Logger::info("Max concurrent downloads changed to: " + 
                          std::to_string(maxConcurrentDownloads));
    
    // Process queue
    processQueue();
}

int DownloadQueue::getMaxConcurrentDownloads() const {
    return maxConcurrentDownloads_;
}

int DownloadQueue::getActiveDownloadsCount() const {
    return activeDownloads_;
}

void DownloadQueue::setQueueProcessorCallback(QueueProcessorCallback callback) {
    queueProcessorCallback_ = callback;
}

void DownloadQueue::onTaskStatusChanged(std::shared_ptr<DownloadTask> task, DownloadStatus oldStatus, DownloadStatus newStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Update active downloads count based on transitions
    if (newStatus == DownloadStatus::DOWNLOADING && oldStatus != DownloadStatus::DOWNLOADING) {
        activeDownloads_++;
        dm::utils::Logger::debug("Active downloads incremented: " + std::to_string(activeDownloads_));
    } else if (oldStatus == DownloadStatus::DOWNLOADING && newStatus != DownloadStatus::DOWNLOADING) {
        if (activeDownloads_ > 0) activeDownloads_--;
        dm::utils::Logger::debug("Active downloads decremented: " + std::to_string(activeDownloads_));
    }
    // Always process the queue after a status change
    processQueue();
    dm::utils::Logger::info("Task status changed: " + task->getId() + " from " + std::to_string(static_cast<int>(oldStatus)) + " to " + std::to_string(static_cast<int>(newStatus)));
}

int DownloadQueue::countActiveDownloads() {
    int count = 0;
    
    for (const auto& pair : tasks_) {
        if (pair.second->getStatus() == DownloadStatus::DOWNLOADING) {
            count++;
        }
    }
    
    return count;
}

} // namespace core
} // namespace dm