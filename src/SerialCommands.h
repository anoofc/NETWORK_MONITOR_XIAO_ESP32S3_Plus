#pragma once
#include "NetworkMonitor.h"
#include "DisplayUI.h"
class SerialCommands
{
    char buffer[80]{};
    unsigned used = 0;
    bool overflow = false;
    void command(AppState &, NetworkMonitor &, DisplayUI &, uint32_t);

public:
    void tick(AppState &, NetworkMonitor &, DisplayUI &, uint32_t);
};
