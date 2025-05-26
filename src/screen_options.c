#include <stdlib.h>
#include "screen.h"
#include "screens/options.h"
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

extern SoundEffect sfx_switch;

typedef struct {
    int32_t  bg_prect_x;
    int32_t  bg_prect_y;
    uint8_t  bg_mode;
    uint8_t  bg_frame;
    uint8_t  bg_state;
    uint16_t bg_timer;
} screen_options_data;

// TIM 15bpp -- TPAGE: 384x0
// Sonic: 0x0+22+40
// Tails: 22x0+40+40
// Knuckles: 62x0+40+40


void
screen_options_load()
{
    screen_options_data *data = screen_alloc(sizeof(screen_options_data));

    uint32_t length;
    TIM_IMAGE tim;
    uint8_t *img = file_read("\\MISC\\LVLSEL.TIM;1", &length);
    load_texture(img, &tim);
    data->bg_mode = tim.mode;
    data->bg_prect_x = tim.prect->x;
    data->bg_prect_y = tim.prect->y;
    free(img);

    data->bg_frame = 0;
    data->bg_state = 0;
    data->bg_timer = BG_PAUSE;

    // Play level select music, but loop
    sound_cdda_play_track(3, 1);
}

void
screen_options_unload(void *)
{
    sound_cdda_stop();
    screen_free();
}

void
screen_options_update(void *d)
{
    screen_options_data *data = (screen_options_data *)d;

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

    /* if(pad_pressed(PAD_RIGHT) && (data->character < CHARA_MAX)) { */
    /*     data->character++; */
    /*     sound_play_vag(sfx_switch, 0); */
    /* } */
    /* if(pad_pressed(PAD_LEFT) && (data->character > 0)) { */
    /*     data->character--; */
    /*     sound_play_vag(sfx_switch, 0); */
    /* } */

    /* if(pad_pressed(PAD_CROSS) || pad_pressed(PAD_START)) { */
    /*     screen_level_setcharacter(data->character); */
    /*     // NOTE: screen_level_setlevel should have */
    /*     // been called from previous screen */
    /*     screen_level_setmode(LEVEL_MODE_NORMAL); */
    /*     scene_change(SCREEN_LEVEL); */
    /* } */

    if(pad_pressed(PAD_CIRCLE)) {
        scene_change(SCREEN_TITLE);
    }
}

void
screen_options_draw(void *d)
{
    screen_options_data *data = (screen_options_data *)d;

    static const char *title = "OPTIONS";

    font_set_color(128, 128, 128);
    uint16_t text_hsize = font_measurew_big(title) >> 1;
    uint16_t text_xpos = CENTERX - text_hsize;
    font_draw_big(title, text_xpos, SCREEN_YRES >> 3);

    // TODO

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
