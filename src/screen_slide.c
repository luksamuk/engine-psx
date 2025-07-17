#include <stdio.h>
#include <stdlib.h>

#include "screen.h"
#include "screens/slide.h"
#include "render.h"
#include "util.h"
#include "input.h"
#include "sound.h"

static SlideOption next_slide = 0xff;

ScreenIndex slide_screen_override = 0xff;

static volatile const struct {
    const char  *image;
    BGMOption   bgm;
    ScreenIndex next;
    SlideOption next_slide;
    int16_t     duration; // In frames (1 second = 60 frames)
} slide_table[] = {
    // Put TIM images at 320x0 (no CLUT):
    // $ img2tim -usealpha -org 320 0 -bpp 16 -o IMAGE.TIM IMAGE.png
    // I recommend images to be 255x240.
    {
        .image = "\\MISC\\SOON.TIM;1",
        .bgm = BGM_TITLESCREEN,
        .next = SCREEN_CREDITS,
        .next_slide = 0xff,
        .duration = 1500
    },
    {
        .image = "\\MISC\\SEGALOGO.TIM;1",
        .bgm = 0xff,
        .next = SCREEN_SLIDE,
        .next_slide = SLIDE_CREATEDBY,
        .duration = 180
    },
    {
        .image = "\\MISC\\CREATED.TIM;1",
        .bgm = 0xff,
        .next = SCREEN_TITLE,
        .next_slide = 0xff,
        .duration = 180
    },
    {
        .image = "\\MISC\\SAGE2025.TIM;1",
        .bgm = 0xff,
        .next = SCREEN_SLIDE,
        .next_slide = SLIDE_CREATEDBY,
        .duration = 180
    },
};

typedef struct {
    SlideOption current;
    int32_t prectx;
    int32_t precty;
    uint8_t mode;
    uint8_t fade;
    uint8_t state;
    int16_t counter;
    BGMOption bgm;
    ScreenIndex next;
} screen_slide_data;

void
screen_slide_load()
{   
    screen_slide_data *data = screen_alloc(sizeof(screen_slide_data));
    data->current = next_slide; next_slide = -1;

    // Prepare for next screen
    const char *slide_image_path = slide_table[data->current].image;
    data->next = slide_table[data->current].next;
    data->bgm = slide_table[data->current].bgm;
    data->counter = slide_table[data->current].duration;

    if(slide_screen_override != 0xff) {
        data->next = slide_screen_override;
        slide_screen_override = 0xff;
    }

    // Load image data
    uint32_t length;
    TIM_IMAGE img;
    uint8_t *bin = file_read(slide_image_path, &length);
    load_texture(bin, &img);
    data->mode = img.mode;
    data->prectx = img.prect->x;
    data->precty = img.prect->y;
    free(bin);

    // Initialize counters
    data->fade = 0;
    data->state = 0;

    // Play BGM if needed
    if(data->bgm != 0xff) sound_bgm_play(data->bgm);
}

void
screen_slide_unload(void *d)
{
    screen_slide_data *data = (screen_slide_data *)d;

    if(data->bgm != 0xff) sound_cdda_stop();
    screen_free();
}

void
screen_slide_update(void *d)
{
    screen_slide_data *data = (screen_slide_data *)d;

    if(data->state == 0) {
        if(data->fade < 128) data->fade += 2;
        else data->state = 1;
    } else if(data->state == 1) {
        data->counter--;

        if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
            data->counter = 0;
        }
        
        if(data->counter <= 0) data->state = 2;
    } else {
        if(data->fade > 0) data->fade -= 2;
        else {
            if(data->next == SCREEN_SLIDE) {
                next_slide = slide_table[data->current].next_slide;
            }
            scene_change(data->next);
        }
    }
}

void
screen_slide_draw(void *d)
{
    screen_slide_data *data = (screen_slide_data *)d;
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
    sort_prim(poly, OTZ_LAYER_TOPMOST);
}


void
screen_slide_set_next(SlideOption opt)
{
    next_slide = opt;
}
