#ifndef BATCH_DOWNLOAD_DIALOG_H
#define BATCH_DOWNLOAD_DIALOG_H

#include <QDialog>
#include <QMap>
#include <QSet>
#include <memory>

class QTabWidget;
class QTextEdit;
class QLineEdit;
class QListWidget;
class QGroupBox;
class QCheckBox;
class QSpinBox;
class QButtonGroup;
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QProgressBar;
class QTimer;

namespace dm {
namespace core {
class DownloadManager;
class BatchDownloader;
class DownloadTask;
enum class DownloadStatus;
enum class BatchUrlSourceType;
}

namespace ui {

/**
 * @brief Dialog for batch download operations
 */
class BatchDownloadDialog : public QDialog {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new BatchDownloadDialog
     * 
     * @param parent The parent widget
     */
    explicit BatchDownloadDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the BatchDownloadDialog
     */
    ~BatchDownloadDialog();

protected:
    /**
     * @brief Drag enter event handler
     * 
     * @param event The drag enter event
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    
    /**
     * @brief Drop event handler
     * 
     * @param event The drop event
     */
    void dropEvent(QDropEvent* event) override;
    
private slots:
    /**
     * @brief Paste URLs from clipboard
     */
    void onPasteClicked();
    
    /**
     * @brief Clear URL edit box
     */
    void onClearUrlsClicked();
    
    /**
     * @brief Browse for input file
     */
    void onBrowseFileClicked();
    
    /**
     * @brief Import URLs from file
     */
    void onImportFileClicked();
    
    /**
     * @brief Generate URLs from pattern
     */
    void onGenerateUrlsClicked();
    
    /**
     * @brief Browse for destination directory
     */
    void onBrowseSavePathClicked();
    
    /**
     * @brief Add URLs to batch queue
     */
    void onAddUrlsClicked();
    
    /**
     * @brief Add a single URL
     */
    void onAddSingleUrlClicked();
    
    /**
     * @brief Remove selected URLs
     */
    void onRemoveUrlClicked();
    
    /**
     * @brief Remove all URLs
     */
    void onRemoveAllUrlsClicked();
    
    /**
     * @brief Start batch download
     */
    void onStartClicked();
    
    /**
     * @brief Pause/resume batch download
     */
    void onPauseClicked();
    
    /**
     * @brief Stop batch download
     */
    void onStopClicked();
    
    /**
     * @brief Clear batch list
     */
    void onClearListClicked();
    
    /**
     * @brief Update progress information
     */
    void updateProgress();
    
    /**
     * @brief Handle batch completion
     * 
     * @param successCount Number of successful downloads
     * @param failureCount Number of failed downloads
     */
    void onBatchCompleted(int successCount, int failureCount);
    
    /**
     * @brief Update batch progress information
     * 
     * @param processedCount Number of processed downloads
     * @param totalCount Total number of downloads
     * @param successCount Number of successful downloads
     * @param failureCount Number of failed downloads
     */
    void updateBatchProgress(int processedCount, int totalCount, int successCount, int failureCount);
    
    /**
     * @brief Update batch item status
     * 
     * @param index Item index
     * @param status Item status
     */
    void updateBatchItem(int index, int status);
    
private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Create the add URLs tab
     * 
     * @return QWidget* The tab widget
     */
    QWidget* createAddTab();
    
    /**
     * @brief Create the URL list tab
     * 
     * @return QWidget* The tab widget
     */
    QWidget* createListTab();
    
    /**
     * @brief Create the progress tab
     * 
     * @return QWidget* The tab widget
     */
    QWidget* createProgressTab();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Initialize the dialog
     */
    void initialize();
    
    /**
     * @brief Check if a file is a text file
     * 
     * @param filePath The file path
     * @return bool True if it's a text file, false otherwise
     */
    bool isTextFile(const QString& filePath);
    
    /**
     * @brief Import URLs from a file
     * 
     * @param filePath The file path
     */
    void importUrlsFromFile(const QString& filePath);
    
    /**
     * @brief Add URLs to the batch table
     * 
     * @param urls The URLs to add
     * @param savePath The save path for these URLs
     */
    void addUrlsToTable(const QStringList& urls, const QString& savePath);
    
    /**
     * @brief Update batch information display
     */
    void updateBatchInfo();
    
    /**
     * @brief Update button states
     */
    void updateButtons();
    
    /**
     * @brief Update progress table
     */
    void updateProgressTable();
    
    /**
     * @brief Format download speed for display
     * 
     * @param bytesPerSecond Speed in bytes per second
     * @return QString Formatted speed
     */
    QString formatSpeed(double bytesPerSecond);
    
    // UI elements - tabs
    QTabWidget* tabWidget_;
    
    // Add tab elements
    QTextEdit* urlListEdit_;
    QLineEdit* filePathEdit_;
    QLineEdit* patternEdit_;
    QSpinBox* patternFromSpin_;
    QSpinBox* patternToSpin_;
    QSpinBox* patternStepSpin_;
    QSpinBox* patternPaddingSpin_;
    QLineEdit* destinationEdit_;
    QSpinBox* maxConcurrentSpin_;
    QCheckBox* createSubdirsCheck_;
    QCheckBox* startImmediatelyCheck_;
    QCheckBox* skipExistingCheck_;
    
    // Add tab buttons
    QPushButton* pasteButton_;
    QPushButton* clearUrlsButton_;
    QPushButton* browseFileButton_;
    QPushButton* importFileButton_;
    QPushButton* generateButton_;
    QPushButton* browseSavePathButton_;
    QPushButton* addUrlsButton_;
    
    // List tab elements
    QTableWidget* urlTable_;
    QPushButton* addUrlButton_;
    QPushButton* removeUrlButton_;
    QPushButton* removeAllUrlsButton_;
    
    // Progress tab elements
    QTableWidget* progressTable_;
    QProgressBar* overallProgress_;
    QLabel* totalCountLabel_;
    QLabel* pendingCountLabel_;
    QLabel* activeCountLabel_;
    QLabel* completedCountLabel_;
    QLabel* failedCountLabel_;
    
    // Bottom buttons
    QPushButton* startButton_;
    QPushButton* pauseButton_;
    QPushButton* stopButton_;
    QPushButton* clearButton_;
    
    // Timer for updates
    QTimer* updateTimer_;
    
    // Batch state
    bool isPaused_ = false;
    
    // Download manager reference
    dm::core::DownloadManager& downloadManager_;
    std::shared_ptr<dm::core::BatchDownloader> batchDownloader_;
};

} // namespace ui
} // namespace dm

#endif // BATCH_DOWNLOAD_DIALOG_H