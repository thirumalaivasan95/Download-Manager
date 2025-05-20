#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QMap>
#include <memory>

#include "core/DownloadManager.h"
#include "core/DownloadTask.h"

namespace dm {
namespace ui {

/**
 * @brief MainWindow class for the download manager
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new MainWindow
     * 
     * @param parent The parent widget
     */
    explicit MainWindow(QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the MainWindow
     */
    ~MainWindow();
    
protected:
    /**
     * @brief Close event handler
     * 
     * @param event The close event
     */
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    /**
     * @brief Add a new download
     */
    void addDownload();
    
    /**
     * @brief Batch add downloads
     */
    void addBatchDownload();
    
    /**
     * @brief Paste URL and add download
     */
    void pasteUrlAndDownload();
    
    /**
     * @brief Start the selected download
     */
    void startDownload();
    
    /**
     * @brief Pause the selected download
     */
    void pauseDownload();
    
    /**
     * @brief Resume the selected download
     */
    void resumeDownload();
    
    /**
     * @brief Cancel the selected download
     */
    void cancelDownload();
    
    /**
     * @brief Remove the selected download
     */
    void removeDownload();
    
    /**
     * @brief Start all downloads
     */
    void startAllDownloads();
    
    /**
     * @brief Pause all downloads
     */
    void pauseAllDownloads();
    
    /**
     * @brief Show the settings dialog
     */
    void showSettings();
    
    /**
     * @brief Show the about dialog
     */
    void showAboutDialog();
    
    /**
     * @brief Handle tray icon activation
     * 
     * @param reason The activation reason
     */
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    
    /**
     * @brief Update the download items in the UI
     */
    void updateDownloadItems();
    
    /**
     * @brief Handle download item selection
     */
    void onDownloadItemSelected();
    
    /**
     * @brief Handle download item double click
     * 
     * @param item The item that was double-clicked
     * @param column The column that was double-clicked
     */
    void onDownloadItemDoubleClicked(QTreeWidgetItem* item, int column);
    
    /**
     * @brief Show the context menu for download items
     * 
     * @param pos The position where to show the menu
     */
    void showContextMenu(const QPoint& pos);
    
private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Create actions
     */
    void createActions();
    
    /**
     * @brief Create menus
     */
    void createMenus();
    
    /**
     * @brief Create toolbars
     */
    void createToolBars();
    
    /**
     * @brief Create the status bar
     */
    void createStatusBar();
    
    /**
     * @brief Create the tray icon
     */
    void createTrayIcon();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Load tasks into the UI
     */
    void loadTasks();
    
    /**
     * @brief Add a download task to the UI
     * 
     * @param task The task to add
     */
    void addDownloadToUi(std::shared_ptr<dm::core::DownloadTask> task);
    
    /**
     * @brief Update a download task in the UI
     * 
     * @param task The task to update
     */
    void updateDownloadInUi(std::shared_ptr<dm::core::DownloadTask> task);
    
    /**
     * @brief Remove a download task from the UI
     * 
     * @param task The task to remove
     */
    void removeDownloadFromUi(std::shared_ptr<dm::core::DownloadTask> task);
    
    /**
     * @brief Get the selected download task
     * 
     * @return std::shared_ptr<dm::core::DownloadTask> The selected task or nullptr
     */
    std::shared_ptr<dm::core::DownloadTask> getSelectedDownloadTask();
    
    /**
     * @brief Get selected download tasks
     * 
     * @return std::vector<std::shared_ptr<dm::core::DownloadTask>> The selected tasks
     */
    std::vector<std::shared_ptr<dm::core::DownloadTask>> getSelectedDownloadTasks();
    
    /**
     * @brief Task added handler
     * 
     * @param task The task that was added
     */
    void onTaskAdded(std::shared_ptr<dm::core::DownloadTask> task);
    
    /**
     * @brief Task removed handler
     * 
     * @param task The task that was removed
     */
    void onTaskRemoved(std::shared_ptr<dm::core::DownloadTask> task);
    
    /**
     * @brief Task status changed handler
     * 
     * @param task The task whose status changed
     * @param status The new status
     */
    void onTaskStatusChanged(std::shared_ptr<dm::core::DownloadTask> task, 
                           dm::core::DownloadStatus status);
    
    /**
     * @brief Update UI state based on selection
     */
    void updateUiState();
    
    // UI elements
    QTreeWidget* downloadList_;
    QToolBar* mainToolBar_;
    QStatusBar* statusBar_;
    QLabel* statusLabel_;
    QSystemTrayIcon* trayIcon_;
    
    // Actions
    QAction* addAction_;
    QAction* batchAddAction_;
    QAction* pasteUrlAction_;
    QAction* startAction_;
    QAction* pauseAction_;
    QAction* resumeAction_;
    QAction* cancelAction_;
    QAction* removeAction_;
    QAction* startAllAction_;
    QAction* pauseAllAction_;
    QAction* settingsAction_;
    QAction* exitAction_;
    QAction* aboutAction_;
    
    // Menus
    QMenu* fileMenu_;
    QMenu* downloadMenu_;
    QMenu* toolsMenu_;
    QMenu* helpMenu_;
    QMenu* trayMenu_;
    QMenu* contextMenu_;
    
    // Timers
    QTimer* updateTimer_;
    
    // Mapping of download IDs to list items
    QMap<QString, QTreeWidgetItem*> downloadItems_;
    
    // Download manager reference
    dm::core::DownloadManager& downloadManager_;
};

} // namespace ui
} // namespace dm

#endif // MAIN_WINDOW_H