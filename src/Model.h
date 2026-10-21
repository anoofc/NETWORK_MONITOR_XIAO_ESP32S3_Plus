#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "Config.h"
inline bool elapsed(uint32_t now, uint32_t then, uint32_t interval) { return uint32_t(now - then) >= interval; }
template <class T, size_t N>
struct Ring
{
    T values[N]{};
    size_t count = 0, head = 0;
    void clear() { count = head = 0; }
    void push(T v)
    {
        values[head] = v;
        head = (head + 1) % N;
        if (count < N)
            ++count;
    }
    const T &oldest(size_t i) const { return values[(head + N - count + i) % N]; }
    const T &newest(size_t i = 0) const { return oldest(count - 1 - i); }
};
enum class Link
{
    Starting,
    Connecting,
    Connected,
    Disconnected,
    Reconnecting
};
struct ConnectionPolicy
{
    Link state = Link::Starting;
    uint32_t since = 0, backoff = 1000;
    bool attemptDue(uint32_t now) const { return state == Link::Starting || (state == Link::Disconnected && elapsed(now, since, backoff)); }
    bool timedOut(uint32_t now) const { return (state == Link::Connecting || state == Link::Reconnecting) && elapsed(now, since, Config::CONNECT_TIMEOUT_MS); }
    void start(uint32_t now, bool retry)
    {
        state = retry ? Link::Reconnecting : Link::Connecting;
        since = now;
    }
    void fail(uint32_t now)
    {
        state = Link::Disconnected;
        since = now;
    }
    void increaseBackoff() { backoff = backoff >= Config::BACKOFF_MAX_MS / 2 ? Config::BACKOFF_MAX_MS : backoff * 2; }
    void connected(uint32_t now)
    {
        state = Link::Connected;
        since = now;
        backoff = 1000;
    }
};
struct Probe
{
    bool ok;
    uint32_t rtt, at;
};
struct PingStats
{
    Ring<Probe, Config::PING_WINDOW> samples;
    unsigned successes() const
    {
        unsigned n = 0;
        for (size_t i = 0; i < samples.count; i++)
            n += samples.oldest(i).ok;
        return n;
    }
    unsigned loss() const { return samples.count ? 100 * (samples.count - successes()) / samples.count : 0; }
    uint32_t average() const
    {
        uint64_t sum = 0;
        for (size_t i = 0; i < samples.count; i++)
            if (samples.oldest(i).ok)
                sum += samples.oldest(i).rtt;
        return successes() ? sum / successes() : 0;
    }
};
enum class EventKind
{
    Connected,
    Lost,
    Reconnecting,
    IpChanged,
    ReplyLost,
    ReplyRestored
};
inline const char *eventLabel(EventKind k)
{
    switch (k)
    {
    case EventKind::Connected:
        return "CONNECTED";
    case EventKind::Lost:
        return "WIFI LOST";
    case EventKind::Reconnecting:
        return "RECONNECTING";
    case EventKind::IpChanged:
        return "IP CHANGED";
    case EventKind::ReplyLost:
        return "GW NO REPLY";
    default:
        return "GW RESTORED";
    }
}
struct Event
{
    EventKind kind;
    uint32_t at;
};
struct Signal
{
    int16_t dbm;
    bool valid;
};
struct AppState
{
    Link link = Link::Starting;
    char ssid[100] = "";
    char ip[16] = "--", gateway[16] = "--";
    int rssi = -100, channel = 0;
    float filtered = -100;
    bool signalValid = false;
    uint32_t connectedAt = 0, reconnects = 0;
    unsigned missed = 0;
    bool noReply = false, probeError = false;
    PingStats ping;
    Ring<Event, Config::EVENT_COUNT> events;
    Ring<Signal, Config::HISTORY_COUNT> history;
    bool connected() const { return link == Link::Connected; }
    void event(EventKind k, uint32_t now) { events.push({k, now}); }
};
inline void durationText(char *out, size_t n, uint32_t ms)
{
    uint32_t s = ms / 1000;
    if (s < 60)
        snprintf(out, n, "%lus", (unsigned long)s);
    else if (s < 3600)
        snprintf(out, n, "%lum", (unsigned long)(s / 60));
    else if (s < 86400)
        snprintf(out, n, "%luh", (unsigned long)(s / 3600));
    else
        snprintf(out, n, "%lud", (unsigned long)(s / 86400));
}
// Display-only ASCII fallback. Never use this string for Wi-Fi association.
inline void asciiSSID(char *out, size_t n, const char *in)
{
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < n; i++)
    {
        unsigned char c = in[i];
        if (c >= 32 && c < 127)
            out[j++] = c;
        else if ((c & 0xC0) != 0x80)
            out[j++] = '?';
    }
    out[j] = 0;
}
