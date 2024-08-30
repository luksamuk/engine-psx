#include "screens/disclaimer.h"
#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "input.h"
#include "screen.h"
#include "render.h"

#include "screens/fmv.h"
#include "screens/level.h"

static uint8_t *disclaimer_bg = NULL;
static uint16_t disclaimer_timer = 0;

extern int debug_mode;

void
screen_disclaimer_load()
{
    uint32_t length;
    disclaimer_bg = file_read("\\MISC\\DISK.TIM;1", &length);
}

void
screen_disclaimer_unload()
{
    free(disclaimer_bg);
    disclaimer_bg = NULL;
    disclaimer_timer = 0;
}

void
screen_disclaimer_update()
{
    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }

    disclaimer_timer++;
    if((disclaimer_timer > 1200) || pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
        if(pad_pressing(PAD_SQUARE)) {
            // Change to level select
            scene_change(SCREEN_LEVELSELECT);
        } else {
            // Prepare intro, but also prepare level
            screen_level_setlevel(0);
            screen_fmv_set_next(SCREEN_LEVEL);
            screen_fmv_enqueue("\\SONICT.STR;1");
            screen_fmv_enqueue("\\INTRO.STR;1");
            scene_change(SCREEN_FMV);
        }
    }
}

void
screen_disclaimer_draw()
{
    TIM_IMAGE tim;
    GetTimInfo((const uint32_t *)disclaimer_bg, &tim);
    RECT r2 = *tim.prect;
    r2.y += 240;
    set_clear_color(0, 0, 0);
    LoadImage(tim.prect, tim.paddr);
    LoadImage(&r2, tim.paddr);
}
