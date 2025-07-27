#include <stdio.h>
#include <stdlib.h>
#include "screen.h"
#include "screens/options.h"
#include "player.h"
#include "render.h"
#include "util.h"
#include "input.h"
#include "basic_font.h"
#include "sound.h"
#include "timer.h"

#include "screens/level.h"

#define BG_PAUSE  90
#define BG_FPS    4
#define BG_FRAMES 4

#define CHARSEL_PADDING (SCREEN_XRES >> 2)

#define SLIDER_STEP 0x1e9

extern SoundEffect sfx_switch;

extern int         debug_mode;

typedef struct {
    int32_t  bg_prect_x;
    int32_t  bg_prect_y;
    uint8_t  bg_mode;
    uint8_t  bg_frame;
    uint8_t  bg_state;
    uint16_t bg_timer;
    int8_t  selection;
    int16_t  master_volume;
    int16_t  bgm_volume;
    int16_t  sfx_volume;
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

    data->selection = 0;
    data->master_volume = sound_master_get_volume();
    data->bgm_volume = sound_cdda_get_volume();
    data->sfx_volume = sound_vag_get_volume();

    // Play level select music, but loop
    sound_cdda_play_track(3, 1);
}

void
screen_options_unload(void *)
{
    sound_cdda_stop();
    screen_free();
}


int
_manage_volume_control(int16_t *gauge)
{
    int16_t oldvalue = *gauge;
    if(pad_pressing(PAD_RIGHT)) (*gauge) += SLIDER_STEP;
    if(pad_pressing(PAD_LEFT)) (*gauge) -= SLIDER_STEP;
    (*gauge) =
        ((*gauge) < 0)
        ? 0
        : ((*gauge) > 0x3fff)
        ? 0x3fff
        : (*gauge);
    return (*gauge) != oldvalue;
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

    if(pad_pressed(PAD_UP)) data->selection--;
    if(pad_pressed(PAD_DOWN)) data->selection++;
    data->selection =
        (data->selection < 0)
        ? 0
        : (data->selection > 5)
        ? 4
        : data->selection;

    if(!(get_global_frames() % 4)) {
        switch(data->selection) {
        case 0:
            if(_manage_volume_control(&data->master_volume)) {
                sound_master_set_volume(data->master_volume);
            }
            break;
        case 1:
            if(_manage_volume_control(&data->bgm_volume)) {
                sound_cdda_set_volume(data->bgm_volume);
            }
            break;
        case 2:
            if(_manage_volume_control(&data->sfx_volume)) {
                sound_vag_set_volume(data->sfx_volume);
                sound_play_vag(sfx_switch, 0);
            }
            break;
        default: break;
        }
    }

    switch(data->selection) {
    case 3:
        {
            uint8_t changed = 0;
            int8_t state = sound_cdda_get_stereo();
            if(pad_pressed(PAD_LEFT)) { state--; changed = 1; }
            if(pad_pressed(PAD_RIGHT)) { state++; changed = 1; }
            if(changed) {
                if(state < 0) state = BGM_REVERSE_STEREO;
                state %= 3;
                sound_play_vag(sfx_switch, 0);
                sound_cdda_set_stereo(state);
            }
        }
        break;
    case 4:
        if(pad_pressed(PAD_LEFT)) {
            debug_mode--;
            sound_play_vag(sfx_switch, 0);
        }
        if(pad_pressed(PAD_RIGHT)) {
            debug_mode++;
            sound_play_vag(sfx_switch, 0);
        }
        debug_mode = (debug_mode < 0)
            ? 2
            : (debug_mode > 2)
            ? 0
            : debug_mode;
        break;
    default: break;
    }

    if(pad_pressed(PAD_CROSS) && (data->selection == 5)) {
        scene_change(SCREEN_TITLE);
    }
}

void
_draw_slider(int16_t vx, int16_t vy, int16_t width, int16_t value)
{
    // Calculate indicator position
    // indicator_x = vx + (value / 0x3fff)
    // p = width * value / 3fff
    // (width / 3fff) * value
    uint32_t w = ((uint32_t)width) << 12;
    uint32_t v = ((uint32_t)value) << 12;
    uint32_t vunit = (w << 12) / 0x3fff000;
    uint32_t vrpos = (v * vunit) >> 12;
    int16_t pos = vx + (int16_t)(vrpos >> 12);

    // Draw indicator
    font_draw_big("I", pos, vy - 1);

    // Draw background
    TILE *tile = (TILE *)get_next_prim();
    increment_prim(sizeof(TILE));
    setTile(tile);
    setRGB0(tile, 0, 0, 0);
    setXY0(tile, vx, vy);
    setWH(tile, width, 8);
    sort_prim(tile, OTZ_LAYER_HUD);
}

void
_draw_control(int16_t vy, const char *caption, int16_t value, uint8_t selected)
{
    char buffer[4];

    // 1% of 0x3fff is roughly 0xa3
    uint16_t perc = value / 0xa3;
    snprintf(buffer, 4, "%3u", perc);
    uint16_t volsz = font_measurew_sm(buffer);

    if(selected) font_set_color(128, 128, 0);
    font_draw_sm(caption, 15, vy + 1);
    _draw_slider(150, vy, 120, value);
    font_draw_sm(buffer, SCREEN_XRES - 15 - volsz, vy + 1);
    font_reset_color();
}

void
screen_options_draw(void *d)
{
    screen_options_data *data = (screen_options_data *)d;

    static const char *title = "OPTIONS";
    uint16_t txtw;

    font_set_color(128, 128, 128);
    uint16_t text_hsize = font_measurew_big(title) >> 1;
    uint16_t text_xpos = CENTERX - text_hsize;
    font_draw_big(title, text_xpos, SCREEN_YRES >> 3);

    _draw_control(60,  "Master Volume", data->master_volume, data->selection == 0);
    _draw_control(80,  "Background Music", data->bgm_volume, data->selection == 1);
    _draw_control(100, "Sound Effects", data->sfx_volume, data->selection == 2);

    if(data->selection == 3) font_set_color(128, 128, 0);
    font_draw_sm("Music Mode", 15, 120);
    const char *sndtxt =
        (sound_cdda_get_stereo() == 1)
        ? "Stereo"
        : (sound_cdda_get_stereo() == 2)
        ? "Reversed Stereo"
        : "Mono";
    txtw = font_measurew_sm(sndtxt);
    font_draw_sm(sndtxt, SCREEN_XRES - 15 - txtw, 120);
    font_reset_color();

    if(data->selection == 4) font_set_color(128, 128, 0);
    font_draw_sm("Debug Mode", 15, 140);

    const char *dbgtxt =
        (debug_mode == 0)
        ? "Off"
        : (debug_mode == 1)
        ? "Basic Debug"
        : "Collision Debug";
    txtw = font_measurew_sm(dbgtxt);
    font_draw_sm(dbgtxt, SCREEN_XRES - 15 - txtw, 140);
    font_reset_color();

    if(data->selection == 5) font_set_color(128, 128, 0);
    font_draw_sm("Back", 270, 200);
    font_reset_color();

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
