#ifndef COMMAND_LINE_INTERFACE_H
#define COMMAND_LINE_INTERFACE_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace dm {
namespace cli {

// Forward declarations
namespace core {
    class DownloadManager;
    class DownloadTask;
    class BatchDownloader;
    class WebsiteCrawler;
    class Settings;
}

/**
 * @brief Command result structure
 */
struct CommandResult {
    bool success = false;            // Whether the command succeeded
    std::string message;             // Result message
    std::vector<std::string> data;   // Additional data
};

/**
 * @brief Command handler function type
 * 
 * @param args Command arguments
 * @return CommandResult Command execution result
 */
using CommandHandler = std::function<CommandResult(const std::vector<std::string>& args)>;

/**
 * @brief Output handler function type
 * 
 * @param message Message to output
 * @param isError Whether the message is an error
 */
using OutputHandler = std::function<void(const std::string& message, bool isError)>;

/**
 * @brief Command line interface class
 * 
 * Provides a command-line interface for the download manager
 */
class CommandLineInterface {
public:
    /**
     * @brief Construct a new CommandLineInterface
     * 
     * @param downloadManager Reference to download manager
     */
    explicit CommandLineInterface(core::DownloadManager& downloadManager);
    
    /**
     * @brief Destroy the CommandLineInterface
     */
    ~CommandLineInterface();
    
    /**
     * @brief Initialize the CLI
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Start the CLI
     * 
     * @param interactive Whether to run in interactive mode
     * @return true if successful, false otherwise
     */
    bool start(bool interactive = true);
    
    /**
     * @brief Stop the CLI
     */
    void stop();
    
    /**
     * @brief Execute a command
     * 
     * @param command The command to execute
     * @return CommandResult The command result
     */
    CommandResult executeCommand(const std::string& command);
    
    /**
     * @brief Execute a script file
     * 
     * @param filePath Path to the script file
     * @return true if successful, false otherwise
     */
    bool executeScript(const std::string& filePath);
    
    /**
     * @brief Set the output handler
     * 
     * @param handler The output handler function
     */
    void setOutputHandler(OutputHandler handler);
    
    /**
     * @brief Register a custom command
     * 
     * @param command The command name
     * @param handler The command handler function
     * @param help Help text for the command
     * @param usage Usage syntax for the command
     * @return true if registered successfully, false otherwise
     */
    bool registerCommand(const std::string& command, 
                        CommandHandler handler,
                        const std::string& help,
                        const std::string& usage);
    
    /**
     * @brief Get help text for a command
     * 
     * @param command The command name
     * @return std::string The help text
     */
    std::string getCommandHelp(const std::string& command) const;
    
    /**
     * @brief Get all available commands
     * 
     * @return std::vector<std::string> The available commands
     */
    std::vector<std::string> getAvailableCommands() const;
    
    /**
     * @brief Process command-line arguments
     * 
     * @param argc Argument count
     * @param argv Argument values
     * @return int Exit code
     */
    int processArguments(int argc, char* argv[]);
    
private:
    /**
     * @brief Parse a command line into command and arguments
     * 
     * @param commandLine The command line to parse
     * @param command Output parameter for the command
     * @param args Output parameter for the arguments
     * @return true if parsed successfully, false otherwise
     */
    bool parseCommandLine(const std::string& commandLine, 
                         std::string& command, 
                         std::vector<std::string>& args);
    
    /**
     * @brief Interactive mode main loop
     */
    void interactiveLoop();
    
    /**
     * @brief Output a message
     * 
     * @param message The message to output
     * @param isError Whether the message is an error
     */
    void output(const std::string& message, bool isError = false);
    
    /**
     * @brief Initialize built-in commands
     */
    void initializeCommands();
    
    /**
     * @brief Help command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandHelp(const std::vector<std::string>& args);
    
    /**
     * @brief Exit command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandExit(const std::vector<std::string>& args);
    
    /**
     * @brief Add download command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandAdd(const std::vector<std::string>& args);
    
    /**
     * @brief List downloads command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandList(const std::vector<std::string>& args);
    
    /**
     * @brief Start download command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandStart(const std::vector<std::string>& args);
    
    /**
     * @brief Pause download command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandPause(const std::vector<std::string>& args);
    
    /**
     * @brief Resume download command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandResume(const std::vector<std::string>& args);
    
    /**
     * @brief Cancel download command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandCancel(const std::vector<std::string>& args);
    
    /**
     * @brief Remove download command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandRemove(const std::vector<std::string>& args);
    
    /**
     * @brief Info command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandInfo(const std::vector<std::string>& args);
    
    /**
     * @brief Settings command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandSettings(const std::vector<std::string>& args);
    
    /**
     * @brief Batch command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandBatch(const std::vector<std::string>& args);
    
    /**
     * @brief Crawl command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandCrawl(const std::vector<std::string>& args);
    
    /**
     * @brief Schedule command handler
     * 
     * @param args Command arguments
     * @return CommandResult Command execution result
     */
    CommandResult commandSchedule(const std::vector<std::string>& args);
    
    // Member variables
    core::DownloadManager& downloadManager_;
    std::shared_ptr<core::BatchDownloader> batchDownloader_;
    std::shared_ptr<core::WebsiteCrawler> websiteCrawler_;
    std::shared_ptr<core::Settings> settings_;
    
    std::map<std::string, CommandHandler> commandHandlers_;
    std::map<std::string, std::string> commandHelp_;
    std::map<std::string, std::string> commandUsage_;
    
    OutputHandler outputHandler_ = nullptr;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> interactiveThread_;
    std::mutex mutex_;
};

} // namespace cli
} // namespace dm

#endif // COMMAND_LINE_INTERFACE_H