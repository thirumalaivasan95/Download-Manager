#include "ui/MainWindow.h"
#include "ui/AddDownloadDialog.h"
#include "ui/SettingsDialog.h"
#include "utils/FileUtils.h"
#include "utils/Logger.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QHeaderView>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QProgressBar>
#include <QTextStream>
#include <QTime>
#include <QColorDialog>
#include <QFontDialog>
#include <QStyle>
#include <QStyleFactory>
#include <QScreen>

namespace dm {
namespace ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), downloadManager_(dm::core::DownloadManager::getInstance()) {
    // Set window title
    setWindowTitle("Download Manager");
    
    // Set window size
    resize(900, 600);
    
    // Set window position to center of screen
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Set up UI
    setupUi();
    
    // Create actions, menus, toolbars
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createTrayIcon();
    
    // Connect signals and slots
    connectSignals();
    
    // Load tasks
    loadTasks();
    
    // Start update timer
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::updateDownloadItems);
    updateTimer_->start(1000);  // Update every second
    
    // Update UI state
    updateUiState();
    
    // Log application start
    dm::utils::Logger::info("MainWindow created");
}

MainWindow::~MainWindow() {
    // Stop update timer
    if (updateTimer_) {
        updateTimer_->stop();
        delete updateTimer_;
        updateTimer_ = nullptr;
    }
    
    // Log application exit
    dm::utils::Logger::info("MainWindow destroyed");
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Check if we should minimize to tray instead of closing
    if (trayIcon_ && trayIcon_->isVisible()) {
        QMessageBox::information(this, "Download Manager",
                               "The program will keep running in the system tray. "
                               "To terminate the program, choose Quit in the context menu "
                               "of the system tray entry.");
        hide();
        event->ignore();
    } else {
        // Save settings
        downloadManager_.saveTasks();
        event->accept();
    }
}

void MainWindow::setupUi() {
    // Create download list
    downloadList_ = new QTreeWidget(this);
    downloadList_->setRootIsDecorated(false);
    downloadList_->setAlternatingRowColors(true);
    downloadList_->setSortingEnabled(true);
    downloadList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    downloadList_->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Set up columns
    QStringList headers;
    headers << "Name" << "Status" << "Size" << "Progress" << "Speed" << "ETA" << "URL";
    downloadList_->setHeaderLabels(headers);
    
    // Set column widths
    downloadList_->header()->setSectionResizeMode(QHeaderView::Interactive);
    downloadList_->header()->setStretchLastSection(true);
    downloadList_->setColumnWidth(0, 200);  // Name
    downloadList_->setColumnWidth(1, 100);  // Status
    downloadList_->setColumnWidth(2, 80);   // Size
    downloadList_->setColumnWidth(3, 150);  // Progress
    downloadList_->setColumnWidth(4, 100);  // Speed
    downloadList_->setColumnWidth(5, 100);  // ETA
    
    // Set central widget
    setCentralWidget(downloadList_);
    
    // Connect signals
    connect(downloadList_, &QTreeWidget::itemSelectionChanged, 
           this, &MainWindow::onDownloadItemSelected);
    connect(downloadList_, &QTreeWidget::itemDoubleClicked, 
           this, &MainWindow::onDownloadItemDoubleClicked);
    connect(downloadList_, &QTreeWidget::customContextMenuRequested,
           this, &MainWindow::showContextMenu);
}

void MainWindow::createActions() {
    // Add download action
    addAction_ = new QAction(QIcon(":/icons/add.png"), "Add Download", this);
    addAction_->setShortcut(QKeySequence::New);
    addAction_->setStatusTip("Add a new download");
    connect(addAction_, &QAction::triggered, this, &MainWindow::addDownload);
    
    // Batch add action
    batchAddAction_ = new QAction(QIcon(":/icons/batch.png"), "Batch Add", this);
    batchAddAction_->setStatusTip("Add multiple downloads from a file");
    connect(batchAddAction_, &QAction::triggered, this, &MainWindow::addBatchDownload);
    
    // Paste URL action
    pasteUrlAction_ = new QAction(QIcon(":/icons/paste.png"), "Paste URL", this);
    pasteUrlAction_->setShortcut(QKeySequence("Ctrl+V"));
    pasteUrlAction_->setStatusTip("Paste URL from clipboard and add download");
    connect(pasteUrlAction_, &QAction::triggered, this, &MainWindow::pasteUrlAndDownload);
    
    // Start action
    startAction_ = new QAction(QIcon(":/icons/start.png"), "Start", this);
    startAction_->setStatusTip("Start the selected download");
    connect(startAction_, &QAction::triggered, this, &MainWindow::startDownload);
    
    // Pause action
    pauseAction_ = new QAction(QIcon(":/icons/pause.png"), "Pause", this);
    pauseAction_->setStatusTip("Pause the selected download");
    connect(pauseAction_, &QAction::triggered, this, &MainWindow::pauseDownload);
    
    // Resume action
    resumeAction_ = new QAction(QIcon(":/icons/resume.png"), "Resume", this);
    resumeAction_->setStatusTip("Resume the selected download");
    connect(resumeAction_, &QAction::triggered, this, &MainWindow::resumeDownload);
    
    // Cancel action
    cancelAction_ = new QAction(QIcon(":/icons/cancel.png"), "Cancel", this);
    cancelAction_->setStatusTip("Cancel the selected download");
    connect(cancelAction_, &QAction::triggered, this, &MainWindow::cancelDownload);
    
    // Remove action
    removeAction_ = new QAction(QIcon(":/icons/remove.png"), "Remove", this);
    removeAction_->setShortcut(QKeySequence::Delete);
    removeAction_->setStatusTip("Remove the selected download");
    connect(removeAction_, &QAction::triggered, this, &MainWindow::removeDownload);
    
    // Start all action
    startAllAction_ = new QAction(QIcon(":/icons/start_all.png"), "Start All", this);
    startAllAction_->setStatusTip("Start all downloads");
    connect(startAllAction_, &QAction::triggered, this, &MainWindow::startAllDownloads);
    
    // Pause all action
    pauseAllAction_ = new QAction(QIcon(":/icons/pause_all.png"), "Pause All", this);
    pauseAllAction_->setStatusTip("Pause all downloads");
    connect(pauseAllAction_, &QAction::triggered, this, &MainWindow::pauseAllDownloads);
    
    // Settings action
    settingsAction_ = new QAction(QIcon(":/icons/settings.png"), "Settings", this);
    settingsAction_->setStatusTip("Configure settings");
    connect(settingsAction_, &QAction::triggered, this, &MainWindow::showSettings);
    
    // Exit action
    exitAction_ = new QAction(QIcon(":/icons/exit.png"), "Exit", this);
    exitAction_->setShortcut(QKeySequence::Quit);
    exitAction_->setStatusTip("Exit the application");
    connect(exitAction_, &QAction::triggered, qApp, &QApplication::quit);
    
    // About action
    aboutAction_ = new QAction(QIcon(":/icons/about.png"), "About", this);
    aboutAction_->setStatusTip("Show the application's About box");
    connect(aboutAction_, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::createMenus() {
    // File menu
    fileMenu_ = menuBar()->addMenu("&File");
    fileMenu_->addAction(addAction_);
    fileMenu_->addAction(batchAddAction_);
    fileMenu_->addAction(pasteUrlAction_);
    fileMenu_->addSeparator();
    fileMenu_->addAction(settingsAction_);
    fileMenu_->addSeparator();
    fileMenu_->addAction(exitAction_);
    
    // Download menu
    downloadMenu_ = menuBar()->addMenu("&Download");
    downloadMenu_->addAction(startAction_);
    downloadMenu_->addAction(pauseAction_);
    downloadMenu_->addAction(resumeAction_);
    downloadMenu_->addAction(cancelAction_);
    downloadMenu_->addAction(removeAction_);
    downloadMenu_->addSeparator();
    downloadMenu_->addAction(startAllAction_);
    downloadMenu_->addAction(pauseAllAction_);
    
    // Help menu
    helpMenu_ = menuBar()->addMenu("&Help");
    helpMenu_->addAction(aboutAction_);
    
    // Context menu
    contextMenu_ = new QMenu(this);
    contextMenu_->addAction(startAction_);
    contextMenu_->addAction(pauseAction_);
    contextMenu_->addAction(resumeAction_);
    contextMenu_->addAction(cancelAction_);
    contextMenu_->addSeparator();
    contextMenu_->addAction(removeAction_);
    
    // Tray menu
    trayMenu_ = new QMenu(this);
    trayMenu_->addAction(addAction_);
    trayMenu_->addSeparator();
    trayMenu_->addAction(startAllAction_);
    trayMenu_->addAction(pauseAllAction_);
    trayMenu_->addSeparator();
    trayMenu_->addAction(settingsAction_);
    trayMenu_->addSeparator();
    trayMenu_->addAction(exitAction_);
}

void MainWindow::createToolBars() {
    // Main toolbar
    mainToolBar_ = addToolBar("Main Toolbar");
    mainToolBar_->setMovable(false);
    mainToolBar_->addAction(addAction_);
    mainToolBar_->addAction(pasteUrlAction_);
    mainToolBar_->addSeparator();
    mainToolBar_->addAction(startAction_);
    mainToolBar_->addAction(pauseAction_);
    mainToolBar_->addAction(resumeAction_);
    mainToolBar_->addAction(cancelAction_);
    mainToolBar_->addAction(removeAction_);
    mainToolBar_->addSeparator();
    mainToolBar_->addAction(startAllAction_);
    mainToolBar_->addAction(pauseAllAction_);
    mainToolBar_->addSeparator();
    mainToolBar_->addAction(settingsAction_);
}

void MainWindow::createStatusBar() {
    // Status bar
    statusBar_ = statusBar();
    
    // Status label
    statusLabel_ = new QLabel("Ready");
    statusBar_->addPermanentWidget(statusLabel_);
}

void MainWindow::createTrayIcon() {
    // Create tray icon
    trayIcon_ = new QSystemTrayIcon(QIcon(":/icons/app.png"), this);
    trayIcon_->setToolTip("Download Manager");
    trayIcon_->setContextMenu(trayMenu_);
    trayIcon_->show();
    
    // Connect signals
    connect(trayIcon_, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
}

void MainWindow::connectSignals() {
    // Connect to download manager signals
    downloadManager_.setTaskAddedCallback(
        std::bind(&MainWindow::onTaskAdded, this, std::placeholders::_1));
    
    downloadManager_.setTaskRemovedCallback(
        std::bind(&MainWindow::onTaskRemoved, this, std::placeholders::_1));
    
    downloadManager_.setTaskStatusChangedCallback(
        std::bind(&MainWindow::onTaskStatusChanged, this, std::placeholders::_1, std::placeholders::_2));
}

void MainWindow::loadTasks() {
    // Load tasks
    std::vector<std::shared_ptr<dm::core::DownloadTask>> tasks = downloadManager_.getAllDownloadTasks();
    
    // Add tasks to UI
    for (const auto& task : tasks) {
        addDownloadToUi(task);
    }
}

void MainWindow::addDownload() {
    // Create add download dialog
    AddDownloadDialog dialog(this);
    
    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Get values from dialog
        QString url = dialog.getUrl();
        QString destination = dialog.getDestination();
        QString filename = dialog.getFilename();
        bool start = dialog.getStartImmediately();
        
        // Add download
        auto task = downloadManager_.addDownload(
            url.toStdString(), 
            destination.toStdString(), 
            filename.toStdString(), 
            start
        );
        
        // Check if download was added successfully
        if (!task) {
            QMessageBox::warning(this, "Error", "Failed to add download");
        }
    }
}

void MainWindow::addBatchDownload() {
    // Open file dialog to select file with URLs
    QString fileName = QFileDialog::getOpenFileName(this, "Open URL List", "", "Text Files (*.txt);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Read file
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open file");
        return;
    }
    
    // Read URLs
    QVector<QString> urls;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            urls.append(line);
        }
    }
    file.close();
    
    // Check if any URLs were found
    if (urls.isEmpty()) {
        QMessageBox::information(this, "Information", "No URLs found in the file");
        return;
    }
    
    // Ask for destination folder
    QString destination = QFileDialog::getExistingDirectory(this, "Select Destination Folder");
    
    if (destination.isEmpty()) {
        return;
    }
    
    // Add downloads
    std::vector<std::string> urlsStd;
    for (const QString& url : urls) {
        urlsStd.push_back(url.toStdString());
    }
    
    downloadManager_.addBatchDownload(urlsStd, destination.toStdString());
    
    // Show confirmation
    QMessageBox::information(this, "Batch Download", 
                           QString("Added %1 downloads").arg(urls.size()));
}

void MainWindow::pasteUrlAndDownload() {
    // Get clipboard text
    QClipboard* clipboard = QApplication::clipboard();
    QString text = clipboard->text().trimmed();
    
    if (text.isEmpty()) {
        QMessageBox::information(this, "Information", "Clipboard is empty");
        return;
    }
    
    // Check if text is a URL
    if (!text.startsWith("http://") && !text.startsWith("https://") && 
        !text.startsWith("ftp://") && !text.startsWith("ftps://")) {
        QMessageBox::information(this, "Information", "Clipboard text is not a URL");
        return;
    }
    
    // Create add download dialog with pre-filled URL
    AddDownloadDialog dialog(this);
    dialog.setUrl(text);
    
    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Get values from dialog
        QString url = dialog.getUrl();
        QString destination = dialog.getDestination();
        QString filename = dialog.getFilename();
        bool start = dialog.getStartImmediately();
        
        // Add download
        auto task = downloadManager_.addDownload(
            url.toStdString(), 
            destination.toStdString(), 
            filename.toStdString(), 
            start
        );
        
        // Check if download was added successfully
        if (!task) {
            QMessageBox::warning(this, "Error", "Failed to add download");
        }
    }
}

void MainWindow::startDownload() {
    // Get selected tasks
    auto tasks = getSelectedDownloadTasks();
    
    // Start tasks
    for (const auto& task : tasks) {
        downloadManager_.startDownload(task->getId());
    }
}

void MainWindow::pauseDownload() {
    // Get selected tasks
    auto tasks = getSelectedDownloadTasks();
    
    // Pause tasks
    for (const auto& task : tasks) {
        downloadManager_.pauseDownload(task->getId());
    }
}

void MainWindow::resumeDownload() {
    // Get selected tasks
    auto tasks = getSelectedDownloadTasks();
    
    // Resume tasks
    for (const auto& task : tasks) {
        downloadManager_.resumeDownload(task->getId());
    }
}

void MainWindow::cancelDownload() {
    // Get selected tasks
    auto tasks = getSelectedDownloadTasks();
    
    if (tasks.empty()) {
        return;
    }
    
    // Confirm cancellation
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Cancel", 
                                "Are you sure you want to cancel the selected download(s)?",
                                QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Cancel tasks
    for (const auto& task : tasks) {
        downloadManager_.cancelDownload(task->getId());
    }
}

void MainWindow::removeDownload() {
    // Get selected tasks
    auto tasks = getSelectedDownloadTasks();
    
    if (tasks.empty()) {
        return;
    }
    
    // Confirm removal
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Remove", 
                                "Are you sure you want to remove the selected download(s)?\n"
                                "This will remove the download from the list but keep the file.",
                                QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Remove tasks
    for (const auto& task : tasks) {
        downloadManager_.removeDownload(task->getId(), false);
    }
}

void MainWindow::startAllDownloads() {
    downloadManager_.startAllDownloads();
}

void MainWindow::pauseAllDownloads() {
    downloadManager_.pauseAllDownloads();
}

void MainWindow::showSettings() {
    // Create settings dialog
    SettingsDialog dialog(this);
    
    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Apply settings
        // This is handled by the dialog itself
    }
}

void MainWindow::showAboutDialog() {
    QMessageBox::about(this, "About Download Manager",
                     "Download Manager v1.0.0\n\n"
                     "A powerful download manager application.\n\n"
                     "© 2025 Example Organization");
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        // Show/hide the window
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
            raise();
        }
    }
}

void MainWindow::updateDownloadItems() {
    // Get all tasks
    std::vector<std::shared_ptr<dm::core::DownloadTask>> tasks = downloadManager_.getAllDownloadTasks();
    
    // Update task items
    for (const auto& task : tasks) {
        updateDownloadInUi(task);
    }
    
    // Update status bar
    int totalTasks = static_cast<int>(tasks.size());
    int activeTasks = 0;
    int completedTasks = 0;
    
    for (const auto& task : tasks) {
        if (task->getStatus() == dm::core::DownloadStatus::DOWNLOADING) {
            activeTasks++;
        } else if (task->getStatus() == dm::core::DownloadStatus::COMPLETED) {
            completedTasks++;
        }
    }
    
    statusLabel_->setText(QString("Total: %1 | Active: %2 | Completed: %3")
                         .arg(totalTasks).arg(activeTasks).arg(completedTasks));
    
    // Update UI state
    updateUiState();
}

void MainWindow::onDownloadItemSelected() {
    updateUiState();
}

void MainWindow::onDownloadItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (!item) {
        return;
    }
    
    // Get task ID
    QString taskId = item->data(0, Qt::UserRole).toString();
    
    // Get task
    auto task = downloadManager_.getDownloadTask(taskId.toStdString());
    if (!task) {
        return;
    }
    
    // Check if download is completed
    if (task->getStatus() == dm::core::DownloadStatus::COMPLETED) {
        // Open file
        QString filePath = QString::fromStdString(task->getDestinationPath() + "/" + task->getFilename());
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else {
        // Show download properties
        // (Not implemented in this version)
    }
}

void MainWindow::showContextMenu(const QPoint& pos) {
    // Show context menu if items are selected
    if (!downloadList_->selectedItems().isEmpty()) {
        contextMenu_->exec(downloadList_->mapToGlobal(pos));
    }
}

void MainWindow::addDownloadToUi(std::shared_ptr<dm::core::DownloadTask> task) {
    if (!task) {
        return;
    }
    
    // Create item
    QTreeWidgetItem* item = new QTreeWidgetItem(downloadList_);
    
    // Set item data
    QString taskId = QString::fromStdString(task->getId());
    item->setData(0, Qt::UserRole, taskId);
    
    // Update item
    updateDownloadInUi(task);
    
    // Add to map
    downloadItems_[taskId] = item;
}

void MainWindow::updateDownloadInUi(std::shared_ptr<dm::core::DownloadTask> task) {
    if (!task) {
        return;
    }
    
    // Get item
    QString taskId = QString::fromStdString(task->getId());
    if (!downloadItems_.contains(taskId)) {
        return;
    }
    
    QTreeWidgetItem* item = downloadItems_[taskId];
    
    // Get task info
    QString filename = QString::fromStdString(task->getFilename());
    QString url = QString::fromStdString(task->getUrl());
    dm::core::DownloadStatus status = task->getStatus();
    dm::core::ProgressInfo progress = task->getProgressInfo();
    
    // Set item text
    item->setText(0, filename);
    
    // Set status
    QString statusText;
    switch (status) {
        case dm::core::DownloadStatus::NONE:
            statusText = "Not Started";
            break;
        case dm::core::DownloadStatus::QUEUED:
            statusText = "Queued";
            break;
        case dm::core::DownloadStatus::CONNECTING:
            statusText = "Connecting...";
            break;
        case dm::core::DownloadStatus::DOWNLOADING:
            statusText = "Downloading";
            break;
        case dm::core::DownloadStatus::PAUSED:
            statusText = "Paused";
            break;
        case dm::core::DownloadStatus::COMPLETED:
            statusText = "Completed";
            break;
        case dm::core::DownloadStatus::DOWNLOAD_ERROR:
            statusText = "Error";
            break;
        case dm::core::DownloadStatus::CANCELED:
            statusText = "Canceled";
            break;
        default:
            statusText = "Unknown";
            break;
    }
    item->setText(1, statusText);
    
    // Set size
    QString sizeText;
    if (task->getFileSize() > 0) {
        sizeText = QString::fromStdString(dm::utils::FileUtils::formatFileSize(task->getFileSize()));
    } else {
        sizeText = "Unknown";
    }
    item->setText(2, sizeText);
    
    // Set progress
    QString progressText = QString("%1%").arg(progress.progressPercent, 0, 'f', 1);
    item->setText(3, progressText);
    
    // Set speed
    QString speedText;
    if (status == dm::core::DownloadStatus::DOWNLOADING) {
        speedText = QString::fromStdString(dm::utils::FileUtils::formatFileSize(progress.downloadSpeed)) + "/s";
    } else {
        speedText = "-";
    }
    item->setText(4, speedText);
    
    // Set ETA
    QString etaText;
    if (status == dm::core::DownloadStatus::DOWNLOADING && progress.timeRemaining > 0) {
        int seconds = static_cast<int>(progress.timeRemaining);
        QTime time(0, 0);
        time = time.addSecs(seconds);
        
        if (seconds < 60) {
            etaText = time.toString("ss") + "s";
        } else if (seconds < 3600) {
            etaText = time.toString("mm:ss");
        } else {
            etaText = time.toString("hh:mm:ss");
        }
    } else {
        etaText = "-";
    }
    item->setText(5, etaText);
    
    // Set URL
    item->setText(6, url);
    
    // Set item colors based on status
    QBrush textBrush;
    switch (status) {
        case dm::core::DownloadStatus::COMPLETED:
            textBrush = QBrush(Qt::darkGreen);
            break;
        case dm::core::DownloadStatus::DOWNLOAD_ERROR:
            textBrush = QBrush(Qt::red);
            break;
        case dm::core::DownloadStatus::PAUSED:
            textBrush = QBrush(Qt::darkYellow);
            break;
        case dm::core::DownloadStatus::CANCELED:
            textBrush = QBrush(Qt::gray);
            break;
        default:
            textBrush = QBrush(Qt::black);
            break;
    }
    
    for (int i = 0; i < 7; i++) {
        item->setForeground(i, textBrush);
    }
}

void MainWindow::removeDownloadFromUi(std::shared_ptr<dm::core::DownloadTask> task) {
    if (!task) {
        return;
    }
    
    // Get item
    QString taskId = QString::fromStdString(task->getId());
    if (!downloadItems_.contains(taskId)) {
        return;
    }
    
    // Remove item
    QTreeWidgetItem* item = downloadItems_[taskId];
    delete item;
    
    // Remove from map
    downloadItems_.remove(taskId);
}

std::shared_ptr<dm::core::DownloadTask> MainWindow::getSelectedDownloadTask() {
    // Get selected items
    QList<QTreeWidgetItem*> selectedItems = downloadList_->selectedItems();
    
    if (selectedItems.isEmpty()) {
        return nullptr;
    }
    
    // Get task ID
    QString taskId = selectedItems.first()->data(0, Qt::UserRole).toString();
    
    // Get task
    return downloadManager_.getDownloadTask(taskId.toStdString());
}

std::vector<std::shared_ptr<dm::core::DownloadTask>> MainWindow::getSelectedDownloadTasks() {
    std::vector<std::shared_ptr<dm::core::DownloadTask>> tasks;
    
    // Get selected items
    QList<QTreeWidgetItem*> selectedItems = downloadList_->selectedItems();
    
    for (QTreeWidgetItem* item : selectedItems) {
        // Get task ID
        QString taskId = item->data(0, Qt::UserRole).toString();
        
        // Get task
        auto task = downloadManager_.getDownloadTask(taskId.toStdString());
        if (task) {
            tasks.push_back(task);
        }
    }
    
    return tasks;
}

void MainWindow::onTaskAdded(std::shared_ptr<dm::core::DownloadTask> task) {
    // Add task to UI
    addDownloadToUi(task);
    
    // Update UI state
    updateUiState();
}

void MainWindow::onTaskRemoved(std::shared_ptr<dm::core::DownloadTask> task) {
    // Remove task from UI
    removeDownloadFromUi(task);
    
    // Update UI state
    updateUiState();
}

void MainWindow::onTaskStatusChanged(std::shared_ptr<dm::core::DownloadTask> task, 
                                  dm::core::DownloadStatus status) {
    // Update task in UI
    updateDownloadInUi(task);
    
    // Update UI state
    updateUiState();
    
    // Show notification for completed downloads
    if (status == dm::core::DownloadStatus::COMPLETED && trayIcon_) {
        trayIcon_->showMessage(
            "Download Completed",
            QString::fromStdString(task->getFilename()) + " has been downloaded",
            QSystemTrayIcon::Information,
            5000
        );
    }
}

void MainWindow::updateUiState() {
    // Get selected tasks
    auto tasks = getSelectedDownloadTasks();
    
    // Enable/disable actions based on selection
    bool hasSelection = !tasks.empty();
    startAction_->setEnabled(hasSelection);
    pauseAction_->setEnabled(hasSelection);
    resumeAction_->setEnabled(hasSelection);
    cancelAction_->setEnabled(hasSelection);
    removeAction_->setEnabled(hasSelection);
    
    // Disable actions based on task status
    if (hasSelection) {
        bool canStart = false;
        bool canPause = false;
        bool canResume = false;
        bool canCancel = false;
        
        for (const auto& task : tasks) {
            dm::core::DownloadStatus status = task->getStatus();
            
            if (status == dm::core::DownloadStatus::NONE ||
                status == dm::core::DownloadStatus::QUEUED ||
                status == dm::core::DownloadStatus::DOWNLOAD_ERROR) {
                canStart = true;
            }
            
            if (status == dm::core::DownloadStatus::DOWNLOADING) {
                canPause = true;
                canCancel = true;
            }
            
            if (status == dm::core::DownloadStatus::PAUSED) {
                canResume = true;
                canCancel = true;
            }
        }
        
        startAction_->setEnabled(canStart);
        pauseAction_->setEnabled(canPause);
        resumeAction_->setEnabled(canResume);
        cancelAction_->setEnabled(canCancel);
    }
}

} // namespace ui
} // namespace dm