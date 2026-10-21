#pragma once
#include "Model.h"
extern "C"
{
#if __has_include("clib/u8g2.h")
#include "clib/u8g2.h"
#else
#include "u8g2.h"
#endif
}
class OledRenderer
{
    u8g2_t *g;
    void font(const uint8_t *f) { u8g2_SetFont(g, f); }
    int width(const char *s) { return u8g2_GetStrWidth(g, s); }
    void text(int x, int y, const char *s) { u8g2_DrawStr(g, x, y, s); }
    void right(int x, int y, const char *s) { text(x - width(s), y, s); }
    void metric(const char *value, const char *unit, int baseline = 42);
    void header(const char *, const char *);
    void footer(const AppState &, unsigned, uint32_t);

public:
    explicit OledRenderer(u8g2_t *display) : g(display) {}
    void render(const AppState &, unsigned page, uint32_t now, uint32_t pageAge);
    void boot(uint32_t now);
};
