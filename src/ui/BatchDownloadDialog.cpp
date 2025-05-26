#include "include/ui/BatchDownloadDialog.h"
#include "include/core/BatchDownloader.h"
#include "core/DownloadManager.h"
#include "include/utils/Logger.h"
#include "include/utils/FileUtils.h"
#include "include/utils/UrlParser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QProgressBar>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QShortcut>
#include <QKeySequence>
#include <QGroupBox>
#include <QTimer>
#include <QIcon>

namespace dm {
namespace ui {

BatchDownloadDialog::BatchDownloadDialog(QWidget* parent)
    : QDialog(parent)
    , downloadManager_(dm::core::DownloadManager::getInstance())
{
    setWindowTitle(tr("Batch Download"));
    setMinimumSize(800, 600);
    
    // Create UI
    setupUi();
    
    // Connect signals
    connectSignals();
    
    // Initialize
    initialize();
    
    // Accept drag & drop
    setAcceptDrops(true);
}

BatchDownloadDialog::~BatchDownloadDialog() {
    // Stop timer if exists
    if (updateTimer_) {
        updateTimer_->stop();
    }
}

void BatchDownloadDialog::setupUi() {
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tabs
    tabWidget_ = new QTabWidget(this);
    
    // Create tabs
    QWidget* addTab = createAddTab();
    QWidget* listTab = createListTab();
    QWidget* progressTab = createProgressTab();
    
    // Add tabs
    tabWidget_->addTab(addTab, tr("Add URLs"));
    tabWidget_->addTab(listTab, tr("URL List"));
    tabWidget_->addTab(progressTab, tr("Progress"));
    
    // Bottom buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    startButton_ = new QPushButton(tr("Start"), this);
    startButton_->setIcon(QIcon(":/icons/batch.png"));
    
    pauseButton_ = new QPushButton(tr("Pause"), this);
    pauseButton_->setIcon(QIcon(":/icons/pause.png"));
    
    stopButton_ = new QPushButton(tr("Stop"), this);
    stopButton_->setIcon(QIcon(":/icons/cancel.png"));
    
    clearButton_ = new QPushButton(tr("Clear List"), this);
    clearButton_->setIcon(QIcon(":/icons/remove.png"));
    
    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(stopButton_);
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    // Add to main layout
    mainLayout->addWidget(tabWidget_);
    mainLayout->addLayout(buttonLayout);
    
    // Create timer for updates
    updateTimer_ = new QTimer(this);
    updateTimer_->setInterval(1000); // 1 second
    
    // Set initial button states
    pauseButton_->setEnabled(false);
    stopButton_->setEnabled(false);
}

QWidget* BatchDownloadDialog::createAddTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // URL input section
    QGroupBox* urlInputGroup = new QGroupBox(tr("URL Input"), tab);
    QVBoxLayout* urlInputLayout = new QVBoxLayout(urlInputGroup);
    
    urlListEdit_ = new QTextEdit(urlInputGroup);
    urlListEdit_->setPlaceholderText(tr("Enter URLs, one per line"));
    urlInputLayout->addWidget(urlListEdit_);
    
    QHBoxLayout* urlButtonsLayout = new QHBoxLayout();
    
    QPushButton* pasteButton = new QPushButton(tr("Paste"), urlInputGroup);
    QPushButton* clearButton = new QPushButton(tr("Clear"), urlInputGroup);
    
    urlButtonsLayout->addWidget(pasteButton);
    urlButtonsLayout->addWidget(clearButton);
    urlButtonsLayout->addStretch();
    
    urlInputLayout->addLayout(urlButtonsLayout);
    
    // File input section
    QGroupBox* fileInputGroup = new QGroupBox(tr("Import from File"), tab);
    QVBoxLayout* fileInputLayout = new QVBoxLayout(fileInputGroup);
    
    QHBoxLayout* fileSelectLayout = new QHBoxLayout();
    
    filePathEdit_ = new QLineEdit(fileInputGroup);
    filePathEdit_->setReadOnly(true);
    
    QPushButton* browseButton = new QPushButton(tr("Browse"), fileInputGroup);
    
    fileSelectLayout->addWidget(filePathEdit_);
    fileSelectLayout->addWidget(browseButton);
    
    fileInputLayout->addLayout(fileSelectLayout);
    
    QPushButton* importButton = new QPushButton(tr("Import URLs from File"), fileInputGroup);
    fileInputLayout->addWidget(importButton);
    
    // Pattern generation section
    QGroupBox* patternGroup = new QGroupBox(tr("URL Pattern Generation"), tab);
    QFormLayout* patternLayout = new QFormLayout(patternGroup);
    
    patternEdit_ = new QLineEdit(patternGroup);
    patternEdit_->setPlaceholderText(tr("URL pattern with {$PATTERN} placeholder, e.g., http://example.com/file{$PATTERN}.zip"));
    
    QHBoxLayout* rangeLayout = new QHBoxLayout();
    QLabel* fromLabel = new QLabel(tr("From:"), patternGroup);
    QLabel* toLabel = new QLabel(tr("To:"), patternGroup);
    QLabel* stepLabel = new QLabel(tr("Step:"), patternGroup);
    QLabel* paddingLabel = new QLabel(tr("Padding:"), patternGroup);
    
    patternFromSpin_ = new QSpinBox(patternGroup);
    patternFromSpin_->setRange(0, 9999);
    patternFromSpin_->setValue(1);
    
    patternToSpin_ = new QSpinBox(patternGroup);
    patternToSpin_->setRange(0, 9999);
    patternToSpin_->setValue(10);
    
    patternStepSpin_ = new QSpinBox(patternGroup);
    patternStepSpin_->setRange(1, 100);
    patternStepSpin_->setValue(1);
    
    patternPaddingSpin_ = new QSpinBox(patternGroup);
    patternPaddingSpin_->setRange(0, 10);
    patternPaddingSpin_->setValue(0);
    patternPaddingSpin_->setSpecialValueText(tr("None"));
    
    rangeLayout->addWidget(fromLabel);
    rangeLayout->addWidget(patternFromSpin_);
    rangeLayout->addWidget(toLabel);
    rangeLayout->addWidget(patternToSpin_);
    rangeLayout->addWidget(stepLabel);
    rangeLayout->addWidget(patternStepSpin_);
    rangeLayout->addWidget(paddingLabel);
    rangeLayout->addWidget(patternPaddingSpin_);
    
    patternLayout->addRow(tr("Pattern:"), patternEdit_);
    patternLayout->addRow(tr("Range:"), rangeLayout);
    
    QPushButton* generateButton = new QPushButton(tr("Generate URLs"), patternGroup);
    patternLayout->addRow("", generateButton);
    
    // Download options section
    QGroupBox* optionsGroup = new QGroupBox(tr("Download Options"), tab);
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);
    
    destinationEdit_ = new QLineEdit(optionsGroup);
    destinationEdit_->setText(QString::fromStdString(downloadManager_.getDefaultDownloadDirectory()));
    
    QPushButton* browseDestButton = new QPushButton(tr("Browse"), optionsGroup);
    
    QHBoxLayout* destLayout = new QHBoxLayout();
    destLayout->addWidget(destinationEdit_);
    destLayout->addWidget(browseDestButton);
    
    maxConcurrentSpin_ = new QSpinBox(optionsGroup);
    maxConcurrentSpin_->setRange(1, 10);
    maxConcurrentSpin_->setValue(3);
    
    createSubdirsCheck_ = new QCheckBox(tr("Create subdirectories based on URL structure"), optionsGroup);
    startImmediatelyCheck_ = new QCheckBox(tr("Start downloads immediately"), optionsGroup);
    skipExistingCheck_ = new QCheckBox(tr("Skip existing files"), optionsGroup);
    
    createSubdirsCheck_->setChecked(true);
    startImmediatelyCheck_->setChecked(true);
    skipExistingCheck_->setChecked(true);
    
    optionsLayout->addRow(tr("Save To:"), destLayout);
    optionsLayout->addRow(tr("Max Concurrent Downloads:"), maxConcurrentSpin_);
    optionsLayout->addRow("", createSubdirsCheck_);
    optionsLayout->addRow("", startImmediatelyCheck_);
    optionsLayout->addRow("", skipExistingCheck_);
    
    // Add URL button
    QPushButton* addUrlsButton = new QPushButton(tr("Add URLs to Queue"), tab);
    addUrlsButton->setIcon(QIcon(":/icons/add.png"));
    
    // Add all to layout
    layout->addWidget(urlInputGroup);
    layout->addWidget(fileInputGroup);
    layout->addWidget(patternGroup);
    layout->addWidget(optionsGroup);
    layout->addWidget(addUrlsButton);
    
    // Store button references
    pasteButton_ = pasteButton;
    clearUrlsButton_ = clearButton;
    browseFileButton_ = browseButton;
    importFileButton_ = importButton;
    generateButton_ = generateButton;
    browseSavePathButton_ = browseDestButton;
    addUrlsButton_ = addUrlsButton;
    
    return tab;
}

QWidget* BatchDownloadDialog::createListTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // URL list table
    urlTable_ = new QTableWidget(0, 3, tab);
    
    // Set headers
    QStringList headers;
    headers << tr("URL") << tr("Status") << tr("Save Path");
    urlTable_->setHorizontalHeaderLabels(headers);
    
    // Set header properties
    urlTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    urlTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    urlTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    
    // Set table properties
    urlTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    urlTable_->setAlternatingRowColors(true);
    urlTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Button panel
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* addButton = new QPushButton(tr("Add URL"), tab);
    addButton->setIcon(QIcon(":/icons/add.png"));
    
    QPushButton* removeButton = new QPushButton(tr("Remove"), tab);
    removeButton->setIcon(QIcon(":/icons/remove.png"));
    
    QPushButton* removeAllButton = new QPushButton(tr("Remove All"), tab);
    
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(removeAllButton);
    buttonLayout->addStretch();
    
    // Add to layout
    layout->addWidget(urlTable_);
    layout->addLayout(buttonLayout);
    
    // Store button references
    addUrlButton_ = addButton;
    removeUrlButton_ = removeButton;
    removeAllUrlsButton_ = removeAllButton;
    
    return tab;
}

QWidget* BatchDownloadDialog::createProgressTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Progress table
    progressTable_ = new QTableWidget(0, 4, tab);
    
    // Set headers
    QStringList headers;
    headers << tr("URL") << tr("Progress") << tr("Status") << tr("Speed");
    progressTable_->setHorizontalHeaderLabels(headers);
    
    // Set header properties
    progressTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    progressTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    progressTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    progressTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    
    // Set table properties
    progressTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    progressTable_->setAlternatingRowColors(true);
    progressTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Overall progress
    QGroupBox* overallGroup = new QGroupBox(tr("Overall Progress"), tab);
    QVBoxLayout* overallLayout = new QVBoxLayout(overallGroup);
    
    overallProgress_ = new QProgressBar(overallGroup);
    overallProgress_->setRange(0, 100);
    overallProgress_->setValue(0);
    overallProgress_->setFormat(tr("%p% (%v/%m)"));
    overallProgress_->setAlignment(Qt::AlignCenter);
    
    // Status labels
    QGridLayout* statusLayout = new QGridLayout();
    
    QLabel* totalLabel = new QLabel(tr("Total:"), overallGroup);
    QLabel* pendingLabel = new QLabel(tr("Pending:"), overallGroup);
    QLabel* activeLabel = new QLabel(tr("Active:"), overallGroup);
    QLabel* completedLabel = new QLabel(tr("Completed:"), overallGroup);
    QLabel* failedLabel = new QLabel(tr("Failed:"), overallGroup);
    
    totalCountLabel_ = new QLabel("0", overallGroup);
    pendingCountLabel_ = new QLabel("0", overallGroup);
    activeCountLabel_ = new QLabel("0", overallGroup);
    completedCountLabel_ = new QLabel("0", overallGroup);
    failedCountLabel_ = new QLabel("0", overallGroup);
    
    statusLayout->addWidget(totalLabel, 0, 0);
    statusLayout->addWidget(totalCountLabel_, 0, 1);
    statusLayout->addWidget(pendingLabel, 0, 2);
    statusLayout->addWidget(pendingCountLabel_, 0, 3);
    statusLayout->addWidget(activeLabel, 1, 0);
    statusLayout->addWidget(activeCountLabel_, 1, 1);
    statusLayout->addWidget(completedLabel, 1, 2);
    statusLayout->addWidget(completedCountLabel_, 1, 3);
    statusLayout->addWidget(failedLabel, 2, 0);
    statusLayout->addWidget(failedCountLabel_, 2, 1);
    
    overallLayout->addWidget(overallProgress_);
    overallLayout->addLayout(statusLayout);
    
    // Add to layout
    layout->addWidget(progressTable_);
    layout->addWidget(overallGroup);
    
    return tab;
}

void BatchDownloadDialog::connectSignals() {
    // Add tab buttons
    connect(pasteButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onPasteClicked);
    connect(clearUrlsButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onClearUrlsClicked);
    connect(browseFileButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onBrowseFileClicked);
    connect(importFileButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onImportFileClicked);
    connect(generateButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onGenerateUrlsClicked);
    connect(browseSavePathButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onBrowseSavePathClicked);
    connect(addUrlsButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onAddUrlsClicked);
    
    // List tab buttons
    connect(addUrlButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onAddSingleUrlClicked);
    connect(removeUrlButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onRemoveUrlClicked);
    connect(removeAllUrlsButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onRemoveAllUrlsClicked);
    
    // Bottom buttons
    connect(startButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onStartClicked);
    connect(pauseButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onPauseClicked);
    connect(stopButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onStopClicked);
    connect(clearButton_, &QPushButton::clicked, this, &BatchDownloadDialog::onClearListClicked);
    
    // Timer
    connect(updateTimer_, &QTimer::timeout, this, &BatchDownloadDialog::updateProgress);
    
    // Shortcut for pasting URLs
    QShortcut* pasteShortcut = new QShortcut(QKeySequence::Paste, urlListEdit_);
    connect(pasteShortcut, &QShortcut::activated, this, &BatchDownloadDialog::onPasteClicked);
}

void BatchDownloadDialog::initialize() {
    // Initialize batch downloader
    batchDownloader_ = std::make_shared<dm::core::BatchDownloader>(downloadManager_);
    
    // Register progress callback
    batchDownloader_->setProgressCallback(
        [this](int processedCount, int totalCount, double overallProgress, int successCount, int failureCount) {
            // Use QMetaObject to ensure thread safety
            QMetaObject::invokeMethod(this, "updateBatchProgress", 
                               Qt::QueuedConnection,
                               Q_ARG(int, processedCount),
                               Q_ARG(int, totalCount),
                               Q_ARG(int, successCount),
                               Q_ARG(int, failureCount));
        }
    );
    
    // Update UI state
    updateButtons();
    
    // Start timer
    updateTimer_->start();
}

void BatchDownloadDialog::dragEnterEvent(QDragEnterEvent* event) {
    // Accept drag & drop of URLs or files
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void BatchDownloadDialog::dropEvent(QDropEvent* event) {
    // Handle URL drops
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        
        // Process URLs
        QString text;
        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                // If it's a local file, check if it's a text file containing URLs
                QString filePath = url.toLocalFile();
                if (isTextFile(filePath)) {
                    // Import URLs from file
                    importUrlsFromFile(filePath);
                } else {
                    // Add the file URL
                    text += url.toString() + "\n";
                }
            } else {
                // Regular URL
                text += url.toString() + "\n";
            }
        }
        
        // Add to text edit if we have URLs
        if (!text.isEmpty()) {
            // Add to current text
            urlListEdit_->append(text);
        }
        
        event->acceptProposedAction();
    } 
    else if (event->mimeData()->hasText()) {
        // Text drop - add to URL edit
        QString text = event->mimeData()->text();
        urlListEdit_->append(text);
        event->acceptProposedAction();
    }
}

bool BatchDownloadDialog::isTextFile(const QString& filePath) {
    // Simple check if file has a text file extension
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    return extension == "txt" || extension == "csv" || extension == "list";
}

void BatchDownloadDialog::importUrlsFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), 
                           tr("Failed to open file: %1").arg(filePath));
        return;
    }
    
    QTextStream in(&file);
    QString text = in.readAll();
    file.close();
    
    // Add to URL edit
    if (!text.isEmpty()) {
        urlListEdit_->append(text);
    }
}

void BatchDownloadDialog::onPasteClicked() {
    QClipboard* clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    
    if (!text.isEmpty()) {
        urlListEdit_->append(text);
    }
}

void BatchDownloadDialog::onClearUrlsClicked() {
    urlListEdit_->clear();
}

void BatchDownloadDialog::onBrowseFileClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Select File with URLs"), 
        QDir::homePath(), 
        tr("Text Files (*.txt);;CSV Files (*.csv);;All Files (*)")
    );
    
    if (!filePath.isEmpty()) {
        filePathEdit_->setText(filePath);
    }
}

void BatchDownloadDialog::onImportFileClicked() {
    QString filePath = filePathEdit_->text();
    
    if (filePath.isEmpty()) {
        // No file selected, show browse dialog
        onBrowseFileClicked();
        filePath = filePathEdit_->text();
        
        if (filePath.isEmpty()) {
            return;
        }
    }
    
    // Import URLs from file
    importUrlsFromFile(filePath);
}

void BatchDownloadDialog::onGenerateUrlsClicked() {
    QString pattern = patternEdit_->text();
    
    if (pattern.isEmpty() || !pattern.contains("{$PATTERN}")) {
        QMessageBox::warning(this, tr("Error"), 
                           tr("Please enter a valid URL pattern with {$PATTERN} placeholder."));
        return;
    }
    
    int from = patternFromSpin_->value();
    int to = patternToSpin_->value();
    int step = patternStepSpin_->value();
    int padding = patternPaddingSpin_->value();
    
    if (from > to) {
        QMessageBox::warning(this, tr("Error"), 
                           tr("Start value must be less than or equal to end value."));
        return;
    }
    
    // Generate URLs
    QStringList urls;
    
    for (int i = from; i <= to; i += step) {
        QString replacement;
        
        if (padding > 0) {
            // Format with leading zeros
            replacement = QString("%1").arg(i, padding, 10, QChar('0'));
        } else {
            replacement = QString::number(i);
        }
        
        // Replace placeholder with current number
        QString url = pattern;
        url.replace("{$PATTERN}", replacement);
        
        urls.append(url);
    }
    
    // Add to URL edit
    urlListEdit_->append(urls.join("\n"));
}

void BatchDownloadDialog::onBrowseSavePathClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Save Location"),
        destinationEdit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        destinationEdit_->setText(dir);
    }
}

void BatchDownloadDialog::onAddUrlsClicked() {
    // Get URLs from text edit
    QString text = urlListEdit_->toPlainText();
    
    if (text.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter at least one URL."));
        return;
    }
    
    QStringList lines = text.split("\n", Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter at least one URL."));
        return;
    }
    
    // Get save path
    QString savePath = destinationEdit_->text();
    if (savePath.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a save location."));
        return;
    }
    
    // Add to table
    addUrlsToTable(lines, savePath);
    
    // Clear URL edit
    urlListEdit_->clear();
}

void BatchDownloadDialog::onAddSingleUrlClicked() {
    // Show dialog to enter a single URL
    QString url = QInputDialog::getText(
        this, tr("Add URL"), 
        tr("Enter URL:"), 
        QLineEdit::Normal
    );
    
    if (url.isEmpty()) {
        return;
    }
    
    // Get save path
    QString savePath = destinationEdit_->text();
    if (savePath.isEmpty()) {
        savePath = QString::fromStdString(downloadManager_.getDefaultDownloadDirectory());
    }
    
    // Add to table
    QStringList urls;
    urls << url;
    addUrlsToTable(urls, savePath);
}

void BatchDownloadDialog::onRemoveUrlClicked() {
    // Get selected rows
    QList<QTableWidgetItem*> selectedItems = urlTable_->selectedItems();
    
    if (selectedItems.isEmpty()) {
        return;
    }
    
    QSet<int> rows;
    for (QTableWidgetItem* item : selectedItems) {
        rows.insert(item->row());
    }
    
    // Remove rows in reverse order
    QList<int> rowList = rows.values();
    std::sort(rowList.begin(), rowList.end(), std::greater<int>());
    
    for (int row : rowList) {
        urlTable_->removeRow(row);
    }
    
    // Update batch info
    updateBatchInfo();
}

void BatchDownloadDialog::onRemoveAllUrlsClicked() {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Confirm"), 
        tr("Are you sure you want to remove all URLs from the list?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        urlTable_->setRowCount(0);
        
        // Update batch info
        updateBatchInfo();
    }
}

void BatchDownloadDialog::onStartClicked() {
    if (urlTable_->rowCount() == 0) {
        QMessageBox::warning(this, tr("Error"), tr("No URLs in the batch list."));
        return;
    }
    
    // Collect URLs and destination directories
    std::vector<std::string> urls;
    std::map<std::string, std::string> urlDirs;
    
    for (int i = 0; i < urlTable_->rowCount(); i++) {
        QString url = urlTable_->item(i, 0)->text();
        QString savePath = urlTable_->item(i, 2)->text();
        
        urls.push_back(url.toStdString());
        urlDirs[url.toStdString()] = savePath.toStdString();
    }
    
    // Clear progress table
    progressTable_->setRowCount(0);
    
    // Create batch download configuration
    dm::core::BatchDownloadConfig config;
    config.destinationDirectory = destinationEdit_->text().toStdString();
    config.createSubdirectories = createSubdirsCheck_->isChecked();
    config.skipExistingFiles = skipExistingCheck_->isChecked();
    config.startImmediately = startImmediatelyCheck_->isChecked();
    config.maxConcurrentFiles = maxConcurrentSpin_->value();
    config.sourceType = dm::core::BatchUrlSourceType::URL_LIST;
    
    // Start batch download
    bool success = batchDownloader_->startBatchJob(
        config,
        // Progress callback is already set in initialize()
        nullptr,
        // Completion callback
        [this](int successCount, int failureCount, const std::vector<std::string>& failedUrls) {
            QMetaObject::invokeMethod(this, "onBatchCompleted", 
                                   Qt::QueuedConnection,
                                   Q_ARG(int, successCount),
                                   Q_ARG(int, failureCount));
        }
    );
    
    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to start batch download."));
        return;
    }
    
    // Update UI
    updateButtons();
    updateBatchInfo();
}

void BatchDownloadDialog::onPauseClicked() {
    if (!batchDownloader_) return;
    
    if (isPaused_) {
        // Resume
        batchDownloader_->resumeBatchJob();
        pauseButton_->setText(tr("Pause"));
        pauseButton_->setIcon(QIcon(":/icons/pause.png"));
        isPaused_ = false;
    } else {
        // Pause
        batchDownloader_->cancelBatchJob();
        pauseButton_->setText(tr("Resume"));
        pauseButton_->setIcon(QIcon(":/icons/resume.png"));
        isPaused_ = true;
    }
    
    // Update UI
    updateButtons();
}

void BatchDownloadDialog::onStopClicked() {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Confirm"), 
        tr("Are you sure you want to stop the batch download?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes && batchDownloader_) {
        // Stop batch download
        batchDownloader_->cancelBatchJob();
        isPaused_ = false;
        
        // Update UI
        updateButtons();
    }
}

void BatchDownloadDialog::onClearListClicked() {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Confirm"), 
        tr("Are you sure you want to clear the batch list?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Stop if running
        if (batchDownloader_ && batchDownloader_->isJobRunning()) {
            batchDownloader_->cancelBatchJob();
        }
        
        // Clear tables
        urlTable_->setRowCount(0);
        progressTable_->setRowCount(0);
        
        // Update UI
        updateButtons();
        updateBatchInfo();
    }
}

void BatchDownloadDialog::addUrlsToTable(const QStringList& urls, const QString& savePath) {
    if (urls.isEmpty()) {
        return;
    }
    
    // Add to table
    for (const QString& url : urls) {
        if (url.trimmed().isEmpty()) {
            continue;
        }
        
        // Check if URL is already in the list
        bool exists = false;
        for (int i = 0; i < urlTable_->rowCount(); i++) {
            if (urlTable_->item(i, 0)->text() == url) {
                exists = true;
                break;
            }
        }
        
        if (exists) {
            continue;
        }
        
        // Add new row
        int row = urlTable_->rowCount();
        urlTable_->insertRow(row);
        
        // Set items
        QTableWidgetItem* urlItem = new QTableWidgetItem(url);
        QTableWidgetItem* statusItem = new QTableWidgetItem(tr("Pending"));
        QTableWidgetItem* savePathItem = new QTableWidgetItem(savePath);
        
        urlTable_->setItem(row, 0, urlItem);
        urlTable_->setItem(row, 1, statusItem);
        urlTable_->setItem(row, 2, savePathItem);
    }
    
    // Update batch info
    updateBatchInfo();
}

void BatchDownloadDialog::updateBatchInfo() {
    // Update counts
    totalCountLabel_->setText(QString::number(urlTable_->rowCount()));
    pendingCountLabel_->setText(QString::number(urlTable_->rowCount()));
    activeCountLabel_->setText("0");
    completedCountLabel_->setText("0");
    failedCountLabel_->setText("0");
    
    // Update progress
    overallProgress_->setValue(0);
    overallProgress_->setMaximum(std::max(1, urlTable_->rowCount()));
    overallProgress_->setFormat(tr("%p% (%v/%m)"));
}

void BatchDownloadDialog::updateButtons() {
    bool isRunning = false;
    
    if (batchDownloader_) {
        isRunning = batchDownloader_->isJobRunning();
    }
    
    startButton_->setEnabled(!isRunning);
    pauseButton_->setEnabled(isRunning);
    stopButton_->setEnabled(isRunning);
    
    // Update pause button text based on state
    if (isRunning && isPaused_) {
        pauseButton_->setText(tr("Resume"));
        pauseButton_->setIcon(QIcon(":/icons/resume.png"));
    } else {
        pauseButton_->setText(tr("Pause"));
        pauseButton_->setIcon(QIcon(":/icons/pause.png"));
    }
}

void BatchDownloadDialog::updateProgress() {
    if (!batchDownloader_) return;
    
    // Update buttons
    updateButtons();
    
    // Get job progress info
    int processedCount = 0;
    int totalCount = 0;
    double overallProgress = 0.0;
    int successCount = 0;
    int failureCount = 0;
    
    batchDownloader_->getJobProgress(processedCount, totalCount, overallProgress, successCount, failureCount);
    
    // Update progress table and info based on these values
    updateBatchProgress(processedCount, totalCount, successCount, failureCount);
    
    // Update progress table
    updateProgressTable();
}

void BatchDownloadDialog::updateProgressTable() {
    if (!batchDownloader_) return;
    
    // Get active download tasks
    auto tasks = downloadManager_.getAllDownloadTasks();
    
    // We need to track which tasks are part of our batch job
    // In a real implementation, this would require proper tracking of batch items
    
    // For now, let's update the UI with what we have
    int row = 0;
    for (const auto& task : tasks) {
        if (progressTable_->rowCount() <= row) {
            progressTable_->insertRow(row);
        }
        
        // URL
        QTableWidgetItem* urlItem = progressTable_->item(row, 0);
        if (!urlItem) {
            urlItem = new QTableWidgetItem(QString::fromStdString(task->getUrl()));
            progressTable_->setItem(row, 0, urlItem);
        }
        
        // Progress bar
        QProgressBar* progressBar = qobject_cast<QProgressBar*>(progressTable_->cellWidget(row, 1));
        if (!progressBar) {
            progressBar = new QProgressBar(progressTable_);
            progressBar->setRange(0, 100);
            progressBar->setValue(0);
            progressBar->setTextVisible(true);
            progressTable_->setCellWidget(row, 1, progressBar);
        }
        
        // Status
        QTableWidgetItem* statusItem = progressTable_->item(row, 2);
        if (!statusItem) {
            statusItem = new QTableWidgetItem();
            progressTable_->setItem(row, 2, statusItem);
        }
        
        // Speed
        QTableWidgetItem* speedItem = progressTable_->item(row, 3);
        if (!speedItem) {
            speedItem = new QTableWidgetItem();
            progressTable_->setItem(row, 3, speedItem);
        }
        
        // Update progress bar
        auto progressInfo = task->getProgressInfo();
        int progressPercent = static_cast<int>(progressInfo.progressPercent);
        progressBar->setValue(progressPercent);
        
        // Update status
        QString statusText;
        switch (task->getStatus()) {
            case dm::core::DownloadStatus::QUEUED:
                statusText = tr("Queued");
                break;
            case dm::core::DownloadStatus::CONNECTING:
                statusText = tr("Connecting");
                break;
            case dm::core::DownloadStatus::DOWNLOADING:
                statusText = tr("Downloading");
                break;
            case dm::core::DownloadStatus::PAUSED:
                statusText = tr("Paused");
                break;
            case dm::core::DownloadStatus::COMPLETED:
                statusText = tr("Completed");
                break;
            case dm::core::DownloadStatus::DOWNLOAD_ERROR:
                statusText = tr("Error");
                break;
            case dm::core::DownloadStatus::CANCELED:
                statusText = tr("Canceled");
                break;
            default:
                statusText = tr("Unknown");
        }
        statusItem->setText(statusText);
        
        // Update speed
        speedItem->setText(formatSpeed(progressInfo.downloadSpeed));
        
        row++;
    }
    
    // Remove extra rows if needed
    while (progressTable_->rowCount() > row) {
        progressTable_->removeRow(progressTable_->rowCount() - 1);
    }
}

void BatchDownloadDialog::updateBatchProgress(int processedCount, int totalCount, int successCount, int failureCount) {
    // Update status labels
    totalCountLabel_->setText(QString::number(totalCount));
    pendingCountLabel_->setText(QString::number(totalCount - processedCount));
    activeCountLabel_->setText(QString::number(processedCount - successCount - failureCount));
    completedCountLabel_->setText(QString::number(successCount));
    failedCountLabel_->setText(QString::number(failureCount));
    
    // Update progress bar
    overallProgress_->setMaximum(std::max(1, totalCount));
    overallProgress_->setValue(successCount);
}

void BatchDownloadDialog::updateBatchItem(int index, int status) {
    // Update URL list table if the item is there
    if (index < urlTable_->rowCount()) {
        QTableWidgetItem* statusItem = urlTable_->item(index, 1);
        if (statusItem) {
            switch (static_cast<dm::core::DownloadStatus>(status)) {
                case dm::core::DownloadStatus::QUEUED:
                    statusItem->setText(tr("Queued"));
                    break;
                case dm::core::DownloadStatus::CONNECTING:
                    statusItem->setText(tr("Connecting"));
                    break;
                case dm::core::DownloadStatus::DOWNLOADING:
                    statusItem->setText(tr("Downloading"));
                    break;
                case dm::core::DownloadStatus::PAUSED:
                    statusItem->setText(tr("Paused"));
                    break;
                case dm::core::DownloadStatus::COMPLETED:
                    statusItem->setText(tr("Completed"));
                    break;
                case dm::core::DownloadStatus::DOWNLOAD_ERROR:
                    statusItem->setText(tr("Error"));
                    break;
                case dm::core::DownloadStatus::CANCELED:
                    statusItem->setText(tr("Canceled"));
                    break;
                default:
                    statusItem->setText(tr("Unknown"));
            }
        }
    }
}

QString BatchDownloadDialog::formatSpeed(double bytesPerSecond) {
    if (bytesPerSecond < 1024) {
        return QString("%1 B/s").arg(bytesPerSecond, 0, 'f', 0);
    } else if (bytesPerSecond < 1024 * 1024) {
        return QString("%1 KB/s").arg(bytesPerSecond / 1024, 0, 'f', 1);
    } else if (bytesPerSecond < 1024 * 1024 * 1024) {
        return QString("%1 MB/s").arg(bytesPerSecond / (1024 * 1024), 0, 'f', 2);
    } else {
        return QString("%1 GB/s").arg(bytesPerSecond / (1024 * 1024 * 1024), 0, 'f', 2);
    }
}

void BatchDownloadDialog::onBatchCompleted(int successCount, int failureCount) {
    QMessageBox::information(this, tr("Batch Download Complete"),
                           tr("Batch download completed with %1 successful and %2 failed downloads.")
                           .arg(successCount).arg(failureCount));
    
    // Update UI
    updateButtons();
}

} // namespace ui
} // namespace dm