#include "DisplayUI.h"
#include <Wire.h>
void DisplayUI::begin()
{
    Wire.begin(Config::SDA_PIN, Config::SCL_PIN);
    Wire.setClock(Config::I2C_HZ);
    Wire.setTimeOut(20);
    Wire.beginTransmission(Config::OLED_ADDRESS);
    present = Wire.endTransmission() == 0;
    if (present)
    {
        panel.setI2CAddress(Config::OLED_ADDRESS << 1);
        panel.setBusClock(Config::I2C_HZ);
        panel.begin();
        panel.setContrast(contrast);
    }
    Serial.printf("OLED %s at 0x%02X (%s)\n", present ? "ACK" : "absent", Config::OLED_ADDRESS, NET_DISPLAY_SH1106 ? "SH1106" : "SSD1306");
}
void DisplayUI::wake(uint32_t now)
{
    lastActivity = now;
    if (present && dimmed) panel.setContrast(contrast);
    dimmed = false;
}
void DisplayUI::setContrast(uint8_t value, uint32_t now)
{
    contrast = value;
    wake(now);
    if (present) panel.setContrast(contrast);
}
void DisplayUI::setPage(unsigned value, uint32_t now)
{
    page = value;
    pageAt = transitionAt = now;
    transition = true;
}
void DisplayUI::tick(const AppState &s, uint32_t now)
{
    streamCapture();
    if (Config::DIM_AFTER_MS && !dimmed && elapsed(now, lastActivity, Config::DIM_AFTER_MS))
    {
        dimmed = true;
        if (present)
            panel.setContrast(Config::DIM_CONTRAST);
    }
    if (automatic && elapsed(now, pageAt, Config::PAGE_MS))
        setPage((page + 1) % 4, now);
    bool anim = transition || now < 1000 || (page == 0 && (strlen(s.ssid) > 19 || !s.connected()));
    if (!elapsed(now, lastFrame, anim ? Config::FRAME_MS : 250))
        return;
    lastFrame = now;
    if (now < 1000)
        renderer.boot(now);
    else
        renderer.render(s, page, now, now - pageAt);
    if (transition)
    {
        uint32_t t = now - transitionAt;
        if (t >= Config::TRANSITION_MS)
            transition = false;
        else
        {
            int rows = (Config::TRANSITION_MS - t) * 40 / Config::TRANSITION_MS;
            panel.setDrawColor(0);
            panel.drawBox(0, 14, 128, rows);
            panel.setDrawColor(1);
        }
    }
    if (Config::PIXEL_SHIFT && ((now / 60000) & 1))
    {
        uint8_t *b = panel.getBufferPtr();
        for (int y = 0; y < 8; y++)
        {
            memmove(b + y * 128 + 1, b + y * 128, 127);
            b[y * 128] = 0;
        }
    }
    if (present)
        panel.sendBuffer();
}
void DisplayUI::capture()
{
    if(capturePosition >= 0) return;
    memcpy(captureData, panel.getBufferPtr(), sizeof captureData);
    capturePosition = 0;
}
void DisplayUI::streamCapture()
{
    if(capturePosition < 0) return;
    const char header[] = "PBM_BEGIN\nP1\n128 64\n";
    const char trailer[] = "PBM_END\n";
    constexpr int body = 64*257;
    constexpr int start = sizeof(header)-1;
    constexpr int total = start + body + sizeof(trailer)-1;
    int available = Serial.availableForWrite();
    int budget = available < 32 ? available : 32;
    while(budget-- > 0 && capturePosition < total){
        int p = capturePosition++;
        char c;
        if(p < start) c=header[p];
        else if(p >= start+body) c=trailer[p-start-body];
        else {int offset=p-start, y=offset/257, column=offset%257;
            if(column==256)c='\n';
            else if(column&1)c=' ';
            else {int x=column/2;c=((captureData[y/8*128+x]>>(y%8))&1)?'1':'0';}
        }
        Serial.write(c);
    }
    if(capturePosition==total)capturePosition=-1;
}
