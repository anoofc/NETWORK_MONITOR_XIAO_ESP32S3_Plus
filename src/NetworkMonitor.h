#pragma once
#include "Model.h"
#include <WiFi.h>
#include <ping/ping_sock.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
class NetworkMonitor
{
    AppState &s;
    ConnectionPolicy policy;
    uint32_t lastSignal = 0, lastProbe = 0, generation = 0;
    bool attempted = false;
    esp_ping_handle_t session = nullptr;
    QueueHandle_t results = nullptr;
    bool active = false;
    uint32_t sessionGeneration = 0;
    struct Result
    {
        bool ok;
        uint32_t rtt, generation;
    };
    Result pending{};
    static void success(esp_ping_handle_t, void *);
    static void timeout(esp_ping_handle_t, void *);
    static void ended(esp_ping_handle_t, void *);
    void invalidate();
    void startProbe(uint32_t);
    void attempt(uint32_t);

public:
    explicit NetworkMonitor(AppState &state) : s(state) {}
    void begin();
    void tick(uint32_t);
    void reconnect(uint32_t);
};
