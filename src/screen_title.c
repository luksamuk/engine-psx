#include "screens/title.h"
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "render.h"
#include "input.h"
#include "screen.h"
#include "sound.h"

#include "screens/fmv.h"
#include "screens/level.h"

uint8_t tim_mode;
int32_t prect_x;
int32_t prect_y;
uint8_t rgb_count;
uint8_t menu_option;
uint8_t selected;
uint8_t next_scene;

static const char *menu_text[] = {
    "Press Start",
    "  New Game >",
    "< Level Select  ",
};

#define MENU_MAX_OPTION 2

void
screen_title_load()
{
    uint32_t length;
    TIM_IMAGE titlecard;
    uint8_t *data = file_read("\\SPRITES\\TITLE.TIM;1", &length);
    load_texture(data, &titlecard);
    tim_mode = titlecard.mode;
    prect_x  = titlecard.prect->x;
    prect_y  = titlecard.prect->y;
    free(data);

    rgb_count = 0;
    menu_option = 0;
    selected = 0;
    next_scene = 0;

    sound_stop_xa();
    sound_play_xa("\\BGM\\BGM003.XA;1", 0, 1, 0);
}

void
screen_title_unload()
{
    sound_stop_xa();
}

void
screen_title_update()
{
    if(!selected) {
        if(rgb_count < 128)
            rgb_count += 4;
        else {
            if(menu_option == 0 && pad_pressed(PAD_START)) {
                menu_option = 1;
            } else if(menu_option > 0) {
                if(pad_pressed(PAD_LEFT) && (menu_option > 1)) menu_option--;
                if(pad_pressed(PAD_RIGHT) && (menu_option < MENU_MAX_OPTION)) menu_option++;

                if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
                    selected = 1;
                    switch(menu_option) {
                    case 1: // New Game
                        screen_level_setlevel(0);
                        screen_fmv_set_next(SCREEN_LEVEL);
                        screen_fmv_enqueue("\\INTRO.STR;1");
                        next_scene = SCREEN_FMV;
                        break;
                    case 2: // Level Select
                        next_scene = SCREEN_LEVELSELECT;
                        break;
                    default: selected = 0; break;
                    }
                }
            }
        }
        return;
    }

    if(rgb_count > 0) rgb_count -= 4;
    else {
        scene_change(next_scene);
    }
}

void
screen_title_draw()
{
    SPRT *sprite = (SPRT *)get_next_prim();
    increment_prim(sizeof(SPRT));
    setSprt(sprite);
    setXY0(sprite, 32, 24);
    setWH(sprite, 256, 175);
    setUV0(sprite, 0, 0);
    setRGB0(sprite, rgb_count, rgb_count, rgb_count);
    sort_prim(sprite, OT_LENGTH - 1);

    DR_TPAGE *tpage = (DR_TPAGE *)get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpage, 0, 1,
                 getTPage(tim_mode & 0x3, 0, prect_x, prect_y));
    sort_prim(tpage, OT_LENGTH - 1);

    if(rgb_count >= 128) {
        const char *text = menu_text[menu_option];
        draw_text(CENTERX - (strlen(text) << 2), 207, 0, text);
    }
}
