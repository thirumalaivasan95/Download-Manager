#include "include/ui/ProgressBar.h"
#include "include/utils/FileUtils.h"

#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionProgressBar>
#include <QApplication>
#include <QStyle>

namespace dm {
namespace ui {

ProgressBar::ProgressBar(QWidget* parent)
    : QProgressBar(parent),
      barColor_(QColor(0, 120, 215)),
      textColor_(Qt::white),
      borderColor_(Qt::lightGray),
      segmentColor_(QColor(0, 90, 160)),
      showSegments_(false),
      showSpeed_(false),
      animated_(false),
      bytesPerSecond_(0),
      animationOffset_(0) {
    
    // Set up animation timer
    animationTimer_ = new QTimer(this);
    connect(animationTimer_, &QTimer::timeout, this, &ProgressBar::onAnimationTimer);
    
    // Set default appearance
    setTextVisible(true);
    setFixedHeight(20);
}

ProgressBar::~ProgressBar() {
    // Stop animation timer
    if (animationTimer_) {
        animationTimer_->stop();
    }
}

QColor ProgressBar::barColor() const {
    return barColor_;
}

void ProgressBar::setBarColor(const QColor& color) {
    barColor_ = color;
    update();
}

QColor ProgressBar::textColor() const {
    return textColor_;
}

void ProgressBar::setTextColor(const QColor& color) {
    textColor_ = color;
    update();
}

QColor ProgressBar::borderColor() const {
    return borderColor_;
}

void ProgressBar::setBorderColor(const QColor& color) {
    borderColor_ = color;
    update();
}

QColor ProgressBar::segmentColor() const {
    return segmentColor_;
}

void ProgressBar::setSegmentColor(const QColor& color) {
    segmentColor_ = color;
    update();
}

bool ProgressBar::showSegments() const {
    return showSegments_;
}

void ProgressBar::setShowSegments(bool show) {
    showSegments_ = show;
    update();
}

bool ProgressBar::showSpeed() const {
    return showSpeed_;
}

void ProgressBar::setShowSpeed(bool show) {
    showSpeed_ = show;
    update();
}

bool ProgressBar::isAnimated() const {
    return animated_;
}

void ProgressBar::setAnimated(bool animated) {
    animated_ = animated;
    
    if (animated_) {
        animationTimer_->start(50);  // 20 fps
    } else {
        animationTimer_->stop();
    }
    
    update();
}

void ProgressBar::setSegments(const std::vector<int>& segments) {
    segments_ = segments;
    update();
}

void ProgressBar::setSpeed(double bytesPerSecond) {
    bytesPerSecond_ = bytesPerSecond;
    update();
}

void ProgressBar::setFormat(const QString& format) {
    format_ = format;
    QProgressBar::setFormat(format);
    update();
}

QString ProgressBar::text() const {
    QString result = QProgressBar::text();
    
    // Add speed if enabled
    if (showSpeed_ && bytesPerSecond_ > 0) {
        result += " - " + formatSpeed(bytesPerSecond_);
    }
    
    return result;
}

void ProgressBar::paintEvent(QPaintEvent* event) {
    // Create painter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Get rect
    QRect rect = this->rect();
    
    // Draw border
    painter.setPen(borderColor_);
    painter.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 3, 3);
    
    // Calculate progress width
    double progress = static_cast<double>(value() - minimum()) / (maximum() - minimum());
    int progressWidth = static_cast<int>(rect.width() * progress);
    
    // Create progress rect
    QRect progressRect = rect.adjusted(0, 0, -rect.width() + progressWidth, 0);
    
    // Draw progress bar
    if (progressWidth > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(barColor_);
        
        // Draw with animation if enabled
        if (animated_ && progress < 1.0) {
            // Create gradient
            QLinearGradient gradient(rect.topLeft(), rect.topRight());
            gradient.setColorAt(0, barColor_);
            gradient.setColorAt(0.5, barColor_.lighter(120));
            gradient.setColorAt(1, barColor_);
            
            // Apply animation offset
            qreal gradPos = static_cast<qreal>(animationOffset_) / 100.0;
            gradient.setStart(rect.width() * gradPos, 0);
            gradient.setFinalStop(rect.width() * (gradPos + 1.0), 0);
            
            painter.setBrush(gradient);
        }
        
        painter.drawRoundedRect(progressRect, 3, 3);
    }
    
    // Draw segments if enabled
    if (showSegments_ && !segments_.empty()) {
        painter.setPen(segmentColor_);
        
        for (int segmentPos : segments_) {
            if (segmentPos > 0 && segmentPos < 100) {
                int x = rect.x() + (rect.width() * segmentPos / 100);
                painter.drawLine(x, rect.y(), x, rect.y() + rect.height());
            }
        }
    }
    
    // Draw text
    if (isTextVisible() && !text().isEmpty()) {
        painter.setPen(textColor_);
        painter.drawText(rect, Qt::AlignCenter, text());
    }
}

QSize ProgressBar::sizeHint() const {
    return QSize(200, 20);
}

QSize ProgressBar::minimumSizeHint() const {
    return QSize(100, 20);
}

void ProgressBar::onAnimationTimer() {
    // Update animation offset
    animationOffset_ = (animationOffset_ + 1) % 100;
    
    // Redraw only if visible and value is changing
    if (isVisible() && value() < maximum()) {
        update();
    }
}

QString ProgressBar::formatSpeed(double bytesPerSecond) const {
    return QString::fromStdString(dm::utils::FileUtils::formatFileSize(bytesPerSecond)) + "/s";
}

} // namespace ui
} // namespace dm