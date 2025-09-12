#ifndef CCONSOLE_H
#define CCONSOLE_H

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

struct ConsoleCommand {
    std::string description;
    std::function<void(const std::vector<std::string>&)> handler;
    std::string usage;
};

class CConsole {
private:
    std::map<std::string, ConsoleCommand> commands;
    std::atomic<bool> running{false};
    std::unique_ptr<std::thread> consoleThread;
    std::mutex outputMutex;
    
    void consoleLoop();
    std::vector<std::string> parseCommand(const std::string& input);
    void printHelp();

public:
    CConsole();
    ~CConsole();
    
    void start();
    void stop();
    void registerCommand(const std::string& name, const ConsoleCommand& command);
    void executeCommand(const std::string& commandLine);
    void print(const std::string& message);
    void println(const std::string& message);
    
    bool isRunning() const { return running.load(); }
};

#endif // CCONSOLE_H