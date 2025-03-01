#include "../../include/utils/DatabaseManager.h"
#include "../../include/utils/Logger.h"
#include "../../include/utils/FileUtils.h"

#include <sqlite3.h>
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace DownloadManager {
namespace Utils {

// Static member initialization
std::shared_ptr<DatabaseManager> DatabaseManager::s_instance = nullptr;
std::mutex DatabaseManager::s_instanceMutex;

DatabaseManager::DatabaseManager() : m_db(nullptr) {
    // Default database file path
    std::string appDataDir = FileUtils::getAppDataDirectory();
    std::string dbPath = appDataDir + "/download_manager.db";
    
    // Ensure directory exists
    std::filesystem::create_directories(std::filesystem::path(appDataDir));
    
    openDatabase(dbPath);
    initializeTables();
}

DatabaseManager::DatabaseManager(const std::string& dbPath) : m_db(nullptr) {
    // Ensure parent directory exists
    std::filesystem::path path(dbPath);
    std::filesystem::create_directories(path.parent_path());
    
    openDatabase(dbPath);
    initializeTables();
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

std::shared_ptr<DatabaseManager> DatabaseManager::instance() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        s_instance = std::shared_ptr<DatabaseManager>(new DatabaseManager());
    }
    return s_instance;
}

void DatabaseManager::openDatabase(const std::string& dbPath) {
    m_dbPath = dbPath;
    
    // Open the database
    int result = sqlite3_open(dbPath.c_str(), &m_db);
    
    if (result != SQLITE_OK) {
        std::string errorMsg = "Failed to open database: ";
        errorMsg += sqlite3_errmsg(m_db);
        Logger::instance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }
    
    Logger::instance().log(LogLevel::INFO, "Database opened successfully: " + dbPath);
    
    // Enable foreign keys
    execute("PRAGMA foreign_keys = ON;");
}

void DatabaseManager::closeDatabase() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
        Logger::instance().log(LogLevel::INFO, "Database closed");
    }
}

void DatabaseManager::initializeTables() {
    try {
        // Create downloads table
        std::string createDownloadsTable = 
            "CREATE TABLE IF NOT EXISTS downloads ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "url TEXT NOT NULL, "
            "destination TEXT NOT NULL, "
            "filename TEXT NOT NULL, "
            "status INTEGER NOT NULL, "
            "total_size INTEGER NOT NULL, "
            "downloaded_size INTEGER NOT NULL, "
            "date_added INTEGER NOT NULL, "
            "date_completed INTEGER, "
            "error_message TEXT, "
            "priority INTEGER NOT NULL DEFAULT 0, "
            "resumable INTEGER NOT NULL DEFAULT 0, "
            "speed_limit INTEGER DEFAULT 0, "
            "category TEXT, "
            "content_type TEXT, "
            "last_modified TEXT);";
        
        execute(createDownloadsTable);
        
        // Create segments table
        std::string createSegmentsTable = 
            "CREATE TABLE IF NOT EXISTS segments ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "download_id INTEGER NOT NULL, "
            "segment_index INTEGER NOT NULL, "
            "start_offset INTEGER NOT NULL, "
            "end_offset INTEGER NOT NULL, "
            "downloaded_size INTEGER NOT NULL, "
            "status INTEGER NOT NULL, "
            "FOREIGN KEY (download_id) REFERENCES downloads(id) ON DELETE CASCADE);";
        
        execute(createSegmentsTable);
        
        // Create settings table
        std::string createSettingsTable = 
            "CREATE TABLE IF NOT EXISTS settings ("
            "key TEXT PRIMARY KEY, "
            "value TEXT NOT NULL);";
        
        execute(createSettingsTable);
        
        // Create categories table
        std::string createCategoriesTable = 
            "CREATE TABLE IF NOT EXISTS categories ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT UNIQUE NOT NULL, "
            "color TEXT, "
            "icon TEXT, "
            "default_dir TEXT);";
        
        execute(createCategoriesTable);
        
        // Create schedules table
        std::string createSchedulesTable = 
            "CREATE TABLE IF NOT EXISTS schedules ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "download_id INTEGER, "
            "type INTEGER NOT NULL, "
            "start_time INTEGER, "
            "end_time INTEGER, "
            "repeat_days INTEGER, "
            "active INTEGER NOT NULL DEFAULT 1, "
            "FOREIGN KEY (download_id) REFERENCES downloads(id) ON DELETE CASCADE);";
        
        execute(createSchedulesTable);
        
        // Create statistics table
        std::string createStatisticsTable = 
            "CREATE TABLE IF NOT EXISTS statistics ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "date INTEGER NOT NULL, "
            "total_downloads INTEGER NOT NULL DEFAULT 0, "
            "successful_downloads INTEGER NOT NULL DEFAULT 0, "
            "failed_downloads INTEGER NOT NULL DEFAULT 0, "
            "total_bytes_downloaded INTEGER NOT NULL DEFAULT 0, "
            "average_speed INTEGER NOT NULL DEFAULT 0);";
        
        execute(createStatisticsTable);
        
        Logger::instance().log(LogLevel::INFO, "Database tables initialized");
    } 
    catch (const std::exception& e) {
        Logger::instance().log(LogLevel::ERROR, "Failed to initialize database tables: " + std::string(e.what()));
        throw;
    }
}

void DatabaseManager::execute(const std::string& sql) {
    char* errorMsg = nullptr;
    
    int result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMsg);
    
    if (result != SQLITE_OK) {
        std::string error = "SQL error: ";
        if (errorMsg) {
            error += errorMsg;
            sqlite3_free(errorMsg);
        }
        
        Logger::instance().log(LogLevel::ERROR, error + " (SQL: " + sql + ")");
        throw std::runtime_error(error);
    }
}

int DatabaseManager::executeInsert(const std::string& sql) {
    execute(sql);
    return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

bool DatabaseManager::executeWithCallback(const std::string& sql, sqlite3_callback callback, void* data) {
    char* errorMsg = nullptr;
    
    int result = sqlite3_exec(m_db, sql.c_str(), callback, data, &errorMsg);
    
    if (result != SQLITE_OK) {
        std::string error = "SQL error: ";
        if (errorMsg) {
            error += errorMsg;
            sqlite3_free(errorMsg);
        }
        
        Logger::instance().log(LogLevel::ERROR, error + " (SQL: " + sql + ")");
        return false;
    }
    
    return true;
}

std::shared_ptr<Statement> DatabaseManager::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::string error = "Failed to prepare statement: ";
        error += sqlite3_errmsg(m_db);
        
        Logger::instance().log(LogLevel::ERROR, error + " (SQL: " + sql + ")");
        throw std::runtime_error(error);
    }
    
    return std::make_shared<Statement>(stmt);
}

bool DatabaseManager::tableExists(const std::string& tableName) {
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    auto stmt = prepare(sql);
    
    stmt->bindText(1, tableName);
    
    if (stmt->step() == SQLITE_ROW) {
        return true;
    }
    
    return false;
}

void DatabaseManager::beginTransaction() {
    execute("BEGIN TRANSACTION;");
}

void DatabaseManager::commitTransaction() {
    execute("COMMIT;");
}

void DatabaseManager::rollbackTransaction() {
    execute("ROLLBACK;");
}

void DatabaseManager::backup(const std::string& backupPath) {
    // Open the backup database
    sqlite3* backupDb = nullptr;
    int result = sqlite3_open(backupPath.c_str(), &backupDb);
    
    if (result != SQLITE_OK) {
        std::string errorMsg = "Failed to open backup database: ";
        errorMsg += sqlite3_errmsg(backupDb);
        sqlite3_close(backupDb);
        Logger::instance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }
    
    // Initialize the backup process
    sqlite3_backup* backup = sqlite3_backup_init(backupDb, "main", m_db, "main");
    
    if (backup) {
        // Perform the backup
        sqlite3_backup_step(backup, -1);
        
        // Check for errors
        int error = sqlite3_backup_finish(backup);
        
        if (error != SQLITE_OK) {
            std::string errorMsg = "Backup failed: ";
            errorMsg += sqlite3_errmsg(backupDb);
            sqlite3_close(backupDb);
            Logger::instance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
        
        Logger::instance().log(LogLevel::INFO, "Database backup completed successfully: " + backupPath);
    } 
    else {
        std::string errorMsg = "Failed to initialize backup: ";
        errorMsg += sqlite3_errmsg(backupDb);
        sqlite3_close(backupDb);
        Logger::instance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }
    
    // Close the backup database
    sqlite3_close(backupDb);
}

void DatabaseManager::restore(const std::string& backupPath) {
    // Check if backup file exists
    if (!std::filesystem::exists(backupPath)) {
        std::string errorMsg = "Backup file does not exist: " + backupPath;
        Logger::instance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }
    
    // Close the current database
    closeDatabase();
    
    // Open the backup database
    sqlite3* backupDb = nullptr;
    int result = sqlite3_open(backupPath.c_str(), &backupDb);
    
    if (result != SQLITE_OK) {
        std::string errorMsg = "Failed to open backup database: ";
        errorMsg += sqlite3_errmsg(backupDb);
        sqlite3_close(backupDb);
        Logger::instance().log(LogLevel::ERROR, errorMsg);
        
        // Reopen the original database
        openDatabase(m_dbPath);
        
        throw std::runtime_error(errorMsg);
    }
    
    // Reopen the main database
    openDatabase(m_dbPath);
    
    // Initialize the backup process (reversed - backup is source, main is destination)
    sqlite3_backup* backup = sqlite3_backup_init(m_db, "main", backupDb, "main");
    
    if (backup) {
        // Perform the restore
        sqlite3_backup_step(backup, -1);
        
        // Check for errors
        int error = sqlite3_backup_finish(backup);
        
        if (error != SQLITE_OK) {
            std::string errorMsg = "Restore failed: ";
            errorMsg += sqlite3_errmsg(m_db);
            sqlite3_close(backupDb);
            Logger::instance().log(LogLevel::ERROR, errorMsg);
            throw std::runtime_error(errorMsg);
        }
        
        Logger::instance().log(LogLevel::INFO, "Database restore completed successfully from: " + backupPath);
    } 
    else {
        std::string errorMsg = "Failed to initialize restore: ";
        errorMsg += sqlite3_errmsg(m_db);
        sqlite3_close(backupDb);
        Logger::instance().log(LogLevel::ERROR, errorMsg);
        throw std::runtime_error(errorMsg);
    }
    
    // Close the backup database
    sqlite3_close(backupDb);
}

int64_t DatabaseManager::getDatabaseSize() {
    try {
        return std::filesystem::file_size(m_dbPath);
    } 
    catch (const std::filesystem::filesystem_error& e) {
        Logger::instance().log(LogLevel::ERROR, "Failed to get database size: " + std::string(e.what()));
        return -1;
    }
}

void DatabaseManager::vacuum() {
    execute("VACUUM;");
    Logger::instance().log(LogLevel::INFO, "Database vacuum completed");
}

void DatabaseManager::setSetting(const std::string& key, const std::string& value) {
    std::string sql = "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    auto stmt = prepare(sql);
    
    stmt->bindText(1, key);
    stmt->bindText(2, value);
    stmt->step();
}

std::string DatabaseManager::getSetting(const std::string& key, const std::string& defaultValue) {
    std::string sql = "SELECT value FROM settings WHERE key = ?;";
    auto stmt = prepare(sql);
    
    stmt->bindText(1, key);
    
    if (stmt->step() == SQLITE_ROW) {
        return stmt->getColumnText(0);
    }
    
    return defaultValue;
}

bool DatabaseManager::deleteSetting(const std::string& key) {
    std::string sql = "DELETE FROM settings WHERE key = ?;";
    auto stmt = prepare(sql);
    
    stmt->bindText(1, key);
    stmt->step();
    
    return sqlite3_changes(m_db) > 0;
}

// Statement class implementation

Statement::Statement(sqlite3_stmt* stmt) : m_stmt(stmt) {
}

Statement::~Statement() {
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
    }
}

void Statement::bindInt(int position, int value) {
    sqlite3_bind_int(m_stmt, position, value);
}

void Statement::bindInt64(int position, int64_t value) {
    sqlite3_bind_int64(m_stmt, position, value);
}

void Statement::bindDouble(int position, double value) {
    sqlite3_bind_double(m_stmt, position, value);
}

void Statement::bindText(int position, const std::string& value) {
    sqlite3_bind_text(m_stmt, position, value.c_str(), -1, SQLITE_TRANSIENT);
}

void Statement::bindNull(int position) {
    sqlite3_bind_null(m_stmt, position);
}

void Statement::bindBlob(int position, const void* data, int size) {
    sqlite3_bind_blob(m_stmt, position, data, size, SQLITE_TRANSIENT);
}

int Statement::step() {
    return sqlite3_step(m_stmt);
}

void Statement::reset() {
    sqlite3_reset(m_stmt);
}

void Statement::clearBindings() {
    sqlite3_clear_bindings(m_stmt);
}

int Statement::getColumnInt(int column) {
    return sqlite3_column_int(m_stmt, column);
}

int64_t Statement::getColumnInt64(int column) {
    return sqlite3_column_int64(m_stmt, column);
}

double Statement::getColumnDouble(int column) {
    return sqlite3_column_double(m_stmt, column);
}

std::string Statement::getColumnText(int column) {
    const unsigned char* text = sqlite3_column_text(m_stmt, column);
    return text ? std::string(reinterpret_cast<const char*>(text)) : std::string();
}

const void* Statement::getColumnBlob(int column) {
    return sqlite3_column_blob(m_stmt, column);
}

int Statement::getColumnBytes(int column) {
    return sqlite3_column_bytes(m_stmt, column);
}

int Statement::getColumnType(int column) {
    return sqlite3_column_type(m_stmt, column);
}

int Statement::getColumnCount() {
    return sqlite3_column_count(m_stmt);
}

std::string Statement::getColumnName(int column) {
    const char* name = sqlite3_column_name(m_stmt, column);
    return name ? std::string(name) : std::string();
}

} // namespace Utils
} // namespace DownloadManager