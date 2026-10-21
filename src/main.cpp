#include <Arduino.h>
#include "NetworkMonitor.h"
#include "DisplayUI.h"
#include "SerialCommands.h"
#include "Demo.h"
AppState state;
NetworkMonitor network(state);
DisplayUI display;
SerialCommands commands;
void setup()
{
    Serial.begin(115200);
    Serial.println("NET / MONITOR - local Wi-Fi and gateway ICMP; type help");
    display.begin();
    if (!NET_DEMO)
        network.begin();
}
void loop()
{
    uint32_t now = millis();
    if (NET_DEMO)
        state = demoState((now / 24000) % 11, now);
    else
        network.tick(now);
    commands.tick(state, network, display, now);
    display.tick(state, now);
    yield();
}
