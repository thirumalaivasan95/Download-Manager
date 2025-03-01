#include "../../include/core/DownloadScheduler.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/TimeUtils.h"
#include "../../include/core/DownloadManager.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace DownloadManager {
namespace Core {

// Initialize static members
std::shared_ptr<DownloadScheduler> DownloadScheduler::s_instance = nullptr;
std::mutex DownloadScheduler::s_instanceMutex;

DownloadScheduler::DownloadScheduler()
    : m_isRunning(false),
      m_schedulerThread(nullptr)
{
}

DownloadScheduler::~DownloadScheduler() {
    stop();
}

std::shared_ptr<DownloadScheduler> DownloadScheduler::instance() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        s_instance = std::shared_ptr<DownloadScheduler>(new DownloadScheduler());
    }
    return s_instance;
}

void DownloadScheduler::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_isRunning) {
        return;
    }
    
    m_isRunning = true;
    m_schedulerThread = std::make_unique<std::thread>(&DownloadScheduler::schedulerLoop, this);
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Download scheduler started");
}

void DownloadScheduler::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    m_condition.notify_all();
    
    if (m_schedulerThread && m_schedulerThread->joinable()) {
        m_schedulerThread->join();
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Download scheduler stopped");
}

int DownloadScheduler::scheduleDownload(std::shared_ptr<DownloadTask> task, time_t startTime) {
    if (!task) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, "Cannot schedule null download task");
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Generate schedule ID
    int scheduleId = generateScheduleId();
    
    // Create schedule item
    ScheduleItem item;
    item.id = scheduleId;
    item.taskId = task->getId();
    item.task = task;
    item.startTime = startTime;
    item.repeat = ScheduleRepeat::NONE;
    item.repeatInterval = 0;
    item.active = true;
    
    // Add to schedule map
    m_schedules[scheduleId] = item;
    
    // Log
    std::string startTimeStr = formatTime(startTime);
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Scheduled download task " + std::to_string(task->getId()) + 
        " to start at " + startTimeStr + 
        " (Schedule ID: " + std::to_string(scheduleId) + ")");
    
    // Notify scheduler thread
    m_condition.notify_one();
    
    return scheduleId;
}

int DownloadScheduler::scheduleDownload(const DownloadOptions& options, time_t startTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Generate schedule ID
    int scheduleId = generateScheduleId();
    
    // Create schedule item
    ScheduleItem item;
    item.id = scheduleId;
    item.taskId = -1; // No task yet
    item.task = nullptr;
    item.options = options;
    item.startTime = startTime;
    item.repeat = ScheduleRepeat::NONE;
    item.repeatInterval = 0;
    item.active = true;
    
    // Add to schedule map
    m_schedules[scheduleId] = item;
    
    // Log
    std::string startTimeStr = formatTime(startTime);
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Scheduled download for URL " + options.url + 
        " to start at " + startTimeStr + 
        " (Schedule ID: " + std::to_string(scheduleId) + ")");
    
    // Notify scheduler thread
    m_condition.notify_one();
    
    return scheduleId;
}

int DownloadScheduler::scheduleRecurringDownload(const DownloadOptions& options, 
                                              ScheduleRepeat repeat, 
                                              int repeatInterval, 
                                              time_t startTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Validate repeat parameters
    if (repeat == ScheduleRepeat::NONE || repeatInterval <= 0) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Invalid repeat parameters for recurring download");
        return -1;
    }
    
    // Generate schedule ID
    int scheduleId = generateScheduleId();
    
    // Create schedule item
    ScheduleItem item;
    item.id = scheduleId;
    item.taskId = -1; // No task yet
    item.task = nullptr;
    item.options = options;
    item.startTime = startTime;
    item.repeat = repeat;
    item.repeatInterval = repeatInterval;
    item.active = true;
    
    // Add to schedule map
    m_schedules[scheduleId] = item;
    
    // Log
    std::string startTimeStr = formatTime(startTime);
    std::string repeatStr = getRepeatString(repeat, repeatInterval);
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Scheduled recurring download for URL " + options.url + 
        " to start at " + startTimeStr + 
        " with repeat " + repeatStr + 
        " (Schedule ID: " + std::to_string(scheduleId) + ")");
    
    // Notify scheduler thread
    m_condition.notify_one();
    
    return scheduleId;
}

bool DownloadScheduler::updateSchedule(int scheduleId, time_t newStartTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
            "Schedule ID not found: " + std::to_string(scheduleId));
        return false;
    }
    
    // Update start time
    it->second.startTime = newStartTime;
    
    // Log
    std::string startTimeStr = formatTime(newStartTime);
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Updated schedule " + std::to_string(scheduleId) + 
        " to start at " + startTimeStr);
    
    // Notify scheduler thread
    m_condition.notify_one();
    
    return true;
}

bool DownloadScheduler::updateRecurringSchedule(int scheduleId, 
                                             ScheduleRepeat repeat, 
                                             int repeatInterval) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
            "Schedule ID not found: " + std::to_string(scheduleId));
        return false;
    }
    
    // Validate repeat parameters
    if (repeat == ScheduleRepeat::NONE && repeatInterval > 0) {
        Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
            "Invalid repeat parameters for schedule update");
        return false;
    }
    
    // Update repeat settings
    it->second.repeat = repeat;
    it->second.repeatInterval = repeatInterval;
    
    // Log
    std::string repeatStr = getRepeatString(repeat, repeatInterval);
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Updated schedule " + std::to_string(scheduleId) + 
        " with repeat " + repeatStr);
    
    return true;
}

bool DownloadScheduler::cancelSchedule(int scheduleId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
            "Schedule ID not found: " + std::to_string(scheduleId));
        return false;
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Cancelled schedule " + std::to_string(scheduleId));
    
    // Remove schedule
    m_schedules.erase(it);
    
    return true;
}

bool DownloadScheduler::pauseSchedule(int scheduleId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
            "Schedule ID not found: " + std::to_string(scheduleId));
        return false;
    }
    
    if (!it->second.active) {
        // Already paused
        return true;
    }
    
    // Pause schedule
    it->second.active = false;
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Paused schedule " + std::to_string(scheduleId));
    
    return true;
}

bool DownloadScheduler::resumeSchedule(int scheduleId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        Utils::Logger::instance().log(Utils::LogLevel::WARNING, 
            "Schedule ID not found: " + std::to_string(scheduleId));
        return false;
    }
    
    if (it->second.active) {
        // Already active
        return true;
    }
    
    // Resume schedule
    it->second.active = true;
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, 
        "Resumed schedule " + std::to_string(scheduleId));
    
    // Notify scheduler thread
    m_condition.notify_one();
    
    return true;
}

std::vector<ScheduleInfo> DownloadScheduler::getAllSchedules() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ScheduleInfo> result;
    
    for (const auto& pair : m_schedules) {
        const ScheduleItem& item = pair.second;
        
        ScheduleInfo info;
        info.id = item.id;
        info.taskId = item.taskId;
        
        if (item.task) {
            info.url = item.task->getUrl();
        } else {
            info.url = item.options.url;
        }
        
        info.startTime = item.startTime;
        info.repeat = item.repeat;
        info.repeatInterval = item.repeatInterval;
        info.active = item.active;
        
        result.push_back(info);
    }
    
    return result;
}

ScheduleInfo DownloadScheduler::getSchedule(int scheduleId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    ScheduleInfo info;
    info.id = -1; // Invalid by default
    
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        return info;
    }
    
    const ScheduleItem& item = it->second;
    
    info.id = item.id;
    info.taskId = item.taskId;
    
    if (item.task) {
        info.url = item.task->getUrl();
    } else {
        info.url = item.options.url;
    }
    
    info.startTime = item.startTime;
    info.repeat = item.repeat;
    info.repeatInterval = item.repeatInterval;
    info.active = item.active;
    
    return info;
}

void DownloadScheduler::schedulerLoop() {
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Scheduler thread started");
    
    while (m_isRunning) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // Find the next schedule to execute
        time_t now = time(nullptr);
        time_t nextScheduleTime = findNextScheduleTime();
        
        if (nextScheduleTime == 0) {
            // No schedules, wait for notification or timeout
            m_condition.wait_for(lock, std::chrono::minutes(1));
            continue;
        }
        
        if (nextScheduleTime <= now) {
            // Execute due schedules
            executeDueSchedules();
            
            // Wait a short time before checking again
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            // Wait until next schedule time or notification
            auto waitTime = std::chrono::seconds(nextScheduleTime - now);
            m_condition.wait_for(lock, waitTime);
        }
    }
    
    Utils::Logger::instance().log(Utils::LogLevel::INFO, "Scheduler thread stopped");
}

time_t DownloadScheduler::findNextScheduleTime() {
    time_t nextTime = 0;
    time_t now = time(nullptr);
    
    for (const auto& pair : m_schedules) {
        const ScheduleItem& item = pair.second;
        
        // Skip inactive schedules
        if (!item.active) {
            continue;
        }
        
        // If schedule is in the future
        if (item.startTime > now) {
            if (nextTime == 0 || item.startTime < nextTime) {
                nextTime = item.startTime;
            }
        }
    }
    
    return nextTime;
}

void DownloadScheduler::executeDueSchedules() {
    time_t now = time(nullptr);
    std::vector<int> schedulesToUpdate;
    
    // Find and execute all due schedules
    for (auto& pair : m_schedules) {
        ScheduleItem& item = pair.second;
        
        // Skip inactive schedules
        if (!item.active) {
            continue;
        }
        
        // Check if it's time to execute
        if (item.startTime <= now) {
            // Execute schedule
            executeSchedule(item);
            
            // If it's a recurring schedule, update for next execution
            if (item.repeat != ScheduleRepeat::NONE && item.repeatInterval > 0) {
                // Calculate next execution time
                time_t nextTime = calculateNextExecutionTime(item);
                
                // Update schedule
                item.startTime = nextTime;
                
                Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                    "Rescheduled schedule " + std::to_string(item.id) + 
                    " for next execution at " + formatTime(nextTime));
            } else {
                // One-time schedule, mark for removal
                schedulesToUpdate.push_back(item.id);
            }
        }
    }
    
    // Remove completed one-time schedules
    for (int scheduleId : schedulesToUpdate) {
        m_schedules.erase(scheduleId);
        
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Removed completed one-time schedule " + std::to_string(scheduleId));
    }
}

void DownloadScheduler::executeSchedule(ScheduleItem& item) {
    // If task already exists, just start/resume it
    if (item.task) {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Executing schedule " + std::to_string(item.id) + 
            " for existing task " + std::to_string(item.taskId));
        
        // Get download manager
        auto& downloadManager = DownloadManager::instance();
        
        // Check current task status
        DownloadStatus status = item.task->getStatus();
        
        if (status == DownloadStatus::QUEUED || 
            status == DownloadStatus::PAUSED) {
            // Resume/start the task
            downloadManager.resumeDownload(item.taskId);
        } else if (status == DownloadStatus::COMPLETED || 
                 status == DownloadStatus::FAILED) {
            // Restart the task
            downloadManager.restartDownload(item.taskId);
        }
        // If already downloading, do nothing
    } else {
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Executing schedule " + std::to_string(item.id) + 
            " by creating new download task for URL " + item.options.url);
        
        // Get download manager
        auto& downloadManager = DownloadManager::instance();
        
        try {
            // Create new download task
            std::shared_ptr<DownloadTask> task = downloadManager.addDownload(item.options);
            
            if (task) {
                // Store task reference
                item.task = task;
                item.taskId = task->getId();
                
                Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                    "Created download task " + std::to_string(item.taskId) + 
                    " for schedule " + std::to_string(item.id));
            } else {
                Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                    "Failed to create download task for schedule " + std::to_string(item.id));
            }
        } catch (const std::exception& e) {
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                "Exception creating download task for schedule " + 
                std::to_string(item.id) + ": " + e.what());
        }
    }
}

time_t DownloadScheduler::calculateNextExecutionTime(const ScheduleItem& item) {
    time_t nextTime = item.startTime;
    
    switch (item.repeat) {
        case ScheduleRepeat::HOURLY:
            nextTime += item.repeatInterval * 3600; // seconds in an hour
            break;
        case ScheduleRepeat::DAILY:
            nextTime += item.repeatInterval * 86400; // seconds in a day
            break;
        case ScheduleRepeat::WEEKLY:
            nextTime += item.repeatInterval * 604800; // seconds in a week
            break;
        case ScheduleRepeat::MONTHLY: {
            // This is approximate (not accounting for varying month lengths)
            nextTime += item.repeatInterval * 2592000; // ~seconds in a 30-day month
            break;
        }
        case ScheduleRepeat::CUSTOM:
            nextTime += item.repeatInterval; // Custom interval in seconds
            break;
        default:
            // No repeat
            break;
    }
    
    return nextTime;
}

int DownloadScheduler::generateScheduleId() {
    static int nextId = 1;
    return nextId++;
}

std::string DownloadScheduler::formatTime(time_t time) {
    struct tm* timeinfo = localtime(&time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

std::string DownloadScheduler::getRepeatString(ScheduleRepeat repeat, int interval) {
    std::string result;
    
    switch (repeat) {
        case ScheduleRepeat::NONE:
            result = "none";
            break;
        case ScheduleRepeat::HOURLY:
            if (interval == 1) {
                result = "hourly";
            } else {
                result = "every " + std::to_string(interval) + " hours";
            }
            break;
        case ScheduleRepeat::DAILY:
            if (interval == 1) {
                result = "daily";
            } else {
                result = "every " + std::to_string(interval) + " days";
            }
            break;
        case ScheduleRepeat::WEEKLY:
            if (interval == 1) {
                result = "weekly";
            } else {
                result = "every " + std::to_string(interval) + " weeks";
            }
            break;
        case ScheduleRepeat::MONTHLY:
            if (interval == 1) {
                result = "monthly";
            } else {
                result = "every " + std::to_string(interval) + " months";
            }
            break;
        case ScheduleRepeat::CUSTOM:
            result = "custom interval of " + std::to_string(interval) + " seconds";
            break;
        default:
            result = "unknown";
    }
    
    return result;
}

} // namespace Core
} // namespace DownloadManager