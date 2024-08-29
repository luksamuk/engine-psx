#include "screens/disclaimer.h"
#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "input.h"
#include "screen.h"
#include "render.h"

static uint8_t *disclaimer_bg = NULL;
static uint16_t disclaimer_timer = 0;

void screen_disclaimer_load()
{
    uint32_t length;
    disclaimer_bg = file_read("\\MISC\\DISK.TIM;1", &length);
}

void screen_disclaimer_unload()
{
    free(disclaimer_bg);
    disclaimer_bg = NULL;
    disclaimer_timer = 0;
}

void screen_disclaimer_update()
{
    disclaimer_timer++;
    if((disclaimer_timer > 1200) || pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
        scene_change(SCREEN_LEVELSELECT);
    }
}

void screen_disclaimer_draw()
{
    TIM_IMAGE tim;
    GetTimInfo((const uint32_t *)disclaimer_bg, &tim);
    RECT r2 = *tim.prect;
    r2.y += 240;
    set_clear_color(0, 0, 0);
    LoadImage(tim.prect, tim.paddr);
    LoadImage(&r2, tim.paddr);
}
