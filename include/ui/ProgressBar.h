#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <QProgressBar>
#include <QColor>
#include <QTimer>
#include <QPaintEvent>
#include <QString>

namespace dm {
namespace ui {

/**
 * @brief Custom progress bar widget
 * 
 * Provides additional features like segment visualization and animations
 */
class ProgressBar : public QProgressBar {
    Q_OBJECT
    
    // Custom properties
    Q_PROPERTY(QColor barColor READ barColor WRITE setBarColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
    Q_PROPERTY(QColor segmentColor READ segmentColor WRITE setSegmentColor)
    Q_PROPERTY(bool showSegments READ showSegments WRITE setShowSegments)
    Q_PROPERTY(bool showSpeed READ showSpeed WRITE setShowSpeed)
    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated)
    
public:
    /**
     * @brief Construct a new ProgressBar
     * 
     * @param parent The parent widget
     */
    explicit ProgressBar(QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the ProgressBar
     */
    ~ProgressBar();
    
    /**
     * @brief Get the bar color
     * 
     * @return QColor The bar color
     */
    QColor barColor() const;
    
    /**
     * @brief Set the bar color
     * 
     * @param color The bar color
     */
    void setBarColor(const QColor& color);
    
    /**
     * @brief Get the text color
     * 
     * @return QColor The text color
     */
    QColor textColor() const;
    
    /**
     * @brief Set the text color
     * 
     * @param color The text color
     */
    void setTextColor(const QColor& color);
    
    /**
     * @brief Get the border color
     * 
     * @return QColor The border color
     */
    QColor borderColor() const;
    
    /**
     * @brief Set the border color
     * 
     * @param color The border color
     */
    void setBorderColor(const QColor& color);
    
    /**
     * @brief Get the segment color
     * 
     * @return QColor The segment color
     */
    QColor segmentColor() const;
    
    /**
     * @brief Set the segment color
     * 
     * @param color The segment color
     */
    void setSegmentColor(const QColor& color);
    
    /**
     * @brief Check if segments are shown
     * 
     * @return true if segments are shown, false otherwise
     */
    bool showSegments() const;
    
    /**
     * @brief Set whether to show segments
     * 
     * @param show True to show segments, false to hide
     */
    void setShowSegments(bool show);
    
    /**
     * @brief Check if speed is shown
     * 
     * @return true if speed is shown, false otherwise
     */
    bool showSpeed() const;
    
    /**
     * @brief Set whether to show speed
     * 
     * @param show True to show speed, false to hide
     */
    void setShowSpeed(bool show);
    
    /**
     * @brief Check if animation is enabled
     * 
     * @return true if animation is enabled, false otherwise
     */
    bool isAnimated() const;
    
    /**
     * @brief Set whether animation is enabled
     * 
     * @param animated True to enable animation, false to disable
     */
    void setAnimated(bool animated);
    
    /**
     * @brief Set the segments
     * 
     * @param segments Vector of segment endpoints (0-100)
     */
    void setSegments(const std::vector<int>& segments);
    
    /**
     * @brief Set the speed
     * 
     * @param bytesPerSecond Speed in bytes per second
     */
    void setSpeed(double bytesPerSecond);
    
    /**
     * @brief Set the format
     * 
     * @param format The format string (same as QProgressBar)
     */
    void setFormat(const QString& format);
    
    /**
     * @brief Get the current text
     * 
     * @return QString The current text
     */
    QString text() const override;
    
protected:
    /**
     * @brief Paint event handler
     * 
     * @param event The paint event
     */
    void paintEvent(QPaintEvent* event) override;
    
    /**
     * @brief Size hint
     * 
     * @return QSize The preferred size
     */
    QSize sizeHint() const override;
    
    /**
     * @brief Minimum size hint
     * 
     * @return QSize The minimum size
     */
    QSize minimumSizeHint() const override;
    
private slots:
    /**
     * @brief Animation timer handler
     */
    void onAnimationTimer();
    
private:
    // Custom properties
    QColor barColor_;
    QColor textColor_;
    QColor borderColor_;
    QColor segmentColor_;
    bool showSegments_;
    bool showSpeed_;
    bool animated_;
    
    // Segments
    std::vector<int> segments_;
    
    // Speed
    double bytesPerSecond_;
    
    // Animation
    QTimer* animationTimer_;
    int animationOffset_;
    
    // Format
    QString format_;
    
    /**
     * @brief Format the speed
     * 
     * @param bytesPerSecond Speed in bytes per second
     * @return QString The formatted speed
     */
    QString formatSpeed(double bytesPerSecond) const;
};

} // namespace ui
} // namespace dm

#endif // PROGRESS_BAR_H