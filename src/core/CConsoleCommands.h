#ifndef CCONSOLECOMMANDS_H
#define CCONSOLECOMMANDS_H

#include "CConsole.h"

class CConsoleCommands {
public:
    static void registerBotCommands(CConsole* console);
    static void registerSystemCommands(CConsole* console);
    static void registerLLMCommands(CConsole* console);
};

#endif // CCONSOLECOMMANDS_H