#include "screens/title.h"
#include <stdlib.h>
#include <string.h>
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

typedef struct {
    texture_props props_title;
    uint8_t rgb_count;
    uint8_t menu_option;
    uint8_t selected;
    uint8_t next_scene;
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
    TIM_IMAGE titlecard;
    uint8_t *data = file_read(filename, &length);
    load_texture(data, &titlecard);
    props->mode = titlecard.mode;
    props->prect_x  = titlecard.prect->x;
    props->prect_y  = titlecard.prect->y;
    free(data);
}

void
screen_title_load()
{
    screen_title_data *data = screen_alloc(sizeof(screen_title_data));
    title_load_texture("\\SPRITES\\TITLE\\TITLE.TIM;1", &data->props_title);

    data->rgb_count = 0;
    data->menu_option = 0;
    data->selected = 0;
    data->next_scene = 0;

    sound_stop_xa();
    sound_play_xa("\\BGM\\BGM003.XA;1", 0, 1, 0);
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

void
screen_title_draw(void *d)
{
    screen_title_data *data = (screen_title_data *)d;

    SPRT *sprite = (SPRT *)get_next_prim();
    increment_prim(sizeof(SPRT));
    setSprt(sprite);
    setXY0(sprite, 32, 24);
    setWH(sprite, 256, 175);
    setUV0(sprite, 0, 0);
    setRGB0(sprite, data->rgb_count, data->rgb_count, data->rgb_count);
    sort_prim(sprite, OT_LENGTH - 1);

    DR_TPAGE *tpage = (DR_TPAGE *)get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpage, 0, 1,
                 getTPage(data->props_title.mode & 0x3,
                          0,
                          data->props_title.prect_x,
                          data->props_title.prect_y));
    sort_prim(tpage, OT_LENGTH - 1);

    if(data->rgb_count >= 128) {
        const char *text = menu_text[data->menu_option];
        draw_text(CENTERX - (strlen(text) << 2), 199, 0, text);

        char buffer[255] = { 0 };
        snprintf(buffer, 255, "%s %s", __DATE__, __TIME__);
        int16_t x = SCREEN_XRES - (strlen(buffer) * 8) - 8;
        draw_text(x, SCREEN_YRES - 16, 0, buffer);
    }
}
