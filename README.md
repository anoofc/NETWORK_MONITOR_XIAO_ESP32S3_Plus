# NET / MONITOR

A four-page, 128×64 Wi-Fi instrument for the **standard Seeed Studio XIAO ESP32S3**. It measures the local Wi-Fi link and gateway ICMP replies. Neither association nor a gateway reply establishes internet access.

## Hardware and assumptions

| OLED pin | XIAO connection |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | D4 / GPIO5 |
| SCL | D5 / GPIO6 |

Use a four-pin I²C module. **SSD1306 is the initial assumption**; 128×64 resolution does not identify its controller. SH1106 is selectable in `include/Config.h` (`NET_DISPLAY_SH1106=1`), or with the provided build environment. Do not select a Plus board merely because the project directory contains “Plus”.

The pin mapping is verified against [Seeed's official pin table](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/). Connect the supplied Wi-Fi antenna; use a 2.4 GHz network.

## Reproducible toolchain

- PlatformIO Core 6.2.0 used for verification.
- `espressif32@6.12.0` platform.
- `seeed_xiao_esp32s3` board, 8 MB flash, Arduino framework.
- Framework package `3.20017.241212+sha.dcc1105b`: Arduino ESP32 **2.0.17**, ESP-IDF **4.4.7**.
- `olikraus/U8g2@2.36.12`, full 1024-byte framebuffer.

The pinned [PlatformIO board definition](https://github.com/platformio/platform-espressif32/blob/v6.12.0/boards/seeed_xiao_esp32s3.json) supplies `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1` (native USB hardware CDC), PSRAM, and core selection. `Serial` is the USB connection, at monitor setting 115200; there is no wait for a terminal in setup. The bundled core includes `ping/ping_sock.h` and the [ESP-IDF 4.4.7 asynchronous ping API](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/protocols/icmp_echo.html). Builds link that API for this board; this is not an assumption about Arduino 3.x support.

## Setup, build, flash

1. Open this folder in VS Code with the PlatformIO extension.
2. Copy `include/secrets.example.h` to `include/secrets.h`; enter the real SSID and password. The real file is ignored by Git. Missing credentials intentionally compile with the template values; they will not join your network.
3. Check `include/Config.h`: controller, seven-bit address, contrast, bus speed, timers.
4. Build and upload:

```sh
pio run -e xiao_ssd1306
pio run -e xiao_ssd1306 -t upload
pio device monitor -b 115200
```

For SH1106 use `-e xiao_sh1106`. To compile every configuration:

```sh
pio run -e xiao_ssd1306 -e xiao_sh1106 -e demo
```

The `demo` environment does not associate with Wi-Fi. It cycles 11 deterministic scenes, each held 24 seconds, through the same four-page renderer. It still drives a connected OLED. Upload it with `pio run -e demo -t upload`.

If `pio` is not on PATH on this Mac, use `~/.platformio/penv/bin/pio`. Select the USB port if PlatformIO cannot choose it (`--upload-port ...`). For initial upload recovery, hold BOOT while connecting USB, release it, upload, then reset. Reopen the monitor if the native USB port re-enumerates.

## Display design and readings

Four pages rotate every six seconds:

1. **NETWORK:** dedicated SSID viewport, large raw RSSI in dBm, four bars using an exponential filter (30% new sample), approximate quality, connection uptime. Quality is `clamp(2 × (raw dBm + 100), 0, 100)`; it is not throughput. Long SSIDs pause for 1.8 seconds, then move one pixel per 50 ms inside their own viewport. Unsupported display characters become `?`; credentials stay unchanged.
2. **ADDRESS:** full local and gateway IPv4 addresses, each with its own label and readable row. Footer shows channel.
3. **GATEWAY:** latest completed RTT, successful-reply average, completed-probe loss and `N` sample count, age of latest result. `WAIT`, `TIMEOUT`, `NO REPLY`, and `UNAVAILABLE` are distinct. Three consecutive timeouts produce `NO REPLY`; the first reply restores reachability. Routers can block ICMP. `PROBE ERROR` indicates session setup failure, not packet loss.
4. **HISTORY:** 60 raw RSSI samples at one-second intervals, fixed −100 to −30 dBm range, gaps for missing/disconnected samples. The latest two events appear first; older pairs cycle every four seconds. Sixteen bounded RAM events cover association, loss, retry, address/gateway changes, and gateway reply loss/restoration. Footer shows retry count.

Headings use `helvB08_tr`, large numbers `helvB18_tn`, and readable supporting text `6x10_tr`. No page uses more than three fonts. Actual U8g2 widths determine badges, units, right-aligned columns, and marquee travel. Measured ascent/descent are 19/−5, 8/−2, and 7/−2 respectively; header/footer baselines use font metrics. Numeric fonts only draw numeric content. The renderer reserves columns 4–122 so optional +1 horizontal pixel shift retains at least four pixels at both sides. The shift is disabled by default.

A one-second boot mark and a 200 ms content reveal use scheduled frames; there is no animation `delay()`. Animated frames target 20 FPS; ordinary frames update at 4 Hz, with one framebuffer transfer per frame. Actual bus transfer time can limit the rate, especially at 100 kHz. Contrast defaults to 180; after 120 seconds without serial input the screen dims to 24. Page changes and network traffic do not reset idle dimming. Adjust or disable it in Config. Dimming and optional pixel shift do not eliminate OLED burn-in.

## Network behavior and ownership

Association attempts time out after 15 seconds. Retry waits increase from 2 to 4, 8, 16, and 30 seconds after failed attempts, capped at 30 seconds. Loss after a working connection initially retries after one second. Arduino automatic reconnect is disabled, leaving one owner of association attempts. Rendering continues throughout. `reconnects` counts association attempts after the initial attempt, including failures.

The application polls Wi-Fi state on its main loop, so no Wi-Fi callback touches display/application buffers. Every two seconds, at most one IDF ping session probes the currently assigned gateway, with a 900 ms timeout. Ping callbacks only collect a small result and publish it through a FreeRTOS queue at session end. The main loop consumes results, deletes completed sessions, and updates state. Generation tags reject results from an earlier address/gateway/connection. In-flight probes finish naturally after disconnect; they are discarded and do not count as loss. The 30-completed-probe rolling window clears on network changes. Average excludes timeouts; loss excludes skipped probes. Session allocation errors are not ICMP failures.

All histories and command buffers are bounded. Rendering uses fixed buffers and no repeated heap allocation; IDF allocates a short-lived session/task per probe. Timing comparisons use unsigned subtraction for `millis()` wraparound. The OLED framebuffer has one owner, the main loop.

## Serial commands

Send a newline after each command. Input is bounded to 79 printable characters; overlong lines are rejected as a whole. Commands are case-sensitive.

| Command | Action |
|---|---|
| `help` | List commands |
| `status` | Wi-Fi state, raw RSSI, addresses, uptime, reconnects, ICMP stats and age |
| `page 1` … `page 4` | Select page, restart rotation timer |
| `auto on` / `auto off` | Enable / stop automatic rotation |
| `contrast 0` … `contrast 255` | Set runtime contrast and wake |
| `wake` | Restore configured runtime contrast |
| `reconnect` | Disconnect, invalidate probes, schedule new association |
| `capture` | Snapshot the actual 1024-byte framebuffer and stream PBM text |

All input wakes the screen. Settings are RAM-only. Use `auto off` before selecting a fixed page. `capture` streams from an independent snapshot in small chunks as USB space permits, allowing rendering/network work to continue. Serial diagnostics never print passwords. SSIDs and local addresses do appear in status/captures.

## Actual framebuffer previews

`previews/contact-sheet-1x.png` and `previews/contact-sheet-4x.png` show all four pages in columns. Rows are: strong, weak, connecting, disconnected, long SSID, full IPv4 address, timeout, empty history, partial ping window (123 ms), no samples, all-timeout/no-reply. Individual native 128×64 PNGs and PBMs are included.

These were executed from **the firmware's `OledRenderer.cpp`, the exact pinned U8g2 C library, and its bitmap fonts** on the host, not redrawn in a browser. The host swaps only the hardware transport for a no-op callback. It exercises the full framebuffer. Connection scenes are deterministic fixtures, not physical measurements.

Regenerate after installing the dependency with the firmware build:

```sh
python3 tools/preview.py
./tools/check.sh
```

Requires a C/C++ compiler and Python 3; no image packages are needed. Preview compilation outputs measured font metrics. PNG generation uses only Python's standard library and integer nearest-neighbor enlargement. PBM exports use `1` for a lit OLED pixel; the included converter maps it to white.

For device capture, log the USB terminal, issue `capture`, then run:

```sh
python3 tools/contact_sheet.py serial.log
```

This extracts `PBM_BEGIN`/`PBM_END` frames into `previews/capture-N.pbm` and converts them into native PNGs. Capture after the 200 ms transition completes. Contact sheets use the deterministic four-page fixtures; individual device captures remain separate PNGs. Capturing also works without an attached OLED because the framebuffer is rendered in RAM.

## OLED troubleshooting

- Check 3.3 V supply, common ground, GPIO5 SDA and GPIO6 SCL.
- Default address is **0x3C**; set `OLED_ADDRESS=0x3D` for the other common module address. This is a seven-bit value; code passes `address << 1` to U8g2, which expects an eight-bit representation.
- The startup ACK check cannot identify SSD1306 versus SH1106. An ACK with shifted/corrupt/blank content calls for verifying the controller, not changing resolution. Select the SH1106 build or Config macro.
- Reduce `I2C_HZ` to 100000 for bus/module compatibility. Short wires help.
- With no ACK, the firmware skips physical display transfers and keeps serial/network diagnostics running. Attach the display and reset to detect it again. A module removed after startup is not hot-plug detected; Wire timeout bounds failed transfers.
- If the display is dim, type `wake` or adjust contrast. After flashing wrong credentials, repair `secrets.h` and rebuild; reconnect alone cannot change credentials.

## Verification and hardware acceptance

Host model checks cover mixed successes/timeouts, empty and all-timeout statistics, rolling-window eviction, event-buffer wraparound, connection timeout/retry/recovery transitions and backoff cap, ASCII fallback, and scheduling across `UINT32_MAX`. All three pinned firmware environments have been compiled. Generated previews were inspected and the gateway supporting rows were separated to remove an overlap.

**No hardware test or flashing was performed.** Before deployment, check:

| Test | Expected result |
|---|---|
| Initial correct credentials | JOIN → UP, DHCP addresses, raw RSSI, history; gateway either RTT or clearly labeled ICMP non-response |
| Shut router down, restore it | WIFI LOST, capped retries, responsive pages, eventual reconnect, new uptime; stale probes discarded |
| Wrong password | Attempts stop at 15 s; capped backoff, no reconnect storm; readable JOIN/DOWN |
| 32-byte/Unicode SSID | Actual association unchanged; display-only fallback and isolated marquee; no clipping outside viewport |
| Gateway blocks ICMP | TIMEOUT then NO REPLY after three; Wi-Fi stays UP; loss uses only completed probes |
| Restore gateway replies | GW RESTORED event, successful RTT, rolling stats recover |
| DHCP IP or gateway changes | IP CHANGED event, probe window cleared, probe target updates |
| OLED missing | Useful USB status remains available; no physical screen required |
| 0x3C and 0x3D modules | Correct Config address ACKs; incorrect address reports absent |
| Both controllers | Corresponding environment renders full-width content without shifted columns |
| Idle and serial wake | Dim after configured interval, wake restores runtime contrast |
| Serial stress / capture | Invalid/overlong inputs rejected, captures complete, Wi-Fi and UI stay responsive |

Host rendering validates geometry and fonts; real OLED contrast, I²C reliability, USB enumeration, Wi-Fi recovery, ping task behavior under stress, and controller-specific scanout remain physical acceptance checks.
