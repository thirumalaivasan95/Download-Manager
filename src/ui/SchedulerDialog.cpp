#include "../../include/ui/SchedulerDialog.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/StringUtils.h"
#include "../../include/utils/TimeUtils.h"
#include "../../include/core/DownloadScheduler.h"
#include "../../include/core/DownloadManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QIcon>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>

namespace DownloadManager {
namespace UI {

SchedulerDialog::SchedulerDialog(QWidget* parent)
    : QDialog(parent),
      m_updateTimer(nullptr)
{
    setWindowTitle(tr("Download Scheduler"));
    setMinimumWidth(700);
    setMinimumHeight(500);
    
    createUI();
    setupConnections();
    
    // Populate the table initially
    updateScheduleTable();
    
    // Start update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &SchedulerDialog::updateScheduleTable);
    m_updateTimer->start(1000); // Update every second
}

SchedulerDialog::~SchedulerDialog() {
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void SchedulerDialog::createUI() {
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tabs
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // Create Schedule tab
    QWidget* createTab = new QWidget(this);
    QVBoxLayout* createLayout = new QVBoxLayout(createTab);
    
    // URL input group
    QGroupBox* urlGroup = new QGroupBox(tr("Download Information"), this);
    QFormLayout* urlFormLayout = new QFormLayout(urlGroup);
    
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("Enter URL to download"));
    
    // Save path input
    QHBoxLayout* savePathLayout = new QHBoxLayout();
    
    m_savePathEdit = new QLineEdit(this);
    m_savePathEdit->setPlaceholderText(tr("Download destination"));
    
    QPushButton* browseButton = new QPushButton(tr("Browse"), this);
    browseButton->setIcon(QIcon(":/resources/icons/add.png"));
    
    savePathLayout->addWidget(m_savePathEdit);
    savePathLayout->addWidget(browseButton);
    
    urlFormLayout->addRow(tr("URL:"), m_urlEdit);
    urlFormLayout->addRow(tr("Save to:"), savePathLayout);
    
    // Schedule options group
    QGroupBox* scheduleGroup = new QGroupBox(tr("Schedule Options"), this);
    QVBoxLayout* scheduleLayout = new QVBoxLayout(scheduleGroup);
    
    // Date and time picker
    QHBoxLayout* dateTimeLayout = new QHBoxLayout();
    QLabel* startLabel = new QLabel(tr("Start Date & Time:"), this);
    
    m_dateTimeEdit = new QDateTimeEdit(this);
    m_dateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_dateTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600)); // Default to 1 hour from now
    m_dateTimeEdit->setCalendarPopup(true);
    
    dateTimeLayout->addWidget(startLabel);
    dateTimeLayout->addWidget(m_dateTimeEdit);
    
    // Repeat options
    QGroupBox* repeatGroup = new QGroupBox(tr("Repeat"), this);
    QVBoxLayout* repeatLayout = new QVBoxLayout(repeatGroup);
    
    m_noRepeatRadio = new QRadioButton(tr("No Repeat (One-Time)"), this);
    m_hourlyRadio = new QRadioButton(tr("Hourly"), this);
    m_dailyRadio = new QRadioButton(tr("Daily"), this);
    m_weeklyRadio = new QRadioButton(tr("Weekly"), this);
    m_monthlyRadio = new QRadioButton(tr("Monthly"), this);
    m_customRadio = new QRadioButton(tr("Custom Interval"), this);
    
    m_noRepeatRadio->setChecked(true);
    
    // Repeat interval
    QHBoxLayout* intervalLayout = new QHBoxLayout();
    
    m_intervalSpinBox = new QSpinBox(this);
    m_intervalSpinBox->setRange(1, 999);
    m_intervalSpinBox->setValue(1);
    m_intervalSpinBox->setEnabled(false);
    
    m_intervalUnitCombo = new QComboBox(this);
    m_intervalUnitCombo->addItem(tr("Hours"), 3600);
    m_intervalUnitCombo->addItem(tr("Days"), 86400);
    m_intervalUnitCombo->addItem(tr("Weeks"), 604800);
    m_intervalUnitCombo->addItem(tr("Months (30 days)"), 2592000);
    m_intervalUnitCombo->addItem(tr("Minutes"), 60);
    m_intervalUnitCombo->addItem(tr("Seconds"), 1);
    m_intervalUnitCombo->setEnabled(false);
    
    intervalLayout->addWidget(m_intervalSpinBox);
    intervalLayout->addWidget(m_intervalUnitCombo);
    
    repeatLayout->addWidget(m_noRepeatRadio);
    repeatLayout->addWidget(m_hourlyRadio);
    repeatLayout->addWidget(m_dailyRadio);
    repeatLayout->addWidget(m_weeklyRadio);
    repeatLayout->addWidget(m_monthlyRadio);
    repeatLayout->addWidget(m_customRadio);
    repeatLayout->addLayout(intervalLayout);
    
    // Add schedule button
    QHBoxLayout* addButtonLayout = new QHBoxLayout();
    m_addScheduleButton = new QPushButton(tr("Add Schedule"), this);
    m_addScheduleButton->setIcon(QIcon(":/resources/icons/add.png"));
    
    addButtonLayout->addStretch();
    addButtonLayout->addWidget(m_addScheduleButton);
    
    // Add widgets to schedule layout
    scheduleLayout->addLayout(dateTimeLayout);
    scheduleLayout->addWidget(repeatGroup);
    
    // Add widgets to create tab
    createLayout->addWidget(urlGroup);
    createLayout->addWidget(scheduleGroup);
    createLayout->addLayout(addButtonLayout);
    
    // Scheduled Tasks tab
    QWidget* tasksTab = new QWidget(this);
    QVBoxLayout* tasksLayout = new QVBoxLayout(tasksTab);
    
    // Schedule table
    m_scheduleTable = new QTableWidget(this);
    m_scheduleTable->setColumnCount(6);
    m_scheduleTable->setHorizontalHeaderLabels({
        tr("URL"),
        tr("Next Execution"),
        tr("Repeat"),
        tr("Status"),
        tr("Task ID"),
        tr("Actions")
    });
    
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_scheduleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_scheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_scheduleTable->verticalHeader()->setVisible(false);
    
    // Control buttons
    QHBoxLayout* controlLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton(tr("Refresh"), this);
    m_refreshButton->setIcon(QIcon(":/resources/icons/refresh.png"));
    
    m_clearAllButton = new QPushButton(tr("Clear All"), this);
    m_clearAllButton->setIcon(QIcon(":/resources/icons/remove.png"));
    
    controlLayout->addWidget(m_refreshButton);
    controlLayout->addStretch();
    controlLayout->addWidget(m_clearAllButton);
    
    tasksLayout->addWidget(m_scheduleTable);
    tasksLayout->addLayout(controlLayout);
    
    // Add tabs to widget
    tabWidget->addTab(createTab, tr("Create Schedule"));
    tabWidget->addTab(tasksTab, tr("Scheduled Tasks"));
    
    // Add to main layout
    mainLayout->addWidget(tabWidget);
    
    // Set default save path
    m_savePathEdit->setText(QDir::homePath() + "/Downloads/");
}

void SchedulerDialog::setupConnections() {
    // Browse button
    QPushButton* browseButton = nullptr;
    
    // Find browse button
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text() == tr("Browse")) {
            browseButton = button;
            break;
        }
    }
    
    if (browseButton) {
        connect(browseButton, &QPushButton::clicked, this, &SchedulerDialog::browseDestination);
    }
    
    // Add schedule button
    connect(m_addScheduleButton, &QPushButton::clicked, this, &SchedulerDialog::addSchedule);
    
    // Refresh button
    connect(m_refreshButton, &QPushButton::clicked, this, &SchedulerDialog::updateScheduleTable);
    
    // Clear all button
    connect(m_clearAllButton, &QPushButton::clicked, this, &SchedulerDialog::clearAllSchedules);
    
    // Repeat radio buttons
    QButtonGroup* repeatGroup = new QButtonGroup(this);
    repeatGroup->addButton(m_noRepeatRadio, 0);
    repeatGroup->addButton(m_hourlyRadio, 1);
    repeatGroup->addButton(m_dailyRadio, 2);
    repeatGroup->addButton(m_weeklyRadio, 3);
    repeatGroup->addButton(m_monthlyRadio, 4);
    repeatGroup->addButton(m_customRadio, 5);
    
    connect(repeatGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &SchedulerDialog::onRepeatOptionChanged);
    
    // Table context menu
    m_scheduleTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_scheduleTable, &QTableWidget::customContextMenuRequested, 
            this, &SchedulerDialog::showContextMenu);
}

void SchedulerDialog::browseDestination() {
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        tr("Select Download Directory"),
        m_savePathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dirPath.isEmpty()) {
        // Ensure path ends with a separator
        if (!dirPath.endsWith('/') && !dirPath.endsWith('\\')) {
            dirPath += '/';
        }
        
        m_savePathEdit->setText(dirPath);
    }
}

void SchedulerDialog::addSchedule() {
    // Validate inputs
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a URL"));
        return;
    }
    
    QString savePath = m_savePathEdit->text();
    if (savePath.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please specify a save location"));
        return;
    }
    
    // Get scheduler
    auto& scheduler = Core::DownloadScheduler::instance();
    
    // Get start time
    QDateTime startDateTime = m_dateTimeEdit->dateTime();
    time_t startTime = startDateTime.toSecsSinceEpoch();
    
    // Create download options
    Core::DownloadOptions options;
    options.url = url.toStdString();
    options.destination = savePath.toStdString();
    
    // Determine schedule type
    Core::ScheduleRepeat repeatType = Core::ScheduleRepeat::NONE;
    int repeatInterval = 0;
    
    if (m_hourlyRadio->isChecked()) {
        repeatType = Core::ScheduleRepeat::HOURLY;
        repeatInterval = m_intervalSpinBox->value();
    } 
    else if (m_dailyRadio->isChecked()) {
        repeatType = Core::ScheduleRepeat::DAILY;
        repeatInterval = m_intervalSpinBox->value();
    } 
    else if (m_weeklyRadio->isChecked()) {
        repeatType = Core::ScheduleRepeat::WEEKLY;
        repeatInterval = m_intervalSpinBox->value();
    } 
    else if (m_monthlyRadio->isChecked()) {
        repeatType = Core::ScheduleRepeat::MONTHLY;
        repeatInterval = m_intervalSpinBox->value();
    } 
    else if (m_customRadio->isChecked()) {
        repeatType = Core::ScheduleRepeat::CUSTOM;
        repeatInterval = m_intervalSpinBox->value() * 
                       m_intervalUnitCombo->currentData().toInt();
    }
    
    // Create schedule
    int scheduleId = -1;
    
    if (repeatType == Core::ScheduleRepeat::NONE) {
        // One-time schedule
        scheduleId = scheduler.scheduleDownload(options, startTime);
    } else {
        // Recurring schedule
        scheduleId = scheduler.scheduleRecurringDownload(options, repeatType, repeatInterval, startTime);
    }
    
    if (scheduleId > 0) {
        QMessageBox::information(this, tr("Success"), 
                              tr("Download scheduled successfully (ID: %1)").arg(scheduleId));
        
        // Switch to Scheduled Tasks tab
        QTabWidget* tabWidget = findChild<QTabWidget*>();
        if (tabWidget) {
            tabWidget->setCurrentIndex(1);
        }
        
        // Clear inputs
        m_urlEdit->clear();
        
        // Update table
        updateScheduleTable();
    } else {
        QMessageBox::warning(this, tr("Error"), 
                          tr("Failed to schedule download"));
    }
}

void SchedulerDialog::updateScheduleTable() {
    auto& scheduler = Core::DownloadScheduler::instance();
    
    // Get all schedules
    std::vector<Core::ScheduleInfo> schedules = scheduler.getAllSchedules();
    
    // Update table
    m_scheduleTable->setRowCount(static_cast<int>(schedules.size()));
    
    for (size_t i = 0; i < schedules.size(); i++) {
        const Core::ScheduleInfo& schedule = schedules[i];
        
        // URL
        QTableWidgetItem* urlItem = new QTableWidgetItem(QString::fromStdString(schedule.url));
        m_scheduleTable->setItem(i, 0, urlItem);
        
        // Next Execution
        QDateTime nextTime = QDateTime::fromSecsSinceEpoch(schedule.startTime);
        QTableWidgetItem* timeItem = new QTableWidgetItem(nextTime.toString("yyyy-MM-dd HH:mm:ss"));
        m_scheduleTable->setItem(i, 1, timeItem);
        
        // Highlight row if scheduled time is passed but schedule is still active
        QDateTime currentTime = QDateTime::currentDateTime();
        if (nextTime < currentTime && schedule.active) {
            // Use a light red background color
            timeItem->setBackground(QColor(255, 200, 200));
        }
        // Use green background for active and future schedules
        else if (schedule.active && nextTime >= currentTime) {
            timeItem->setBackground(QColor(200, 255, 200));
        }
        
        // Repeat
        QString repeatText = getRepeatText(schedule.repeat, schedule.repeatInterval);
        QTableWidgetItem* repeatItem = new QTableWidgetItem(repeatText);
        m_scheduleTable->setItem(i, 2, repeatItem);
        
        // Status
        QString statusText = schedule.active ? tr("Active") : tr("Paused");
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
        m_scheduleTable->setItem(i, 3, statusItem);
        
        // Task ID
        QString taskIdText = (schedule.taskId > 0) ? 
                          QString::number(schedule.taskId) : tr("Not Started");
        QTableWidgetItem* taskIdItem = new QTableWidgetItem(taskIdText);
        m_scheduleTable->setItem(i, 4, taskIdItem);
        
        // Actions
        QWidget* actionWidget = new QWidget();
        QHBoxLayout* actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(2, 2, 2, 2);
        
        QPushButton* pauseResumeButton = new QPushButton(this);
        pauseResumeButton->setMaximumWidth(30);
        
        if (schedule.active) {
            pauseResumeButton->setIcon(QIcon(":/resources/icons/pause.png"));
            pauseResumeButton->setToolTip(tr("Pause"));
        } else {
            pauseResumeButton->setIcon(QIcon(":/resources/icons/resume.png"));
            pauseResumeButton->setToolTip(tr("Resume"));
        }
        
        QPushButton* editButton = new QPushButton(QIcon(":/resources/icons/settings.png"), "", this);
        editButton->setToolTip(tr("Edit"));
        editButton->setMaximumWidth(30);
        
        QPushButton* removeButton = new QPushButton(QIcon(":/resources/icons/remove.png"), "", this);
        removeButton->setToolTip(tr("Remove"));
        removeButton->setMaximumWidth(30);
        
        // Connect buttons to specific row actions
        connect(pauseResumeButton, &QPushButton::clicked, [this, schedule]() {
            this->toggleScheduleActive(schedule.id);
        });
        
        connect(editButton, &QPushButton::clicked, [this, schedule]() {
            this->editSchedule(schedule.id);
        });
        
        connect(removeButton, &QPushButton::clicked, [this, schedule]() {
            this->removeSchedule(schedule.id);
        });
        
        actionLayout->addWidget(pauseResumeButton);
        actionLayout->addWidget(editButton);
        actionLayout->addWidget(removeButton);
        actionLayout->addStretch();
        
        m_scheduleTable->setCellWidget(i, 5, actionWidget);
    }
}

void SchedulerDialog::clearAllSchedules() {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Clear All"),
        tr("Are you sure you want to remove all scheduled downloads?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    auto& scheduler = Core::DownloadScheduler::instance();
    
    // Get all schedules
    std::vector<Core::ScheduleInfo> schedules = scheduler.getAllSchedules();
    
    // Remove each schedule
    for (const auto& schedule : schedules) {
        scheduler.cancelSchedule(schedule.id);
    }
    
    // Update table
    updateScheduleTable();
    
    QMessageBox::information(this, tr("Success"), 
                          tr("All scheduled downloads have been removed"));
}

void SchedulerDialog::onRepeatOptionChanged(int id) {
    bool enableInterval = (id != 0); // Not "No Repeat"
    m_intervalSpinBox->setEnabled(enableInterval);
    
    if (id == 5) { // Custom
        m_intervalUnitCombo->setEnabled(true);
    } else {
        m_intervalUnitCombo->setEnabled(false);
        
        // Set appropriate unit and default value based on repeat type
        switch (id) {
            case 1: // Hourly
                m_intervalUnitCombo->setCurrentIndex(0); // Hours
                m_intervalSpinBox->setValue(1);
                break;
            case 2: // Daily
                m_intervalUnitCombo->setCurrentIndex(1); // Days
                m_intervalSpinBox->setValue(1);
                break;
            case 3: // Weekly
                m_intervalUnitCombo->setCurrentIndex(2); // Weeks
                m_intervalSpinBox->setValue(1);
                break;
            case 4: // Monthly
                m_intervalUnitCombo->setCurrentIndex(3); // Months
                m_intervalSpinBox->setValue(1);
                break;
        }
    }
}

void SchedulerDialog::toggleScheduleActive(int scheduleId) {
    auto& scheduler = Core::DownloadScheduler::instance();
    
    // Get schedule info
    Core::ScheduleInfo info = scheduler.getSchedule(scheduleId);
    
    if (info.id <= 0) {
        QMessageBox::warning(this, tr("Error"), 
                          tr("Failed to find schedule with ID: %1").arg(scheduleId));
        return;
    }
    
    // Toggle active status
    if (info.active) {
        scheduler.pauseSchedule(scheduleId);
    } else {
        scheduler.resumeSchedule(scheduleId);
    }
    
    // Update table
    updateScheduleTable();
}

void SchedulerDialog::editSchedule(int scheduleId) {
    auto& scheduler = Core::DownloadScheduler::instance();
    
    // Get schedule info
    Core::ScheduleInfo info = scheduler.getSchedule(scheduleId);
    
    if (info.id <= 0) {
        QMessageBox::warning(this, tr("Error"), 
                          tr("Failed to find schedule with ID: %1").arg(scheduleId));
        return;
    }
    
    // Create edit dialog
    QDialog editDialog(this);
    editDialog.setWindowTitle(tr("Edit Schedule"));
    editDialog.setMinimumWidth(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&editDialog);
    
    // Start time widget
    QGroupBox* timeGroup = new QGroupBox(tr("Schedule Time"), &editDialog);
    QVBoxLayout* timeLayout = new QVBoxLayout(timeGroup);
    
    QDateTimeEdit* dateTimeEdit = new QDateTimeEdit(&editDialog);
    dateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    dateTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(info.startTime));
    dateTimeEdit->setCalendarPopup(true);
    
    timeLayout->addWidget(dateTimeEdit);
    
    // Repeat options
    QGroupBox* repeatGroup = new QGroupBox(tr("Repeat Options"), &editDialog);
    QVBoxLayout* repeatLayout = new QVBoxLayout(repeatGroup);
    
    QComboBox* repeatTypeCombo = new QComboBox(&editDialog);
    repeatTypeCombo->addItem(tr("No Repeat (One-Time)"), static_cast<int>(Core::ScheduleRepeat::NONE));
    repeatTypeCombo->addItem(tr("Hourly"), static_cast<int>(Core::ScheduleRepeat::HOURLY));
    repeatTypeCombo->addItem(tr("Daily"), static_cast<int>(Core::ScheduleRepeat::DAILY));
    repeatTypeCombo->addItem(tr("Weekly"), static_cast<int>(Core::ScheduleRepeat::WEEKLY));
    repeatTypeCombo->addItem(tr("Monthly"), static_cast<int>(Core::ScheduleRepeat::MONTHLY));
    repeatTypeCombo->addItem(tr("Custom Interval"), static_cast<int>(Core::ScheduleRepeat::CUSTOM));
    
    // Set current repeat type
    repeatTypeCombo->setCurrentIndex(static_cast<int>(info.repeat));
    
    QHBoxLayout* intervalLayout = new QHBoxLayout();
    QLabel* intervalLabel = new QLabel(tr("Interval:"), &editDialog);
    
    QSpinBox* intervalSpinBox = new QSpinBox(&editDialog);
    intervalSpinBox->setRange(1, 999);
    intervalSpinBox->setValue(info.repeatInterval);
    intervalSpinBox->setEnabled(info.repeat != Core::ScheduleRepeat::NONE);
    
    intervalLayout->addWidget(intervalLabel);
    intervalLayout->addWidget(intervalSpinBox);
    
    repeatLayout->addWidget(repeatTypeCombo);
    repeatLayout->addLayout(intervalLayout);
    
    // Connect repeat type combo to enable/disable interval
    connect(repeatTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            [intervalSpinBox](int index) {
        intervalSpinBox->setEnabled(index > 0);
    });
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &editDialog);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &editDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &editDialog, &QDialog::reject);
    
    // Add widgets to layout
    layout->addWidget(timeGroup);
    layout->addWidget(repeatGroup);
    layout->addWidget(buttonBox);
    
    // Show dialog
    if (editDialog.exec() == QDialog::Accepted) {
        // Update schedule
        time_t newStartTime = dateTimeEdit->dateTime().toSecsSinceEpoch();
        Core::ScheduleRepeat newRepeat = static_cast<Core::ScheduleRepeat>(
            repeatTypeCombo->currentData().toInt());
        int newInterval = intervalSpinBox->value();
        
        // Update start time
        scheduler.updateSchedule(scheduleId, newStartTime);
        
        // Update repeat settings
        scheduler.updateRecurringSchedule(scheduleId, newRepeat, newInterval);
        
        // Update table
        updateScheduleTable();
        
        QMessageBox::information(this, tr("Success"), 
                              tr("Schedule updated successfully"));
    }
}

void SchedulerDialog::removeSchedule(int scheduleId) {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Remove"),
        tr("Are you sure you want to remove this scheduled download?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    auto& scheduler = Core::DownloadScheduler::instance();
    
    // Cancel schedule
    if (scheduler.cancelSchedule(scheduleId)) {
        // Update table
        updateScheduleTable();
        
        QMessageBox::information(this, tr("Success"), 
                              tr("Schedule removed successfully"));
    } else {
        QMessageBox::warning(this, tr("Error"), 
                          tr("Failed to remove schedule"));
    }
}

void SchedulerDialog::showContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = m_scheduleTable->itemAt(pos);
    if (!item) {
        return;
    }
    
    int row = item->row();
    QTableWidgetItem* idItem = m_scheduleTable->item(row, 4); // Task ID column
    
    if (!idItem) {
        return;
    }
    
    // Get schedule ID
    QVariant scheduleIdVar = idItem->data(Qt::UserRole);
    if (!scheduleIdVar.isValid()) {
        return;
    }
    
    int scheduleId = scheduleIdVar.toInt();
    
    QMenu contextMenu(this);
    
    // Create actions
    QAction* editAction = new QAction(tr("Edit Schedule"), this);
    QAction* removeAction = new QAction(tr("Remove Schedule"), this);
    QAction* viewTaskAction = new QAction(tr("View Download Task"), this);
    
    // Connect actions
    connect(editAction, &QAction::triggered, [this, scheduleId]() {
        this->editSchedule(scheduleId);
    });
    
    connect(removeAction, &QAction::triggered, [this, scheduleId]() {
        this->removeSchedule(scheduleId);
    });
    
    connect(viewTaskAction, &QAction::triggered, [this, row]() {
        QTableWidgetItem* taskIdItem = m_scheduleTable->item(row, 4);
        if (!taskIdItem) {
            return;
        }
        
        QString taskIdText = taskIdItem->text();
        bool ok;
        int taskId = taskIdText.toInt(&ok);
        
        if (ok && taskId > 0) {
            // Emit signal to view task details
            emit viewDownloadTask(taskId);
        } else {
            QMessageBox::information(this, tr("Information"), 
                                  tr("This scheduled download has not started yet."));
        }
    });
    
    // Add actions to menu
    contextMenu.addAction(editAction);
    contextMenu.addAction(removeAction);
    contextMenu.addSeparator();
    
    // Only add view task option if task ID is available
    QTableWidgetItem* taskIdItem = m_scheduleTable->item(row, 4);
    if (taskIdItem && taskIdItem->text() != tr("Not Started")) {
        contextMenu.addAction(viewTaskAction);
    }
    
    // Show menu
    contextMenu.exec(m_scheduleTable->viewport()->mapToGlobal(pos));
}

QString SchedulerDialog::getRepeatText(Core::ScheduleRepeat repeat, int interval) {
    switch (repeat) {
        case Core::ScheduleRepeat::NONE:
            return tr("One-time");
        case Core::ScheduleRepeat::HOURLY:
            return interval == 1 ? 
                tr("Every hour") : 
                tr("Every %1 hours").arg(interval);
        case Core::ScheduleRepeat::DAILY:
            return interval == 1 ? 
                tr("Every day") : 
                tr("Every %1 days").arg(interval);
        case Core::ScheduleRepeat::WEEKLY:
            return interval == 1 ? 
                tr("Every week") : 
                tr("Every %1 weeks").arg(interval);
        case Core::ScheduleRepeat::MONTHLY:
            return interval == 1 ? 
                tr("Every month") : 
                tr("Every %1 months").arg(interval);
        case Core::ScheduleRepeat::CUSTOM:
            // Convert seconds to appropriate unit
            if (interval < 60) {
                return tr("Every %1 seconds").arg(interval);
            } else if (interval < 3600) {
                return tr("Every %1 minutes").arg(interval / 60);
            } else if (interval < 86400) {
                return tr("Every %1 hours").arg(interval / 3600);
            } else if (interval < 604800) {
                return tr("Every %1 days").arg(interval / 86400);
            } else if (interval < 2592000) {
                return tr("Every %1 weeks").arg(interval / 604800);
            } else {
                return tr("Every %1 months").arg(interval / 2592000);
            }
        default:
            return tr("Unknown");
    }
}

} // namespace UI
} // namespace DownloadManager