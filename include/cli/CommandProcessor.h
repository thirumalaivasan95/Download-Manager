#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

namespace dm {
namespace cli {

// Forward declarations
class CommandLineInterface;
namespace core {
    class DownloadManager;
}

/**
 * @brief Command result structure
 */
struct CommandResult {
    bool success = false;             // Whether the command succeeded
    std::string message;              // Result message
    std::vector<std::string> data;    // Additional data
};

/**
 * @brief Command handler function type
 * 
 * @param args Command arguments
 * @return CommandResult Command execution result
 */
using CommandHandler = std::function<CommandResult(const std::vector<std::string>& args)>;

/**
 * @brief Command information structure
 */
struct CommandInfo {
    std::string name;                  // Command name
    std::string description;           // Command description
    std::string usage;                 // Command usage syntax
    std::vector<std::string> examples; // Command usage examples
    CommandHandler handler;            // Command handler function
    bool requiresAuth = false;         // Whether the command requires authentication
    int minArgs = 0;                   // Minimum number of arguments
    int maxArgs = -1;                  // Maximum number of arguments (-1 for unlimited)
};

/**
 * @brief Command processor class
 * 
 * Processes and executes commands for the command-line interface
 */
class CommandProcessor {
public:
    /**
     * @brief Construct a new CommandProcessor
     * 
     * @param cli Reference to the command-line interface
     * @param downloadManager Reference to the download manager
     */
    CommandProcessor(CommandLineInterface& cli, core::DownloadManager& downloadManager);
    
    /**
     * @brief Destroy the CommandProcessor
     */
    ~CommandProcessor();
    
    /**
     * @brief Initialize the command processor
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Process a command line
     * 
     * @param commandLine The command line to process
     * @return CommandResult The command result
     */
    CommandResult processCommand(const std::string& commandLine);
    
    /**
     * @brief Register a command
     * 
     * @param commandInfo The command information
     * @return true if registered successfully, false otherwise
     */
    bool registerCommand(const CommandInfo& commandInfo);
    
    /**
     * @brief Unregister a command
     * 
     * @param commandName The command name
     * @return true if unregistered successfully, false otherwise
     */
    bool unregisterCommand(const std::string& commandName);
    
    /**
     * @brief Get command information
     * 
     * @param commandName The command name
     * @return CommandInfo The command information
     */
    CommandInfo getCommandInfo(const std::string& commandName) const;
    
    /**
     * @brief Get all command names
     * 
     * @return std::vector<std::string> The command names
     */
    std::vector<std::string> getCommandNames() const;
    
    /**
     * @brief Get help text for a command
     * 
     * @param commandName The command name
     * @return std::string The help text
     */
    std::string getCommandHelp(const std::string& commandName) const;
    
    /**
     * @brief Get general help text
     * 
     * @return std::string The general help text
     */
    std::string getGeneralHelp() const;
    
private:
    /**
     * @brief Parse a command line
     * 
     * @param commandLine The command line to parse
     * @param commandName Output parameter for the command name
     * @param args Output parameter for the command arguments
     * @return true if parsed successfully, false otherwise
     */
    bool parseCommandLine(const std::string& commandLine, 
                         std::string& commandName, 
                         std::vector<std::string>& args);
    
    /**
     * @brief Validate command arguments
     * 
     * @param commandInfo The command information
     * @param args The command arguments
     * @return bool true if valid, false otherwise
     */
    bool validateCommandArgs(const CommandInfo& commandInfo, 
                            const std::vector<std::string>& args);
    
    /**
     * @brief Execute a command
     * 
     * @param commandInfo The command information
     * @param args The command arguments
     * @return CommandResult The command result
     */
    CommandResult executeCommand(const CommandInfo& commandInfo, 
                               const std::vector<std::string>& args);
    
    /**
     * @brief Register built-in commands
     */
    void registerBuiltInCommands();
    
    /**
     * @brief Format command help text
     * 
     * @param commandInfo The command information
     * @return std::string The formatted help text
     */
    std::string formatCommandHelp(const CommandInfo& commandInfo) const;
    
    // Member variables
    CommandLineInterface& cli_;
    core::DownloadManager& downloadManager_;
    std::map<std::string, CommandInfo> commands_;
    std::map<std::string, std::vector<std::string>> commandAliases_;
};

} // namespace cli
} // namespace dm

#endif // COMMAND_PROCESSOR_H