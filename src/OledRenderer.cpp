#include "OledRenderer.h"
#include <math.h>
static const uint8_t *const small = u8g2_font_6x10_tr;
static const uint8_t *const heading = u8g2_font_helvB08_tr;
static const uint8_t *const numeric = u8g2_font_helvB18_tn;
void OledRenderer::header(const char *title, const char *badge)
{
    font(heading);
    const int baseline = 1 + u8g2_GetAscent(g);
    text(4, baseline, title);
    if (badge && *badge)
    {
        int w = width(badge) + 8;
        u8g2_DrawBox(g, 122 - w, 0, w, 11);
        u8g2_SetDrawColor(g, 0);
        text(126 - w, baseline, badge);
        u8g2_SetDrawColor(g, 1);
    }
    u8g2_DrawHLine(g, 4, 12, 118);
}
void OledRenderer::metric(const char *value, const char *unit, int baseline)
{
    font(numeric);
    text(4, baseline, value);
    int x = 4 + width(value) + 4;
    font(small);
    if (x + width(unit) <= 122)
        text(x, baseline, unit);
}
void OledRenderer::footer(const AppState &s, unsigned page, uint32_t now)
{
    for (unsigned i = 0; i < 4; i++)
    {
        if (i == page)
            u8g2_DrawBox(g, 4 + i * 7, 59, 4, 3);
        else
            u8g2_DrawPixel(g, 5 + i * 7, 60);
    }
    font(small);
    char b[24];
    if (page == 0)
    {
        char d[12];
        durationText(d, sizeof d, s.connected() ? now - s.connectedAt : 0);
        snprintf(b, sizeof b, "UP %s", d);
    }
    else if (page == 1)
        snprintf(b, sizeof b, "CH %d", s.channel);
    else if (page == 2 && s.ping.samples.count)
    {
        char d[12];
        durationText(d, sizeof d, now - s.ping.samples.newest().at);
        snprintf(b, sizeof b, "AGE %s", d);
    }
    else if (page == 2)
        snprintf(b, sizeof b, "LOCAL ICMP");
    else
        snprintf(b, sizeof b, "REJOIN %lu", (unsigned long)s.reconnects);
    right(122, 64 + u8g2_GetDescent(g), b);
}
void OledRenderer::render(const AppState &s, unsigned page, uint32_t now, uint32_t age)
{
    u8g2_ClearBuffer(g);
    u8g2_SetDrawColor(g, 1);
    u8g2_SetFontMode(g, 1);
    const bool up = s.connected();
    char b[48];
    if (page == 0)
    {
        const char *badge = up ? "UP" : (s.link == Link::Connecting || s.link == Link::Reconnecting || s.link == Link::Starting) ? "JOIN"
                                                                                                                                 : "DOWN";
        header("NETWORK", badge);
        font(small);
        const char *name = *s.ssid ? s.ssid : "No network selected";
        int w = width(name), offset = 0;
        if (w > 118 && age > Config::MARQUEE_PAUSE_MS)
        {
            unsigned travel = w - 118;
            unsigned cycle = travel * 50 + 1600;
            unsigned t = (age - Config::MARQUEE_PAUSE_MS) % cycle;
            offset = t / 50 > travel ? travel : t / 50;
        }
        u8g2_SetClipWindow(g, 4, 14, 122, 25);
        text(4 - offset, 23, name);
        u8g2_SetMaxClipWindow(g);
        if (up && s.signalValid)
        {
            snprintf(b, sizeof b, "%d", s.rssi);
            metric(b, "dBm");
        }
        else
        {
            font(heading);
            text(4, 40, up ? "MEASURING" : badge);
        }
        unsigned bars = up && s.signalValid ? (s.filtered > -55 ? 4 : s.filtered > -67 ? 3
                                                                  : s.filtered > -78   ? 2
                                                                                       : 1)
                                            : 0;
        for (unsigned i = 0; i < 4; i++)
        {
            int h = 4 + i * 4, x = 96 + i * 7;
            if (i < bars)
                u8g2_DrawBox(g, x, 43 - h, 4, h);
            else
                u8g2_DrawHLine(g, x, 42, 4);
        }
        font(small);
        if (up && s.signalValid)
        {
            int q = (s.rssi + 100) * 2;
            if (q < 0)
                q = 0;
            if (q > 100)
                q = 100;
            snprintf(b, sizeof b, "%s  ~%d%% quality", q >= 70 ? "GOOD" : q >= 40 ? "FAIR"
                                                                                  : "WEAK",
                     q);
            text(4, 51, b);
        }
        else
        {
            text(4, 51, up ? "Awaiting RSSI" : s.link == Link::Disconnected ? "Retry scheduled"
                                                                            : "Joining Wi-Fi");
            if (s.link != Link::Disconnected)
                u8g2_DrawBox(g, 115 + (now / 300) % 3 * 3, 49, 2, 2);
        }
    }
    else if (page == 1)
    {
        header("ADDRESS", up ? "LAN" : "DOWN");
        font(heading);
        text(4, 23, "LOCAL IP");
        font(small);
        text(4, 33, up ? s.ip : "--");
        font(heading);
        text(4, 44, "GATEWAY");
        font(small);
        text(4, 53, up ? s.gateway : "--");
    }
    else if (page == 2)
    {
        header("GATEWAY", !up ? "N/A" : s.noReply                                        ? "ICMP?"
                                    : s.ping.samples.count && s.ping.samples.newest().ok ? "REPLY"
                                                                                         : "PROBE");
        if (!up || !s.ping.samples.count || !s.ping.samples.newest().ok)
        {
            font(heading);
            text(4, 32, !up ? "UNAVAILABLE" : s.probeError        ? "PROBE ERROR"
                                          : !s.ping.samples.count ? "WAIT"
                                          : s.noReply             ? "NO REPLY"
                                                                  : "TIMEOUT");
        }
        else
        {
            uint32_t r = s.ping.samples.newest().rtt;
            if (r > 999) {font(heading); text(4, 35, ">999 ms");}
            else {snprintf(b, sizeof b, "%lu", (unsigned long)r);metric(b, "ms", 34); }
        }
        font(small);
        if (s.ping.successes())
            snprintf(b, sizeof b, "AVG %lums", (unsigned long)s.ping.average());
        else
            snprintf(b, sizeof b, "AVG --");
        text(4, 43, b);
        if (s.ping.samples.count)
            snprintf(b, sizeof b, "PROBE LOSS %u%% N%u", s.ping.loss(), unsigned(s.ping.samples.count));
        else
            snprintf(b, sizeof b, "PROBE LOSS --  N0");
        text(4, 53, b);
    }
    else
    {
        header("HISTORY", "RSSI");
        // Fixed -100..-30 dBm scale; missing samples break the line.
        u8g2_DrawPixel(g, 4, 17);
        u8g2_DrawPixel(g, 4, 33);
        bool prev = false;
        int px = 0, py = 0;
        for (size_t i = 0; i < s.history.count; i++)
        {
            auto v = s.history.oldest(i);
            int x = 4 + (60 - s.history.count + i) * 2;
            int db = v.dbm;
            if (db < -100)
                db = -100;
            if (db > -30)
                db = -30;
            int y = 33 - (db + 100) * 16 / 70;
            if (v.valid)
            {
                if (prev)
                    u8g2_DrawLine(g, px, py, x, y);
                else
                    u8g2_DrawPixel(g, x, y);
                px = x;
                py = y;
            }
            prev = v.valid;
        }
        font(small);
        if (!s.history.count)
            text(4, 28, "Awaiting signal");
        if (!s.events.count)
            text(4, 49, "No events yet");
        else
        {
            size_t pairs = (s.events.count + 1) / 2;
            size_t start = (age / Config::EVENT_PAIR_MS % pairs) * 2;
            for (size_t i = 0; i < 2 && start + i < s.events.count; i++)
            {
                auto e = s.events.newest(start + i);
                durationText(b, sizeof b, now - e.at);
                right(23, 42 + i * 11, b);
                text(30, 42 + i * 11, eventLabel(e.kind));
            }
        }
    }
    footer(s, page, now);
}
void OledRenderer::boot(uint32_t now)
{
    u8g2_ClearBuffer(g);
    u8g2_SetDrawColor(g, 1);
    font(heading);
    text(32, 28, "NET /");
    text(32, 42, "MONITOR");
    u8g2_DrawFrame(g, 6, 27, 6, 6);
    u8g2_DrawFrame(g, 18, 27, 6, 6);
    u8g2_DrawFrame(g, 12, 39, 6, 6);
    u8g2_DrawLine(g, 12, 30, 18, 30);
    u8g2_DrawLine(g, 9, 33, 15, 39);
    u8g2_DrawLine(g, 21, 33, 15, 39);
    u8g2_DrawHLine(g, 32, 49, now < 900 ? now * 88 / 900 : 88);
}
