#include "screens/disclaimer.h"
#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "input.h"
#include "screen.h"
#include "render.h"

#include "screens/fmv.h"
#include "screens/level.h"

#include "object.h"
#include <stdio.h>

extern int debug_mode;

typedef struct {
    uint8_t *disclaimer_bg;
    uint16_t disclaimer_timer;
} screen_disclaimer_data;

void
screen_disclaimer_load()
{
    screen_disclaimer_data *data = screen_alloc(sizeof(screen_disclaimer_data));
    uint32_t length;
    data->disclaimer_bg = file_read("\\MISC\\DISK.TIM;1", &length);
    data->disclaimer_timer = 0;
}

void
screen_disclaimer_unload(void *d)
{
    screen_disclaimer_data *data = (screen_disclaimer_data *) d;
    free(data->disclaimer_bg);
    screen_free();
}

void
screen_disclaimer_update(void *d)
{
    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }

    screen_disclaimer_data *data = (screen_disclaimer_data *) d;

    data->disclaimer_timer++;
    if((data->disclaimer_timer > 1200) || pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
        if(pad_pressing(PAD_SQUARE)) {
            // Change to level select
            scene_change(SCREEN_LEVELSELECT);
        } else {
            // Prepare intro, but also prepare level
            screen_fmv_set_next(SCREEN_TITLE);
            screen_fmv_enqueue("\\SONICT.STR;1");
            scene_change(SCREEN_FMV);
        }
    }
}

void
screen_disclaimer_draw(void *d)
{
    screen_disclaimer_data *data = (screen_disclaimer_data *) d;
    
    // Render disclaimer
    TIM_IMAGE tim;
    GetTimInfo((const uint32_t *)data->disclaimer_bg, &tim);
    RECT r2 = *tim.prect;
    r2.y += 240;
    set_clear_color(0, 0, 0);
    LoadImage(tim.prect, tim.paddr);
    LoadImage(&r2, tim.paddr);
}
