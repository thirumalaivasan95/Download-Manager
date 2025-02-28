#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QMessageBox>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSettings>

#include "include/core/DownloadManager.h"
#include "include/ui/MainWindow.h"
#include "include/utils/Logger.h"

/**
 * @brief Initialize the application
 * 
 * @return true if initialization succeeded, false otherwise
 */
bool initializeApplication() {
    // Create necessary directories
    QDir appDataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!appDataDir.exists()) {
        if (!appDataDir.mkpath(".")) {
            QMessageBox::critical(nullptr, "Error", 
                                 "Failed to create application data directory.");
            return false;
        }
    }
    
    // Initialize logger
    try {
        dm::utils::Logger::initialize(appDataDir.filePath("log.txt"));
    } catch (const std::exception& e) {
        QMessageBox::warning(nullptr, "Warning", 
                            QString("Failed to initialize logger: %1").arg(e.what()));
        // Continue anyway, logging is not critical
    }
    
    // Log application start
    dm::utils::Logger::info("Application starting...");
    
    // Initialize download manager
    if (!dm::core::DownloadManager::getInstance().initialize()) {
        QMessageBox::critical(nullptr, "Error", 
                             "Failed to initialize download manager.");
        return false;
    }
    
    return true;
}

/**
 * @brief Cleanup on application exit
 */
void cleanupApplication() {
    // Save settings
    dm::core::DownloadManager::getInstance().saveTasks();
    
    // Shutdown download manager
    dm::core::DownloadManager::getInstance().shutdown();
    
    // Log application exit
    dm::utils::Logger::info("Application exiting...");
    
    // Shutdown logger
    dm::utils::Logger::shutdown();
}

/**
 * @brief Main function
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return int Exit code
 */
int main(int argc, char *argv[]) {
    // Set application attributes
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // Create application
    QApplication app(argc, argv);
    app.setApplicationName("Download Manager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MyOrganization");
    app.setOrganizationDomain("example.com");
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Download Manager");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("url", "URL to download", "url"));
    parser.process(app);
    
    // Create splash screen
    QPixmap splashPixmap(":/images/splash.png");
    if (splashPixmap.isNull()) {
        // Use a default splash if the image is not available
        splashPixmap = QPixmap(400, 300);
        splashPixmap.fill(Qt::white);
    }
    
    QSplashScreen splash(splashPixmap);
    splash.show();
    app.processEvents();
    
    // Load translations
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                     QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);
    
    QTranslator appTranslator;
    appTranslator.load("downloadmanager_" + QLocale::system().name(),
                      ":/translations");
    app.installTranslator(&appTranslator);
    
    // Initialize the application
    splash.showMessage("Initializing application...", Qt::AlignBottom | Qt::AlignHCenter, Qt::black);
    app.processEvents();
    
    if (!initializeApplication()) {
        splash.close();
        return 1;
    }
    
    // Create main window
    splash.showMessage("Creating user interface...", Qt::AlignBottom | Qt::AlignHCenter, Qt::black);
    app.processEvents();
    
    dm::ui::MainWindow mainWindow;
    
    // Clean up on exit
    QObject::connect(&app, &QApplication::aboutToQuit, &cleanupApplication);
    
    // Show the main window
    mainWindow.show();
    splash.finish(&mainWindow);
    
    // Process URL from command line if provided
    if (parser.isSet("url")) {
        QString url = parser.value("url");
        if (!url.isEmpty()) {
            dm::core::DownloadManager::getInstance().addDownload(url.toStdString());
        }
    }
    
    // Run the application
    return app.exec();
}