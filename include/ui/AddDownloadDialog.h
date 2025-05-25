#ifndef ADD_DOWNLOAD_DIALOG_H
#define ADD_DOWNLOAD_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QSpinBox>
#include <QDateTimeEdit>

namespace dm {
namespace ui {

/**
 * @brief Dialog for adding a new download
 */
class AddDownloadDialog : public QDialog {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a new AddDownloadDialog
     * 
     * @param parent The parent widget
     */
    explicit AddDownloadDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Destroy the AddDownloadDialog
     */
    ~AddDownloadDialog();
    
    /**
     * @brief Get the URL
     * 
     * @return QString The URL
     */
    QString getUrl() const;
    
    /**
     * @brief Get the destination folder
     * 
     * @return QString The destination folder
     */
    QString getDestination() const;
    
    /**
     * @brief Get the filename
     * 
     * @return QString The filename
     */
    QString getFilename() const;
    
    /**
     * @brief Get whether to start the download immediately
     * 
     * @return bool True if the download should start immediately
     */
    bool getStartImmediately() const;
    
    /**
     * @brief Set the URL
     * 
     * @param url The URL
     */
    void setUrl(const QString& url);
    
    /**
     * @brief Set the destination folder
     * 
     * @param destination The destination folder
     */
    void setDestination(const QString& destination);
    
    /**
     * @brief Set the filename
     * 
     * @param filename The filename
     */
    void setFilename(const QString& filename);
    
    /**
     * @brief Set whether to start the download immediately
     * 
     * @param start True if the download should start immediately
     */
    void setStartImmediately(bool start);
    
private slots:
    /**
     * @brief URL changed handler
     * 
     * @param url The new URL
     */
    void onUrlChanged(const QString& url);
    
    /**
     * @brief Browse for destination folder
     */
    void browseDestination();
    
    /**
     * @brief Validate inputs
     */
    void validateInputs();
    
private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Extract filename from URL
     * 
     * @param url The URL
     * @return QString The extracted filename
     */
    QString extractFilenameFromUrl(const QString& url);
    
    // UI elements (match implementation)
    QLineEdit* m_urlEdit;
    QLineEdit* m_filenameEdit;
    QLineEdit* m_savePathEdit;
    QLabel* m_fileSizeLabel;
    QLabel* m_fileTypeLabel;
    QLabel* m_resumableLabel;
    QSpinBox* m_segmentsSpinBox;
    QCheckBox* m_limitSpeedCheckBox;
    QSpinBox* m_speedLimitSpinBox;
    QCheckBox* m_authRequiredCheckBox;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QCheckBox* m_scheduleCheckBox;
    QDateTimeEdit* m_scheduleDateTime;
    QPushButton* m_startButton;
    QPushButton* m_cancelButton_;
    QLabel* m_statusLabel_;
};

} // namespace ui
} // namespace dm

#endif // ADD_DOWNLOAD_DIALOG_H