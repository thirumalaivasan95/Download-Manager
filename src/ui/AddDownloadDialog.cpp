#include "ui/AddDownloadDialog.h"
#include "utils/Logger.h"
#include "utils/UrlParser.h"
#include "utils/FileUtils.h"
#include "core/DownloadManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QGroupBox>
#include <QTabWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QIcon>

namespace dm {
namespace ui {

AddDownloadDialog::AddDownloadDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Add New Download"));
    setMinimumWidth(550);
    
    createUI();
    setupConnections();
    
    // Check clipboard for URL
    checkClipboard();
}

AddDownloadDialog::AddDownloadDialog(const QString& url, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Add New Download"));
    setMinimumWidth(550);
    
    createUI();
    setupConnections();
    
    // Set URL
    m_urlEdit->setText(url);
    analyzeUrl();
}

AddDownloadDialog::~AddDownloadDialog() {
}

void AddDownloadDialog::createUI() {
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tabs
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // Basic tab
    QWidget* basicTab = new QWidget(this);
    QVBoxLayout* basicLayout = new QVBoxLayout(basicTab);
    
    // URL input
    QFormLayout* urlForm = new QFormLayout();
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("Enter URL here"));
    QPushButton* analyzeButton = new QPushButton(tr("Analyze"), this);
    analyzeButton->setIcon(QIcon(":/resources/icons/add.png"));
    
    QHBoxLayout* urlLayout = new QHBoxLayout();
    urlLayout->addWidget(m_urlEdit);
    urlLayout->addWidget(analyzeButton);
    
    urlForm->addRow(tr("URL:"), urlLayout);
    
    // Filename input
    m_filenameEdit = new QLineEdit(this);
    m_filenameEdit->setPlaceholderText(tr("Filename will be detected automatically"));
    
    // Save to location
    m_savePathEdit = new QLineEdit(this);
    m_savePathEdit->setReadOnly(true);
    
    QPushButton* browseButton = new QPushButton(tr("Browse"), this);
    
    QHBoxLayout* savePathLayout = new QHBoxLayout();
    savePathLayout->addWidget(m_savePathEdit);
    savePathLayout->addWidget(browseButton);
    
    urlForm->addRow(tr("Save As:"), m_filenameEdit);
    urlForm->addRow(tr("Save To:"), savePathLayout);
    
    // File information group
    QGroupBox* fileInfoGroup = new QGroupBox(tr("File Information"), this);
    QFormLayout* fileInfoLayout = new QFormLayout(fileInfoGroup);
    
    m_fileSizeLabel = new QLabel(tr("Unknown"), this);
    m_fileTypeLabel = new QLabel(tr("Unknown"), this);
    m_resumableLabel = new QLabel(tr("Unknown"), this);
    
    fileInfoLayout->addRow(tr("Size:"), m_fileSizeLabel);
    fileInfoLayout->addRow(tr("Type:"), m_fileTypeLabel);
    fileInfoLayout->addRow(tr("Resumable:"), m_resumableLabel);
    
    // Add to basic tab
    basicLayout->addLayout(urlForm);
    basicLayout->addWidget(fileInfoGroup);
    basicLayout->addStretch();
    
    // Advanced tab
    QWidget* advancedTab = new QWidget(this);
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedTab);
    
    // Download Options
    QGroupBox* optionsGroup = new QGroupBox(tr("Download Options"), this);
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);
    
    // Segments
    m_segmentsSpinBox = new QSpinBox(this);
    m_segmentsSpinBox->setRange(1, 16);
    m_segmentsSpinBox->setValue(4);
    optionsLayout->addRow(tr("Number of Segments:"), m_segmentsSpinBox);
    
    // Speed limit
    m_limitSpeedCheckBox = new QCheckBox(tr("Limit Download Speed"), this);
    m_speedLimitSpinBox = new QSpinBox(this);
    m_speedLimitSpinBox->setRange(1, 100000);
    m_speedLimitSpinBox->setValue(1000);
    m_speedLimitSpinBox->setSuffix(" KB/s");
    m_speedLimitSpinBox->setEnabled(false);
    
    QHBoxLayout* speedLimitLayout = new QHBoxLayout();
    speedLimitLayout->addWidget(m_limitSpeedCheckBox);
    speedLimitLayout->addWidget(m_speedLimitSpinBox);
    speedLimitLayout->addStretch();
    
    optionsLayout->addRow(tr("Speed Limit:"), speedLimitLayout);
    
    // Authentication
    QGroupBox* authGroup = new QGroupBox(tr("Authentication"), this);
    QFormLayout* authLayout = new QFormLayout(authGroup);
    
    m_authRequiredCheckBox = new QCheckBox(tr("Authentication Required"), this);
    m_usernameEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    
    m_usernameEdit->setEnabled(false);
    m_passwordEdit->setEnabled(false);
    
    authLayout->addRow(tr(""), m_authRequiredCheckBox);
    authLayout->addRow(tr("Username:"), m_usernameEdit);
    authLayout->addRow(tr("Password:"), m_passwordEdit);
    
    // Schedule
    QGroupBox* scheduleGroup = new QGroupBox(tr("Schedule"), this);
    QFormLayout* scheduleLayout = new QFormLayout(scheduleGroup);
    
    m_scheduleCheckBox = new QCheckBox(tr("Schedule Download"), this);
    m_scheduleDateTime = new QDateTimeEdit(this);
    m_scheduleDateTime->setDateTime(QDateTime::currentDateTime().addSecs(3600)); // Default to 1 hour later
    m_scheduleDateTime->setCalendarPopup(true);
    m_scheduleDateTime->setEnabled(false);
    
    scheduleLayout->addRow(tr(""), m_scheduleCheckBox);
    scheduleLayout->addRow(tr("Start Time:"), m_scheduleDateTime);
    
    // Add to advanced tab
    advancedLayout->addWidget(optionsGroup);
    advancedLayout->addWidget(authGroup);
    advancedLayout->addWidget(scheduleGroup);
    advancedLayout->addStretch();
    
    // Add tabs to widget
    tabWidget->addTab(basicTab, tr("Basic"));
    tabWidget->addTab(advancedTab, tr("Advanced"));
    
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_startButton = buttonBox->button(QDialogButtonBox::Ok);
    m_startButton->setText(tr("Start Download"));
    m_startButton->setEnabled(false);
    
    // Main layout setup
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    
    // Set default save path
    m_savePathEdit->setText(QDir::homePath() + "/Downloads/");
}

void AddDownloadDialog::setupConnections() {
    // URL analysis
    connect(m_urlEdit, &QLineEdit::textChanged, this, &AddDownloadDialog::urlChanged);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &AddDownloadDialog::analyzeUrl);
    
    QPushButton* analyzeButton = findChild<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
    if (analyzeButton) {
        connect(analyzeButton, &QPushButton::clicked, this, &AddDownloadDialog::analyzeUrl);
    }
    
    // Browse button
    QPushButton* browseButton = nullptr;
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text() == tr("Browse")) {
            browseButton = button;
            break;
        }
    }
    
    if (browseButton) {
        connect(browseButton, &QPushButton::clicked, this, &AddDownloadDialog::browseSavePath);
    }
    
    // Speed limit checkbox
    connect(m_limitSpeedCheckBox, &QCheckBox::toggled, m_speedLimitSpinBox, &QSpinBox::setEnabled);
    
    // Auth checkbox
    connect(m_authRequiredCheckBox, &QCheckBox::toggled, m_usernameEdit, &QLineEdit::setEnabled);
    connect(m_authRequiredCheckBox, &QCheckBox::toggled, m_passwordEdit, &QLineEdit::setEnabled);
    
    // Schedule checkbox
    connect(m_scheduleCheckBox, &QCheckBox::toggled, m_scheduleDateTime, &QDateTimeEdit::setEnabled);
    
    // Button box
    QDialogButtonBox* buttonBox = findChild<QDialogButtonBox*>();
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddDownloadDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AddDownloadDialog::reject);
}

void AddDownloadDialog::urlChanged() {
    m_startButton->setEnabled(!m_urlEdit->text().isEmpty());
}

void AddDownloadDialog::analyzeUrl() {
    QString url = m_urlEdit->text().trimmed();
    
    if (url.isEmpty()) {
        return;
    }
    
    // Validate URL
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(url.toStdString());
    
    if (!parsedUrl.valid) {
        QMessageBox::warning(this, tr("Invalid URL"), 
                           tr("The URL you entered is not valid. Please check and try again."));
        return;
    }
    
    // Show analyzing status
    m_fileSizeLabel->setText(tr("Analyzing..."));
    m_fileTypeLabel->setText(tr("Analyzing..."));
    m_resumableLabel->setText(tr("Analyzing..."));
    
    // Get download manager instance
    auto& downloadManager = Core::DownloadManager::instance();
    
    // Analyze URL in a separate thread to avoid UI freezing
    QtConcurrent::run([this, url, &downloadManager]() {
        try {
            // Get file info
            Core::DownloadFileInfo fileInfo;
            bool success = downloadManager.getFileInfo(url.toStdString(), fileInfo);
            
            // Update UI in the main thread
            QMetaObject::invokeMethod(this, "updateFileInfo", Qt::QueuedConnection,
                                     Q_ARG(bool, success),
                                     Q_ARG(QString, QString::fromStdString(fileInfo.filename)),
                                     Q_ARG(qint64, fileInfo.size),
                                     Q_ARG(QString, QString::fromStdString(fileInfo.contentType)),
                                     Q_ARG(bool, fileInfo.resumable));
        } 
        catch (const std::exception& e) {
            // Handle errors
            QString errorMsg = QString::fromStdString(e.what());
            QMetaObject::invokeMethod(this, "showAnalysisError", Qt::QueuedConnection,
                                     Q_ARG(QString, errorMsg));
        }
    });
}

void AddDownloadDialog::updateFileInfo(bool success, const QString& filename, qint64 size, 
                                     const QString& contentType, bool resumable) {
    if (success) {
        // Update filename if not manually set
        if (m_filenameEdit->text().isEmpty()) {
            m_filenameEdit->setText(filename);
        }
        
        // Update file size
        if (size > 0) {
            m_fileSizeLabel->setText(formatFileSize(size));
        } else {
            m_fileSizeLabel->setText(tr("Unknown"));
        }
        
        // Update content type
        m_fileTypeLabel->setText(contentType.isEmpty() ? tr("Unknown") : contentType);
        
        // Update resumable status
        m_resumableLabel->setText(resumable ? tr("Yes") : tr("No"));
        
        // Enable start button
        m_startButton->setEnabled(true);
    } else {
        // Show error
        m_fileSizeLabel->setText(tr("Unknown"));
        m_fileTypeLabel->setText(tr("Unknown"));
        m_resumableLabel->setText(tr("Unknown"));
        
        QMessageBox::warning(this, tr("Analysis Failed"), 
                           tr("Failed to analyze the URL. Please check if the URL is correct."));
    }
}

void AddDownloadDialog::showAnalysisError(const QString& errorMsg) {
    m_fileSizeLabel->setText(tr("Unknown"));
    m_fileTypeLabel->setText(tr("Unknown"));
    m_resumableLabel->setText(tr("Unknown"));
    
    QMessageBox::warning(this, tr("Analysis Error"), 
                       tr("Error analyzing URL: %1").arg(errorMsg));
}

void AddDownloadDialog::browseSavePath() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Save Location"),
                                                 m_savePathEdit->text(),
                                                 QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        // Ensure path ends with a separator
        if (!dir.endsWith('/') && !dir.endsWith('\\')) {
            dir += '/';
        }
        
        m_savePathEdit->setText(dir);
    }
}

void AddDownloadDialog::checkClipboard() {
    QClipboard* clipboard = QApplication::clipboard();
    QString clipboardText = clipboard->text().trimmed();
    
    // Check if clipboard contains a URL
    Utils::UrlParser urlParser;
    auto parsedUrl = urlParser.parse(clipboardText.toStdString());
    
    if (parsedUrl.valid) {
        // Ask user if they want to use the URL
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            tr("URL Found in Clipboard"), 
            tr("Would you like to use the URL from clipboard?\n\n%1").arg(clipboardText),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            m_urlEdit->setText(clipboardText);
            analyzeUrl();
        }
    }
}

QString AddDownloadDialog::formatFileSize(qint64 size) {
    const qint64 KB = 1024;
    const qint64 MB = 1024 * KB;
    const qint64 GB = 1024 * MB;
    const qint64 TB = 1024 * GB;
    
    if (size < KB) {
        return tr("%1 bytes").arg(size);
    } else if (size < MB) {
        double sizeKB = size / static_cast<double>(KB);
        return tr("%1 KB").arg(sizeKB, 0, 'f', 2);
    } else if (size < GB) {
        double sizeMB = size / static_cast<double>(MB);
        return tr("%1 MB").arg(sizeMB, 0, 'f', 2);
    } else if (size < TB) {
        double sizeGB = size / static_cast<double>(GB);
        return tr("%1 GB").arg(sizeGB, 0, 'f', 2);
    } else {
        double sizeTB = size / static_cast<double>(TB);
        return tr("%1 TB").arg(sizeTB, 0, 'f', 2);
    }
}

void AddDownloadDialog::accept() {
    // Validate input
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Missing URL"), 
                           tr("Please enter a URL for the download."));
        return;
    }
    
    QString filename = m_filenameEdit->text().trimmed();
    if (filename.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Filename"), 
                           tr("Please enter a filename for the download."));
        return;
    }
    
    QString savePath = m_savePathEdit->text();
    if (savePath.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Save Path"), 
                           tr("Please select a save location for the download."));
        return;
    }
    
    // Make sure savePath ends with a separator
    if (!savePath.endsWith('/') && !savePath.endsWith('\\')) {
        savePath += '/';
    }
    
    // Check if file already exists
    QString fullPath = savePath + filename;
    QFileInfo fileInfo(fullPath);
    
    if (fileInfo.exists()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            tr("File Already Exists"), 
            tr("The file '%1' already exists.\n\nDo you want to overwrite it?").arg(filename),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) {
            return;
        }
    }
    
    // Create download options
    Core::DownloadOptions options;
    
    // Basic options
    options.url = url.toStdString();
    options.destination = fullPath.toStdString();
    options.segments = m_segmentsSpinBox->value();
    
    // Speed limit
    if (m_limitSpeedCheckBox->isChecked()) {
        options.maxSpeed = m_speedLimitSpinBox->value() * 1024; // Convert KB/s to bytes/s
    }
    
    // Authentication
    if (m_authRequiredCheckBox->isChecked()) {
        options.username = m_usernameEdit->text().toStdString();
        options.password = m_passwordEdit->text().toStdString();
    }
    
    // Schedule
    if (m_scheduleCheckBox->isChecked()) {
        options.scheduledTime = m_scheduleDateTime->dateTime().toSecsSinceEpoch();
    }
    
    // Get download manager instance
    auto& downloadManager = Core::DownloadManager::instance();
    
    // Add download
    try {
        downloadManager.addDownload(options);
        QDialog::accept();
    } 
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), 
                            tr("Failed to add download: %1").arg(e.what()));
    }
}

} // namespace ui
} // namespace dm