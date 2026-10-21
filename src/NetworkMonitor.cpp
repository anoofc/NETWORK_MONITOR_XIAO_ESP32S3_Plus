#include "NetworkMonitor.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif
void NetworkMonitor::begin()
{
    asciiSSID(s.ssid, sizeof s.ssid, WIFI_SSID);
    results = xQueueCreate(2, sizeof(Result));
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
}
void NetworkMonitor::success(esp_ping_handle_t h, void *arg)
{
    auto *p = static_cast<NetworkMonitor *>(arg);
    uint32_t rtt = 0;
    esp_ping_get_profile(h, ESP_PING_PROF_TIMEGAP, &rtt, sizeof rtt);
    p->pending = {true, rtt, p->sessionGeneration};
}
void NetworkMonitor::timeout(esp_ping_handle_t, void *arg)
{
    auto *p = static_cast<NetworkMonitor *>(arg);
    p->pending = {false, 0, p->sessionGeneration};
}
void NetworkMonitor::ended(esp_ping_handle_t, void *arg)
{
    auto *p = static_cast<NetworkMonitor *>(arg);
    xQueueSend(p->results, &p->pending, 0);
}
void NetworkMonitor::invalidate()
{
    ++generation;
    s.ping.samples.clear();
    s.missed = 0;
    s.noReply = false;
    s.probeError = false;
}
void NetworkMonitor::attempt(uint32_t now)
{
    policy.start(now, attempted);
    s.link = policy.state;
    if (attempted)
    {
        ++s.reconnects;
        s.event(EventKind::Reconnecting, now);
    }
    attempted = true;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
void NetworkMonitor::reconnect(uint32_t now)
{
    WiFi.disconnect();
    if (s.connected())
        s.event(EventKind::Lost, now);
    invalidate();
    s.signalValid = false;
    policy.fail(now);
    s.link = policy.state;
}
void NetworkMonitor::startProbe(uint32_t now)
{
    lastProbe = now;
    if (!results || strcmp(s.gateway, "0.0.0.0") == 0)
    {
        s.probeError = true;
        return;
    }
    esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
    cfg.count = 1;
    cfg.timeout_ms = Config::PROBE_TIMEOUT_MS;
    cfg.interval_ms = 1;
    if (!ipaddr_aton(s.gateway, &cfg.target_addr))
    {
        s.probeError = true;
        return;
    }
    esp_ping_callbacks_t callbacks{};
    callbacks.cb_args = this;
    callbacks.on_ping_success = success;
    callbacks.on_ping_timeout = timeout;
    callbacks.on_ping_end = ended;
    sessionGeneration = generation;
    pending = {false, 0, generation};
    if (esp_ping_new_session(&cfg, &callbacks, &session) != ESP_OK)
    {
        s.probeError = true;
        session = nullptr;
        return;
    }
    active = true;
    if (esp_ping_start(session) != ESP_OK)
    {
        esp_ping_delete_session(session);
        session = nullptr;
        active = false;
        s.probeError = true;
    }
}
void NetworkMonitor::tick(uint32_t now)
{
    Result r;
    if (results && xQueueReceive(results, &r, 0) == pdTRUE)
    {
        // End callback publishes the final result. The worker owns no application state.
        if (session)
        {
            esp_ping_delete_session(session);
            session = nullptr;
        }
        active = false;
        if (s.connected() && r.generation == generation)
        {
            s.probeError = false;
            s.ping.samples.push({r.ok, r.rtt, now});
            if (r.ok)
            {
                s.missed = 0;
                if (s.noReply)
                    s.event(EventKind::ReplyRestored, now);
                s.noReply = false;
            }
            else
            {
                if (s.missed < 100)
                    ++s.missed;
                if (s.missed >= 3 && !s.noReply)
                {
                    s.noReply = true;
                    s.event(EventKind::ReplyLost, now);
                }
            }
        }
    }
    bool up = WiFi.status() == WL_CONNECTED;
    if (up)
    {
        char ip[16], gw[16];
        snprintf(ip, sizeof ip, "%s", WiFi.localIP().toString().c_str());
        snprintf(gw, sizeof gw, "%s", WiFi.gatewayIP().toString().c_str());
        if (!s.connected())
        {
            policy.connected(now);
            s.link = policy.state;
            s.connectedAt = now;
            
            asciiSSID(s.ssid, sizeof s.ssid, WiFi.SSID().c_str());
            s.event(EventKind::Connected, now);
            invalidate();
            lastProbe = now - Config::PROBE_MS;
        }
        else if (strcmp(ip, s.ip) || strcmp(gw, s.gateway))
        {
            s.event(EventKind::IpChanged, now);
            invalidate();
            lastProbe = now - Config::PROBE_MS;
        }
        snprintf(s.ip, sizeof s.ip, "%s", ip);
        snprintf(s.gateway, sizeof s.gateway, "%s", gw);
        s.channel = WiFi.channel();
    }
    else if (s.connected())
    {
        s.event(EventKind::Lost, now);
        invalidate();
        s.signalValid = false;
        policy.fail(now);
        s.link = policy.state;
    }
    if (!up)
    {
        if (policy.timedOut(now))
        {
            WiFi.disconnect();
            policy.fail(now);
            policy.increaseBackoff();
            s.link = policy.state;
        }
        else if (policy.attemptDue(now))
            attempt(now);
    }
    if (elapsed(now, lastSignal, Config::RSSI_MS))
    {
        uint32_t steps = (now - lastSignal) / Config::RSSI_MS;
        lastSignal = now;
        for (uint32_t i = 1; i < steps && i < Config::HISTORY_COUNT; i++)
            s.history.push({0, false});
        if (s.connected())
        {
            s.rssi = WiFi.RSSI();
            s.filtered = s.signalValid ? s.filtered * 0.7f + s.rssi * 0.3f : s.rssi;
            s.signalValid = true;
            s.history.push({int16_t(s.rssi), true});
        }
        else
            s.history.push({0, false});
    }
    if (s.connected() && !active && elapsed(now, lastProbe, Config::PROBE_MS))
        startProbe(now);
}
