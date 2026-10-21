#pragma once
#include <stdint.h>
#ifndef NET_DISPLAY_SH1106
#define NET_DISPLAY_SH1106 0
#endif
#ifndef NET_DEMO
#define NET_DEMO 0
#endif
namespace Config {
constexpr int SDA_PIN=5, SCL_PIN=6;
constexpr uint8_t OLED_ADDRESS=0x3C; // Seven-bit address; U8g2 receives address << 1.
constexpr uint32_t I2C_HZ=400000; // Set to 100000 for a slower module/bus.
constexpr uint8_t CONTRAST=180, DIM_CONTRAST=24;
constexpr uint32_t DIM_AFTER_MS=120000; // 0 disables; serial activity wakes.
constexpr bool PIXEL_SHIFT=false; // Alternates 0/+1 x; renderer reserves right margin.
constexpr uint32_t PAGE_MS=6000, TRANSITION_MS=200, FRAME_MS=50;
constexpr uint32_t RSSI_MS=1000, PROBE_MS=2000, PROBE_TIMEOUT_MS=900;
constexpr uint32_t CONNECT_TIMEOUT_MS=15000, BACKOFF_MAX_MS=30000;
constexpr uint32_t MARQUEE_PAUSE_MS=1800, EVENT_PAIR_MS=4000;
constexpr unsigned PING_WINDOW=30, EVENT_COUNT=16, HISTORY_COUNT=60;
}
