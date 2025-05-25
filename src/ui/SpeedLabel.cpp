#include "ui/SpeedLabel.h"
#include "utils/FileUtils.h"

#include <QFontMetrics>
#include <cmath>

namespace dm {
namespace ui {

SpeedLabel::SpeedLabel(QWidget* parent)
    : QLabel(parent),
      bytesPerSecond_(0),
      showUnits_(true),
      animateChanges_(true),
      decimals_(2),
      targetSpeed_(0),
      displaySpeed_(0),
      speedChanging_(false) {
    
    // Set default text
    setText("0 B/s");
    
    // Set alignment
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    // Create animation timer
    animationTimer_ = new QTimer(this);
    connect(animationTimer_, &QTimer::timeout, this, &SpeedLabel::onAnimationTimer);
}

SpeedLabel::~SpeedLabel() {
    // Stop animation timer
    if (animationTimer_) {
        animationTimer_->stop();
    }
}

void SpeedLabel::setSpeed(double bytesPerSecond) {
    // Update speed
    bytesPerSecond_ = bytesPerSecond;
    
    if (animateChanges_ && bytesPerSecond_ > 0) {
        // Set target speed
        targetSpeed_ = bytesPerSecond_;
        
        // Start animation if not already running
        if (!speedChanging_) {
            displaySpeed_ = bytesPerSecond_;
            speedChanging_ = true;
            animationTimer_->start(50);  // 20 fps
        }
    } else {
        // Update immediately
        setText(formatSpeed(bytesPerSecond_));
    }
}

double SpeedLabel::getSpeed() const {
    return bytesPerSecond_;
}

void SpeedLabel::setShowUnits(bool show) {
    showUnits_ = show;
    setText(formatSpeed(bytesPerSecond_));
}

bool SpeedLabel::showUnits() const {
    return showUnits_;
}

void SpeedLabel::setAnimateChanges(bool animate) {
    animateChanges_ = animate;
    
    if (!animateChanges_ && animationTimer_->isActive()) {
        animationTimer_->stop();
        speedChanging_ = false;
        setText(formatSpeed(bytesPerSecond_));
    }
}

bool SpeedLabel::animateChanges() const {
    return animateChanges_;
}

void SpeedLabel::setDecimals(int decimals) {
    decimals_ = decimals;
    setText(formatSpeed(bytesPerSecond_));
}

int SpeedLabel::decimals() const {
    return decimals_;
}

void SpeedLabel::onAnimationTimer() {
    // Calculate new display speed (smooth transition)
    double diff = targetSpeed_ - displaySpeed_;
    
    if (std::abs(diff) < 1.0) {
        // Close enough, set to target
        displaySpeed_ = targetSpeed_;
        speedChanging_ = false;
        animationTimer_->stop();
    } else {
        // Move 10% closer to target each step
        displaySpeed_ += diff * 0.1;
    }
    
    // Update text
    setText(formatSpeed(displaySpeed_));
}

QString SpeedLabel::formatSpeed(double bytesPerSecond) const {
    // Special case for zero speed
    if (bytesPerSecond <= 0) {
        return showUnits_ ? "0 B/s" : "0";
    }
    
    // Get formatted size
    std::string formattedSize = dm::utils::FileUtils::formatFileSize(bytesPerSecond, decimals_);
    
    // Add units if needed
    if (showUnits_) {
        return QString::fromStdString(formattedSize) + "/s";
    } else {
        // Extract just the number
        size_t spacePos = formattedSize.find(' ');
        if (spacePos != std::string::npos) {
            return QString::fromStdString(formattedSize.substr(0, spacePos));
        } else {
            return QString::fromStdString(formattedSize);
        }
    }
}

} // namespace ui
} // namespace dm