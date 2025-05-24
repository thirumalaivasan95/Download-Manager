#include "ui/DownloadItemWidget.h"
#include "include/utils/FileUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIcon>
#include <QTime>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>

namespace dm {
namespace ui {

DownloadItemWidget::DownloadItemWidget(std::shared_ptr<dm::core::DownloadTask> task, QWidget* parent)
    : QWidget(parent), task_(task) {
    // Set up UI
    setupUi();
    
    // Connect signals and slots
    connect(startButton_, &QPushButton::clicked, this, &DownloadItemWidget::onStartClicked);
    connect(pauseButton_, &QPushButton::clicked, this, &DownloadItemWidget::onPauseClicked);
    connect(resumeButton_, &QPushButton::clicked, this, &DownloadItemWidget::onResumeClicked);
    connect(cancelButton_, &QPushButton::clicked, this, &DownloadItemWidget::onCancelClicked);
    connect(removeButton_, &QPushButton::clicked, this, &DownloadItemWidget::onRemoveClicked);
    connect(openButton_, &QPushButton::clicked, this, &DownloadItemWidget::onOpenClicked);
    
    // Create update timer
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &DownloadItemWidget::onUpdateTimer);
    updateTimer_->start(500);  // Update every 500ms
    
    // Initial update
    update();
}

DownloadItemWidget::~DownloadItemWidget() {
    // Stop update timer
    if (updateTimer_) {
        updateTimer_->stop();
    }
}

std::shared_ptr<dm::core::DownloadTask> DownloadItemWidget::getTask() const {
    return task_;
}

void DownloadItemWidget::update() {
    if (!task_) {
        return;
    }
    
    // Get task info
    std::string filename = task_->getFilename();
    dm::core::DownloadStatus status = task_->getStatus();
    dm::core::ProgressInfo progress = task_->getProgressInfo();
    
    // Update labels
    nameLabel_->setText(QString::fromStdString(filename));
    
    // Set status text and color
    QString statusText;
    QColor statusColor;
    
    switch (status) {
        case dm::core::DownloadStatus::NONE:
            statusText = "Not Started";
            statusColor = Qt::gray;
            break;
        case dm::core::DownloadStatus::QUEUED:
            statusText = "Queued";
            statusColor = Qt::blue;
            break;
        case dm::core::DownloadStatus::CONNECTING:
            statusText = "Connecting...";
            statusColor = Qt::blue;
            break;
        case dm::core::DownloadStatus::DOWNLOADING:
            statusText = "Downloading";
            statusColor = Qt::darkGreen;
            break;
        case dm::core::DownloadStatus::PAUSED:
            statusText = "Paused";
            statusColor = Qt::darkYellow;
            break;
        case dm::core::DownloadStatus::COMPLETED:
            statusText = "Completed";
            statusColor = Qt::darkGreen;
            break;
        case dm::core::DownloadStatus::ERROR:
            statusText = "Error: " + QString::fromStdString(task_->getError());
            statusColor = Qt::red;
            break;
        case dm::core::DownloadStatus::CANCELED:
            statusText = "Canceled";
            statusColor = Qt::gray;
            break;
        default:
            statusText = "Unknown";
            statusColor = Qt::black;
            break;
    }
    
    statusLabel_->setText(statusText);
    QPalette palette = statusLabel_->palette();
    palette.setColor(QPalette::WindowText, statusColor);
    statusLabel_->setPalette(palette);
    
    // Update size
    int64_t fileSize = task_->getFileSize();
    int64_t downloadedBytes = progress.downloadedBytes;
    
    if (fileSize > 0) {
        sizeLabel_->setText(formatFileSize(downloadedBytes) + " / " + formatFileSize(fileSize));
    } else {
        sizeLabel_->setText(formatFileSize(downloadedBytes));
    }
    
    // Update progress
    progressBar_->setValue(static_cast<int>(progress.progressPercent));
    
    // Update speed
    if (status == dm::core::DownloadStatus::DOWNLOADING) {
        speedLabel_->setText(formatSpeed(progress.downloadSpeed));
    } else {
        speedLabel_->setText("-");
    }
    
    // Update time
    if (status == dm::core::DownloadStatus::DOWNLOADING && progress.timeRemaining > 0) {
        timeLabel_->setText(formatTime(progress.timeRemaining));
    } else {
        timeLabel_->setText("-");
    }
    
    // Update button states
    updateButtonStates();
}

void DownloadItemWidget::onStartClicked() {
    emit startClicked();
}

void DownloadItemWidget::onPauseClicked() {
    emit pauseClicked();
}

void DownloadItemWidget::onResumeClicked() {
    emit resumeClicked();
}

void DownloadItemWidget::onCancelClicked() {
    // Confirm cancellation
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Cancel",
                                                           "Are you sure you want to cancel this download?",
                                                           QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        emit cancelClicked();
    }
}

void DownloadItemWidget::onRemoveClicked() {
    // Confirm removal
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Remove",
                                                           "Are you sure you want to remove this download from the list?",
                                                           QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        emit removeClicked();
    }
}

void DownloadItemWidget::onOpenClicked() {
    emit openClicked();
}

void DownloadItemWidget::onUpdateTimer() {
    update();
}

void DownloadItemWidget::setupUi() {
    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create info layout
    QGridLayout* infoLayout = new QGridLayout();
    
    // Create labels
    nameLabel_ = new QLabel(this);
    nameLabel_->setFont(QFont("", 10, QFont::Bold));
    nameLabel_->setTextFormat(Qt::PlainText);
    nameLabel_->setWordWrap(true);
    
    statusLabel_ = new QLabel(this);
    sizeLabel_ = new QLabel(this);
    speedLabel_ = new QLabel(this);
    timeLabel_ = new QLabel(this);
    
    // Create progress bar
    progressBar_ = new ProgressBar(this);
    progressBar_->setShowSpeed(true);
    progressBar_->setAnimated(true);
    
    // Add labels to info layout
    infoLayout->addWidget(nameLabel_, 0, 0, 1, 3);
    infoLayout->addWidget(statusLabel_, 1, 0);
    infoLayout->addWidget(sizeLabel_, 1, 1);
    infoLayout->addWidget(progressBar_, 2, 0, 1, 3);
    infoLayout->addWidget(speedLabel_, 3, 0);
    infoLayout->addWidget(timeLabel_, 3, 1);
    
    // Create button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // Create buttons
    startButton_ = new QPushButton(QIcon(":/icons/start.png"), "", this);
    pauseButton_ = new QPushButton(QIcon(":/icons/pause.png"), "", this);
    resumeButton_ = new QPushButton(QIcon(":/icons/resume.png"), "", this);
    cancelButton_ = new QPushButton(QIcon(":/icons/cancel.png"), "", this);
    removeButton_ = new QPushButton(QIcon(":/icons/remove.png"), "", this);
    openButton_ = new QPushButton(QIcon(":/icons/open.png"), "", this);
    
    // Set button tooltips
    startButton_->setToolTip("Start Download");
    pauseButton_->setToolTip("Pause Download");
    resumeButton_->setToolTip("Resume Download");
    cancelButton_->setToolTip("Cancel Download");
    removeButton_->setToolTip("Remove Download");
    openButton_->setToolTip("Open File");
    
    // Set button size
    QSize buttonSize(32, 32);
    startButton_->setFixedSize(buttonSize);
    pauseButton_->setFixedSize(buttonSize);
    resumeButton_->setFixedSize(buttonSize);
    cancelButton_->setFixedSize(buttonSize);
    removeButton_->setFixedSize(buttonSize);
    openButton_->setFixedSize(buttonSize);
    
    // Add buttons to button layout
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(resumeButton_);
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(removeButton_);
    buttonLayout->addWidget(openButton_);
    buttonLayout->addStretch();
    
    // Add layouts to main layout
    mainLayout->addLayout(infoLayout);
    mainLayout->addLayout(buttonLayout);
    
    // Set layout
    setLayout(mainLayout);
    
    // Set background color
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(245, 245, 245));
    setPalette(pal);
    
    // Set size policy
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QString DownloadItemWidget::formatFileSize(int64_t size) const {
    return QString::fromStdString(dm::utils::FileUtils::formatFileSize(size));
}

QString DownloadItemWidget::formatSpeed(double bytesPerSecond) const {
    return QString::fromStdString(dm::utils::FileUtils::formatFileSize(bytesPerSecond)) + "/s";
}

QString DownloadItemWidget::formatTime(int64_t seconds) const {
    if (seconds < 60) {
        return QString::number(seconds) + " sec";
    } else if (seconds < 3600) {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        return QString::number(minutes) + " min " + QString::number(secs) + " sec";
    } else {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        return QString::number(hours) + " hr " + QString::number(minutes) + " min";
    }
}

void DownloadItemWidget::updateButtonStates() {
    if (!task_) {
        return;
    }
    
    dm::core::DownloadStatus status = task_->getStatus();
    
    // Enable/disable buttons based on status
    startButton_->setEnabled(status == dm::core::DownloadStatus::NONE || 
                            status == dm::core::DownloadStatus::QUEUED ||
                            status == dm::core::DownloadStatus::ERROR);
    
    pauseButton_->setEnabled(status == dm::core::DownloadStatus::DOWNLOADING);
    resumeButton_->setEnabled(status == dm::core::DownloadStatus::PAUSED);
    cancelButton_->setEnabled(status == dm::core::DownloadStatus::DOWNLOADING || 
                             status == dm::core::DownloadStatus::PAUSED);
    
    removeButton_->setEnabled(status != dm::core::DownloadStatus::DOWNLOADING);
    openButton_->setEnabled(status == dm::core::DownloadStatus::COMPLETED);
}

} // namespace ui
} // namespace dm