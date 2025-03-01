#include "include/ui/CrawlerDialog.h"
#include "include/core/DownloadManager.h"
#include "include/core/WebsiteCrawler.h"
#include "include/utils/Logger.h"
#include "include/utils/FileUtils.h"
#include "include/utils/UrlParser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QProgressBar>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QCloseEvent>
#include <QUrl>
#include <QTextStream>
#include <QFile>
#include <QDateTime>

namespace dm {
namespace ui {

CrawlerDialog::CrawlerDialog(dm::core::DownloadManager& downloadManager, QWidget* parent)
    : QDialog(parent)
    , downloadManager_(downloadManager)
{
    setWindowTitle(tr("Website Crawler"));
    setMinimumSize(900, 700);
    
    // Setup UI
    setupUi();
    
    // Connect signals
    connectSignals();
    
    // Initialize crawler
    initializeCrawler();
    
    // Update UI state
    updateButtons();
}

CrawlerDialog::~CrawlerDialog() {
    if (statusTimer_) {
        statusTimer_->stop();
    }
    
    // Make sure crawling is stopped
    if (crawler_ && crawler_->isRunning()) {
        crawler_->stopCrawling();
    }
}

void CrawlerDialog::closeEvent(QCloseEvent* event) {
    if (crawler_ && crawler_->isRunning()) {
        QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Crawling in Progress"),
            tr("Website crawling is still in progress. Stop crawling and close?"),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (answer == QMessageBox::No) {
            event->ignore();
            return;
        }
        
        // Stop crawling
        crawler_->stopCrawling();
    }
    
    event->accept();
}

void CrawlerDialog::setupUi() {
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create a split view with configuration on left and results on right
    QHBoxLayout* splitLayout = new QHBoxLayout();
    
    // Left side - configuration
    QVBoxLayout* configLayout = new QVBoxLayout();
    
    // URL group
    QGroupBox* urlGroup = new QGroupBox(tr("Crawl Settings"), this);
    QFormLayout* urlFormLayout = new QFormLayout(urlGroup);
    
    // Start URL
    urlEdit_ = new QLineEdit(urlGroup);
    urlEdit_->setPlaceholderText(tr("https://example.com"));
    urlFormLayout->addRow(tr("Start URL:"), urlEdit_);
    
    // Crawl mode
    crawlModeCombo_ = new QComboBox(urlGroup);
    crawlModeCombo_->addItem(tr("Same Domain"), static_cast<int>(dm::core::CrawlMode::SAME_DOMAIN));
    crawlModeCombo_->addItem(tr("Same Host"), static_cast<int>(dm::core::CrawlMode::SAME_HOST));
    crawlModeCombo_->addItem(tr("Specified Domains"), static_cast<int>(dm::core::CrawlMode::SPECIFIED_DOMAINS));
    crawlModeCombo_->addItem(tr("Follow All Links"), static_cast<int>(dm::core::CrawlMode::FOLLOW_ALL));
    urlFormLayout->addRow(tr("Crawl Mode:"), crawlModeCombo_);
    
    // Max depth
    maxDepthSpin_ = new QSpinBox(urlGroup);
    maxDepthSpin_->setRange(1, 10);
    maxDepthSpin_->setValue(3);
    urlFormLayout->addRow(tr("Max Depth:"), maxDepthSpin_);
    
    // Max pages
    maxPagesSpin_ = new QSpinBox(urlGroup);
    maxPagesSpin_->setRange(1, 1000);
    maxPagesSpin_->setValue(100);
    urlFormLayout->addRow(tr("Max Pages:"), maxPagesSpin_);
    
    // Max concurrent connections
    maxConcurrentSpin_ = new QSpinBox(urlGroup);
    maxConcurrentSpin_->setRange(1, 10);
    maxConcurrentSpin_->setValue(5);
    urlFormLayout->addRow(tr("Max Concurrent Connections:"), maxConcurrentSpin_);
    
    // Placeholder for domain input (will be added dynamically based on mode)
    setupDomainInput();
    
    // Options
    respectRobotsTxtCheck_ = new QCheckBox(tr("Respect robots.txt"), urlGroup);
    respectRobotsTxtCheck_->setChecked(true);
    urlFormLayout->addRow("", respectRobotsTxtCheck_);
    
    downloadResourcesCheck_ = new QCheckBox(tr("Download Resources"), urlGroup);
    downloadResourcesCheck_->setChecked(true);
    urlFormLayout->addRow("", downloadResourcesCheck_);
    
    // File types group
    QGroupBox* fileTypesGroup = new QGroupBox(tr("File Types to Download"), this);
    QGridLayout* fileTypesLayout = new QGridLayout(fileTypesGroup);
    
    // File type checkboxes
    struct FileTypeData {
        QString name;
        QStringList extensions;
    };
    
    QList<FileTypeData> fileTypes = {
        { tr("Images"), { "jpg", "jpeg", "png", "gif", "bmp", "svg", "webp" } },
        { tr("Documents"), { "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "txt", "rtf", "odt" } },
        { tr("Archives"), { "zip", "rar", "7z", "tar", "gz", "bz2" } },
        { tr("Audio"), { "mp3", "wav", "ogg", "flac", "aac", "m4a" } },
        { tr("Video"), { "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm" } },
        { tr("Code"), { "html", "htm", "css", "js", "json", "xml", "csv" } }
    };
    
    int row = 0;
    int col = 0;
    for (const auto& type : fileTypes) {
        QCheckBox* checkbox = new QCheckBox(type.name + " (" + type.extensions.join(", ") + ")", fileTypesGroup);
        checkbox->setProperty("extensions", type.extensions);
        checkbox->setChecked(true);
        fileTypesLayout->addWidget(checkbox, row, col);
        fileTypeCheckBoxes_[type.name] = checkbox;
        
        // Alternate between columns
        col = (col + 1) % 2;
        if (col == 0) row++;
    }
    
    // Save options group
    QGroupBox* saveGroup = new QGroupBox(tr("Save Options"), this);
    QVBoxLayout* saveLayout = new QVBoxLayout(saveGroup);
    
    QHBoxLayout* savePathLayout = new QHBoxLayout();
    QLabel* savePathLabel = new QLabel(tr("Save Path:"), saveGroup);
    savePathEdit_ = new QLineEdit(saveGroup);
    savePathEdit_->setText(QString::fromStdString(downloadManager_.getDefaultDownloadDirectory()));
    browseSavePathButton_ = new QPushButton(tr("Browse..."), saveGroup);
    
    savePathLayout->addWidget(savePathLabel);
    savePathLayout->addWidget(savePathEdit_);
    savePathLayout->addWidget(browseSavePathButton_);
    
    createSubdirCheck_ = new QCheckBox(tr("Create subdirectories based on domain"), saveGroup);
    createSubdirCheck_->setChecked(true);
    
    saveLayout->addLayout(savePathLayout);
    saveLayout->addWidget(createSubdirCheck_);
    
    // Action buttons
    QHBoxLayout* actionButtonLayout = new QHBoxLayout();
    
    startButton_ = new QPushButton(tr("Start Crawling"), this);
    pauseButton_ = new QPushButton(tr("Pause"), this);
    stopButton_ = new QPushButton(tr("Stop"), this);
    
    actionButtonLayout->addWidget(startButton_);
    actionButtonLayout->addWidget(pauseButton_);
    actionButtonLayout->addWidget(stopButton_);
    actionButtonLayout->addStretch();
    
    // Add all elements to config layout
    configLayout->addWidget(urlGroup);
    configLayout->addWidget(fileTypesGroup);
    configLayout->addWidget(saveGroup);
    configLayout->addLayout(actionButtonLayout);
    configLayout->addStretch();
    
    // Right side - results
    QVBoxLayout* resultsLayout = new QVBoxLayout();
    
    // URL tree
    QLabel* urlsLabel = new QLabel(tr("Discovered Resources:"), this);
    urlTree_ = new QTreeWidget(this);
    urlTree_->setHeaderLabels({tr("URL"), tr("Type"), tr("Status")});
    urlTree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    urlTree_->setAlternatingRowColors(true);
    urlTree_->setSortingEnabled(true);
    urlTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    urlTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    urlTree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    
    // Results buttons
    QHBoxLayout* resultButtonLayout = new QHBoxLayout();
    
    addToDownloadButton_ = new QPushButton(tr("Add Selected to Downloads"), this);
    addAllToDownloadButton_ = new QPushButton(tr("Add All to Downloads"), this);
    saveUrlListButton_ = new QPushButton(tr("Save URL List"), this);
    clearUrlListButton_ = new QPushButton(tr("Clear List"), this);
    
    resultButtonLayout->addWidget(addToDownloadButton_);
    resultButtonLayout->addWidget(addAllToDownloadButton_);
    resultButtonLayout->addWidget(saveUrlListButton_);
    resultButtonLayout->addWidget(clearUrlListButton_);
    
    // Status group
    QGroupBox* statusGroup = new QGroupBox(tr("Crawling Status"), this);
    QGridLayout* statusLayout = new QGridLayout(statusGroup);
    
    statusLabel_ = new QLabel(tr("Ready to crawl"), statusGroup);
    progressBar_ = new QProgressBar(statusGroup);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    
    QLabel* visitedLabel = new QLabel(tr("Pages Visited:"), statusGroup);
    QLabel* foundLabel = new QLabel(tr("URLs Found:"), statusGroup);
    QLabel* resourcesLabel = new QLabel(tr("Resources Found:"), statusGroup);
    QLabel* queuedLabel = new QLabel(tr("Downloads Queued:"), statusGroup);
    
    visitedPagesLabel_ = new QLabel("0", statusGroup);
    foundUrlsLabel_ = new QLabel("0", statusGroup);
    resourcesFoundLabel_ = new QLabel("0", statusGroup);
    downloadQueuedLabel_ = new QLabel("0", statusGroup);
    
    statusLayout->addWidget(statusLabel_, 0, 0, 1, 4);
    statusLayout->addWidget(progressBar_, 1, 0, 1, 4);
    statusLayout->addWidget(visitedLabel, 2, 0);
    statusLayout->addWidget(visitedPagesLabel_, 2, 1);
    statusLayout->addWidget(foundLabel, 2, 2);
    statusLayout->addWidget(foundUrlsLabel_, 2, 3);
    statusLayout->addWidget(resourcesLabel, 3, 0);
    statusLayout->addWidget(resourcesFoundLabel_, 3, 1);
    statusLayout->addWidget(queuedLabel, 3, 2);
    statusLayout->addWidget(downloadQueuedLabel_, 3, 3);
    
    // Add all elements to results layout
    resultsLayout->addWidget(urlsLabel);
    resultsLayout->addWidget(urlTree_);
    resultsLayout->addLayout(resultButtonLayout);
    resultsLayout->addWidget(statusGroup);
    
    // Add left and right layouts to split view
    splitLayout->addLayout(configLayout, 1);
    splitLayout->addLayout(resultsLayout, 2);
    
    // Add split view to main layout
    mainLayout->addLayout(splitLayout);
    
    // Add dialog buttons at bottom
    QHBoxLayout* bottomButtonLayout = new QHBoxLayout();
    
    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    bottomButtonLayout->addStretch();
    bottomButtonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(bottomButtonLayout);
    
    // Create timer for status updates
    statusTimer_ = new QTimer(this);
    statusTimer_->setInterval(1000); // 1 second
    
    // Connect close button
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void CrawlerDialog::setupDomainInput() {
    // Remove previous widget if it exists
    if (allowedDomainsEdit_) {
        delete allowedDomainsEdit_;
    }
    
    // Only create domain input for SPECIFIED_DOMAINS mode
    if (crawlModeCombo_->currentData().toInt() == static_cast<int>(dm::core::CrawlMode::SPECIFIED_DOMAINS)) {
        allowedDomainsEdit_ = new QLineEdit(this);
        allowedDomainsEdit_->setPlaceholderText(tr("domain1.com, domain2.com, ..."));
        
        // Add to the form layout
        QFormLayout* formLayout = qobject_cast<QFormLayout*>(crawlModeCombo_->parentWidget()->layout());
        if (formLayout) {
            formLayout->addRow(tr("Allowed Domains:"), allowedDomainsEdit_);
        }
    } else {
        allowedDomainsEdit_ = nullptr;
    }
}

void CrawlerDialog::connectSignals() {
    // Crawl mode change
    connect(crawlModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &CrawlerDialog::onCrawlModeChanged);
    
    // Action buttons
    connect(startButton_, &QPushButton::clicked, this, &CrawlerDialog::onStartClicked);
    connect(pauseButton_, &QPushButton::clicked, this, &CrawlerDialog::onPauseClicked);
    connect(stopButton_, &QPushButton::clicked, this, &CrawlerDialog::onStopClicked);
    
    // Result buttons
    connect(addToDownloadButton_, &QPushButton::clicked, this, &CrawlerDialog::onAddToDownloadClicked);
    connect(addAllToDownloadButton_, &QPushButton::clicked, this, &CrawlerDialog::onAddAllToDownloadClicked);
    connect(saveUrlListButton_, &QPushButton::clicked, this, &CrawlerDialog::onSaveUrlListClicked);
    connect(clearUrlListButton_, &QPushButton::clicked, this, &CrawlerDialog::onClearUrlListClicked);
    
    // Browse button
    connect(browseSavePathButton_, &QPushButton::clicked, this, &CrawlerDialog::onBrowseSavePathClicked);
    
    // URL selection
    connect(urlTree_, &QTreeWidget::itemSelectionChanged, this, &CrawlerDialog::onUrlSelectionChanged);
    
    // File type changes
    for (auto checkbox : fileTypeCheckBoxes_.values()) {
        connect(checkbox, &QCheckBox::stateChanged, this, &CrawlerDialog::onFileTypesSelectionChanged);
    }
    
    // Timer
    connect(statusTimer_, &QTimer::timeout, this, &CrawlerDialog::updateStatus);
}

void CrawlerDialog::initializeCrawler() {
    // Create the crawler
    crawler_ = std::make_shared<dm::core::WebsiteCrawler>(downloadManager_);
    
    // Set initial UI state
    updateButtons();
}

void CrawlerDialog::updateButtons() {
    bool isRunning = false;
    bool isPaused = false;
    
    if (crawler_) {
        isRunning = crawler_->isRunning();
        isPaused = crawler_->isPaused();
    }
    
    // Configure buttons based on state
    startButton_->setEnabled(!isRunning);
    pauseButton_->setEnabled(isRunning);
    stopButton_->setEnabled(isRunning);
    
    // Update pause button text
    if (isPaused) {
        pauseButton_->setText(tr("Resume"));
    } else {
        pauseButton_->setText(tr("Pause"));
    }
    
    // Configure input fields
    urlEdit_->setEnabled(!isRunning);
    crawlModeCombo_->setEnabled(!isRunning);
    maxDepthSpin_->setEnabled(!isRunning);
    maxPagesSpin_->setEnabled(!isRunning);
    maxConcurrentSpin_->setEnabled(!isRunning);
    if (allowedDomainsEdit_) {
        allowedDomainsEdit_->setEnabled(!isRunning);
    }
    respectRobotsTxtCheck_->setEnabled(!isRunning);
    downloadResourcesCheck_->setEnabled(!isRunning);
    
    // File type checkboxes
    for (auto checkbox : fileTypeCheckBoxes_.values()) {
        checkbox->setEnabled(!isRunning);
    }
    
    // Save path
    savePathEdit_->setEnabled(!isRunning);
    browseSavePathButton_->setEnabled(!isRunning);
    createSubdirCheck_->setEnabled(!isRunning);
    
    // Result buttons
    bool hasItems = urlTree_->topLevelItemCount() > 0;
    bool hasSelection = !urlTree_->selectedItems().isEmpty();
    
    addToDownloadButton_->setEnabled(hasSelection);
    addAllToDownloadButton_->setEnabled(hasItems);
    saveUrlListButton_->setEnabled(hasItems);
    clearUrlListButton_->setEnabled(hasItems);
}

std::vector<std::string> CrawlerDialog::getSelectedFileTypes() const {
    std::vector<std::string> fileTypes;
    
    for (auto it = fileTypeCheckBoxes_.begin(); it != fileTypeCheckBoxes_.end(); ++it) {
        if (it.value()->isChecked()) {
            QStringList extensions = it.value()->property("extensions").toStringList();
            for (const QString& ext : extensions) {
                fileTypes.push_back(ext.toStdString());
            }
        }
    }
    
    return fileTypes;
}

void CrawlerDialog::onCrawlModeChanged(int index) {
    // Update domain input based on selected mode
    setupDomainInput();
}

void CrawlerDialog::onFileTypesSelectionChanged() {
    // This method is called when any file type checkbox changes
    // In a full implementation, you might want to update some UI accordingly
}

void CrawlerDialog::onUrlSelectionChanged() {
    // Enable/disable add to download button based on selection
    addToDownloadButton_->setEnabled(!urlTree_->selectedItems().isEmpty());
}

void CrawlerDialog::onStartClicked() {
    // Validate input
    QString url = urlEdit_->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a start URL."));
        return;
    }
    
    // Validate URL format
    QUrl qurl(url);
    if (!qurl.isValid() || qurl.scheme().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a valid URL including the scheme (e.g., http://, https://)."));
        return;
    }
    
    // Configure crawler options
    dm::core::CrawlOptions options;
    
    // Set crawl mode
    options.mode = static_cast<dm::core::CrawlMode>(
        crawlModeCombo_->currentData().toInt());
    
    // Set depth and pages limits
    options.maxDepth = maxDepthSpin_->value();
    options.maxPages = maxPagesSpin_->value();
    options.maxConcurrent = maxConcurrentSpin_->value();
    
    // Set robot and resource options
    options.respectRobotsTxt = respectRobotsTxtCheck_->isChecked();
    options.downloadResources = downloadResourcesCheck_->isChecked();
    
    // Set file types
    options.fileTypesToDownload = getSelectedFileTypes();
    
    // Set download directory
    options.downloadDirectory = savePathEdit_->text().toStdString();
    
    // Set allowed domains if in specified domains mode
    if (options.mode == dm::core::CrawlMode::SPECIFIED_DOMAINS && allowedDomainsEdit_) {
        QString domainsStr = allowedDomainsEdit_->text().trimmed();
        QStringList domainList = domainsStr.split(",", Qt::SkipEmptyParts);
        
        for (QString& domain : domainList) {
            domain = domain.trimmed();
            options.allowedDomains.push_back(domain.toStdString());
        }
        
        if (options.allowedDomains.empty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please enter at least one domain for the specified domains mode."));
            return;
        }
    }
    
    // Set custom download directory handler
    options.resourceHandler = [this](const std::string& url, dm::core::ResourceType type) {
        QString resourceType;
        
        // Convert resource type to string
        switch (type) {
            case dm::core::ResourceType::HTML_PAGE:
                resourceType = tr("HTML Page");
                break;
            case dm::core::ResourceType::IMAGE:
                resourceType = tr("Image");
                break;
            case dm::core::ResourceType::VIDEO:
                resourceType = tr("Video");
                break;
            case dm::core::ResourceType::AUDIO:
                resourceType = tr("Audio");
                break;
            case dm::core::ResourceType::DOCUMENT:
                resourceType = tr("Document");
                break;
            case dm::core::ResourceType::ARCHIVE:
                resourceType = tr("Archive");
                break;
            case dm::core::ResourceType::EXECUTABLE:
                resourceType = tr("Executable");
                break;
            default:
                resourceType = tr("Other");
        }
        
        // Add URL to tree (use QMetaObject for thread safety)
        QMetaObject::invokeMethod(this, "addUrlToTree", 
                               Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(url)),
                               Q_ARG(QString, resourceType));
    };
    
    // Clear current URL tree
    urlTree_->clear();
    
    // Start crawling
    bool success = crawler_->startCrawling(url.toStdString(), options, 
        [this](int pagesVisited, int totalUrls, int resourcesFound, int downloadQueued) {
            // Forward to our slot (use QMetaObject for thread safety)
            QMetaObject::invokeMethod(this, "onProgress", 
                                   Qt::QueuedConnection,
                                   Q_ARG(int, pagesVisited),
                                   Q_ARG(int, totalUrls),
                                   Q_ARG(int, resourcesFound),
                                   Q_ARG(int, downloadQueued));
        }
    );
    
    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to start crawling."));
        return;
    }
    
    // Update UI
    crawlingStarted_ = true;
    crawlingPaused_ = false;
    statusLabel_->setText(tr("Crawling in progress..."));
    updateButtons();
    
    // Start timer for status updates
    statusTimer_->start();
}

void CrawlerDialog::onPauseClicked() {
    if (!crawler_) return;
    
    if (crawler_->isPaused()) {
        // Resume
        crawler_->resumeCrawling();
        crawlingPaused_ = false;
        pauseButton_->setText(tr("Pause"));
        statusLabel_->setText(tr("Crawling resumed..."));
    } else {
        // Pause
        crawler_->pauseCrawling();
        crawlingPaused_ = true;
        pauseButton_->setText(tr("Resume"));
        statusLabel_->setText(tr("Crawling paused..."));
    }
    
    // Update UI
    updateButtons();
}

void CrawlerDialog::onStopClicked() {
    if (!crawler_) return;
    
    // Confirm with user
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Stop Crawling"),
        tr("Are you sure you want to stop the crawler?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (answer == QMessageBox::No) {
        return;
    }
    
    // Stop crawler
    crawler_->stopCrawling();
    
    // Update UI
    crawlingStarted_ = false;
    crawlingPaused_ = false;
    statusLabel_->setText(tr("Crawling stopped."));
    updateButtons();
    
    // Stop timer
    statusTimer_->stop();
}

void CrawlerDialog::onAddToDownloadClicked() {
    QList<QTreeWidgetItem*> selectedItems = urlTree_->selectedItems();
    
    if (selectedItems.isEmpty()) {
        return;
    }
    
    // Get destination path
    QString destinationPath = savePathEdit_->text();
    if (destinationPath.isEmpty()) {
        destinationPath = QString::fromStdString(downloadManager_.getDefaultDownloadDirectory());
    }
    
    // Add selected items to download manager
    int addedCount = 0;
    for (QTreeWidgetItem* item : selectedItems) {
        QString url = item->text(0);
        
        // Skip HTML pages if not explicitly selected
        if (item->text(1) == tr("HTML Page")) {
            continue;
        }
        
        // Create subdirectories if enabled
        QString savePath = destinationPath;
        if (createSubdirCheck_->isChecked()) {
            // Extract domain for subdirectory
            QUrl qurl(url);
            QString domain = qurl.host();
            if (!domain.isEmpty()) {
                savePath = QDir(savePath).filePath(domain);
                QDir().mkpath(savePath);
            }
        }
        
        // Add to download manager
        downloadManager_.addDownload(url.toStdString(), savePath.toStdString());
        
        // Update tree item status
        item->setText(2, tr("Added to Queue"));
        
        addedCount++;
    }
    
    // Show confirmation
    QMessageBox::information(this, tr("Downloads Added"),
                           tr("Added %1 items to download queue.").arg(addedCount));
}

void CrawlerDialog::onAddAllToDownloadClicked() {
    int itemCount = urlTree_->topLevelItemCount();
    
    if (itemCount == 0) {
        return;
    }
    
    // Confirm with user if a lot of items
    if (itemCount > 20) {
        QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Add All to Download"),
            tr("Are you sure you want to add all %1 items to the download queue?").arg(itemCount),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (answer == QMessageBox::No) {
            return;
        }
    }
    
    // Get destination path
    QString destinationPath = savePathEdit_->text();
    if (destinationPath.isEmpty()) {
        destinationPath = QString::fromStdString(downloadManager_.getDefaultDownloadDirectory());
    }
    
    // Add all items to download manager
    int addedCount = 0;
    for (int i = 0; i < itemCount; i++) {
        QTreeWidgetItem* item = urlTree_->topLevelItem(i);
        QString url = item->text(0);
        
        // Skip HTML pages
        if (item->text(1) == tr("HTML Page")) {
            continue;
        }
        
        // Create subdirectories if enabled
        QString savePath = destinationPath;
        if (createSubdirCheck_->isChecked()) {
            // Extract domain for subdirectory
            QUrl qurl(url);
            QString domain = qurl.host();
            if (!domain.isEmpty()) {
                savePath = QDir(savePath).filePath(domain);
                QDir().mkpath(savePath);
            }
        }
        
        // Add to download manager
        downloadManager_.addDownload(url.toStdString(), savePath.toStdString());
        
        // Update tree item status
        item->setText(2, tr("Added to Queue"));
        
        addedCount++;
    }
    
    // Show confirmation
    QMessageBox::information(this, tr("Downloads Added"),
                           tr("Added %1 items to download queue.").arg(addedCount));
}

void CrawlerDialog::onSaveUrlListClicked() {
    int itemCount = urlTree_->topLevelItemCount();
    
    if (itemCount == 0) {
        return;
    }
    
    // Get save file name
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save URL List"),
        QDir::homePath() + "/url_list.txt",
        tr("Text Files (*.txt);;All Files (*)")
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Open file for writing
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
                           tr("Could not open file for writing."));
        return;
    }
    
    QTextStream out(&file);
    
    // Write header
    out << "# URL List generated by Download Manager on " 
        << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << "# Format: URL\tType\n\n";
    
    // Write URLs
    for (int i = 0; i < itemCount; i++) {
        QTreeWidgetItem* item = urlTree_->topLevelItem(i);
        out << item->text(0) << "\t" << item->text(1) << "\n";
    }
    
    file.close();
    
    QMessageBox::information(this, tr("URL List Saved"),
                           tr("Saved %1 URLs to %2").arg(itemCount).arg(fileName));
}

void CrawlerDialog::onClearUrlListClicked() {
    int itemCount = urlTree_->topLevelItemCount();
    
    if (itemCount == 0) {
        return;
    }
    
    // Confirm with user
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Clear URL List"),
        tr("Are you sure you want to clear all %1 URLs from the list?").arg(itemCount),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (answer == QMessageBox::Yes) {
        urlTree_->clear();
        updateButtons();
    }
}

void CrawlerDialog::onBrowseSavePathClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Save Location"),
        savePathEdit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        savePathEdit_->setText(dir);
    }
}

void CrawlerDialog::updateStatus() {
    if (!crawler_) return;
    
    // Update UI based on crawler state
    bool isRunning = crawler_->isRunning();
    bool isPaused = crawler_->isPaused();
    
    if (!isRunning && crawlingStarted_) {
        // Crawler has stopped
        crawlingStarted_ = false;
        statusTimer_->stop();
        statusLabel_->setText(tr("Crawling completed."));
    }
    
    // Update buttons
    updateButtons();
}

void CrawlerDialog::onProgress(int pagesVisited, int totalUrls, int resourcesFound, int downloadQueued) {
    // Update statistics labels
    visitedPagesLabel_->setText(QString::number(pagesVisited));
    foundUrlsLabel_->setText(QString::number(totalUrls));
    resourcesFoundLabel_->setText(QString::number(resourcesFound));
    downloadQueuedLabel_->setText(QString::number(downloadQueued));
    
    // Update progress bar
    int maxPages = maxPagesSpin_->value();
    int progress = (pagesVisited * 100) / maxPages;
    progress = std::min(progress, 100);
    progressBar_->setValue(progress);
}

void CrawlerDialog::addUrlToTree(const QString& url, const QString& type) {
    // Create a new item
    QTreeWidgetItem* item = new QTreeWidgetItem(urlTree_);
    item->setText(0, url);
    item->setText(1, type);
    item->setText(2, tr("Found"));
    
    // Update button states
    updateButtons();
}

} // namespace ui
} // namespace dm