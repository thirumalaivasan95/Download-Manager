#ifndef CRAWLER_DIALOG_H
#define CRAWLER_DIALOG_H

#include <QDialog>
#include <QMap>
#include <memory>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QTableWidget;
class QProgressBar;
class QTreeWidget;
class QLabel;
class QPushButton;
class QGroupBox;
class QTimer;

namespace dm {
namespace core {
class DownloadManager;
class WebsiteCrawler;
enum class CrawlMode;
}

namespace ui {

/**
 * @brief Dialog for website crawling and download
 */
class CrawlerDialog : public QDialog {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new CrawlerDialog
     * 
     * @param downloadManager Reference to download manager
     * @param parent The parent widget
     */
    explicit CrawlerDialog(dm::core::DownloadManager& downloadManager, QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the CrawlerDialog
     */
    ~CrawlerDialog();
    
protected:
    /**
     * @brief Close event handler
     * 
     * @param event The close event
     */
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    /**
     * @brief Start crawling
     */
    void onStartClicked();
    
    /**
     * @brief Pause or resume crawling
     */
    void onPauseClicked();
    
    /**
     * @brief Stop crawling
     */
    void onStopClicked();
    
    /**
     * @brief Add selected URLs to download queue
     */
    void onAddToDownloadClicked();
    
    /**
     * @brief Add all URLs to download queue
     */
    void onAddAllToDownloadClicked();
    
    /**
     * @brief Save URL list to a file
     */
    void onSaveUrlListClicked();
    
    /**
     * @brief Clear URL list
     */
    void onClearUrlListClicked();
    
    /**
     * @brief Browse for save directory
     */
    void onBrowseSavePathClicked();
    
    /**
     * @brief URL tree selection changed handler
     */
    void onUrlSelectionChanged();
    
    /**
     * @brief Crawl mode changed handler
     * 
     * @param index The new combo box index
     */
    void onCrawlModeChanged(int index);
    
    /**
     * @brief File types selection changed handler
     */
    void onFileTypesSelectionChanged();
    
    /**
     * @brief Update status timer handler
     */
    void updateStatus();
    
    /**
     * @brief Progress callback handler
     * 
     * @param pagesVisited Number of pages visited
     * @param totalUrls Total number of URLs found
     * @param resourcesFound Number of resources found
     * @param downloadQueued Number of downloads queued
     */
    void onProgress(int pagesVisited, int totalUrls, int resourcesFound, int downloadQueued);
    
private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Initialize the crawler
     */
    void initializeCrawler();
    
    /**
     * @brief Update button states
     */
    void updateButtons();
    
    /**
     * @brief Add domains input field based on crawl mode
     */
    void setupDomainInput();
    
    /**
     * @brief Get selected file types
     * 
     * @return std::vector<std::string> Selected file types
     */
    std::vector<std::string> getSelectedFileTypes() const;
    
    /**
     * @brief Add URL to tree view
     * 
     * @param url The URL to add
     * @param type The resource type
     */
    void addUrlToTree(const QString& url, const QString& type);
    
    // UI elements - core inputs
    QLineEdit* urlEdit_;
    QSpinBox* maxDepthSpin_;
    QSpinBox* maxPagesSpin_;
    QSpinBox* maxConcurrentSpin_;
    QComboBox* crawlModeCombo_;
    QLineEdit* allowedDomainsEdit_;
    QCheckBox* respectRobotsTxtCheck_;
    QCheckBox* downloadResourcesCheck_;
    
    // UI elements - file types and options
    QMap<QString, QCheckBox*> fileTypeCheckBoxes_;
    QLineEdit* savePathEdit_;
    QCheckBox* createSubdirCheck_;
    
    // UI elements - URLs and resources view
    QTreeWidget* urlTree_;
    QLabel* statusLabel_;
    QProgressBar* progressBar_;
    
    // Statistics labels
    QLabel* visitedPagesLabel_;
    QLabel* foundUrlsLabel_;
    QLabel* resourcesFoundLabel_;
    QLabel* downloadQueuedLabel_;
    
    // Buttons
    QPushButton* startButton_;
    QPushButton* pauseButton_;
    QPushButton* stopButton_;
    QPushButton* addToDownloadButton_;
    QPushButton* addAllToDownloadButton_;
    QPushButton* saveUrlListButton_;
    QPushButton* clearUrlListButton_;
    QPushButton* browseSavePathButton_;
    
    // Timer for status updates
    QTimer* statusTimer_;
    
    // State variables
    bool crawlingStarted_ = false;
    bool crawlingPaused_ = false;
    
    // Download manager and crawler
    dm::core::DownloadManager& downloadManager_;
    std::shared_ptr<dm::core::WebsiteCrawler> crawler_;
};

} // namespace ui
} // namespace dm

#endif // CRAWLER_DIALOG_H