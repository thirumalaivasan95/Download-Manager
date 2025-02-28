#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>

#include "include/core/Settings.h"

namespace dm {
namespace ui {

/**
 * @brief Dialog for application settings
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new SettingsDialog
     * 
     * @param parent The parent widget
     */
    explicit SettingsDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the SettingsDialog
     */
    ~SettingsDialog();
    
private slots:
    /**
     * @brief Browse for download directory
     */
    void browseDownloadDirectory();
    
    /**
     * @brief Apply settings
     */
    void applySettings();
    
    /**
     * @brief Reset settings to defaults
     */
    void resetToDefaults();
    
private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Create the general settings tab
     * 
     * @return QWidget* The tab widget
     */
    QWidget* createGeneralTab();
    
    /**
     * @brief Create the connections settings tab
     * 
     * @return QWidget* The tab widget
     */
    QWidget* createConnectionsTab();
    
    /**
     * @brief Create the interface settings tab
     * 
     * @return QWidget* The tab widget
     */
    QWidget* createInterfaceTab();
    
    /**
     * @brief Create the advanced settings tab
     * 
     * @return QWidget* The tab widget
     */
    QWidget* createAdvancedTab();
    
    /**
     * @brief Load settings from Settings
     */
    void loadSettings();
    
    // UI elements
    QTabWidget* tabWidget_;
    
    // General tab
    QLineEdit* downloadDirEdit_;
    QPushButton* browseDirButton_;
    QCheckBox* startMinimizedCheckBox_;
    QCheckBox* startWithSystemCheckBox_;
    QCheckBox* closeToTrayCheckBox_;
    QCheckBox* showNotificationsCheckBox_;
    QCheckBox* autoStartDownloadsCheckBox_;
    
    // Connections tab
    QSpinBox* maxConcurrentDownloadsSpinBox_;
    QSpinBox* segmentCountSpinBox_;
    QSpinBox* maxDownloadSpeedSpinBox_;
    QComboBox* connectionTimeoutComboBox_;
    QCheckBox* limitBandwidthCheckBox_;
    
    // Interface tab
    QComboBox* themeComboBox_;
    QComboBox* languageComboBox_;
    QCheckBox* showSpeedInTitleCheckBox_;
    QCheckBox* confirmRemovalCheckBox_;
    QCheckBox* showGridLinesCheckBox_;
    
    // Advanced tab
    QCheckBox* verifyDownloadsCheckBox_;
    QSpinBox* retryCountSpinBox_;
    QCheckBox* saveLogFilesCheckBox_;
    QComboBox* logLevelComboBox_;
    
    // Buttons
    QPushButton* okButton_;
    QPushButton* cancelButton_;
    QPushButton* applyButton_;
    QPushButton* resetButton_;
    
    // Settings reference
    dm::core::Settings& settings_;
    
    // Flag to avoid recursive updates
    bool updatingUI_;
};

} // namespace ui
} // namespace dm

#endif // SETTINGS_DIALOG_H