#pragma once
#include <U8g2lib.h>
#include "OledRenderer.h"
#if NET_DISPLAY_SH1106
using Panel = U8G2_SH1106_128X64_NONAME_F_HW_I2C;
#else
using Panel = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
#endif
class DisplayUI
{
    Panel panel{U8G2_R0, U8X8_PIN_NONE};
    OledRenderer renderer{panel.getU8g2()};
    bool present = false, automatic = true, dimmed = false;
    unsigned page = 0;
    uint8_t contrast = Config::CONTRAST;
    uint32_t pageAt = 0, lastFrame = 0, lastActivity = 0, transitionAt = 0;
    bool transition = false;

uint8_t captureData[1024]{};
    int capturePosition = -1;
    void streamCapture();
public:
    void begin();
    void tick(const AppState &, uint32_t);
    void setPage(unsigned, uint32_t);
    void setAuto(bool value) { automatic = value; }
    void wake(uint32_t);
    void setContrast(uint8_t, uint32_t);
    void capture();
    bool available() const { return present; }
};
