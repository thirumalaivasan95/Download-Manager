#ifndef DOWNLOAD_SCHEDULER_H
#define DOWNLOAD_SCHEDULER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

namespace dm {
namespace core {

// Forward declarations
class DownloadManager;
class DownloadTask;

/**
 * @brief Day of week enumeration
 */
enum class DayOfWeek {
    SUNDAY = 0,
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6
};

/**
 * @brief Schedule recurrence type
 */
enum class RecurrenceType {
    ONCE,           // Run once at specified time
    DAILY,          // Run every day at specified time
    WEEKLY,         // Run on specified days of the week
    MONTHLY,        // Run on specified day of the month
    INTERVAL        // Run at specified interval
};

/**
 * @brief Scheduled action type
 */
enum class ScheduledActionType {
    START_DOWNLOAD,     // Start a download
    PAUSE_DOWNLOAD,     // Pause a download
    RESUME_DOWNLOAD,    // Resume a download
    LIMIT_BANDWIDTH,    // Limit bandwidth
    UNLIMITED_BANDWIDTH // Remove bandwidth limit
};

/**
 * @brief Schedule entry structure
 */
struct ScheduleEntry {
    std::string id;                  // Unique identifier
    std::string name;                // User-friendly name
    RecurrenceType recurrenceType;   // How often to run
    std::string taskId;              // Associated download task ID (if any)
    ScheduledActionType actionType;  // Type of action to perform
    
    // Timing parameters
    int hour;                        // Hour (0-23)
    int minute;                      // Minute (0-59)
    int dayOfMonth;                  // Day of month (1-31)
    std::vector<DayOfWeek> daysOfWeek; // Days of week (for weekly recurrence)
    int intervalMinutes;             // Interval in minutes (for interval recurrence)
    
    // Specific parameters for actions
    int bandwidthLimit;              // Bandwidth limit in KB/s (for LIMIT_BANDWIDTH)
    
    // Execution tracking
    bool enabled;                    // Whether the schedule is enabled
    std::chrono::system_clock::time_point lastRun; // When the schedule last ran
    std::chrono::system_clock::time_point nextRun; // When the schedule will next run
};

/**
 * @brief Schedule event callback function type
 */
using ScheduleEventCallback = std::function<void(const ScheduleEntry& entry)>;

/**
 * @brief Download scheduler class
 * 
 * Manages scheduling of download-related actions
 */
class DownloadScheduler {
public:
    /**
     * @brief Construct a new DownloadScheduler
     * 
     * @param downloadManager Reference to the download manager
     */
    explicit DownloadScheduler(DownloadManager& downloadManager);
    
    /**
     * @brief Destroy the DownloadScheduler
     */
    ~DownloadScheduler();
    
    /**
     * @brief Start the scheduler
     */
    void start();
    
    /**
     * @brief Stop the scheduler
     */
    void stop();
    
    /**
     * @brief Add a schedule entry
     * 
     * @param entry The schedule entry to add
     * @return std::string The ID of the added entry
     */
    std::string addSchedule(const ScheduleEntry& entry);
    
    /**
     * @brief Update a schedule entry
     * 
     * @param entry The schedule entry to update
     * @return true if the entry was updated, false otherwise
     */
    bool updateSchedule(const ScheduleEntry& entry);
    
    /**
     * @brief Remove a schedule entry
     * 
     * @param id The ID of the entry to remove
     * @return true if the entry was removed, false otherwise
     */
    bool removeSchedule(const std::string& id);
    
    /**
     * @brief Get a schedule entry
     * 
     * @param id The ID of the entry to get
     * @return ScheduleEntry The schedule entry, or an empty entry if not found
     */
    ScheduleEntry getSchedule(const std::string& id) const;
    
    /**
     * @brief Get all schedule entries
     * 
     * @return std::vector<ScheduleEntry> All schedule entries
     */
    std::vector<ScheduleEntry> getAllSchedules() const;
    
    /**
     * @brief Enable a schedule entry
     * 
     * @param id The ID of the entry to enable
     * @return true if the entry was enabled, false otherwise
     */
    bool enableSchedule(const std::string& id);
    
    /**
     * @brief Disable a schedule entry
     * 
     * @param id The ID of the entry to disable
     * @return true if the entry was disabled, false otherwise
     */
    bool disableSchedule(const std::string& id);
    
    /**
     * @brief Check if the scheduler is running
     * 
     * @return true if the scheduler is running, false otherwise
     */
    bool isRunning() const;
    
    /**
     * @brief Set the event callback
     * 
     * Called when a scheduled action is executed
     * 
     * @param callback The callback function
     */
    void setEventCallback(ScheduleEventCallback callback);
    
    /**
     * @brief Execute a schedule entry immediately
     * 
     * @param id The ID of the entry to execute
     * @return true if the entry was executed, false otherwise
     */
    bool executeScheduleNow(const std::string& id);
    
    /**
     * @brief Load schedules from file
     * 
     * @param filePath The file path
     * @return true if successful, false otherwise
     */
    bool loadSchedules(const std::string& filePath = "");
    
    /**
     * @brief Save schedules to file
     * 
     * @param filePath The file path
     * @return true if successful, false otherwise
     */
    bool saveSchedules(const std::string& filePath = "");
    
    /**
     * @brief Get schedule entry by task ID
     * 
     * @param taskId The task ID
     * @return std::vector<ScheduleEntry> Schedule entries for the task
     */
    std::vector<ScheduleEntry> getSchedulesByTaskId(const std::string& taskId) const;
    
    /**
     * @brief Get schedule entries by action type
     * 
     * @param actionType The action type
     * @return std::vector<ScheduleEntry> Schedule entries for the action type
     */
    std::vector<ScheduleEntry> getSchedulesByActionType(ScheduledActionType actionType) const;
    
private:
    /**
     * @brief Process scheduled actions
     */
    void processSchedules();
    
    /**
     * @brief Execute a scheduled action
     * 
     * @param entry The schedule entry to execute
     * @return true if successful, false otherwise
     */
    bool executeAction(const ScheduleEntry& entry);
    
    /**
     * @brief Update next run time for a schedule entry
     * 
     * @param entry The schedule entry to update
     */
    void updateNextRunTime(ScheduleEntry& entry);
    
    /**
     * @brief Generate a unique ID
     * 
     * @return std::string The generated ID
     */
    std::string generateUniqueId() const;
    
    /**
     * @brief Get the scheduler file path
     * 
     * @return std::string The file path
     */
    std::string getSchedulerFilePath() const;
    
    /**
     * @brief Check if a day of week is in a list
     * 
     * @param dayOfWeek The day of week to check
     * @param daysOfWeek The list of days of week
     * @return true if the day is in the list, false otherwise
     */
    bool isDayInList(DayOfWeek dayOfWeek, const std::vector<DayOfWeek>& daysOfWeek) const;
    
    /**
     * @brief Get the current day of week
     * 
     * @return DayOfWeek The current day of week
     */
    DayOfWeek getCurrentDayOfWeek() const;
    
    // Member variables
    DownloadManager& downloadManager_;
    std::atomic<bool> running_;
    
    std::unique_ptr<std::thread> schedulerThread_;
    std::mutex mutex_;
    
    std::map<std::string, ScheduleEntry> schedules_;
    ScheduleEventCallback eventCallback_;
    
    std::chrono::seconds checkInterval_;
};

} // namespace core
} // namespace dm

#endif // DOWNLOAD_SCHEDULER_H