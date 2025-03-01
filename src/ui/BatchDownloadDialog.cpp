#include "../../include/ui/BatchDownloadDialog.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/FileUtils.h"
#include "../../include/utils/StringUtils.h"
#include "../../include/core/BatchDownloader.h"
#include "../../include/core/DownloadManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QTextEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QTabWidget>
#include <QProgressBar>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDialogButtonBox>
#include <QIcon>
#include <QPainter>
#include <QFont>
#include <QApplication>
#include <QClipboard>
#include <QShortcut>
#include <QKeySequence>

namespace DownloadManager {
namespace UI {

BatchDownloadDialog::BatchDownloadDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Batch Download"));
    setMinimumSize(800, 600);
    
    // Create UI
    createUI();
    
    // Connect signals
    connectSignals();
    
    // Initialize
    initialize();
    
    // Accept drag & drop
    setAcceptDrops(true);
}

BatchDownloadDialog::~BatchDownloadDialog() {
    // Stop timer
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void BatchDownloadDialog::createUI() {
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tabs
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // Create tabs
    QWidget* addTab = createAddTab();
    QWidget* listTab = createListTab();
    QWidget* progressTab = createProgressTab();
    
    // Add tabs
    tabWidget->addTab(addTab, tr("Add URLs"));
    tabWidget->addTab(listTab, tr("URL List"));
    tabWidget->addTab(progressTab, tr("Progress"));
    
    // Bottom buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_startButton = new QPushButton(tr("Start"), this);
    m_startButton->setIcon(QIcon(":/resources/icons/batch.png"));
    
    m_pauseButton = new QPushButton(tr("Pause"), this);
    m_pauseButton->setIcon(QIcon(":/resources/icons/pause.png"));
    
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setIcon(QIcon(":/resources/icons/cancel.png"));
    
    m_clearButton = new QPushButton(tr("Clear List"), this);
    m_clearButton->setIcon(QIcon(":/resources/icons/remove.png"));
    
    m_closeButton = new QPushButton(tr("Close"), this);
    
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    
    // Add to main layout
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);
    
    // Create timer for updates
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000); // 1 second
}

QWidget* BatchDownloadDialog::createAddTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // URL input section
    QGroupBox* urlInputGroup = new QGroupBox(tr("URL Input"), tab);
    QVBoxLayout* urlInputLayout = new QVBoxLayout(urlInputGroup);
    
    m_urlEdit = new QTextEdit(urlInputGroup);
    m_urlEdit->setPlaceholderText(tr("Enter URLs, one per line"));
    urlInputLayout->addWidget(m_urlEdit);
    
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
    
    m_filePathEdit = new QLineEdit(fileInputGroup);
    m_filePathEdit->setReadOnly(true);
    
    QPushButton* browseButton = new QPushButton(tr("Browse"), fileInputGroup);
    
    fileSelectLayout->addWidget(m_filePathEdit);
    fileSelectLayout->addWidget(browseButton);
    
    fileInputLayout->addLayout(fileSelectLayout);
    
    QPushButton* importButton = new QPushButton(tr("Import URLs from File"), fileInputGroup);
    fileInputLayout->addWidget(importButton);
    
    // Pattern generation section
    QGroupBox* patternGroup = new QGroupBox(tr("URL Pattern Generation"), tab);
    QFormLayout* patternLayout = new QFormLayout(patternGroup);
    
    m_patternEdit = new QLineEdit(patternGroup);
    m_patternEdit->setPlaceholderText(tr("URL pattern with {$PATTERN} placeholder, e.g., http://example.com/file{$PATTERN}.zip"));
    
    QHBoxLayout* rangeLayout = new QHBoxLayout();
    QLabel* fromLabel = new QLabel(tr("From:"), patternGroup);
    QLabel* toLabel = new QLabel(tr("To:"), patternGroup);
    QLabel* stepLabel = new QLabel(tr("Step:"), patternGroup);
    QLabel* paddingLabel = new QLabel(tr("Padding:"), patternGroup);
    
    m_patternFromSpin = new QSpinBox(patternGroup);
    m_patternFromSpin->setRange(0, 9999);
    m_patternFromSpin->setValue(1);
    
    m_patternToSpin = new QSpinBox(patternGroup);
    m_patternToSpin->setRange(0, 9999);
    m_patternToSpin->setValue(10);
    
    m_patternStepSpin = new QSpinBox(patternGroup);
    m_patternStepSpin->setRange(1, 100);
    m_patternStepSpin->setValue(1);
    
    m_patternPaddingSpin = new QSpinBox(patternGroup);
    m_patternPaddingSpin->setRange(0, 10);
    m_patternPaddingSpin->setValue(0);
    m_patternPaddingSpin->setSpecialValueText(tr("None"));
    
    rangeLayout->addWidget(fromLabel);
    rangeLayout->addWidget(m_patternFromSpin);
    rangeLayout->addWidget(toLabel);
    rangeLayout->addWidget(m_patternToSpin);
    rangeLayout->addWidget(stepLabel);
    rangeLayout->addWidget(m_patternStepSpin);
    rangeLayout->addWidget(paddingLabel);
    rangeLayout->addWidget(m_patternPaddingSpin);
    
    patternLayout->addRow(tr("Pattern:"), m_patternEdit);
    patternLayout->addRow(tr("Range:"), rangeLayout);
    
    QPushButton* generateButton = new QPushButton(tr("Generate URLs"), patternGroup);
    patternLayout->addRow("", generateButton);
    
    // Download options section
    QGroupBox* optionsGroup = new QGroupBox(tr("Download Options"), tab);
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);
    
    m_savePathEdit = new QLineEdit(optionsGroup);
    m_savePathEdit->setText(getDefaultSavePath());
    
    QPushButton* browseSavePathButton = new QPushButton(tr("Browse"), optionsGroup);
    
    QHBoxLayout* savePathLayout = new QHBoxLayout();
    savePathLayout->addWidget(m_savePathEdit);
    savePathLayout->addWidget(browseSavePathButton);
    
    m_maxSimultaneousSpin = new QSpinBox(optionsGroup);
    m_maxSimultaneousSpin->setRange(1, 10);
    m_maxSimultaneousSpin->setValue(3);
    
    optionsLayout->addRow(tr("Save To:"), savePathLayout);
    optionsLayout->addRow(tr("Max Simultaneous Downloads:"), m_maxSimultaneousSpin);
    
    // Add URL button
    QPushButton* addUrlsButton = new QPushButton(tr("Add URLs to Queue"), tab);
    addUrlsButton->setIcon(QIcon(":/resources/icons/add.png"));
    
    // Add all to layout
    layout->addWidget(urlInputGroup);
    layout->addWidget(fileInputGroup);
    layout->addWidget(patternGroup);
    layout->addWidget(optionsGroup);
    layout->addWidget(addUrlsButton);
    
    // Store button references
    m_pasteButton = pasteButton;
    m_clearUrlsButton = clearButton;
    m_browseFileButton = browseButton;
    m_importFileButton = importButton;
    m_generateButton = generateButton;
    m_browseSavePathButton = browseSavePathButton;
    m_addUrlsButton = addUrlsButton;
    
    return tab;
}

QWidget* BatchDownloadDialog::createListTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // URL list table
    m_urlTable = new QTableWidget(0, 3, tab);
    
    // Set headers
    QStringList headers;
    headers << tr("URL") << tr("Status") << tr("Save Path");
    m_urlTable->setHorizontalHeaderLabels(headers);
    
    // Set header properties
    m_urlTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_urlTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_urlTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    
    // Set table properties
    m_urlTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_urlTable->setAlternatingRowColors(true);
    m_urlTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Button panel
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* addButton = new QPushButton(tr("Add URL"), tab);
    addButton->setIcon(QIcon(":/resources/icons/add.png"));
    
    QPushButton* removeButton = new QPushButton(tr("Remove"), tab);
    removeButton->setIcon(QIcon(":/resources/icons/remove.png"));
    
    QPushButton* removeAllButton = new QPushButton(tr("Remove All"), tab);
    
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(removeAllButton);
    buttonLayout->addStretch();
    
    // Add to layout
    layout->addWidget(m_urlTable);
    layout->addLayout(buttonLayout);
    
    // Store button references
    m_addUrlButton = addButton;
    m_removeUrlButton = removeButton;
    m_removeAllUrlsButton = removeAllButton;
    
    return tab;
}

QWidget* BatchDownloadDialog::createProgressTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Progress table
    m_progressTable = new QTableWidget(0, 4, tab);
    
    // Set headers
    QStringList headers;
    headers << tr("URL") << tr("Progress") << tr("Status") << tr("Speed");
    m_progressTable->setHorizontalHeaderLabels(headers);
    
    // Set header properties
    m_progressTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_progressTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_progressTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_progressTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    
    // Set table properties
    m_progressTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_progressTable->setAlternatingRowColors(true);
    m_progressTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Overall progress
    QGroupBox* overallGroup = new QGroupBox(tr("Overall Progress"), tab);
    QVBoxLayout* overallLayout = new QVBoxLayout(overallGroup);
    
    m_overallProgress = new QProgressBar(overallGroup);
    m_overallProgress->setRange(0, 100);
    m_overallProgress->setValue(0);
    m_overallProgress->setFormat(tr("%p% (%v/%m)"));
    m_overallProgress->setAlignment(Qt::AlignCenter);
    
    // Status labels
    QGridLayout* statusLayout = new QGridLayout();
    
    QLabel* totalLabel = new QLabel(tr("Total:"), overallGroup);
    QLabel* pendingLabel = new QLabel(tr("Pending:"), overallGroup);
    QLabel* activeLabel = new QLabel(tr("Active:"), overallGroup);
    QLabel* completedLabel = new QLabel(tr("Completed:"), overallGroup);
    QLabel* failedLabel = new QLabel(tr("Failed:"), overallGroup);
    
    m_totalCountLabel = new QLabel("0", overallGroup);
    m_pendingCountLabel = new QLabel("0", overallGroup);
    m_activeCountLabel = new QLabel("0", overallGroup);
    m_completedCountLabel = new QLabel("0", overallGroup);
    m_failedCountLabel = new QLabel("0", overallGroup);
    
    statusLayout->addWidget(totalLabel, 0, 0);
    statusLayout->addWidget(m_totalCountLabel, 0, 1);
    statusLayout->addWidget(pendingLabel, 0, 2);
    statusLayout->addWidget(m_pendingCountLabel, 0, 3);
    statusLayout->addWidget(activeLabel, 1, 0);
    statusLayout->addWidget(m_activeCountLabel, 1, 1);
    statusLayout->addWidget(completedLabel, 1, 2);
    statusLayout->addWidget(m_completedCountLabel, 1, 3);
    statusLayout->addWidget(failedLabel, 2, 0);
    statusLayout->addWidget(m_failedCountLabel, 2, 1);
    
    overallLayout->addWidget(m_overallProgress);
    overallLayout->addLayout(statusLayout);
    
    // Add to layout
    layout->addWidget(m_progressTable);
    layout->addWidget(overallGroup);
    
    return tab;
}

void BatchDownloadDialog::connectSignals() {
    // Add tab buttons
    connect(m_pasteButton, &QPushButton::clicked, this, &BatchDownloadDialog::onPasteClicked);
    connect(m_clearUrlsButton, &QPushButton::clicked, this, &BatchDownloadDialog::onClearUrlsClicked);
    connect(m_browseFileButton, &QPushButton::clicked, this, &BatchDownloadDialog::onBrowseFileClicked);
    connect(m_importFileButton, &QPushButton::clicked, this, &BatchDownloadDialog::onImportFileClicked);
    connect(m_generateButton, &QPushButton::clicked, this, &BatchDownloadDialog::onGenerateUrlsClicked);
    connect(m_browseSavePathButton, &QPushButton::clicked, this, &BatchDownloadDialog::onBrowseSavePathClicked);
    connect(m_addUrlsButton, &QPushButton::clicked, this, &BatchDownloadDialog::onAddUrlsClicked);
    
    // List tab buttons
    connect(m_addUrlButton, &QPushButton::clicked, this, &BatchDownloadDialog::onAddSingleUrlClicked);
    connect(m_removeUrlButton, &QPushButton::clicked, this, &BatchDownloadDialog::onRemoveUrlClicked);
    connect(m_removeAllUrlsButton, &QPushButton::clicked, this, &BatchDownloadDialog::onRemoveAllUrlsClicked);
    
    // Bottom buttons
    connect(m_startButton, &QPushButton::clicked, this, &BatchDownloadDialog::onStartClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &BatchDownloadDialog::onPauseClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &BatchDownloadDialog::onStopClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &BatchDownloadDialog::onClearListClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &BatchDownloadDialog::close);
    
    // Timer
    connect(m_updateTimer, &QTimer::timeout, this, &BatchDownloadDialog::updateProgress);
    
    // Shortcut for pasting URLs
    QShortcut* pasteShortcut = new QShortcut(QKeySequence::Paste, m_urlEdit);
    connect(pasteShortcut, &QShortcut::activated, this, &BatchDownloadDialog::onPasteClicked);
}

void BatchDownloadDialog::initialize() {
    // Get batch downloader instance
    auto& batchDownloader = Core::BatchDownloader::instance();
    
    // Register progress callback
    batchDownloader.addBatchProgressCallback(
        [this](int total, int completed, int failed, int active, int pending) {
            QMetaObject::invokeMethod(this, "updateBatchProgress", 
                               Qt::QueuedConnection,
                               Q_ARG(int, total),
                               Q_ARG(int, completed),
                               Q_ARG(int, failed),
                               Q_ARG(int, active),
                               Q_ARG(int, pending));
        }
    );
    
    // Register item callback
    batchDownloader.addBatchItemCallback(
        [this](int index, Core::BatchItemStatus status) {
            QMetaObject::invokeMethod(this, "updateBatchItem", 
                               Qt::QueuedConnection,
                               Q_ARG(int, index),
                               Q_ARG(int, static_cast<int>(status)));
        }
    );
    
    // Initialize state
    updateButtons();
    
    // Start timer
    m_updateTimer->start();
}

void BatchDownloadDialog::dragEnterEvent(QDragEnterEvent* event) {
    // Accept drag & drop of text/uri-list (URLs or files)
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
            m_urlEdit->append(text);
        }
        
        event->acceptProposedAction();
    } 
    else if (event->mimeData()->hasText()) {
        // Text drop - add to URL edit
        QString text = event->mimeData()->text();
        m_urlEdit->append(text);
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
        m_urlEdit->append(text);
    }
}

QString BatchDownloadDialog::getDefaultSavePath() {
    // Get default download directory from download manager
    auto& downloadManager = Core::DownloadManager::instance();
    return QString::fromStdString(downloadManager.getDefaultDownloadDirectory());
}

void BatchDownloadDialog::onPasteClicked() {
    QClipboard* clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    
    if (!text.isEmpty()) {
        m_urlEdit->append(text);
    }
}

void BatchDownloadDialog::onClearUrlsClicked() {
    m_urlEdit->clear();
}

void BatchDownloadDialog::onBrowseFileClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Select File with URLs"), 
        QDir::homePath(), 
        tr("Text Files (*.txt);;All Files (*)")
    );
    
    if (!filePath.isEmpty()) {
        m_filePathEdit->setText(filePath);
    }
}

void BatchDownloadDialog::onImportFileClicked() {
    QString filePath = m_filePathEdit->text();
    
    if (filePath.isEmpty()) {
        // No file selected, show browse dialog
        onBrowseFileClicked();
        filePath = m_filePathEdit->text();
        
        if (filePath.isEmpty()) {
            return;
        }
    }
    
    // Import URLs from file
    importUrlsFromFile(filePath);
}

void BatchDownloadDialog::onGenerateUrlsClicked() {
    QString pattern = m_patternEdit->text();
    
    if (pattern.isEmpty() || !pattern.contains("{$PATTERN}")) {
        QMessageBox::warning(this, tr("Error"), 
                           tr("Please enter a valid URL pattern with {$PATTERN} placeholder."));
        return;
    }
    
    int from = m_patternFromSpin->value();
    int to = m_patternToSpin->value();
    int step = m_patternStepSpin->value();
    int padding = m_patternPaddingSpin->value();
    
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
    m_urlEdit->append(urls.join("\n"));
}

void BatchDownloadDialog::onBrowseSavePathClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Save Location"),
        m_savePathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        m_savePathEdit->setText(dir);
    }
}

void BatchDownloadDialog::onAddUrlsClicked() {
    // Get URLs from text edit
    QString text = m_urlEdit->toPlainText();
    
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
    QString savePath = m_savePathEdit->text();
    if (savePath.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a save location."));
        return;
    }
    
    // Add to table
    addUrlsToTable(lines, savePath);
    
    // Clear URL edit
    m_urlEdit->clear();
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
    QString savePath = m_savePathEdit->text();
    if (savePath.isEmpty()) {
        savePath = getDefaultSavePath();
    }
    
    // Add to table
    QStringList urls;
    urls << url;
    addUrlsToTable(urls, savePath);
}

void BatchDownloadDialog::onRemoveUrlClicked() {
    // Get selected rows
    QList<QTableWidgetItem*> selectedItems = m_urlTable->selectedItems();
    
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
        m_urlTable->removeRow(row);
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
        m_urlTable->setRowCount(0);
        
        // Update batch info
        updateBatchInfo();
    }
}

void BatchDownloadDialog::onStartClicked() {
    if (m_urlTable->rowCount() == 0) {
        QMessageBox::warning(this, tr("Error"), tr("No URLs in the batch list."));
        return;
    }
    
    // Get batch downloader
    auto& batchDownloader = Core::BatchDownloader::instance();
    
    // Set options
    batchDownloader.setMaxConcurrentDownloads(m_maxSimultaneousSpin->value());
    
    // Collect URLs and destination directories
    std::vector<std::string> urls;
    std::map<std::string, std::string> urlDirs;
    
    for (int i = 0; i < m_urlTable->rowCount(); i++) {
        QString url = m_urlTable->item(i, 0)->text();
        QString savePath = m_urlTable->item(i, 2)->text();
        
        urls.push_back(url.toStdString());
        urlDirs[url.toStdString()] = savePath.toStdString();
    }
    
    // Clear progress table
    m_progressTable->setRowCount(0);
    
    // Add batch URLs
    for (const auto& url : urls) {
        batchDownloader.addBatchUrls({url}, urlDirs[url]);
    }
    
    // Start batch download
    batchDownloader.start();
    
    // Update UI
    updateButtons();
    updateBatchInfo();
}

void BatchDownloadDialog::onPauseClicked() {
    auto& batchDownloader = Core::BatchDownloader::instance();
    
    if (batchDownloader.isPaused()) {
        // Resume
        batchDownloader.resume();
        m_pauseButton->setText(tr("Pause"));
        m_pauseButton->setIcon(QIcon(":/resources/icons/pause.png"));
    } else {
        // Pause
        batchDownloader.pause();
        m_pauseButton->setText(tr("Resume"));
        m_pauseButton->setIcon(QIcon(":/resources/icons/resume.png"));
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
    
    if (reply == QMessageBox::Yes) {
        // Stop batch download
        auto& batchDownloader = Core::BatchDownloader::instance();
        batchDownloader.stop();
        
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
        auto& batchDownloader = Core::BatchDownloader::instance();
        if (batchDownloader.isRunning()) {
            batchDownloader.stop();
        }
        
        // Clear queue
        batchDownloader.clearQueue();
        
        // Clear tables
        m_urlTable->setRowCount(0);
        m_progressTable->setRowCount(0);
        
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
        for (int i = 0; i < m_urlTable->rowCount(); i++) {
            if (m_urlTable->item(i, 0)->text() == url) {
                exists = true;
                break;
            }
        }
        
        if (exists) {
            continue;
        }
        
        // Add new row
        int row = m_urlTable->rowCount();
        m_urlTable->insertRow(row);
        
        // Set items
        QTableWidgetItem* urlItem = new QTableWidgetItem(url);
        QTableWidgetItem* statusItem = new QTableWidgetItem(tr("Pending"));
        QTableWidgetItem* savePathItem = new QTableWidgetItem(savePath);
        
        m_urlTable->setItem(row, 0, urlItem);
        m_urlTable->setItem(row, 1, statusItem);
        m_urlTable->setItem(row, 2, savePathItem);
    }
    
    // Update batch info
    updateBatchInfo();
}

void BatchDownloadDialog::updateBatchInfo() {
    // Update counts
    m_totalCountLabel->setText(QString::number(m_urlTable->rowCount()));
    m_pendingCountLabel->setText(QString::number(m_urlTable->rowCount()));
    m_activeCountLabel->setText("0");
    m_completedCountLabel->setText("0");
    m_failedCountLabel->setText("0");
    
    // Update progress
    m_overallProgress->setValue(0);
    m_overallProgress->setMaximum(m_urlTable->rowCount());
    m_overallProgress->setFormat(tr("%p% (%v/%m)"));
}

void BatchDownloadDialog::updateButtons() {
    auto& batchDownloader = Core::BatchDownloader::instance();
    
    bool isRunning = batchDownloader.isRunning();
    bool isPaused = batchDownloader.isPaused();
    
    m_startButton->setEnabled(!isRunning);
    m_pauseButton->setEnabled(isRunning);
    m_stopButton->setEnabled(isRunning);
    
    if (isRunning && isPaused) {
        m_pauseButton->setText(tr("Resume"));
        m_pauseButton->setIcon(QIcon(":/resources/icons/resume.png"));
    } else {
        m_pauseButton->setText(tr("Pause"));
        m_pauseButton->setIcon(QIcon(":/resources/icons/pause.png"));
    }
}

void BatchDownloadDialog::updateProgress() {
    auto& batchDownloader = Core::BatchDownloader::instance();
    
    // Update buttons
    updateButtons();
    
    // Get batch items
    auto batchItems = batchDownloader.getBatchItems();
    
    // Update progress table
    updateProgressTable(batchItems);
}

void BatchDownloadDialog::updateProgressTable(const std::vector<Core::BatchItem>& items) {
    // Ensure table has the right number of rows
    while (m_progressTable->rowCount() < static_cast<int>(items.size())) {
        m_progressTable->insertRow(m_progressTable->rowCount());
    }
    while (m_progressTable->rowCount() > static_cast<int>(items.size())) {
        m_progressTable->removeRow(m_progressTable->rowCount() - 1);
    }
    
    // Update each row
    for (size_t i = 0; i < items.size(); i++) {
        const Core::BatchItem& item = items[i];
        
        // URL
        QTableWidgetItem* urlItem = m_progressTable->item(i, 0);
        if (!urlItem) {
            urlItem = new QTableWidgetItem(QString::fromStdString(item.url));
            m_progressTable->setItem(i, 0, urlItem);
        } else {
            urlItem->setText(QString::fromStdString(item.url));
        }
        
        // Progress
        QProgressBar* progressBar = qobject_cast<QProgressBar*>(m_progressTable->cellWidget(i, 1));
        if (!progressBar) {
            progressBar = new QProgressBar(m_progressTable);
            progressBar->setRange(0, 100);
            progressBar->setValue(0);
            progressBar->setTextVisible(true);
            m_progressTable->setCellWidget(i, 1, progressBar);
        }
        
        // Update progress if task exists
        if (item.taskId > 0 && item.task) {
            int64_t fileSize = item.task->getFileSize();
            int64_t downloaded = item.task->getDownloadedBytes();
            
            if (fileSize > 0) {
                int percent = static_cast<int>((downloaded * 100) / fileSize);
                progressBar->setValue(percent);
            }
        }
        
        // Status
        QTableWidgetItem* statusItem = m_progressTable->item(i, 2);
        if (!statusItem) {
            statusItem = new QTableWidgetItem();
            m_progressTable->setItem(i, 2, statusItem);
        }
        
        // Speed
        QTableWidgetItem* speedItem = m_progressTable->item(i, 3);
        if (!speedItem) {
            speedItem = new QTableWidgetItem();
            m_progressTable->setItem(i, 3, speedItem);
        }
        
        // Update status and speed based on batch item status
        switch (item.status) {
            case Core::BatchItemStatus::PENDING:
                statusItem->setText(tr("Pending"));
                speedItem->setText("");
                break;
            case Core::BatchItemStatus::ACTIVE:
                statusItem->setText(tr("Downloading"));
                if (item.task) {
                    int speed = item.task->getCurrentSpeed();
                    speedItem->setText(QString::fromStdString(Utils::StringUtils::formatBitrate(speed)));
                } else {
                    speedItem->setText("");
                }
                break;
            case Core::BatchItemStatus::COMPLETED:
                statusItem->setText(tr("Completed"));
                speedItem->setText("");
                break;
            case Core::BatchItemStatus::FAILED:
                statusItem->setText(tr("Failed"));
                speedItem->setText("");
                break;
        }
    }
}

void BatchDownloadDialog::updateBatchProgress(int total, int completed, int failed, int active, int pending) {
    // Update status labels
    m_totalCountLabel->setText(QString::number(total));
    m_pendingCountLabel->setText(QString::number(pending));
    m_activeCountLabel->setText(QString::number(active));
    m_completedCountLabel->setText(QString::number(completed));
    m_failedCountLabel->setText(QString::number(failed));
    
    // Update progress bar
    m_overallProgress->setMaximum(total);
    m_overallProgress->setValue(completed);
}

void BatchDownloadDialog::updateBatchItem(int index, int status) {
    // Update URL list table if the item is there
    if (index < m_urlTable->rowCount()) {
        QTableWidgetItem* statusItem = m_urlTable->item(index, 1);
        if (statusItem) {
            switch (static_cast<Core::BatchItemStatus>(status)) {
                case Core::BatchItemStatus::PENDING:
                    statusItem->setText(tr("Pending"));
                    break;
                case Core::BatchItemStatus::ACTIVE:
                    statusItem->setText(tr("Downloading"));
                    break;
                case Core::BatchItemStatus::COMPLETED:
                    statusItem->setText(tr("Completed"));
                    break;
                case Core::BatchItemStatus::FAILED:
                    statusItem->setText(tr("Failed"));
                    break;
            }
        }
    }
}

} // namespace UI
} // namespace DownloadManager