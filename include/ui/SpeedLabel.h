#ifndef SPEED_LABEL_H
#define SPEED_LABEL_H

#include <QLabel>
#include <QTimer>

namespace dm {
namespace ui {

/**
 * @brief Label for displaying download speed
 * 
 * Provides automatic formatting and unit selection
 */
class SpeedLabel : public QLabel {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new SpeedLabel
     * 
     * @param parent The parent widget
     */
    explicit SpeedLabel(QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the SpeedLabel
     */
    ~SpeedLabel();
    
    /**
     * @brief Set the speed
     * 
     * @param bytesPerSecond Speed in bytes per second
     */
    void setSpeed(double bytesPerSecond);
    
    /**
     * @brief Get the speed
     * 
     * @return double Speed in bytes per second
     */
    double getSpeed() const;
    
    /**
     * @brief Set whether to show units
     * 
     * @param show True to show units, false to hide
     */
    void setShowUnits(bool show);
    
    /**
     * @brief Check if units are shown
     * 
     * @return true if units are shown, false otherwise
     */
    bool showUnits() const;
    
    /**
     * @brief Set whether to show animation when speed changes
     * 
     * @param animate True to animate, false to disable animation
     */
    void setAnimateChanges(bool animate);
    
    /**
     * @brief Check if animation is enabled
     * 
     * @return true if animation is enabled, false otherwise
     */
    bool animateChanges() const;
    
    /**
     * @brief Set the number of decimal places
     * 
     * @param decimals Number of decimal places
     */
    void setDecimals(int decimals);
    
    /**
     * @brief Get the number of decimal places
     * 
     * @return int Number of decimal places
     */
    int decimals() const;
    
private slots:
    /**
     * @brief Animation timer handler
     */
    void onAnimationTimer();
    
private:
    /**
     * @brief Format speed
     * 
     * @param bytesPerSecond Speed in bytes per second
     * @return QString Formatted speed
     */
    QString formatSpeed(double bytesPerSecond) const;
    
    // Member variables
    double bytesPerSecond_;
    bool showUnits_;
    bool animateChanges_;
    int decimals_;
    
    // Animation
    QTimer* animationTimer_;
    double targetSpeed_;
    double displaySpeed_;
    bool speedChanging_;
};

} // namespace ui
} // namespace dm

#endif // SPEED_LABEL_H