#include "include/ui/SettingsDialog.h"
#include "core/DownloadManager.h"
#include "include/utils/FileUtils.h"
#include "include/utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QIcon>
#include <QApplication>

namespace dm {
namespace ui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent), 
      settings_(*dm::core::DownloadManager::getInstance().getSettings()),
      updatingUI_(false) {
    // Set up UI
    setupUi();
    
    // Load settings
    loadSettings();
    
    // Connect signals and slots
    connect(browseDirButton_, &QPushButton::clicked, this, &SettingsDialog::browseDownloadDirectory);
    connect(applyButton_, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    connect(resetButton_, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
    connect(okButton_, &QPushButton::clicked, this, [this]() {
        applySettings();
        accept();
    });
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::browseDownloadDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Download Directory",
                                                  downloadDirEdit_->text(),
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        downloadDirEdit_->setText(dir);
    }
}

void SettingsDialog::applySettings() {
    // General tab
    settings_.setDownloadDirectory(downloadDirEdit_->text().toStdString());
    settings_.setStartMinimized(startMinimizedCheckBox_->isChecked());
    settings_.setStartWithSystem(startWithSystemCheckBox_->isChecked());
    settings_.setCloseToTray(closeToTrayCheckBox_->isChecked());
    settings_.setShowNotifications(showNotificationsCheckBox_->isChecked());
    settings_.setAutoStartDownloads(autoStartDownloadsCheckBox_->isChecked());
    
    // Connections tab
    settings_.setMaxConcurrentDownloads(maxConcurrentDownloadsSpinBox_->value());
    settings_.setSegmentCount(segmentCountSpinBox_->value());
    settings_.setMaxDownloadSpeed(limitBandwidthCheckBox_->isChecked() ? maxDownloadSpeedSpinBox_->value() : 0);
    settings_.setIntSetting("connection_timeout", connectionTimeoutComboBox_->currentIndex());
    
    // Interface tab
    settings_.setStringSetting("theme", themeComboBox_->currentText().toStdString());
    settings_.setStringSetting("language", languageComboBox_->currentText().toStdString());
    settings_.setBoolSetting("show_speed_in_title", showSpeedInTitleCheckBox_->isChecked());
    settings_.setBoolSetting("confirm_removal", confirmRemovalCheckBox_->isChecked());
    settings_.setBoolSetting("show_grid_lines", showGridLinesCheckBox_->isChecked());
    
    // Advanced tab
    settings_.setBoolSetting("verify_downloads", verifyDownloadsCheckBox_->isChecked());
    settings_.setIntSetting("retry_count", retryCountSpinBox_->value());
    settings_.setBoolSetting("save_log_files", saveLogFilesCheckBox_->isChecked());
    
    // Set log level
    int logLevelIndex = logLevelComboBox_->currentIndex();
    dm::utils::LogLevel logLevel;
    switch (logLevelIndex) {
        case 0:
            logLevel = dm::utils::LogLevel::DEBUG;
            break;
        case 1:
            logLevel = dm::utils::LogLevel::INFO;
            break;
        case 2:
            logLevel = dm::utils::LogLevel::WARNING;
            break;
        case 3:
            logLevel = dm::utils::LogLevel::ERROR;
            break;
        case 4:
            logLevel = dm::utils::LogLevel::CRITICAL;
            break;
        default:
            logLevel = dm::utils::LogLevel::INFO;
            break;
    }
    dm::utils::Logger::setLogLevel(logLevel);
    settings_.setIntSetting("log_level", logLevelIndex);
    
    // Save settings
    settings_.save();
    
    // Update download manager
    dm::core::DownloadManager::getInstance().setMaxConcurrentDownloads(
        settings_.getMaxConcurrentDownloads());
    dm::core::DownloadManager::getInstance().setDefaultDownloadDirectory(
        settings_.getDownloadDirectory());
    
    // Enable apply button
    applyButton_->setEnabled(false);
}

void SettingsDialog::resetToDefaults() {
    // Confirm reset
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Reset Settings",
                                                            "Are you sure you want to reset all settings to their default values?",
                                                            QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Reset settings
    settings_.resetToDefaults();
    
    // Reload UI
    loadSettings();
    
    // Enable apply button
    applyButton_->setEnabled(true);
}

void SettingsDialog::setupUi() {
    // Set window title
    setWindowTitle("Settings");
    
    // Set window icon
    setWindowIcon(QIcon(":/icons/settings.png"));
    
    // Set window size
    resize(500, 400);
    
    // Create tab widget
    tabWidget_ = new QTabWidget(this);
    
    // Create tabs
    tabWidget_->addTab(createGeneralTab(), "General");
    tabWidget_->addTab(createConnectionsTab(), "Connections");
    tabWidget_->addTab(createInterfaceTab(), "Interface");
    tabWidget_->addTab(createAdvancedTab(), "Advanced");
    
    // Create buttons
    okButton_ = new QPushButton("OK", this);
    cancelButton_ = new QPushButton("Cancel", this);
    applyButton_ = new QPushButton("Apply", this);
    resetButton_ = new QPushButton("Reset", this);
    
    // Set button properties
    okButton_->setDefault(true);
    applyButton_->setEnabled(false);
    
    // Create button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(resetButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton_);
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(applyButton_);
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget_);
    mainLayout->addLayout(buttonLayout);
    
    // Set layout
    setLayout(mainLayout);
    
    // Connect signals for detecting changes
    connect(downloadDirEdit_, &QLineEdit::textChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(startMinimizedCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(startWithSystemCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(closeToTrayCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(showNotificationsCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(autoStartDownloadsCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(maxConcurrentDownloadsSpinBox_, &QSpinBox::valueChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(segmentCountSpinBox_, &QSpinBox::valueChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(maxDownloadSpeedSpinBox_, &QSpinBox::valueChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(limitBandwidthCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) {
            maxDownloadSpeedSpinBox_->setEnabled(limitBandwidthCheckBox_->isChecked());
            applyButton_->setEnabled(true);
        }
    });
    
    connect(connectionTimeoutComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(themeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(languageComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(showSpeedInTitleCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(confirmRemovalCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(showGridLinesCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(verifyDownloadsCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(retryCountSpinBox_, &QSpinBox::valueChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(saveLogFilesCheckBox_, &QCheckBox::stateChanged, [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
    
    connect(logLevelComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (!updatingUI_) applyButton_->setEnabled(true);
    });
}

QWidget* SettingsDialog::createGeneralTab() {
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Download directory
    QGroupBox* dirGroup = new QGroupBox("Download Directory", tab);
    QHBoxLayout* dirLayout = new QHBoxLayout(dirGroup);
    downloadDirEdit_ = new QLineEdit(dirGroup);
    browseDirButton_ = new QPushButton("Browse...", dirGroup);
    dirLayout->addWidget(downloadDirEdit_);
    dirLayout->addWidget(browseDirButton_);
    
    // Startup and shutdown
    QGroupBox* startupGroup = new QGroupBox("Startup and Shutdown", tab);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    startMinimizedCheckBox_ = new QCheckBox("Start minimized", startupGroup);
    startWithSystemCheckBox_ = new QCheckBox("Start with system", startupGroup);
    closeToTrayCheckBox_ = new QCheckBox("Close to system tray", startupGroup);
    startupLayout->addWidget(startMinimizedCheckBox_);
    startupLayout->addWidget(startWithSystemCheckBox_);
    startupLayout->addWidget(closeToTrayCheckBox_);
    
    // Notifications
    QGroupBox* notifGroup = new QGroupBox("Notifications", tab);
    QVBoxLayout* notifLayout = new QVBoxLayout(notifGroup);
    showNotificationsCheckBox_ = new QCheckBox("Show notifications", notifGroup);
    autoStartDownloadsCheckBox_ = new QCheckBox("Auto-start new downloads", notifGroup);
    notifLayout->addWidget(showNotificationsCheckBox_);
    notifLayout->addWidget(autoStartDownloadsCheckBox_);
    
    // Add to layout
    layout->addWidget(dirGroup);
    layout->addWidget(startupGroup);
    layout->addWidget(notifGroup);
    layout->addStretch();
    
    return tab;
}

QWidget* SettingsDialog::createConnectionsTab() {
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Connection settings
    QGroupBox* connGroup = new QGroupBox("Connection Settings", tab);
    QFormLayout* connLayout = new QFormLayout(connGroup);
    
    maxConcurrentDownloadsSpinBox_ = new QSpinBox(connGroup);
    maxConcurrentDownloadsSpinBox_->setRange(1, 10);
    connLayout->addRow("Maximum concurrent downloads:", maxConcurrentDownloadsSpinBox_);
    
    segmentCountSpinBox_ = new QSpinBox(connGroup);
    segmentCountSpinBox_->setRange(1, 16);
    connLayout->addRow("Segments per download:", segmentCountSpinBox_);
    
    connectionTimeoutComboBox_ = new QComboBox(connGroup);
    connectionTimeoutComboBox_->addItems({"10 seconds", "30 seconds", "1 minute", "5 minutes", "Never"});
    connLayout->addRow("Connection timeout:", connectionTimeoutComboBox_);
    
    // Bandwidth settings
    QGroupBox* bwGroup = new QGroupBox("Bandwidth", tab);
    QVBoxLayout* bwLayout = new QVBoxLayout(bwGroup);
    
    limitBandwidthCheckBox_ = new QCheckBox("Limit download speed:", bwGroup);
    
    QHBoxLayout* speedLayout = new QHBoxLayout;
    maxDownloadSpeedSpinBox_ = new QSpinBox(bwGroup);
    maxDownloadSpeedSpinBox_->setRange(1, 100000);
    maxDownloadSpeedSpinBox_->setSuffix(" KB/s");
    speedLayout->addWidget(maxDownloadSpeedSpinBox_);
    speedLayout->addStretch();
    
    bwLayout->addWidget(limitBandwidthCheckBox_);
    bwLayout->addLayout(speedLayout);
    
    // Add to layout
    layout->addWidget(connGroup);
    layout->addWidget(bwGroup);
    layout->addStretch();
    
    return tab;
}

QWidget* SettingsDialog::createInterfaceTab() {
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Appearance
    QGroupBox* appearanceGroup = new QGroupBox("Appearance", tab);
    QFormLayout* appearanceLayout = new QFormLayout(appearanceGroup);
    
    themeComboBox_ = new QComboBox(appearanceGroup);
    themeComboBox_->addItems({"System", "Light", "Dark"});
    appearanceLayout->addRow("Theme:", themeComboBox_);
    
    languageComboBox_ = new QComboBox(appearanceGroup);
    languageComboBox_->addItems({"System", "English", "French", "German", "Spanish", "Italian", "Japanese", "Chinese"});
    appearanceLayout->addRow("Language:", languageComboBox_);
    
    // Display options
    QGroupBox* displayGroup = new QGroupBox("Display Options", tab);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    
    showSpeedInTitleCheckBox_ = new QCheckBox("Show download speed in window title", displayGroup);
    confirmRemovalCheckBox_ = new QCheckBox("Confirm download removal", displayGroup);
    showGridLinesCheckBox_ = new QCheckBox("Show grid lines in download list", displayGroup);
    
    displayLayout->addWidget(showSpeedInTitleCheckBox_);
    displayLayout->addWidget(confirmRemovalCheckBox_);
    displayLayout->addWidget(showGridLinesCheckBox_);
    
    // Add to layout
    layout->addWidget(appearanceGroup);
    layout->addWidget(displayGroup);
    layout->addStretch();
    
    return tab;
}

QWidget* SettingsDialog::createAdvancedTab() {
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Download options
    QGroupBox* downloadGroup = new QGroupBox("Download Options", tab);
    QVBoxLayout* downloadLayout = new QVBoxLayout(downloadGroup);
    
    verifyDownloadsCheckBox_ = new QCheckBox("Verify downloads after completion", downloadGroup);
    
    QHBoxLayout* retryLayout = new QHBoxLayout;
    QLabel* retryLabel = new QLabel("Number of retries on error:", downloadGroup);
    retryCountSpinBox_ = new QSpinBox(downloadGroup);
    retryCountSpinBox_->setRange(0, 10);
    retryLayout->addWidget(retryLabel);
    retryLayout->addWidget(retryCountSpinBox_);
    retryLayout->addStretch();
    
    downloadLayout->addWidget(verifyDownloadsCheckBox_);
    downloadLayout->addLayout(retryLayout);
    
    // Logging
    QGroupBox* logGroup = new QGroupBox("Logging", tab);
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    saveLogFilesCheckBox_ = new QCheckBox("Save log files", logGroup);
    
    QHBoxLayout* logLevelLayout = new QHBoxLayout;
    QLabel* logLevelLabel = new QLabel("Log level:", logGroup);
    logLevelComboBox_ = new QComboBox(logGroup);
    logLevelComboBox_->addItems({"Debug", "Info", "Warning", "Error", "Critical"});
    logLevelLayout->addWidget(logLevelLabel);
    logLevelLayout->addWidget(logLevelComboBox_);
    logLevelLayout->addStretch();
    
    logLayout->addWidget(saveLogFilesCheckBox_);
    logLayout->addLayout(logLevelLayout);
    
    // Add to layout
    layout->addWidget(downloadGroup);
    layout->addWidget(logGroup);
    layout->addStretch();
    
    return tab;
}

void SettingsDialog::loadSettings() {
    // Set flag to avoid triggering change signals
    updatingUI_ = true;
    
    // General tab
    downloadDirEdit_->setText(QString::fromStdString(settings_.getDownloadDirectory()));
    startMinimizedCheckBox_->setChecked(settings_.getStartMinimized());
    startWithSystemCheckBox_->setChecked(settings_.getStartWithSystem());
    closeToTrayCheckBox_->setChecked(settings_.getCloseToTray());
    showNotificationsCheckBox_->setChecked(settings_.getShowNotifications());
    autoStartDownloadsCheckBox_->setChecked(settings_.getAutoStartDownloads());
    
    // Connections tab
    maxConcurrentDownloadsSpinBox_->setValue(settings_.getMaxConcurrentDownloads());
    segmentCountSpinBox_->setValue(settings_.getSegmentCount());
    
    int maxSpeed = settings_.getMaxDownloadSpeed();
    limitBandwidthCheckBox_->setChecked(maxSpeed > 0);
    maxDownloadSpeedSpinBox_->setValue(maxSpeed > 0 ? maxSpeed : 1024);
    maxDownloadSpeedSpinBox_->setEnabled(maxSpeed > 0);
    
    int timeout = settings_.getIntSetting("connection_timeout", 1);
    connectionTimeoutComboBox_->setCurrentIndex(timeout);
    
    // Interface tab
    std::string theme = settings_.getStringSetting("theme", "System");
    themeComboBox_->setCurrentText(QString::fromStdString(theme));
    
    std::string language = settings_.getStringSetting("language", "System");
    languageComboBox_->setCurrentText(QString::fromStdString(language));
    
    showSpeedInTitleCheckBox_->setChecked(settings_.getBoolSetting("show_speed_in_title", false));
    confirmRemovalCheckBox_->setChecked(settings_.getBoolSetting("confirm_removal", true));
    showGridLinesCheckBox_->setChecked(settings_.getBoolSetting("show_grid_lines", true));
    
    // Advanced tab
    verifyDownloadsCheckBox_->setChecked(settings_.getBoolSetting("verify_downloads", true));
    retryCountSpinBox_->setValue(settings_.getIntSetting("retry_count", 3));
    saveLogFilesCheckBox_->setChecked(settings_.getBoolSetting("save_log_files", true));
    
    int logLevel = settings_.getIntSetting("log_level", 1); // Default to INFO
    logLevelComboBox_->setCurrentIndex(logLevel);
    
    // Reset flag
    updatingUI_ = false;
    
    // Disable apply button
    applyButton_->setEnabled(false);
}

} // namespace ui
} // namespace dm