#include <stdlib.h>

#include "screen.h"
#include "screens/comingsoon.h"
#include "render.h"
#include "util.h"
#include "input.h"
#include "sound.h"

#include "screens/fmv.h"

typedef struct {
    int32_t prectx;
    int32_t precty;
    uint8_t mode;
    uint8_t fade;
    int16_t counter;
    uint8_t state;
} screen_comingsoon_data;

void
screen_comingsoon_load()
{
    screen_comingsoon_data *data = screen_alloc(sizeof(screen_comingsoon_data));

    uint32_t length;
    TIM_IMAGE img;
    uint8_t *bin = file_read("\\MISC\\SOON.TIM;1", &length);
    load_texture(bin, &img);
    data->mode = img.mode;
    data->prectx = img.prect->x;
    data->precty = img.prect->y;
    free(bin);

    data->fade = 0;
    data->counter = 0;
    data->state = 0;

    sound_play_xa("\\BGM\\MNU001.XA;1", 0, 1, 0);
}

void
screen_comingsoon_unload(void *)
{
    screen_free();
}

void
screen_comingsoon_update(void *d)
{
    screen_comingsoon_data *data = (screen_comingsoon_data *)d;
    if(data->state == 0) {
        if(data->fade < 128) {
            data->fade += 2;
        } else {
            data->state = 1;
            data->counter = 1500; // 25 seconds
        }
    } else if(data->state == 1) {
        data->counter--;

        if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
            data->counter = 0;
        }
        
        if(data->counter <= 0) {
            data->state = 2;
        }
    } else {
        if(data->fade > 0) data->fade -= 2;
        else {
            screen_fmv_set_next(SCREEN_TITLE);
            screen_fmv_enqueue("\\SONICT.STR;1");
            scene_change(SCREEN_FMV);
        }
    }
}

void screen_comingsoon_draw(void *d)
{
    screen_comingsoon_data *data = (screen_comingsoon_data *)d;
    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, data->fade, data->fade, data->fade);
    poly->tpage = getTPage(data->mode & 0x3,
                           0,
                           data->prectx,
                           data->precty);
    poly->clut = 0;
    setXYWH(poly, 32, 0, 255, 240);
    setUVWH(poly, 0, 0, 255, 239);
    sort_prim(poly, 0);
}
