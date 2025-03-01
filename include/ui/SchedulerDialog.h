#ifndef SCHEDULER_DIALOG_H
#define SCHEDULER_DIALOG_H

#include <QDialog>
#include <QPoint>
#include <QString>

class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QComboBox;
class QTableWidget;
class QDateTimeEdit;
class QTimer;

namespace DownloadManager {
namespace Core {
enum class ScheduleRepeat;
}

namespace UI {

/**
 * @brief Dialog for managing scheduled downloads
 */
class SchedulerDialog : public QDialog {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new SchedulerDialog
     * 
     * @param parent The parent widget
     */
    explicit SchedulerDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the SchedulerDialog
     */
    ~SchedulerDialog();
    
signals:
    /**
     * @brief Signal emitted when user requests to view a download task
     * 
     * @param taskId The task ID to view
     */
    void viewDownloadTask(int taskId);
    
private slots:
    /**
     * @brief Browse for destination directory
     */
    void browseDestination();
    
    /**
     * @brief Add a new scheduled download
     */
    void addSchedule();
    
    /**
     * @brief Update the schedule table
     */
    void updateScheduleTable();
    
    /**
     * @brief Clear all scheduled downloads
     */
    void clearAllSchedules();
    
    /**
     * @brief Handle repeat option changes
     * 
     * @param id The selected repeat option ID
     */
    void onRepeatOptionChanged(int id);
    
    /**
     * @brief Toggle a schedule's active status
     * 
     * @param scheduleId The schedule ID
     */
    void toggleScheduleActive(int scheduleId);
    
    /**
     * @brief Edit a scheduled download
     * 
     * @param scheduleId The schedule ID
     */
    void editSchedule(int scheduleId);
    
    /**
     * @brief Remove a scheduled download
     * 
     * @param scheduleId The schedule ID
     */
    void removeSchedule(int scheduleId);
    
    /**
     * @brief Show context menu for schedule table
     * 
     * @param pos The position where to show the menu
     */
    void showContextMenu(const QPoint& pos);
    
private:
    /**
     * @brief Create the user interface
     */
    void createUI();
    
    /**
     * @brief Set up signal/slot connections
     */
    void setupConnections();
    
    /**
     * @brief Get human-readable text for repeat settings
     * 
     * @param repeat The repeat type
     * @param interval The repeat interval
     * @return QString The formatted repeat text
     */
    QString getRepeatText(Core::ScheduleRepeat repeat, int interval);
    
    // UI elements
    QLineEdit* m_urlEdit;
    QLineEdit* m_savePathEdit;
    QDateTimeEdit* m_dateTimeEdit;
    QRadioButton* m_noRepeatRadio;
    QRadioButton* m_hourlyRadio;
    QRadioButton* m_dailyRadio;
    QRadioButton* m_weeklyRadio;
    QRadioButton* m_monthlyRadio;
    QRadioButton* m_customRadio;
    QSpinBox* m_intervalSpinBox;
    QComboBox* m_intervalUnitCombo;
    QPushButton* m_addScheduleButton;
    QTableWidget* m_scheduleTable;
    QPushButton* m_refreshButton;
    QPushButton* m_clearAllButton;
    
    // Timer for updates
    QTimer* m_updateTimer;
};

} // namespace UI
} // namespace DownloadManager

#endif // SCHEDULER_DIALOG_H