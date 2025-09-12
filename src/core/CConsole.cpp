#include "CConsole.h"
#include "../CApp.h"
#include "../utils/JsonResponse.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

CConsole::CConsole() {
    registerCommand("help", {
        "Show available commands or help for a specific command",
        [this](const std::vector<std::string>& args) {
            if (args.size() > 1) {
                auto it = commands.find(args[1]);
                if (it != commands.end()) {
                    println("Command: " + args[1]);
                    println("Description: " + it->second.description);
                    println("Usage: " + it->second.usage);
                } else {
                    println("Unknown command: " + args[1]);
                }
            } else {
                printHelp();
            }
        },
        "help [command]"
    });
    
    registerCommand("quit", {
        "Exit the console (does not shutdown the application)",
        [this](const std::vector<std::string>&) {
            println("Exiting console...");
            stop();
        },
        "quit"
    });
    
    registerCommand("clear", {
        "Clear the console screen",
        [this](const std::vector<std::string>&) {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
        },
        "clear"
    });
}

CConsole::~CConsole() {
    stop();
}

void CConsole::start() {
    if (running.load()) {
        return;
    }
    
    running.store(true);
    consoleThread = std::make_unique<std::thread>(&CConsole::consoleLoop, this);
}

void CConsole::stop() {
    if (!running.load()) {
        return;
    }
    
    running.store(false);
    if (consoleThread && consoleThread->joinable()) {
        consoleThread->join();
    }
}

void CConsole::consoleLoop() {
    std::string input;
    
    while (running.load()) {
        
        if (!std::getline(std::cin, input)) {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        executeCommand(input);
    }
}

void CConsole::executeCommand(const std::string& commandLine) {
    auto tokens = parseCommand(commandLine);
    if (tokens.empty()) {
        return;
    }
    
    std::string command = tokens[0];
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);
    
    auto it = commands.find(command);
    if (it != commands.end()) {
        try {
            it->second.handler(tokens);
        } catch (const std::exception& e) {
            println("Error executing command: " + std::string(e.what()));
        }
    } else {
        println("Unknown command: " + command + ". Type 'help' for available commands.");
    }
}

std::vector<std::string> CConsole::parseCommand(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void CConsole::registerCommand(const std::string& name, const ConsoleCommand& command) {
    commands[name] = command;
}

void CConsole::printHelp() {
    println("\nAvailable commands:");
    println("==================");
    
    for (const auto& [name, cmd] : commands) {
        std::ostringstream oss;
        oss << std::left << std::setw(15) << name << " - " << cmd.description;
        println(oss.str());
    }
    println("");
}

void CConsole::print(const std::string& message) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << message << std::flush;
}

void CConsole::println(const std::string& message) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << message << std::endl;
}