#include "../../include/ui/StatisticsDialog.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/StringUtils.h"
#include "../../include/utils/TimeUtils.h"
#include "../../include/core/Statistics.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QDateEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextEdit>
#include <QTimer>
#include <QPainter>
#include <QIcon>
#include <QPixmap>
#include <QFont>
#include <QPrinter>
#include <QPrintDialog>
#include <QRectF>
#include <QTextDocument>
#include <QBarSet>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QChartView>
#include <QLineSeries>
#include <QDateTime>
#include <QPieSeries>
#include <QCheckBox>
#include <QSpinBox>
#include <QScrollArea>

// Include or forward declare QtCharts classes
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

QT_CHARTS_USE_NAMESPACE

namespace DownloadManager {
namespace UI {

StatisticsDialog::StatisticsDialog(QWidget* parent)
    : QDialog(parent),
      m_updateTimer(nullptr)
{
    setWindowTitle(tr("Download Statistics"));
    setMinimumWidth(800);
    setMinimumHeight(600);
    
    createUI();
    setupConnections();
    
    // Initial update
    updateStatistics();
    
    // Start update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &StatisticsDialog::updateStatistics);
    m_updateTimer->start(5000); // Update every 5 seconds
}

StatisticsDialog::~StatisticsDialog() {
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void StatisticsDialog::createUI() {
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tabs
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // Summary tab
    QWidget* summaryTab = new QWidget(this);
    QVBoxLayout* summaryLayout = new QVBoxLayout(summaryTab);
    
    // Create a scroll area for the summary tab
    QScrollArea* summaryScrollArea = new QScrollArea(this);
    summaryScrollArea->setWidgetResizable(true);
    QWidget* summaryContent = new QWidget(this);
    QVBoxLayout* scrollLayout = new QVBoxLayout(summaryContent);
    
    // Summary boxes
    QGroupBox* downloadsGroup = new QGroupBox(tr("Download Statistics"), this);
    QGridLayout* downloadsLayout = new QGridLayout(downloadsGroup);
    
    // Create labels for statistics
    QFont boldFont;
    boldFont.setBold(true);
    
    // Row 1
    QLabel* totalDownloadsLabel = new QLabel(tr("Total Downloads:"), this);
    totalDownloadsLabel->setFont(boldFont);
    m_totalDownloadsValue = new QLabel("0", this);
    
    QLabel* completedDownloadsLabel = new QLabel(tr("Completed:"), this);
    completedDownloadsLabel->setFont(boldFont);
    m_completedDownloadsValue = new QLabel("0", this);
    
    QLabel* failedDownloadsLabel = new QLabel(tr("Failed:"), this);
    failedDownloadsLabel->setFont(boldFont);
    m_failedDownloadsValue = new QLabel("0", this);
    
    // Row 2
    QLabel* canceledDownloadsLabel = new QLabel(tr("Canceled:"), this);
    canceledDownloadsLabel->setFont(boldFont);
    m_canceledDownloadsValue = new QLabel("0", this);
    
    QLabel* totalBytesLabel = new QLabel(tr("Total Bytes Downloaded:"), this);
    totalBytesLabel->setFont(boldFont);
    m_totalBytesValue = new QLabel("0 bytes", this);
    
    QLabel* totalTimeLabel = new QLabel(tr("Total Download Time:"), this);
    totalTimeLabel->setFont(boldFont);
    m_totalTimeValue = new QLabel("0s", this);
    
    // Row 3
    QLabel* avgSpeedLabel = new QLabel(tr("Average Speed:"), this);
    avgSpeedLabel->setFont(boldFont);
    m_avgSpeedValue = new QLabel("0 B/s", this);
    
    QLabel* maxSpeedLabel = new QLabel(tr("Maximum Speed:"), this);
    maxSpeedLabel->setFont(boldFont);
    m_maxSpeedValue = new QLabel("0 B/s", this);
    
    QLabel* uptimeLabel = new QLabel(tr("Uptime:"), this);
    uptimeLabel->setFont(boldFont);
    m_uptimeValue = new QLabel("0s", this);
    
    // Add widgets to grid layout
    downloadsLayout->addWidget(totalDownloadsLabel, 0, 0);
    downloadsLayout->addWidget(m_totalDownloadsValue, 0, 1);
    downloadsLayout->addWidget(completedDownloadsLabel, 0, 2);
    downloadsLayout->addWidget(m_completedDownloadsValue, 0, 3);
    downloadsLayout->addWidget(failedDownloadsLabel, 0, 4);
    downloadsLayout->addWidget(m_failedDownloadsValue, 0, 5);
    
    downloadsLayout->addWidget(canceledDownloadsLabel, 1, 0);
    downloadsLayout->addWidget(m_canceledDownloadsValue, 1, 1);
    downloadsLayout->addWidget(totalBytesLabel, 1, 2);
    downloadsLayout->addWidget(m_totalBytesValue, 1, 3);
    downloadsLayout->addWidget(totalTimeLabel, 1, 4);
    downloadsLayout->addWidget(m_totalTimeValue, 1, 5);
    
    downloadsLayout->addWidget(avgSpeedLabel, 2, 0);
    downloadsLayout->addWidget(m_avgSpeedValue, 2, 1);
    downloadsLayout->addWidget(maxSpeedLabel, 2, 2);
    downloadsLayout->addWidget(m_maxSpeedValue, 2, 3);
    downloadsLayout->addWidget(uptimeLabel, 2, 4);
    downloadsLayout->addWidget(m_uptimeValue, 2, 5);
    
    // Daily statistics
    QGroupBox* dailyGroup = new QGroupBox(tr("Today's Statistics"), this);
    QGridLayout* dailyLayout = new QGridLayout(dailyGroup);
    
    // Row 1
    QLabel* dailyDownloadsLabel = new QLabel(tr("Downloads Today:"), this);
    dailyDownloadsLabel->setFont(boldFont);
    m_dailyDownloadsValue = new QLabel("0", this);
    
    QLabel* dailyCompletedLabel = new QLabel(tr("Completed Today:"), this);
    dailyCompletedLabel->setFont(boldFont);
    m_dailyCompletedValue = new QLabel("0", this);
    
    QLabel* dailyFailedLabel = new QLabel(tr("Failed Today:"), this);
    dailyFailedLabel->setFont(boldFont);
    m_dailyFailedValue = new QLabel("0", this);
    
    // Row 2
    QLabel* dailyBytesLabel = new QLabel(tr("Bytes Downloaded Today:"), this);
    dailyBytesLabel->setFont(boldFont);
    m_dailyBytesValue = new QLabel("0 bytes", this);
    
    QLabel* dailyAvgSpeedLabel = new QLabel(tr("Average Speed Today:"), this);
    dailyAvgSpeedLabel->setFont(boldFont);
    m_dailyAvgSpeedValue = new QLabel("0 B/s", this);
    
    QLabel* dailyPeakSpeedLabel = new QLabel(tr("Peak Speed Today:"), this);
    dailyPeakSpeedLabel->setFont(boldFont);
    m_dailyPeakSpeedValue = new QLabel("0 B/s", this);
    
    // Add widgets to daily layout
    dailyLayout->addWidget(dailyDownloadsLabel, 0, 0);
    dailyLayout->addWidget(m_dailyDownloadsValue, 0, 1);
    dailyLayout->addWidget(dailyCompletedLabel, 0, 2);
    dailyLayout->addWidget(m_dailyCompletedValue, 0, 3);
    dailyLayout->addWidget(dailyFailedLabel, 0, 4);
    dailyLayout->addWidget(m_dailyFailedValue, 0, 5);
    
    dailyLayout->addWidget(dailyBytesLabel, 1, 0);
    dailyLayout->addWidget(m_dailyBytesValue, 1, 1);
    dailyLayout->addWidget(dailyAvgSpeedLabel, 1, 2);
    dailyLayout->addWidget(m_dailyAvgSpeedValue, 1, 3);
    dailyLayout->addWidget(dailyPeakSpeedLabel, 1, 4);
    dailyLayout->addWidget(m_dailyPeakSpeedValue, 1, 5);
    
    // Summary charts
    QHBoxLayout* chartsLayout = new QHBoxLayout();
    
    // Downloads by status chart
    m_statusChart = new QChart();
    m_statusChart->setTitle(tr("Downloads by Status"));
    m_statusChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QChartView* statusChartView = new QChartView(m_statusChart, this);
    statusChartView->setRenderHint(QPainter::Antialiasing);
    statusChartView->setMinimumSize(350, 300);
    
    // File types chart
    m_typeChart = new QChart();
    m_typeChart->setTitle(tr("Downloads by File Type"));
    m_typeChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QChartView* typeChartView = new QChartView(m_typeChart, this);
    typeChartView->setRenderHint(QPainter::Antialiasing);
    typeChartView->setMinimumSize(350, 300);
    
    chartsLayout->addWidget(statusChartView);
    chartsLayout->addWidget(typeChartView);
    
    // Add buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton(tr("Refresh"), this);
    m_refreshButton->setIcon(QIcon(":/resources/icons/refresh.png"));
    
    m_resetButton = new QPushButton(tr("Reset Statistics"), this);
    m_resetButton->setIcon(QIcon(":/resources/icons/cancel.png"));
    
    m_exportButton = new QPushButton(tr("Export Report"), this);
    m_exportButton->setIcon(QIcon(":/resources/icons/batch.png"));
    
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addWidget(m_exportButton);
    
    // Add widgets to scroll layout
    scrollLayout->addWidget(downloadsGroup);
    scrollLayout->addWidget(dailyGroup);
    scrollLayout->addLayout(chartsLayout);
    scrollLayout->addStretch();
    
    // Set the scroll area widget
    summaryScrollArea->setWidget(summaryContent);
    
    // Add to summary layout
    summaryLayout->addWidget(summaryScrollArea);
    summaryLayout->addLayout(buttonLayout);
    
    // History tab
    QWidget* historyTab = new QWidget(this);
    QVBoxLayout* historyLayout = new QVBoxLayout(historyTab);
    
    // Time period selection
    QHBoxLayout* timeRangeLayout = new QHBoxLayout();
    
    QLabel* periodLabel = new QLabel(tr("Time Period:"), this);
    
    m_periodCombo = new QComboBox(this);
    m_periodCombo->addItem(tr("Last 24 Hours"), 1);
    m_periodCombo->addItem(tr("Last 7 Days"), 7);
    m_periodCombo->addItem(tr("Last 30 Days"), 30);
    m_periodCombo->addItem(tr("Last 90 Days"), 90);
    m_periodCombo->addItem(tr("Last 365 Days"), 365);
    m_periodCombo->addItem(tr("Custom Range"), 0);
    
    QLabel* fromLabel = new QLabel(tr("From:"), this);
    m_fromDateEdit = new QDateEdit(this);
    m_fromDateEdit->setCalendarPopup(true);
    m_fromDateEdit->setDate(QDate::currentDate().addDays(-7));
    m_fromDateEdit->setEnabled(false);
    
    QLabel* toLabel = new QLabel(tr("To:"), this);
    m_toDateEdit = new QDateEdit(this);
    m_toDateEdit->setCalendarPopup(true);
    m_toDateEdit->setDate(QDate::currentDate());
    m_toDateEdit->setEnabled(false);
    
    QPushButton* applyButton = new QPushButton(tr("Apply"), this);
    
    timeRangeLayout->addWidget(periodLabel);
    timeRangeLayout->addWidget(m_periodCombo);
    timeRangeLayout->addWidget(fromLabel);
    timeRangeLayout->addWidget(m_fromDateEdit);
    timeRangeLayout->addWidget(toLabel);
    timeRangeLayout->addWidget(m_toDateEdit);
    timeRangeLayout->addWidget(applyButton);
    
    // Download history chart
    m_historyChart = new QChart();
    m_historyChart->setTitle(tr("Download History"));
    m_historyChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QChartView* historyChartView = new QChartView(m_historyChart, this);
    historyChartView->setRenderHint(QPainter::Antialiasing);
    historyChartView->setMinimumHeight(300);
    
    // Speed history chart
    m_speedChart = new QChart();
    m_speedChart->setTitle(tr("Speed History"));
    m_speedChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QChartView* speedChartView = new QChartView(m_speedChart, this);
    speedChartView->setRenderHint(QPainter::Antialiasing);
    speedChartView->setMinimumHeight(200);
    
    // Downloads history table
    m_historyTable = new QTableWidget(this);
    m_historyTable->setColumnCount(5);
    m_historyTable->setHorizontalHeaderLabels({
        tr("Date & Time"),
        tr("Bytes Downloaded"),
        tr("Download Time"),
        tr("Average Speed"),
        tr("Status")
    });
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->verticalHeader()->setVisible(false);
    
    // Add to history layout
    historyLayout->addLayout(timeRangeLayout);
    historyLayout->addWidget(historyChartView);
    historyLayout->addWidget(speedChartView);
    historyLayout->addWidget(m_historyTable);
    
    // Reports tab
    QWidget* reportsTab = new QWidget(this);
    QVBoxLayout* reportsLayout = new QVBoxLayout(reportsTab);
    
    // Report options
    QGroupBox* reportOptionsGroup = new QGroupBox(tr("Report Options"), this);
    QGridLayout* reportOptionsLayout = new QGridLayout(reportOptionsGroup);
    
    QLabel* reportTypeLabel = new QLabel(tr("Report Type:"), this);
    m_reportTypeCombo = new QComboBox(this);
    m_reportTypeCombo->addItem(tr("Summary Report"), "summary");
    m_reportTypeCombo->addItem(tr("Daily Stats Report"), "daily");
    m_reportTypeCombo->addItem(tr("File Type Analysis"), "filetype");
    m_reportTypeCombo->addItem(tr("Full Report"), "full");
    
    QLabel* reportPeriodLabel = new QLabel(tr("Time Period:"), this);
    m_reportPeriodCombo = new QComboBox(this);
    m_reportPeriodCombo->addItem(tr("Last 24 Hours"), 1);
    m_reportPeriodCombo->addItem(tr("Last 7 Days"), 7);
    m_reportPeriodCombo->addItem(tr("Last 30 Days"), 30);
    m_reportPeriodCombo->addItem(tr("Last 90 Days"), 90);
    m_reportPeriodCombo->addItem(tr("All Time"), 0);
    
    QLabel* includeChartsLabel = new QLabel(tr("Include Charts:"), this);
    m_includeChartsCheck = new QCheckBox(this);
    m_includeChartsCheck->setChecked(true);
    
    reportOptionsLayout->addWidget(reportTypeLabel, 0, 0);
    reportOptionsLayout->addWidget(m_reportTypeCombo, 0, 1);
    reportOptionsLayout->addWidget(reportPeriodLabel, 0, 2);
    reportOptionsLayout->addWidget(m_reportPeriodCombo, 0, 3);
    reportOptionsLayout->addWidget(includeChartsLabel, 0, 4);
    reportOptionsLayout->addWidget(m_includeChartsCheck, 0, 5);
    
    // Report preview
    QGroupBox* reportPreviewGroup = new QGroupBox(tr("Report Preview"), this);
    QVBoxLayout* reportPreviewLayout = new QVBoxLayout(reportPreviewGroup);
    
    m_reportPreview = new QTextEdit(this);
    m_reportPreview->setReadOnly(true);
    
    reportPreviewLayout->addWidget(m_reportPreview);
    
    // Report buttons
    QHBoxLayout* reportButtonLayout = new QHBoxLayout();
    
    m_generateReportButton = new QPushButton(tr("Generate Report"), this);
    m_printReportButton = new QPushButton(tr("Print Report"), this);
    m_saveReportButton = new QPushButton(tr("Save Report"), this);
    
    reportButtonLayout->addWidget(m_generateReportButton);
    reportButtonLayout->addStretch();
    reportButtonLayout->addWidget(m_printReportButton);
    reportButtonLayout->addWidget(m_saveReportButton);
    
    // Add to reports layout
    reportsLayout->addWidget(reportOptionsGroup);
    reportsLayout->addWidget(reportPreviewGroup);
    reportsLayout->addLayout(reportButtonLayout);
    
    // Add tabs to widget
    tabWidget->addTab(summaryTab, tr("Summary"));
    tabWidget->addTab(historyTab, tr("History"));
    tabWidget->addTab(reportsTab, tr("Reports"));
    
    // Main layout
    mainLayout->addWidget(tabWidget);
}

void StatisticsDialog::setupConnections() {
    // Refresh button
    connect(m_refreshButton, &QPushButton::clicked, this, &StatisticsDialog::updateStatistics);
    
    // Reset button
    connect(m_resetButton, &QPushButton::clicked, this, &StatisticsDialog::resetStatistics);
    
    // Export button
    connect(m_exportButton, &QPushButton::clicked, this, &StatisticsDialog::exportReport);
    
    // Period combobox
    connect(m_periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatisticsDialog::onPeriodChanged);
    
    // Apply button
    QPushButton* applyButton = nullptr;
    
    // Find apply button
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text() == tr("Apply")) {
            applyButton = button;
            break;
        }
    }
    
    if (applyButton) {
        connect(applyButton, &QPushButton::clicked, this, &StatisticsDialog::updateHistoryCharts);
    }
    
    // Report generation and actions
    connect(m_generateReportButton, &QPushButton::clicked, this, &StatisticsDialog::generateReport);
    connect(m_printReportButton, &QPushButton::clicked, this, &StatisticsDialog::printReport);
    connect(m_saveReportButton, &QPushButton::clicked, this, &StatisticsDialog::saveReport);
}

void StatisticsDialog::updateStatistics() {
    auto& stats = Core::Statistics::instance();
    
    // Update summary statistics
    m_totalDownloadsValue->setText(QString::number(stats.getTotalDownloads()));
    m_completedDownloadsValue->setText(QString::number(stats.getCompletedDownloads()));
    m_failedDownloadsValue->setText(QString::number(stats.getFailedDownloads()));
    m_canceledDownloadsValue->setText(QString::number(stats.getCanceledDownloads()));
    
    QString totalBytesStr = QString::fromStdString(
        Utils::StringUtils::formatFileSize(stats.getTotalBytesDownloaded())
    );
    m_totalBytesValue->setText(totalBytesStr);
    
    QString totalTimeStr = QString::fromStdString(
        Utils::StringUtils::formatTime(stats.getTotalDownloadTime())
    );
    m_totalTimeValue->setText(totalTimeStr);
    
    QString avgSpeedStr = QString::fromStdString(
        Utils::StringUtils::formatBitrate(stats.getAverageSpeed())
    );
    m_avgSpeedValue->setText(avgSpeedStr);
    
    QString maxSpeedStr = QString::fromStdString(
        Utils::StringUtils::formatBitrate(stats.getMaxSpeed())
    );
    m_maxSpeedValue->setText(maxSpeedStr);
    
    QString uptimeStr = QString::fromStdString(
        Utils::StringUtils::formatTime(stats.getUptime())
    );
    m_uptimeValue->setText(uptimeStr);
    
    // Update daily statistics
    Core::DailyStats dailyStats = stats.getDailyStats();
    
    m_dailyDownloadsValue->setText(QString::number(dailyStats.totalDownloads));
    m_dailyCompletedValue->setText(QString::number(dailyStats.successfulDownloads));
    m_dailyFailedValue->setText(QString::number(dailyStats.failedDownloads));
    
    QString dailyBytesStr = QString::fromStdString(
        Utils::StringUtils::formatFileSize(dailyStats.totalBytesDownloaded)
    );
    m_dailyBytesValue->setText(dailyBytesStr);
    
    int dailyAvgSpeed = 0;
    if (dailyStats.totalDownloadTime > 0) {
        dailyAvgSpeed = static_cast<int>(dailyStats.totalBytesDownloaded / dailyStats.totalDownloadTime);
    }
    
    QString dailyAvgSpeedStr = QString::fromStdString(
        Utils::StringUtils::formatBitrate(dailyAvgSpeed)
    );
    m_dailyAvgSpeedValue->setText(dailyAvgSpeedStr);
    
    QString dailyPeakSpeedStr = QString::fromStdString(
        Utils::StringUtils::formatBitrate(dailyStats.peakSpeed)
    );
    m_dailyPeakSpeedValue->setText(dailyPeakSpeedStr);
    
    // Update charts
    updateStatusChart();
    updateTypeChart();
    
    // If History tab is selected, update history charts
    QTabWidget* tabWidget = findChild<QTabWidget*>();
    if (tabWidget && tabWidget->currentWidget()->objectName().contains("History", Qt::CaseInsensitive)) {
        updateHistoryCharts();
    }
}

void StatisticsDialog::updateStatusChart() {
    auto& stats = Core::Statistics::instance();
    
    // Clear old series
    m_statusChart->removeAllSeries();
    
    // Create pie series
    QPieSeries* series = new QPieSeries();
    
    // Add slices
    int completed = stats.getCompletedDownloads();
    int failed = stats.getFailedDownloads();
    int canceled = stats.getCanceledDownloads();
    int total = completed + failed + canceled;
    
    // Only add slices with values
    if (completed > 0) {
        QPieSlice* completedSlice = series->append(tr("Completed"), completed);
        completedSlice->setColor(QColor(0, 170, 0)); // Green
        completedSlice->setLabelVisible(true);
        completedSlice->setLabelPosition(QPieSlice::LabelOutside);
        completedSlice->setLabelColor(Qt::black);
        
        // Show percentage if there's enough data
        if (total > 10) {
            double percentage = (static_cast<double>(completed) / total) * 100;
            completedSlice->setLabel(tr("Completed: %1 (%2%)").arg(completed).arg(percentage, 0, 'f', 1));
        } else {
            completedSlice->setLabel(tr("Completed: %1").arg(completed));
        }
    }
    
    if (failed > 0) {
        QPieSlice* failedSlice = series->append(tr("Failed"), failed);
        failedSlice->setColor(QColor(170, 0, 0)); // Red
        failedSlice->setLabelVisible(true);
        failedSlice->setLabelPosition(QPieSlice::LabelOutside);
        failedSlice->setLabelColor(Qt::black);
        
        // Show percentage if there's enough data
        if (total > 10) {
            double percentage = (static_cast<double>(failed) / total) * 100;
            failedSlice->setLabel(tr("Failed: %1 (%2%)").arg(failed).arg(percentage, 0, 'f', 1));
        } else {
            failedSlice->setLabel(tr("Failed: %1").arg(failed));
        }
    }
    
    if (canceled > 0) {
        QPieSlice* canceledSlice = series->append(tr("Canceled"), canceled);
        canceledSlice->setColor(QColor(200, 170, 0)); // Yellow/Orange
        canceledSlice->setLabelVisible(true);
        canceledSlice->setLabelPosition(QPieSlice::LabelOutside);
        canceledSlice->setLabelColor(Qt::black);
        
        // Show percentage if there's enough data
        if (total > 10) {
            double percentage = (static_cast<double>(canceled) / total) * 100;
            canceledSlice->setLabel(tr("Canceled: %1 (%2%)").arg(canceled).arg(percentage, 0, 'f', 1));
        } else {
            canceledSlice->setLabel(tr("Canceled: %1").arg(canceled));
        }
    }
    
    // Add series to chart
    m_statusChart->addSeries(series);
    
    // Optional customization
    m_statusChart->setTitle(tr("Downloads by Status"));
    m_statusChart->legend()->hide();
}

void StatisticsDialog::updateTypeChart() {
    auto& stats = Core::Statistics::instance();
    
    // Get file type statistics
    auto fileTypeStats = stats.getDownloadsByFileType();
    
    // Clear old series
    m_typeChart->removeAllSeries();
    
    // Check if we have data
    if (fileTypeStats.empty()) {
        // Create a message for no data
        m_typeChart->setTitle(tr("No File Type Data Available"));
        return;
    }
    
    // Create pie series
    QPieSeries* series = new QPieSeries();
    
    // Calculate total for percentages
    int64_t totalBytes = 0;
    for (const auto& pair : fileTypeStats) {
        totalBytes += pair.second;
    }
    
    // Convert map to vector for sorting
    std::vector<std::pair<std::string, int64_t>> fileTypes;
    for (const auto& pair : fileTypeStats) {
        fileTypes.push_back(pair);
    }
    
    // Sort by size (descending)
    std::sort(fileTypes.begin(), fileTypes.end(), 
             [](const std::pair<std::string, int64_t>& a, const std::pair<std::string, int64_t>& b) {
                 return a.second > b.second;
             });
    
    // Limit to top 10 types
    const int maxTypes = 10;
    int64_t otherBytes = 0;
    
    for (size_t i = 0; i < fileTypes.size(); i++) {
        if (i < maxTypes) {
            // Add this type as a slice
            QString label = QString::fromStdString(fileTypes[i].first);
            double percentage = (static_cast<double>(fileTypes[i].second) / totalBytes) * 100;
            
            // Skip types with very small percentages
            if (percentage < 1.0) {
                otherBytes += fileTypes[i].second;
                continue;
            }
            
            QString sizeStr = QString::fromStdString(
                Utils::StringUtils::formatFileSize(fileTypes[i].second)
            );
            
            QPieSlice* slice = series->append(label, fileTypes[i].second);
            slice->setLabel(QString("%1: %2 (%3%)").arg(label).arg(sizeStr).arg(percentage, 0, 'f', 1));
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabelColor(Qt::black);
            
            // Generate a color based on index
            QColor color = QColor::fromHsv((i * 50) % 360, 200, 200);
            slice->setColor(color);
        } else {
            // Sum remaining types into "Other" category
            otherBytes += fileTypes[i].second;
        }
    }
    
    // Add "Other" slice if needed
    if (otherBytes > 0) {
        double percentage = (static_cast<double>(otherBytes) / totalBytes) * 100;
        QString sizeStr = QString::fromStdString(
            Utils::StringUtils::formatFileSize(otherBytes)
        );
        
        QPieSlice* otherSlice = series->append(tr("Other"), otherBytes);
        otherSlice->setLabel(QString("Other: %1 (%2%)").arg(sizeStr).arg(percentage, 0, 'f', 1));
        otherSlice->setLabelVisible(true);
        otherSlice->setLabelPosition(QPieSlice::LabelOutside);
        otherSlice->setLabelColor(Qt::black);
        otherSlice->setColor(QColor(150, 150, 150)); // Gray
    }
    
    // Add series to chart
    m_typeChart->addSeries(series);
    
    // Optional customization
    m_typeChart->setTitle(tr("Downloads by File Type"));
    m_typeChart->legend()->hide();
}

void StatisticsDialog::onPeriodChanged(int index) {
    // Enable/disable date inputs for custom range
    bool isCustom = (m_periodCombo->currentData().toInt() == 0);
    
    m_fromDateEdit->setEnabled(isCustom);
    m_toDateEdit->setEnabled(isCustom);
    
    // Set default range based on selection
    if (!isCustom) {
        int days = m_periodCombo->currentData().toInt();
        m_fromDateEdit->setDate(QDate::currentDate().addDays(-days));
        m_toDateEdit->setDate(QDate::currentDate());
    }
    
    // Update charts
    updateHistoryCharts();
}

void StatisticsDialog::updateHistoryCharts() {
    auto& stats = Core::Statistics::instance();
    
    // Get date range
    QDateTime fromDateTime(m_fromDateEdit->date());
    QDateTime toDateTime(m_toDateEdit->date().addDays(1)); // Include the entire end day
    
    time_t fromTime = fromDateTime.toSecsSinceEpoch();
    time_t toTime = toDateTime.toSecsSinceEpoch();
    
    // Get history data
    std::vector<Core::DownloadHistoryItem> history = 
        stats.getDownloadHistory(fromTime, toTime);
    
    std::vector<Core::SpeedHistoryItem> speedHistory = 
        stats.getSpeedHistory(fromTime, toTime);
    
    // Update history chart
    updateDownloadHistoryChart(history);
    
    // Update speed chart
    updateSpeedHistoryChart(speedHistory);
    
    // Update history table
    updateHistoryTable(history);
}

void StatisticsDialog::updateDownloadHistoryChart(const std::vector<Core::DownloadHistoryItem>& history) {
    // Clear old series
    m_historyChart->removeAllSeries();
    
    // Check if we have data
    if (history.empty()) {
        m_historyChart->setTitle(tr("No Download History Available"));
        return;
    }
    
    // Create series for completed, failed, and canceled downloads
    QLineSeries* completedSeries = new QLineSeries();
    completedSeries->setName(tr("Completed"));
    
    QLineSeries* failedSeries = new QLineSeries();
    failedSeries->setName(tr("Failed"));
    
    QLineSeries* canceledSeries = new QLineSeries();
    canceledSeries->setName(tr("Canceled"));
    
    // Create series for bytes downloaded
    QLineSeries* bytesSeries = new QLineSeries();
    bytesSeries->setName(tr("Bytes Downloaded"));
    
    // Group data by day
    std::map<QDate, std::map<Core::DownloadStatus, int>> dailyCounts;
    std::map<QDate, int64_t> dailyBytes;
    
    for (const auto& item : history) {
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(item.timestamp);
        QDate date = dateTime.date();
        
        dailyCounts[date][item.status]++;
        dailyBytes[date] += item.bytesDownloaded;
    }
    
    // Add data points to series
    QVector<QDateTime> categories;
    
    // Sort dates
    std::vector<QDate> dates;
    for (const auto& pair : dailyCounts) {
        dates.push_back(pair.first);
    }
    
    std::sort(dates.begin(), dates.end());
    
    // Add data points
    for (const auto& date : dates) {
        QDateTime dateTime(date);
        qreal x = dateTime.toMSecsSinceEpoch();
        categories.append(dateTime);
        
        completedSeries->append(x, dailyCounts[date][Core::DownloadStatus::COMPLETED]);
        failedSeries->append(x, dailyCounts[date][Core::DownloadStatus::FAILED]);
        canceledSeries->append(x, dailyCounts[date][Core::DownloadStatus::CANCELED]);
        
        // Convert bytes to MB for better scaling
        double mbDownloaded = static_cast<double>(dailyBytes[date]) / (1024 * 1024);
        bytesSeries->append(x, mbDownloaded);
    }
    
    // Set colors
    completedSeries->setColor(QColor(0, 170, 0)); // Green
    failedSeries->setColor(QColor(170, 0, 0));    // Red
    canceledSeries->setColor(QColor(200, 170, 0)); // Yellow/Orange
    bytesSeries->setColor(QColor(0, 0, 170));     // Blue
    
    // Set line width
    completedSeries->setPen(QPen(completedSeries->pen().color(), 2));
    failedSeries->setPen(QPen(failedSeries->pen().color(), 2));
    canceledSeries->setPen(QPen(canceledSeries->pen().color(), 2));
    bytesSeries->setPen(QPen(bytesSeries->pen().color(), 2));
    
    // Add series to chart
    m_historyChart->addSeries(completedSeries);
    m_historyChart->addSeries(failedSeries);
    m_historyChart->addSeries(canceledSeries);
    
    // Create axes
    QDateTimeAxis* axisX = new QDateTimeAxis();
    axisX->setFormat("MM-dd");
    axisX->setTitleText(tr("Date"));
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setLabelFormat("%i");
    axisY->setTitleText(tr("Downloads"));
    
    // Set ranges
    axisX->setRange(categories.first(), categories.last());
    
    int maxCount = 0;
    for (const auto& date : dates) {
        int total = dailyCounts[date][Core::DownloadStatus::COMPLETED] +
                  dailyCounts[date][Core::DownloadStatus::FAILED] +
                  dailyCounts[date][Core::DownloadStatus::CANCELED];
        maxCount = std::max(maxCount, total);
    }
    
    axisY->setRange(0, maxCount * 1.1); // Add 10% padding
    
    // Add axes to chart
    m_historyChart->addAxis(axisX, Qt::AlignBottom);
    m_historyChart->addAxis(axisY, Qt::AlignLeft);
    
    // Attach axes to series
    completedSeries->attachAxis(axisX);
    completedSeries->attachAxis(axisY);
    failedSeries->attachAxis(axisX);
    failedSeries->attachAxis(axisY);
    canceledSeries->attachAxis(axisX);
    canceledSeries->attachAxis(axisY);
    
    // Set chart title
    m_historyChart->setTitle(tr("Download History"));
    
    // Show legend
    m_historyChart->legend()->setVisible(true);
    m_historyChart->legend()->setAlignment(Qt::AlignBottom);
}

void StatisticsDialog::updateSpeedHistoryChart(const std::vector<Core::SpeedHistoryItem>& speedHistory) {
    // Clear old series
    m_speedChart->removeAllSeries();
    
    // Check if we have data
    if (speedHistory.empty()) {
        m_speedChart->setTitle(tr("No Speed History Available"));
        return;
    }
    
    // Create series for speed
    QLineSeries* speedSeries = new QLineSeries();
    speedSeries->setName(tr("Download Speed"));
    
    // Get time range
    QDateTime fromTime = QDateTime::fromSecsSinceEpoch(speedHistory.front().timestamp);
    QDateTime toTime = QDateTime::fromSecsSinceEpoch(speedHistory.back().timestamp);
    
    // If we have too many data points, reduce them
    const int maxPoints = 500;
    int step = static_cast<int>(speedHistory.size() / maxPoints) + 1;
    
    // Add data points to series
    for (size_t i = 0; i < speedHistory.size(); i += step) {
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(speedHistory[i].timestamp);
        qreal x = dateTime.toMSecsSinceEpoch();
        
        // Convert B/s to KB/s for better scaling
        double kbSpeed = static_cast<double>(speedHistory[i].speed) / 1024;
        speedSeries->append(x, kbSpeed);
    }
    
    // Set color and line width
    speedSeries->setColor(QColor(0, 120, 215)); // Blue
    speedSeries->setPen(QPen(speedSeries->pen().color(), 2));
    
    // Add series to chart
    m_speedChart->addSeries(speedSeries);
    
    // Create axes
    QDateTimeAxis* axisX = new QDateTimeAxis();
    
    // Format based on time range
    int daysDiff = fromTime.daysTo(toTime);
    if (daysDiff > 7) {
        axisX->setFormat("MM-dd");
    } else if (daysDiff > 1) {
        axisX->setFormat("MM-dd hh:mm");
    } else {
        axisX->setFormat("hh:mm");
    }
    
    axisX->setTitleText(tr("Time"));
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setLabelFormat("%.1f");
    axisY->setTitleText(tr("Speed (KB/s)"));
    
    // Set ranges
    axisX->setRange(fromTime, toTime);
    
    // Find maximum speed
    int maxSpeed = 0;
    for (const auto& item : speedHistory) {
        maxSpeed = std::max(maxSpeed, item.speed);
    }
    
    // Convert to KB/s
    double maxKBSpeed = static_cast<double>(maxSpeed) / 1024;
    axisY->setRange(0, maxKBSpeed * 1.1); // Add 10% padding
    
    // Add axes to chart
    m_speedChart->addAxis(axisX, Qt::AlignBottom);
    m_speedChart->addAxis(axisY, Qt::AlignLeft);
    
    // Attach axes to series
    speedSeries->attachAxis(axisX);
    speedSeries->attachAxis(axisY);
    
    // Set chart title
    m_speedChart->setTitle(tr("Download Speed History"));
    
    // Hide legend if only one series
    m_speedChart->legend()->setVisible(false);
}

void StatisticsDialog::updateHistoryTable(const std::vector<Core::DownloadHistoryItem>& history) {
    // Create a copy of history to sort
    std::vector<Core::DownloadHistoryItem> sortedHistory = history;
    
    // Sort by timestamp (most recent first)
    std::sort(sortedHistory.begin(), sortedHistory.end(), 
             [](const Core::DownloadHistoryItem& a, const Core::DownloadHistoryItem& b) {
                 return a.timestamp > b.timestamp;
             });
    
    // Update table
    m_historyTable->setRowCount(static_cast<int>(sortedHistory.size()));
    
    for (size_t i = 0; i < sortedHistory.size(); i++) {
        const auto& item = sortedHistory[i];
        
        // Date & Time
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(item.timestamp);
        QTableWidgetItem* dateItem = new QTableWidgetItem(
            dateTime.toString("yyyy-MM-dd HH:mm:ss")
        );
        m_historyTable->setItem(i, 0, dateItem);
        
        // Bytes Downloaded
        QString bytesStr = QString::fromStdString(
            Utils::StringUtils::formatFileSize(item.bytesDownloaded)
        );
        QTableWidgetItem* bytesItem = new QTableWidgetItem(bytesStr);
        m_historyTable->setItem(i, 1, bytesItem);
        
        // Download Time
        QString timeStr = QString::fromStdString(
            Utils::StringUtils::formatTime(item.downloadTime)
        );
        QTableWidgetItem* timeItem = new QTableWidgetItem(timeStr);
        m_historyTable->setItem(i, 2, timeItem);
        
        // Average Speed
        QString speedStr = QString::fromStdString(
            Utils::StringUtils::formatBitrate(item.averageSpeed)
        );
        QTableWidgetItem* speedItem = new QTableWidgetItem(speedStr);
        m_historyTable->setItem(i, 3, speedItem);
        
        // Status
        QString statusStr;
        QColor statusColor;
        
        switch (item.status) {
            case Core::DownloadStatus::COMPLETED:
                statusStr = tr("Completed");
                statusColor = QColor(0, 170, 0); // Green
                break;
            case Core::DownloadStatus::FAILED:
                statusStr = tr("Failed");
                statusColor = QColor(170, 0, 0); // Red
                break;
            case Core::DownloadStatus::CANCELED:
                statusStr = tr("Canceled");
                statusColor = QColor(200, 170, 0); // Yellow/Orange
                break;
            default:
                statusStr = tr("Unknown");
                statusColor = QColor(100, 100, 100); // Gray
        }
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusStr);
        statusItem->setForeground(statusColor);
        m_historyTable->setItem(i, 4, statusItem);
    }
}

void StatisticsDialog::resetStatistics() {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Reset"),
        tr("Are you sure you want to reset all statistics data? This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    auto& stats = Core::Statistics::instance();
    
    // Reset statistics
    stats.resetAllStatistics();
    
    // Update UI
    updateStatistics();
    
    QMessageBox::information(this, tr("Statistics Reset"), 
                          tr("All statistics have been reset."));
}

void StatisticsDialog::exportReport() {
    // Prepare report content
    auto& stats = Core::Statistics::instance();
    std::string reportText = stats.getStatsReport();
    
    // Ask for save location
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Statistics Report"),
        QDir::homePath() + "/download_stats_report.txt",
        tr("Text Files (*.txt);;All Files (*.*)")
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Save report to file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QString::fromStdString(reportText);
        file.close();
        
        QMessageBox::information(this, tr("Report Exported"), 
                              tr("Statistics report has been saved to:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, tr("Export Error"), 
                          tr("Failed to save the report to:\n%1").arg(fileName));
    }
}

void StatisticsDialog::generateReport() {
    QString reportType = m_reportTypeCombo->currentData().toString();
    int period = m_reportPeriodCombo->currentData().toInt();
    bool includeCharts = m_includeChartsCheck->isChecked();
    
    // Generate report content
    QString reportContent = generateReportContent(reportType, period, includeCharts);
    
    // Update preview
    m_reportPreview->setHtml(reportContent);
}

void StatisticsDialog::printReport() {
    // First ensure we have a generated report
    if (m_reportPreview->toPlainText().isEmpty()) {
        generateReport();
    }
    
    // Print the report
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);
    
    if (printDialog.exec() == QDialog::Accepted) {
        m_reportPreview->print(&printer);
        
        QMessageBox::information(this, tr("Report Printed"), 
                              tr("Statistics report has been sent to the printer."));
    }
}

void StatisticsDialog::saveReport() {
    // First ensure we have a generated report
    if (m_reportPreview->toPlainText().isEmpty()) {
        generateReport();
    }
    
    // Ask for save location
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Statistics Report"),
        QDir::homePath() + "/download_stats_report.html",
        tr("HTML Files (*.html);;Text Files (*.txt);;All Files (*.*)")
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Save report to file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        
        if (fileName.endsWith(".txt", Qt::CaseInsensitive)) {
            stream << m_reportPreview->toPlainText();
        } else {
            stream << m_reportPreview->toHtml();
        }
        
        file.close();
        
        QMessageBox::information(this, tr("Report Saved"), 
                              tr("Statistics report has been saved to:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, tr("Save Error"), 
                          tr("Failed to save the report to:\n%1").arg(fileName));
    }
}

QString StatisticsDialog::generateReportContent(const QString& reportType, int period, bool includeCharts) {
    auto& stats = Core::Statistics::instance();
    
    // Get time range
    time_t now = Utils::TimeUtils::getCurrentTimeSeconds();
    time_t startTime = now;
    
    if (period > 0) {
        startTime = now - (period * 86400); // Convert days to seconds
    }
    
    // Get relevant data
    std::vector<Core::DownloadHistoryItem> history = 
        stats.getDownloadHistory(startTime, now);
    
    std::vector<Core::DailyStatsItem> dailyStats = 
        stats.getDailyDownloadStats(period > 0 ? period : 30);
    
    // Start HTML content
    QString html = "<html><head>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }";
    html += "h1, h2 { color: #333; }";
    html += "table { border-collapse: collapse; width: 100%; margin-top: 10px; margin-bottom: 20px; }";
    html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
    html += "th { background-color: #f2f2f2; }";
    html += "tr:nth-child(even) { background-color: #f9f9f9; }";
    html += ".chart-container { margin: 20px 0; text-align: center; }";
    html += ".section { margin-bottom: 30px; }";
    html += "</style>";
    html += "</head><body>";
    
    // Title and date
    html += "<h1>Download Manager Statistics Report</h1>";
    html += "<p>Generated: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "</p>";
    
    if (period > 0) {
        QDateTime startDate = QDateTime::fromSecsSinceEpoch(startTime);
        html += "<p>Period: " + startDate.toString("yyyy-MM-dd") + " to " + 
              QDateTime::currentDateTime().toString("yyyy-MM-dd") + " (" + QString::number(period) + " days)</p>";
    } else {
        html += "<p>Period: All Time</p>";
    }
    
    // Summary statistics section
    if (reportType == "summary" || reportType == "full") {
        html += "<div class='section'>";
        html += "<h2>Summary Statistics</h2>";
        
        html += "<table>";
        html += "<tr><th>Metric</th><th>Value</th></tr>";
        
        html += "<tr><td>Total Downloads</td><td>" + QString::number(stats.getTotalDownloads()) + "</td></tr>";
        html += "<tr><td>Completed Downloads</td><td>" + QString::number(stats.getCompletedDownloads()) + "</td></tr>";
        html += "<tr><td>Failed Downloads</td><td>" + QString::number(stats.getFailedDownloads()) + "</td></tr>";
        html += "<tr><td>Canceled Downloads</td><td>" + QString::number(stats.getCanceledDownloads()) + "</td></tr>";
        
        html += "<tr><td>Total Bytes Downloaded</td><td>" + 
              QString::fromStdString(Utils::StringUtils::formatFileSize(stats.getTotalBytesDownloaded())) + 
              "</td></tr>";
        
        html += "<tr><td>Total Download Time</td><td>" + 
              QString::fromStdString(Utils::StringUtils::formatTime(stats.getTotalDownloadTime())) + 
              "</td></tr>";
        
        html += "<tr><td>Average Speed</td><td>" + 
              QString::fromStdString(Utils::StringUtils::formatBitrate(stats.getAverageSpeed())) + 
              "</td></tr>";
        
        html += "<tr><td>Maximum Speed</td><td>" + 
              QString::fromStdString(Utils::StringUtils::formatBitrate(stats.getMaxSpeed())) + 
              "</td></tr>";
        
        html += "<tr><td>Uptime</td><td>" + 
              QString::fromStdString(Utils::StringUtils::formatTime(stats.getUptime())) + 
              "</td></tr>";
        
        html += "</table>";
        
        // Add status chart if requested
        if (includeCharts) {
            html += "<div class='chart-container'>";
            html += "<h3>Downloads by Status</h3>";
            html += "<p><i>Chart not available in printed report</i></p>";
            html += "</div>";
        }
        
        html += "</div>";
    }
    
    // Daily statistics section
    if (reportType == "daily" || reportType == "full") {
        html += "<div class='section'>";
        html += "<h2>Daily Statistics</h2>";
        
        html += "<table>";
        html += "<tr><th>Date</th><th>Downloads</th><th>Completed</th><th>Failed</th><th>Bytes Downloaded</th><th>Average Speed</th></tr>";
        
        for (const auto& day : dailyStats) {
            QDateTime date = QDateTime::fromSecsSinceEpoch(day.date);
            
            html += "<tr>";
            html += "<td>" + date.toString("yyyy-MM-dd") + "</td>";
            html += "<td>" + QString::number(day.totalDownloads) + "</td>";
            html += "<td>" + QString::number(day.successfulDownloads) + "</td>";
            html += "<td>" + QString::number(day.failedDownloads) + "</td>";
            html += "<td>" + QString::fromStdString(Utils::StringUtils::formatFileSize(day.totalBytesDownloaded)) + "</td>";
            html += "<td>" + QString::fromStdString(Utils::StringUtils::formatBitrate(day.averageSpeed)) + "</td>";
            html += "</tr>";
        }
        
        html += "</table>";
        
        // Add daily chart if requested
        if (includeCharts) {
            html += "<div class='chart-container'>";
            html += "<h3>Daily Download Trends</h3>";
            html += "<p><i>Chart not available in printed report</i></p>";
            html += "</div>";
        }
        
        html += "</div>";
    }
    
    // File type analysis section
    if (reportType == "filetype" || reportType == "full") {
        html += "<div class='section'>";
        html += "<h2>File Type Analysis</h2>";
        
        auto fileTypeStats = stats.getDownloadsByFileType();
        
        if (!fileTypeStats.empty()) {
            // Convert map to vector for sorting
            std::vector<std::pair<std::string, int64_t>> fileTypes;
            for (const auto& pair : fileTypeStats) {
                fileTypes.push_back(pair);
            }
            
            // Sort by size (descending)
            std::sort(fileTypes.begin(), fileTypes.end(), 
                     [](const std::pair<std::string, int64_t>& a, const std::pair<std::string, int64_t>& b) {
                         return a.second > b.second;
                     });
            
            html += "<table>";
            html += "<tr><th>File Type</th><th>Total Bytes</th><th>Percentage</th></tr>";
            
            // Calculate total for percentages
            int64_t totalBytes = 0;
            for (const auto& pair : fileTypes) {
                totalBytes += pair.second;
            }
            
            for (const auto& type : fileTypes) {
                double percentage = (static_cast<double>(type.second) / totalBytes) * 100;
                
                html += "<tr>";
                html += "<td>" + QString::fromStdString(type.first) + "</td>";
                html += "<td>" + QString::fromStdString(Utils::StringUtils::formatFileSize(type.second)) + "</td>";
                html += "<td>" + QString::number(percentage, 'f', 2) + "%</td>";
                html += "</tr>";
            }
            
            html += "</table>";
            
            // Add file type chart if requested
            if (includeCharts) {
                html += "<div class='chart-container'>";
                html += "<h3>Downloads by File Type</h3>";
                html += "<p><i>Chart not available in printed report</i></p>";
                html += "</div>";
            }
        } else {
            html += "<p>No file type data available.</p>";
        }
        
        html += "</div>";
    }
    
    // End HTML content
    html += "</body></html>";
    
    return html;
}

} // namespace UI
} // namespace DownloadManager