#include "SerialCommands.h"
#include <stdlib.h>
void SerialCommands::command(AppState &s, NetworkMonitor &net, DisplayUI &ui, uint32_t now)
{
    if (!strcmp(buffer, "help"))
        Serial.println("help | status | page 1..4 | auto on/off | contrast 0..255 | wake | reconnect | capture");
    else if (!strcmp(buffer, "status"))
    {
        char uptime[16];
        durationText(uptime, sizeof uptime, s.connected() ? now - s.connectedAt : 0);
        Serial.printf("state=%u SSID=%s IP=%s gateway=%s channel=%d\nRSSI raw=%d dBm valid=%d uptime=%s reconnects=%lu\n", unsigned(s.link), s.ssid, s.ip, s.gateway, s.channel, s.rssi, s.signalValid, uptime, (unsigned long)s.reconnects);
        if (s.ping.samples.count)
            Serial.printf("ICMP probes=%u loss=%u%% avg=%lums last=%s age=%lums\n", unsigned(s.ping.samples.count), s.ping.loss(), (unsigned long)s.ping.average(), s.ping.samples.newest().ok ? "reply" : "timeout", (unsigned long)(now - s.ping.samples.newest().at));
        else
            Serial.println("ICMP: no completed probes");
    }
    else if (!strncmp(buffer, "page ", 5) && strlen(buffer) == 6 && buffer[5] >= '1' && buffer[5] <= '4')
        ui.setPage(buffer[5] - '1', now);
    else if (!strcmp(buffer, "auto on"))
        ui.setAuto(true);
    else if (!strcmp(buffer, "auto off"))
        ui.setAuto(false);
    else if (!strncmp(buffer, "contrast ", 9))
    {
        char *end = nullptr;
        long v = strtol(buffer + 9, &end, 10);
        if (buffer[9] && *end == 0 && v >= 0 && v <= 255)
            ui.setContrast(v, now);
        else
            Serial.println("Use contrast 0..255");
    }
    else if (!strcmp(buffer, "wake"))
        ui.wake(now);
    else if (!strcmp(buffer, "reconnect"))
        net.reconnect(now);
    else if (!strcmp(buffer, "capture"))
        ui.capture();
    else if (*buffer)
        Serial.println("Unknown command. Type help.");
}
void SerialCommands::tick(AppState &s, NetworkMonitor &n, DisplayUI &ui, uint32_t now)
{
    for (unsigned budget = 0; budget < 64 && Serial.available(); budget++)
    {
        char c = Serial.read();
        ui.wake(now);
        if (c == '\r' || c == '\n')
        {
            if (overflow)
                Serial.println("Command too long (79 max)");
            else
            {
                buffer[used] = 0;
                command(s, n, ui, now);
            }
            used = 0;
            overflow = false;
        }
        else if (c == 8 || c == 127)
        {
            if (used)
                --used;
        }
        else if (c >= 32 && c <= 126)
        {
            if (used < sizeof(buffer) - 1 && !overflow)
                buffer[used++] = c;
            else
                overflow = true;
        }
    }
}
