#include "OledRenderer.h"
#include "Demo.h"
#include <stdlib.h>
#include <initializer_list>
static uint8_t noop(u8x8_t *, uint8_t, uint8_t, void *) { return 1; }
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    u8g2_t g;
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&g, U8G2_R0, noop, noop);
    u8g2_InitDisplay(&g);
    u8g2_SetPowerSave(&g, 0);
    for (auto f : {u8g2_font_helvB18_tn, u8g2_font_helvB08_tr, u8g2_font_6x10_tr}) {u8g2_SetFont(&g,f);fprintf(stderr,"font ascent=%d descent=%d\n",u8g2_GetAscent(&g),u8g2_GetDescent(&g));}
 OledRenderer renderer(&g);
    const char *names[] = {"strong", "weak", "connecting", "disconnected", "long-ssid", "full-ip", "timeout", "empty-history", "partial-window", "wait", "no-reply"};
    for (unsigned scene = 0; scene < 11; scene++)
        for (unsigned page = 0; page < 4; page++)
        {
            auto s = demoState(scene);
            renderer.render(s, page, 120000, 2500);
            char path[512];
            snprintf(path, sizeof path, "%s/%02u-%s-page%u.pbm", argv[1], scene, names[scene], page + 1);
            FILE *f = fopen(path, "wb");
            if (!f)
                return 3;
            fprintf(f, "P1\n128 64\n");
            auto b = u8g2_GetBufferPtr(&g);
            for (int y = 0; y < 64; y++)
            {
                for (int x = 0; x < 128; x++)
                    fprintf(f, "%d ", (b[y / 8 * 128 + x] >> (y % 8)) & 1);
                fprintf(f, "\n");
            }
            fclose(f);
        }
}
