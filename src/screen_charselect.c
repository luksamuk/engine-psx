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

#define YRADIUS 26
#define XRADIUS 42

extern SoundEffect sfx_switch;
extern int campaign_finished;

typedef struct {
    int8_t   character;
    int32_t  bg_prect_x;
    int32_t  bg_prect_y;
    uint8_t  bg_mode;
    uint8_t  bg_frame;
    uint8_t  bg_state;
    uint16_t bg_timer;

    int32_t char_angles[4];
    VECTOR pos[4];
    int32_t alpha;
    int32_t alpha_target;
    int32_t alpha_speed;
} screen_charselect_data;

// TIM 15bpp -- TPAGE: 384x0
// Each frame is 112x127
// Sonic:      0x0
// Tails:    112x0
// Knuckles:   0x128
// Amy:      112x127
#define CHAR_WIDTH  112
#define CHAR_HEIGHT 126
#define CHAR_CHANGE_SPEED 40

static void
_calculate_positions(screen_charselect_data *data)
{
    for(int i = 0; i < 4; i++) {
        int32_t angle = (data->alpha % ONE) + data->char_angles[i];
        if(angle < 0) angle += ONE;
        data->pos[i].vx = (CENTERX << 12) + (XRADIUS * -rsin(angle));
        data->pos[i].vy = (CENTERY << 12) + (YRADIUS * rcos(angle));
    }
}

void
screen_charselect_load()
{
    screen_charselect_data *data = screen_alloc(sizeof(screen_charselect_data));
    data->character = screen_level_getcharacter();
    data->alpha_speed = 0;
    data->char_angles[0] = 0x0000;
    data->char_angles[1] = 0x0400;
    data->char_angles[2] = 0x0800;
    data->char_angles[3] = 0x0c00;
    data->alpha_target = data->alpha = data->char_angles[data->character];
    _calculate_positions(data);

    uint32_t length;
    TIM_IMAGE tim;
    uint8_t *img = file_read("\\MISC\\LVLSEL.TIM;1", &length);
    load_texture(img, &tim);
    data->bg_mode = tim.mode;
    data->bg_prect_x = tim.prect->x;
    data->bg_prect_y = tim.prect->y;
    free(img);

    img = file_read("\\MISC\\CHARAS.TIM;1", &length);
    load_texture(img, &tim);
    free(img);

    data->bg_frame = 0;
    data->bg_state = 0;
    data->bg_timer = BG_PAUSE;

    sound_bgm_play(BGM_TITLESCREEN);
}

void
screen_charselect_unload(void *d)
{
    (void)(d);
    sound_cdda_stop();
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

    if(data->alpha_speed == 0) {
        if(pad_pressed(PAD_CROSS) || pad_pressed(PAD_START)) {
            // Cannot select Amy if campaign is not finished
            if(!((data->character == CHARA_AMY) && !campaign_finished)) {
                screen_level_setcharacter(data->character);
                // NOTE: screen_level_setlevel should have
                // been called from previous screen
                screen_level_setmode(LEVEL_MODE_NORMAL);
                scene_change(SCREEN_LEVEL);
            }
        } else {
            uint8_t changed = 0;
            if(pad_pressing(PAD_RIGHT)) {
                data->alpha_speed = CHAR_CHANGE_SPEED;
                data->character++;
                changed = 1;
            }
            if(pad_pressing(PAD_LEFT)) {
                data->alpha_speed = -CHAR_CHANGE_SPEED;
                data->character--;
                changed = 1;
            }
            if(changed) {
                data->character = (data->character < 0) ? 3 : (data->character % 4);
                data->alpha_target = data->char_angles[data->character];
                sound_play_vag(sfx_switch, 0);
            }
        }
    } else {
        data->alpha += data->alpha_speed;
        if(data->alpha < 0) data->alpha += ONE;
        if(data->alpha >= ONE) data->alpha -= ONE;

        int32_t target_min = data->alpha_target - abs(data->alpha_speed);
        int32_t target_max = data->alpha_target + abs(data->alpha_speed);

        if((data->alpha >= target_min) && (data->alpha <= target_max)) {
            data->alpha_speed = 0;
            data->alpha = data->alpha_target;
        }
        _calculate_positions(data);
    }
}

const char *
_get_char_name(int8_t character)
{
    switch(character) {
    case CHARA_SONIC:    return "\asSONIC\r";
    case CHARA_MILES:    return "\atTAILS\r";
    case CHARA_KNUCKLES: return "\akKNUCKLES\r";
    case CHARA_AMY:
        return (!campaign_finished) ? "\r" : "\aaAMY\r";
    }
    return "\awWECHNIA\r";
}

const char *
_get_char_subtitle(int8_t character)
{
    switch(character) {
    case CHARA_SONIC:    return "\awThe Hedgehog\r";
    case CHARA_MILES:    return "\awMiles Prower\r";
    case CHARA_KNUCKLES: return "\awThe Echidna\r";
    case CHARA_AMY:
        return (!campaign_finished)
            ? ("Play main levels\n"
               "   to  unlock   ")
            : "\awRose\r";
    }
    return "\awUNKNOWN\r";
}

static void
_draw_character(uint8_t c, VECTOR *v)
{
    uint8_t intensity = ((v->vy >> 12) < CENTERY + (YRADIUS >> 1) + (YRADIUS >> 2)) ? 64 : 128;
    if((c == 1) && (!campaign_finished)) intensity = 0;
    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, intensity, intensity, intensity);
    setTPage(poly, 2, 0, 384, 0);
    poly->clut = 0;
    setXYWH(poly,
            (v->vx >> 12) - (CHAR_WIDTH >> 1),
            (v->vy >> 12) - (CHAR_HEIGHT >> 1),
            CHAR_WIDTH,
            CHAR_HEIGHT);
    switch(c) {
    default: setUVWH(poly,   0,   0, CHAR_WIDTH, CHAR_HEIGHT); break; // Sonic
    case 3:  setUVWH(poly, 112,   0, CHAR_WIDTH, CHAR_HEIGHT); break; // Tails
    case 2:  setUVWH(poly,   0, 128, CHAR_WIDTH, CHAR_HEIGHT); break; // Knux
    case 1:  setUVWH(poly, 112, 128, CHAR_WIDTH, CHAR_HEIGHT); break; // Amy
    }
    sort_prim(poly,
              ((v->vy >> 12) < CENTERY - (YRADIUS >> 1))
              ? OTZ_LAYER_LEVEL_FG_BACK
              : ((v->vy >> 12) < (CENTERY + (CENTERY >> 3)))
              ? OTZ_LAYER_UNDER_PLAYER
              : OTZ_LAYER_PLAYER);
}

void
screen_charselect_draw(void *d)
{
    screen_charselect_data *data = (screen_charselect_data *)d;

    static const char *title = "\awCHARACTER SELECT\r";

    font_set_color(128, 128, 128);
    uint16_t text_hsize = font_measurew_big(title) >> 1;
    uint16_t text_xpos = CENTERX - text_hsize;
    font_draw_big(title, text_xpos, SCREEN_YRES >> 3);

    // Draw character
    for(int i = 0; i < 4; i++) {
        _draw_character(i, &data->pos[i]);
    }

    const char *name = _get_char_name(data->character);
    const char *subtitle = _get_char_subtitle(data->character);
    text_hsize = font_measurew_md(name) >> 1;
    font_draw_md(name, CENTERX - text_hsize, (SCREEN_YRES - (CENTERY >> 1)));
    text_hsize = font_measurew_sm(subtitle) >> 1;
    font_draw_sm(subtitle, CENTERX - text_hsize, (SCREEN_YRES - (CENTERY >> 1)) + GLYPH_MD_WHITE_HEIGHT);
    

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
