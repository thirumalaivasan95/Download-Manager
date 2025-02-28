#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <functional>

// Forward declaration for SQLite3 handle
struct sqlite3;
struct sqlite3_stmt;

namespace dm {
namespace utils {

/**
 * @brief Database connection mode enumeration
 */
enum class DbConnectionMode {
    READ_ONLY,           // Read-only access
    READ_WRITE,          // Read-write access
    CREATE               // Create if not exists
};

/**
 * @brief Database query result structure
 */
struct DbQueryResult {
    bool success = false;        // Whether the query succeeded
    int rowsAffected = 0;        // Number of rows affected (for non-SELECT queries)
    int lastInsertId = 0;        // Last insert ID (for INSERT queries)
    std::vector<std::vector<std::string>> rows;  // Result rows (for SELECT queries)
    std::vector<std::string> columns;            // Column names
    std::string errorMessage;    // Error message if query failed
};

/**
 * @brief Database transaction scope class
 * 
 * Provides RAII-style transaction management
 */
class DbTransaction {
public:
    /**
     * @brief Construct a new DbTransaction
     * 
     * @param db Database manager reference
     */
    explicit DbTransaction(class DatabaseManager& db);
    
    /**
     * @brief Destroy the DbTransaction
     * 
     * Automatically rolls back if not committed
     */
    ~DbTransaction();
    
    /**
     * @brief Commit the transaction
     * 
     * @return true if committed successfully, false otherwise
     */
    bool commit();
    
    /**
     * @brief Roll back the transaction
     * 
     * @return true if rolled back successfully, false otherwise
     */
    bool rollback();
    
    /**
     * @brief Check if the transaction is active
     * 
     * @return true if active, false otherwise
     */
    bool isActive() const;
    
private:
    DatabaseManager& db_;
    bool active_ = true;
    bool committed_ = false;
};

/**
 * @brief Database statement class
 * 
 * Represents a prepared SQL statement
 */
class DbStatement {
public:
    /**
     * @brief Construct a new DbStatement
     * 
     * @param db Database handle
     * @param stmt Statement handle
     */
    DbStatement(sqlite3* db, sqlite3_stmt* stmt);
    
    /**
     * @brief Destroy the DbStatement
     */
    ~DbStatement();
    
    /**
     * @brief Bind a null value to a parameter
     * 
     * @param index Parameter index (1-based)
     * @return true if successful, false otherwise
     */
    bool bindNull(int index);
    
    /**
     * @brief Bind an integer value to a parameter
     * 
     * @param index Parameter index (1-based)
     * @param value The integer value
     * @return true if successful, false otherwise
     */
    bool bindInt(int index, int value);
    
    /**
     * @brief Bind a 64-bit integer value to a parameter
     * 
     * @param index Parameter index (1-based)
     * @param value The 64-bit integer value
     * @return true if successful, false otherwise
     */
    bool bindInt64(int index, int64_t value);
    
    /**
     * @brief Bind a double value to a parameter
     * 
     * @param index Parameter index (1-based)
     * @param value The double value
     * @return true if successful, false otherwise
     */
    bool bindDouble(int index, double value);
    
    /**
     * @brief Bind a text value to a parameter
     * 
     * @param index Parameter index (1-based)
     * @param value The text value
     * @return true if successful, false otherwise
     */
    bool bindText(int index, const std::string& value);
    
    /**
     * @brief Bind a blob value to a parameter
     * 
     * @param index Parameter index (1-based)
     * @param data The blob data
     * @param size The blob size
     * @return true if successful, false otherwise
     */
    bool bindBlob(int index, const void* data, int size);
    
    /**
     * @brief Step the statement (execute or fetch next row)
     * 
     * @return int Result code
     */
    int step();
    
    /**
     * @brief Reset the statement
     * 
     * @return true if successful, false otherwise
     */
    bool reset();
    
    /**
     * @brief Clear bindings
     * 
     * @return true if successful, false otherwise
     */
    bool clearBindings();
    
    /**
     * @brief Get column count
     * 
     * @return int The number of columns
     */
    int getColumnCount() const;
    
    /**
     * @brief Get column name
     * 
     * @param index Column index (0-based)
     * @return std::string The column name
     */
    std::string getColumnName(int index) const;
    
    /**
     * @brief Get integer value from column
     * 
     * @param index Column index (0-based)
     * @return int The integer value
     */
    int getInt(int index) const;
    
    /**
     * @brief Get 64-bit integer value from column
     * 
     * @param index Column index (0-based)
     * @return int64_t The 64-bit integer value
     */
    int64_t getInt64(int index) const;
    
    /**
     * @brief Get double value from column
     * 
     * @param index Column index (0-based)
     * @return double The double value
     */
    double getDouble(int index) const;
    
    /**
     * @brief Get text value from column
     * 
     * @param index Column index (0-based)
     * @return std::string The text value
     */
    std::string getText(int index) const;
    
    /**
     * @brief Get blob value from column
     * 
     * @param index Column index (0-based)
     * @param data Output parameter for blob data
     * @param size Output parameter for blob size
     * @return true if successful, false otherwise
     */
    bool getBlob(int index, const void** data, int* size) const;
    
    /**
     * @brief Check if column value is null
     * 
     * @param index Column index (0-based)
     * @return true if null, false otherwise
     */
    bool isNull(int index) const;
    
private:
    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
};

/**
 * @brief Database manager class
 * 
 * Manages database connections and operations
 */
class DatabaseManager {
public:
    /**
     * @brief Construct a new DatabaseManager
     */
    DatabaseManager();
    
    /**
     * @brief Destroy the DatabaseManager
     */
    ~DatabaseManager();
    
    /**
     * @brief Open a database connection
     * 
     * @param dbPath Path to the database file
     * @param mode Connection mode
     * @return true if successful, false otherwise
     */
    bool open(const std::string& dbPath, DbConnectionMode mode = DbConnectionMode::READ_WRITE);
    
    /**
     * @brief Close the database connection
     * 
     * @return true if successful, false otherwise
     */
    bool close();
    
    /**
     * @brief Check if the database is open
     * 
     * @return true if open, false otherwise
     */
    bool isOpen() const;
    
    /**
     * @brief Execute a SQL query
     * 
     * @param sql The SQL query
     * @return DbQueryResult The query result
     */
    DbQueryResult execute(const std::string& sql);
    
    /**
     * @brief Execute a SQL query with parameters
     * 
     * @param sql The SQL query
     * @param params The query parameters
     * @return DbQueryResult The query result
     */
    DbQueryResult execute(const std::string& sql, const std::vector<std::string>& params);
    
    /**
     * @brief Execute a SQL query with parameters
     * 
     * @param sql The SQL query
     * @param params The query parameters
     * @return DbQueryResult The query result
     */
    DbQueryResult execute(const std::string& sql, const std::map<std::string, std::string>& params);
    
    /**
     * @brief Prepare a SQL statement
     * 
     * @param sql The SQL query
     * @return std::shared_ptr<DbStatement> The prepared statement
     */
    std::shared_ptr<DbStatement> prepare(const std::string& sql);
    
    /**
     * @brief Begin a transaction
     * 
     * @return true if successful, false otherwise
     */
    bool beginTransaction();
    
    /**
     * @brief Commit a transaction
     * 
     * @return true if successful, false otherwise
     */
    bool commitTransaction();
    
    /**
     * @brief Roll back a transaction
     * 
     * @return true if successful, false otherwise
     */
    bool rollbackTransaction();
    
    /**
     * @brief Check if a transaction is active
     * 
     * @return true if active, false otherwise
     */
    bool isTransactionActive() const;
    
    /**
     * @brief Create a transaction scope
     * 
     * @return std::unique_ptr<DbTransaction> The transaction scope
     */
    std::unique_ptr<DbTransaction> transaction();
    
    /**
     * @brief Get the last error message
     * 
     * @return std::string The error message
     */
    std::string getLastError() const;
    
    /**
     * @brief Get the database path
     * 
     * @return std::string The database path
     */
    std::string getDatabasePath() const;
    
    /**
     * @brief Check if a table exists
     * 
     * @param tableName The table name
     * @return true if exists, false otherwise
     */
    bool tableExists(const std::string& tableName);
    
    /**
     * @brief Get table columns
     * 
     * @param tableName The table name
     * @return std::vector<std::string> The column names
     */
    std::vector<std::string> getTableColumns(const std::string& tableName);
    
    /**
     * @brief Create a backup of the database
     * 
     * @param backupPath Path to save the backup
     * @return true if successful, false otherwise
     */
    bool createBackup(const std::string& backupPath);
    
    /**
     * @brief Restore from a backup
     * 
     * @param backupPath Path to the backup file
     * @return true if successful, false otherwise
     */
    bool restoreFromBackup(const std::string& backupPath);
    
    /**
     * @brief Vacuum the database
     * 
     * @return true if successful, false otherwise
     */
    bool vacuum();
    
    /**
     * @brief Get the database size
     * 
     * @return int64_t The database size in bytes
     */
    int64_t getDatabaseSize() const;
    
    /**
     * @brief Enable or disable foreign keys
     * 
     * @param enable True to enable, false to disable
     * @return true if successful, false otherwise
     */
    bool enableForeignKeys(bool enable);
    
    /**
     * @brief Set busy timeout
     * 
     * @param milliseconds Timeout in milliseconds
     * @return true if successful, false otherwise
     */
    bool setBusyTimeout(int milliseconds);
    
private:
    /**
     * @brief Bind parameters to a statement
     * 
     * @param stmt The statement
     * @param params The parameters
     * @return true if successful, false otherwise
     */
    bool bindParameters(sqlite3_stmt* stmt, const std::vector<std::string>& params);
    
    /**
     * @brief Bind parameters to a statement
     * 
     * @param stmt The statement
     * @param params The parameters
     * @return true if successful, false otherwise
     */
    bool bindParameters(sqlite3_stmt* stmt, const std::map<std::string, std::string>& params);
    
    /**
     * @brief Fill result from statement
     * 
     * @param stmt The statement
     * @param result Output parameter for result
     * @return true if successful, false otherwise
     */
    bool fillResult(sqlite3_stmt* stmt, DbQueryResult& result);
    
    // Member variables
    sqlite3* db_ = nullptr;
    std::string dbPath_;
    mutable std::mutex mutex_;
    std::atomic<bool> transactionActive_;
};

} // namespace utils
} // namespace dm

#endif // DATABASE_MANAGER_H