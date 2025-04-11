#include <stdlib.h>
#include "screen.h"
#include "screens/charselect.h"
#include "player.h"
#include "render.h"
#include "util.h"
#include "input.h"
#include "basic_font.h"
#include "sound.h"

#include "screens/level.h"

#define BG_PAUSE  90
#define BG_FPS    4
#define BG_FRAMES 4

#define CHARSEL_PADDING (SCREEN_XRES >> 2)

typedef struct {
    int8_t   character;
    int32_t  bg_prect_x;
    int32_t  bg_prect_y;
    uint8_t  bg_mode;
    uint8_t  bg_frame;
    uint8_t  bg_state;
    uint16_t bg_timer;
    uint8_t  charsel_mode;
} screen_charselect_data;

// TIM 15bpp -- TPAGE: 384x0
// Sonic: 0x0+22+40
// Tails: 22x0+40+40
// Knuckles: 62x0+40+40


void
screen_charselect_load()
{
    screen_charselect_data *data = screen_alloc(sizeof(screen_charselect_data));
    data->character = screen_level_getcharacter();

    uint32_t length;
    TIM_IMAGE tim;
    uint8_t *img = file_read("\\MISC\\LVLSEL.TIM;1", &length);
    load_texture(img, &tim);
    data->bg_mode = tim.mode;
    data->bg_prect_x = tim.prect->x;
    data->bg_prect_y = tim.prect->y;
    free(img);

    img = file_read("\\MISC\\CHARSEL.TIM;1", &length);
    load_texture(img, &tim);
    data->charsel_mode = tim.mode;
    free(img);

    data->bg_frame = 0;
    data->bg_state = 0;
    data->bg_timer = BG_PAUSE;

    sound_bgm_play(BGM_TITLESCREEN);
}

void
screen_charselect_unload(void *)
{
    sound_stop_xa();
    screen_free();
}

void
screen_charselect_update(void *d)
{
    screen_charselect_data *data = (screen_charselect_data *)d;

    if((data->bg_state == 0) || (data->bg_state == 2)) {
        data->bg_timer--;
        if(data->bg_timer == 0) {
            data->bg_state++;
            data->bg_timer = BG_FPS;
        }
    } else if(data->bg_state == 1) {
        data->bg_timer--;
        if(data->bg_timer == 0) {
            data->bg_frame++;
            data->bg_timer = BG_FPS;
            if(data->bg_frame == BG_FRAMES - 1) {
                data->bg_state++;
                data->bg_timer = BG_PAUSE;
            }
        }
    } else if(data->bg_state == 3) {
        data->bg_timer--;
        if(data->bg_timer == 0) {
            data->bg_frame--;
            data->bg_timer = BG_FPS;
            if(data->bg_frame == 0) {
                data->bg_state = 0;
                data->bg_timer = BG_PAUSE;
            }
        }
    }

    if(pad_pressed(PAD_RIGHT)) data->character++;
    if(pad_pressed(PAD_LEFT)) data->character--;
    data->character =
        (data->character < 0)
        ? 0
        : ((data->character > CHARA_MAX)
           ? CHARA_MAX
           : data->character);

    if(pad_pressed(PAD_CROSS) || pad_pressed(PAD_START)) {
        screen_level_setcharacter(data->character);
        screen_level_setlevel(6);
        screen_level_setmode(LEVEL_MODE_NORMAL);
        scene_change(SCREEN_LEVEL);
    }
}

const char *
_get_char_name(int8_t character)
{
    switch(character) {
    case CHARA_SONIC: return "SONIC";
    case CHARA_MILES: return "TAILS";
    case CHARA_KNUCKLES: return "KNUCKLES";
    }
    return "UNKNOWN";
}

void
screen_charselect_draw(void *d)
{
    screen_charselect_data *data = (screen_charselect_data *)d;

    static const char *title = "CHARACTER SELECT";

    font_set_color(128, 128, 128);
    uint16_t text_hsize = font_measurew_big(title) >> 1;
    uint16_t text_xpos = CENTERX - text_hsize;
    font_draw_big(title, text_xpos, SCREEN_YRES >> 3);

    // Draw characters
    for(uint16_t i = 0; i < 3; i++) {
        int16_t xpos = (CHARSEL_PADDING + (i * CHARSEL_PADDING));
        int16_t ypos = (SCREEN_YRES >> 1);
        uint8_t is_current_char = (data->character == i);
        uint8_t dim = is_current_char ? 128 : 64;
        POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
        increment_prim(sizeof(POLY_FT4));
        setPolyFT4(poly);
        setRGB0(poly, dim, dim, dim);
        setTPage(poly, data->charsel_mode & 0x3, 0, 384, 0);
        poly->clut = 0;
        setXYWH(poly, xpos - 20, ypos - 20, 40, 40);
        setUVWH(poly, 40 * i, 0, 40, 40);
        sort_prim(poly, OTZ_LAYER_PLAYER);

        if(is_current_char) {
            const char *charname = _get_char_name(data->character);
            text_hsize = font_measurew_sm(charname) >> 1;
            font_draw_sm(charname, xpos - text_hsize, ypos + 30);
        }
    }

    // Draw background
    for(uint16_t y = 0; y < SCREEN_YRES; y += 32) {
        for(uint16_t x = 0; x < SCREEN_XRES; x += 48) {
            POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, 96, 96, 96);
            poly->tpage = getTPage(data->bg_mode & 0x3,
                                   0,
                                   data->bg_prect_x,
                                   data->bg_prect_y);
            poly->clut = 0;
            setXYWH(poly, x, y, 48, 32);
            setUVWH(poly, 0, 32 * data->bg_frame, 48, 32);
            sort_prim(poly, OTZ_LAYER_LEVEL_BG);
        }
    }
}
