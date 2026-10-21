#pragma once
#include "Model.h"
inline AppState demoState(unsigned scene, uint32_t now = 120000)
{
    AppState s;
    s.link = Link::Connected;
    snprintf(s.ssid, sizeof s.ssid, "Studio / 2.4 GHz");
    snprintf(s.ip, sizeof s.ip, "192.168.100.254");
    snprintf(s.gateway, sizeof s.gateway, "192.168.100.1");
    s.rssi = -48;
    s.filtered = -48;
    s.signalValid = true;
    s.connectedAt = now - 72000;
    s.channel = 6;
    s.reconnects = 2;
    for (unsigned i = 0; i < 60; i++)
        s.history.push({int16_t(-60 + (int(i % 13) - 6)), !(i >= 20 && i < 25)});
    s.event(EventKind::Reconnecting, now - 60000);
    s.event(EventKind::Connected, now - 12000);
    for (unsigned i = 0; i < 12; i++)
        s.ping.samples.push({i % 5 != 0, 12 + i, now - (12 - i) * 2000});
    switch (scene)
    {
    case 1:
        s.rssi = -89;
        s.filtered = -89;
        break;
    case 2:
        s.link = Link::Connecting;
        s.signalValid = false;
        break;
    case 3:
        s.link = Link::Disconnected;
        s.signalValid = false;
        break;
    case 4:
        snprintf(s.ssid, sizeof s.ssid, "A very long studio network SSID!!");
        break;
    case 5:
        snprintf(s.gateway, sizeof s.gateway, "255.255.255.254");
        break;
    case 6:
        s.ping.samples.push({false, 0, now});
        break;
    case 7:
        s.history.clear();
        s.events.clear();
        break;
    case 8:
        s.ping.samples.clear();
        s.ping.samples.push({true, 123, now});
        break;
    case 9:
        s.ping.samples.clear();
        break;
    case 10:
        s.noReply = true;
        s.ping.samples.clear();
        for (int i = 0; i < 30; i++)
            s.ping.samples.push({false, 0, now});
        break;
    }
    if (!s.connected()) {s.ping.samples.clear(); s.channel=0;}
    return s;
}
