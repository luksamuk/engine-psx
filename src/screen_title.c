#include "screens/title.h"
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>

#include "util.h"
#include "render.h"
#include "input.h"
#include "screen.h"
#include "sound.h"

#include "screens/fmv.h"
#include "screens/level.h"

typedef struct {
    int32_t prect_x;
    int32_t prect_y;
    uint8_t mode;
} texture_props;

/* Parallax data */
#define PRL_INFO_PER_PIECE 6
#define PRL_NUM_PIECES     7
static const int16_t prl_data[] = {
    //  W    H  v0  spd      ypos  single
    // Island, immovable
    256, 47, 62, 0x0000,  170,  1,
    // Cloud data
    256, 37, 25, 0x019a,  181,  0,
    // Water data
    64,   6,  0, 0x0666,  216,  0,
    64,   5,  6, 0x0800,  222,  0,
    62,   5, 11, 0x099a,  227,  0,
    64,   5, 16, 0x0b33,  232,  0,
    66,   4, 20, 0x0ccd,  237,  0,
};

typedef struct {
    texture_props props_title;
    texture_props props_prl;
    uint8_t rgb_count;
    uint8_t menu_option;
    uint8_t selected;
    uint8_t next_scene;
    int32_t prl_pos[PRL_NUM_PIECES];
} screen_title_data;


static const char *menu_text[] = {
    "Press Start",
    "     Continue   >",
    "<    New Game   >",
    "<  Level Select  ",
};

#define MENU_MAX_OPTION 3

static void
title_load_texture(const char *filename, texture_props *props)
{
    uint32_t length;
    TIM_IMAGE img;
    uint8_t *data = file_read(filename, &length);
    load_texture(data, &img);
    props->mode = img.mode;
    props->prect_x  = img.prect->x;
    props->prect_y  = img.prect->y;
    free(data);
}

void
screen_title_load()
{
    screen_title_data *data = screen_alloc(sizeof(screen_title_data));

    title_load_texture("\\SPRITES\\TITLE\\TITLE.TIM;1", &data->props_title);
    title_load_texture("\\SPRITES\\TITLE\\PRL.TIM;1", &data->props_prl);

    data->rgb_count = 0;
    data->menu_option = 0;
    data->selected = 0;
    data->next_scene = 0;

    bzero(data->prl_pos, PRL_NUM_PIECES * sizeof(int32_t));
    data->prl_pos[0] = 32 << 12; // Island center

    sound_stop_xa();
    sound_play_xa("\\BGM\\BGM003.XA;1", 0, 1, 0);

    set_clear_color(56, 104, 200);
}

void
screen_title_unload(void *)
{
    sound_stop_xa();
    screen_free();
}

void
screen_title_update(void *d)
{
    screen_title_data *data = (screen_title_data *)d;

    for(int wp = 0; wp < PRL_NUM_PIECES; wp++) {
        int32_t wp_spd = prl_data[(wp * PRL_INFO_PER_PIECE) + 3];
        int32_t wp_w = ((int32_t)prl_data[(wp * PRL_INFO_PER_PIECE)] << 12);
        data->prl_pos[wp] -= wp_spd;
        if(data->prl_pos[wp] < -wp_w) data->prl_pos[wp] = 0;
    }
    
    if(!data->selected) {
        if(data->rgb_count < 128)
            data->rgb_count += 4;
        else {
            if(data->menu_option == 0 && pad_pressed(PAD_START)) {
                data->menu_option = 2;
            } else if(data->menu_option > 0) {
                if(pad_pressed(PAD_LEFT) && (data->menu_option > 1))
                    data->menu_option--;
                if(pad_pressed(PAD_RIGHT) && (data->menu_option < MENU_MAX_OPTION))
                    data->menu_option++;

                if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
                    data->selected = 1;
                    switch(data->menu_option) {
                    case 2: // New Game
                        screen_level_setlevel(0);
                        screen_fmv_set_next(SCREEN_LEVEL);
                        screen_fmv_enqueue("\\INTRO.STR;1");
                        data->next_scene = SCREEN_FMV;
                        break;
                    case 3: // Level Select
                        data->next_scene = SCREEN_LEVELSELECT;
                        break;
                    default: data->selected = 0; break;
                    }
                }
            }
        }
        return;
    }

    if(data->rgb_count > 0) data->rgb_count -= 4;
    else {
        scene_change(data->next_scene);
    }
}

static void
screen_title_drawtitle(screen_title_data *data)
{
    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, data->rgb_count, data->rgb_count, data->rgb_count);
    poly->tpage = getTPage(data->props_title.mode & 0x3,
                           0,
                           data->props_title.prect_x,
                           data->props_title.prect_y);
    poly->clut = 0;
    setXY4(poly,
           32,       20,
           32 + 256, 20,
           32,       20 + 175,
           32 + 256, 20 + 175);
    setUV4(poly,
           0,   0,
           255, 0,
           0,   174,
           255, 174);
    sort_prim(poly, OT_LENGTH - 1);
}

static void
screen_title_drawprl(screen_title_data *data)
{
    for(int p = 0; p < PRL_NUM_PIECES; p++) {
        int idx = (p * PRL_INFO_PER_PIECE);
        int32_t w  = prl_data[idx];
        int32_t h  = prl_data[idx + 1];
        int32_t v0 = prl_data[idx + 2];
        int32_t y  = prl_data[idx + 4];
        int32_t s  = prl_data[idx + 5];

        for(int32_t wx = data->prl_pos[p];
            wx < ((int32_t)(SCREEN_XRES + w) << 12);
            wx += ((int32_t)w << 12)) {
            int16_t x = (int16_t)(wx >> 12);
            POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, 128, 128, 128);
            poly->tpage = getTPage(data->props_prl.mode & 0x3,
                                   0,
                                   data->props_prl.prect_x,
                                   data->props_prl.prect_y);
            poly->clut = 0;
            setXY4(poly,
                   x,     y,
                   x + w, y,
                   x,     y + h,
                   x + w, y + h);
            setUV4(poly,
                   0, v0,
                   w - 1, v0,
                   0, v0 + h,
                   w - 1, v0 + h);
            sort_prim(poly, OT_LENGTH - 1);

            if(s) break;
        }
    }
}

void
screen_title_draw(void *d)
{
    screen_title_data *data = (screen_title_data *)d;
    screen_title_drawtitle(data);
    screen_title_drawprl(data);

    if(data->rgb_count >= 128) {
        const char *text = menu_text[data->menu_option];
        draw_text(CENTERX - (strlen(text) << 2), 204, 0, text);

        char buffer[255] = { 0 };
        snprintf(buffer, 255, "%s %s", __DATE__, __TIME__);
        int16_t x = SCREEN_XRES - (strlen(buffer) * 8) - 8;
        draw_text(x, SCREEN_YRES - 16, 0, buffer);
    }
}
