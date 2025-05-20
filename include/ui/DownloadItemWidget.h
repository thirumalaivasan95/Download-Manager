#ifndef DOWNLOAD_ITEM_WIDGET_H
#define DOWNLOAD_ITEM_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <memory>

#include "core/DownloadTask.h"
#include "include/ui/ProgressBar.h"

namespace dm {
namespace ui {

/**
 * @brief Widget for displaying a download item
 */
class DownloadItemWidget : public QWidget {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new DownloadItemWidget
     * 
     * @param task The download task
     * @param parent The parent widget
     */
    explicit DownloadItemWidget(std::shared_ptr<dm::core::DownloadTask> task, QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the DownloadItemWidget
     */
    ~DownloadItemWidget();
    
    /**
     * @brief Get the download task
     * 
     * @return std::shared_ptr<dm::core::DownloadTask> The download task
     */
    std::shared_ptr<dm::core::DownloadTask> getTask() const;
    
    /**
     * @brief Update the widget
     */
    void update();
    
signals:
    /**
     * @brief Signal emitted when the start button is clicked
     */
    void startClicked();
    
    /**
     * @brief Signal emitted when the pause button is clicked
     */
    void pauseClicked();
    
    /**
     * @brief Signal emitted when the resume button is clicked
     */
    void resumeClicked();
    
    /**
     * @brief Signal emitted when the cancel button is clicked
     */
    void cancelClicked();
    
    /**
     * @brief Signal emitted when the remove button is clicked
     */
    void removeClicked();
    
    /**
     * @brief Signal emitted when the open button is clicked
     */
    void openClicked();
    
private slots:
    /**
     * @brief Start button handler
     */
    void onStartClicked();
    
    /**
     * @brief Pause button handler
     */
    void onPauseClicked();
    
    /**
     * @brief Resume button handler
     */
    void onResumeClicked();
    
    /**
     * @brief Cancel button handler
     */
    void onCancelClicked();
    
    /**
     * @brief Remove button handler
     */
    void onRemoveClicked();
    
    /**
     * @brief Open button handler
     */
    void onOpenClicked();
    
    /**
     * @brief Update timer handler
     */
    void onUpdateTimer();
    
private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Format file size
     * 
     * @param size Size in bytes
     * @return QString Formatted size
     */
    QString formatFileSize(int64_t size) const;
    
    /**
     * @brief Format download speed
     * 
     * @param bytesPerSecond Speed in bytes per second
     * @return QString Formatted speed
     */
    QString formatSpeed(double bytesPerSecond) const;
    
    /**
     * @brief Format time
     * 
     * @param seconds Time in seconds
     * @return QString Formatted time
     */
    QString formatTime(int64_t seconds) const;
    
    /**
     * @brief Update button states based on download status
     */
    void updateButtonStates();
    
    // Download task
    std::shared_ptr<dm::core::DownloadTask> task_;
    
    // UI elements
    QLabel* nameLabel_;
    QLabel* statusLabel_;
    QLabel* sizeLabel_;
    ProgressBar* progressBar_;
    QLabel* speedLabel_;
    QLabel* timeLabel_;
    
    QPushButton* startButton_;
    QPushButton* pauseButton_;
    QPushButton* resumeButton_;
    QPushButton* cancelButton_;
    QPushButton* removeButton_;
    QPushButton* openButton_;
    
    // Update timer
    QTimer* updateTimer_;
};

} // namespace ui
} // namespace dm

#endif // DOWNLOAD_ITEM_WIDGET_H