#include "screens/title.h"
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <inline_c.h>

#include "util.h"
#include "render.h"
#include "input.h"
#include "screen.h"
#include "sound.h"
#include "timer.h"
#include "basic_font.h"

#include "screens/level.h"

#define AUTODEMO_WAIT_FRAMES 1620

#ifdef ALLOW_DEBUG
extern int      debug_mode;
#endif
extern uint32_t level_score_count;
extern int      campaign_finished;

uint16_t demo_number = 6;
uint8_t  demo_character = CHARA_SONIC;

typedef struct {
    int32_t prect_x;
    int32_t prect_y;
    uint8_t mode;
} texture_props;

static const char *title_text[] = {
    "PRESS START",

    "PLAY TEST LEVELS",
    "START CAMPAIGN",
    "LEVEL SELECT",
    "OPTIONS",
};

#define MENU_MAX_OPTION 4

/* Parallax data */
#define PRL_INFO_PER_PIECE 6
#define PRL_NUM_PIECES     7
static const int16_t prl_data[] = {
    //  W    H  v0  spd      ypos  single
    // Island, immovable
    256, 48, 62, 0x0000,  170,  1,
    // Cloud data
    256, 37, 25, 0x019a,  181,  0,
    // Water data
    64,   6,  0, 0x0666,  216,  0,
    64,   5,  6, 0x0800,  222,  0,
    62,   5, 11, 0x099a,  227,  0,
    64,   5, 16, 0x0b33,  232,  0,
    66,   4, 20, 0x0ccd,  237,  0,
};

extern SoundEffect sfx_switch;
extern SoundEffect sfx_ring;

typedef struct {
    texture_props props_title;
    texture_props props_prl;
    texture_props props_cld;
    uint8_t rgb_count;
    uint8_t menu_option;
    uint8_t selected;
    uint8_t next_scene;
    int32_t prl_pos[PRL_NUM_PIECES];

    SVECTOR rot;
    VECTOR  pos;
    VECTOR  scale;
    MATRIX  world;

    uint32_t autodemo_timer;

    uint16_t code_state[5];

    /* Model planet; */
} screen_title_data;

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
    title_load_texture("\\SPRITES\\TITLE\\CLD.TIM;1", &data->props_cld);

    data->rot   = (SVECTOR) { 0 };
    data->pos   = (VECTOR)  { 0, 0, 450 };
    data->scale = (VECTOR)  { ONE, ONE, ONE };
    data->world = (MATRIX)  { 0 };

    data->pos.vx = 0xfffffe7a;
    data->pos.vy = 0xffffffb6;
    data->rot.vx = 0x00000320;

    data->rgb_count = 0;
    data->menu_option = 0;
    data->selected = 0;
    data->next_scene = 0;

    data->autodemo_timer = 0;

    bzero(data->code_state, 5 * sizeof(uint16_t));

    bzero(data->prl_pos, PRL_NUM_PIECES * sizeof(int32_t));
    data->prl_pos[0] = 32 << 12; // Island center

    // Planet model
    /* load_model(&data->planet, "\\MODELS\\COMMON\\RING.MDL;1"); */
    /* data->planet.rot.vx = 0x478; */
    /* data->planet.pos.vz = 4288; */
    /* data->planet.pos.vx = 2048; */
    /* data->planet.pos.vy = -1280; */
    /* data->planet.scl.vx = */
    /*     data->planet.scl.vy = */
    /*     data->planet.scl.vz = 2048; */

    set_clear_color(0, 0, 0);

    printf("Commit: %s:%s\n", GIT_SHA1, GIT_REFSPEC);
    sound_bgm_play(BGM_TITLESCREEN);
}

void
screen_title_unload(void *d)
{
    (void)(d);
    sound_cdda_stop();
    screen_free();
}

void
screen_title_update(void *d)
{
#ifdef ALLOW_DEBUG
    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }
#endif

    screen_title_data *data = (screen_title_data *)d;

    data->pos.vx -= 1;
    if(data->pos.vx < -646) {
        data->pos.vx = -390;
    }

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
            if(data->menu_option == 0) {
                // Play AutoDemo if you took too long!
                // Wait until music stops playing.
                data->autodemo_timer++;
                if(data->autodemo_timer > AUTODEMO_WAIT_FRAMES) {
                    data->selected = 1;
                }

                // Input cheat codes. Buffer has 5 positions.
                // Input is always pushed at end
                {
                    uint16_t current;
                    if((current = pad_pressed_any())) {
                        // Move everyone backwards
                        for(int i = 1; i < 5; i++) {
                            data->code_state[i-1] = data->code_state[i];
                        }
                        data->code_state[4] = current;
                    }
                }

                if(pad_pressed(PAD_START)) {
                    // Check for campaign unlock code.
                    // UP, DOWN, LEFT, RIGHT, SQUARE+START
                    if((data->code_state[0] == PAD_UP)
                       && (data->code_state[1] == PAD_DOWN)
                       && (data->code_state[2] == PAD_LEFT)
                       && (data->code_state[3] == PAD_RIGHT)
                       && (data->code_state[4] == PAD_SQUARE)
                       && pad_pressing(data->code_state[4]))
                    {
                        campaign_finished = 1;
                        sound_play_vag(sfx_ring, 0);
                    }
                    
                    // TODO: Check for saved data?
                    data->menu_option = 2; // New Game
                    //data->menu_option = 1; // Continue
                }
            } else if(data->menu_option > 0) {
                if(pad_pressed(PAD_LEFT) && (data->menu_option > 1)) {
                    sound_play_vag(sfx_switch, 0);
                    data->menu_option--;

                    // Jump over level select if campaign was not finished
                    if(data->menu_option == 3 && !campaign_finished)
                        data->menu_option--;
                } else if(pad_pressed(PAD_RIGHT)
                          && (data->menu_option < MENU_MAX_OPTION)) {
                    sound_play_vag(sfx_switch, 0);
                    data->menu_option++;

                    // Jump over level select if campaign was not finished
                    if(data->menu_option == 3 && !campaign_finished)
                        data->menu_option++;
                }

                if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
                    data->selected = 1;
                    switch(data->menu_option) {
                    case 1: // Engine Test
                        screen_title_reset_demo();
                        // Go to Engine Test
                        screen_level_setlevel(0);
                        screen_level_setmode(LEVEL_MODE_NORMAL);
                        screen_level_setcharacter(CHARA_SONIC);
                        data->next_scene = SCREEN_CHARSELECT;
                        level_score_count = 0;
                        break;
                    case 2: // New Game
                        screen_title_reset_demo();
                        // Use Surely Wood Zone 1 as first level
                        screen_level_setlevel(6);
                        screen_level_setmode(LEVEL_MODE_NORMAL);
                        screen_level_setcharacter(CHARA_SONIC);
                        data->next_scene = SCREEN_CHARSELECT;
                        level_score_count = 0;
                        break;
                    case 3: // Level Select
                        data->next_scene = SCREEN_LEVELSELECT;
                        break;
                    case 4: // Options
                        data->next_scene = SCREEN_OPTIONS;
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
        if(data->autodemo_timer >= AUTODEMO_WAIT_FRAMES) {
            screen_level_setlevel(demo_number);
            screen_level_setcharacter(demo_character);
            screen_level_setmode(LEVEL_MODE_DEMO);
            level_score_count = 0;
            // Cycle to next demo if we ever come back here
            screen_title_cycle_demo();
            scene_change(SCREEN_LEVEL);
        } else scene_change(data->next_scene);
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
    sort_prim(poly, OTZ_LAYER_LEVEL_FG_FRONT);
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
            setRGB0(poly, data->rgb_count, data->rgb_count, data->rgb_count);
            poly->tpage = getTPage(
                data->props_prl.mode & 0x3,
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
            sort_prim(poly, OTZ_LAYER_LEVEL_FG_BACK);

            if(s) break;
        }
    }
}

static void
screen_title_drawcld(screen_title_data *data)
{
    VECTOR pos = data->pos;
    for(; pos.vx < 0x000000c8 + (256 * 2); pos.vx += 256) {
        TransMatrix(&data->world, &pos);
        RotMatrix(&data->rot, &data->world);
        ScaleMatrix(&data->world, &data->scale);
        gte_SetRotMatrix(&data->world);
        gte_SetTransMatrix(&data->world);

        POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
        setPolyFT4(poly);
        setRGB0(poly, data->rgb_count, data->rgb_count, data->rgb_count);
        poly->tpage = getTPage(
            data->props_cld.mode & 0x3,
            0,
            data->props_cld.prect_x,
            data->props_cld.prect_y);
        poly->clut = 0;
        setUV4(poly,
               0, 0,
               255, 0,
               0, 255,
               255, 255);

        static const SVECTOR vertices[] = {
            {-128, -128, 0, 0},
            { 128, -128, 0, 0},
            {-128,  128, 0, 0},
            { 128,  128, 0, 0},
        };

        int nclip, otz;
        nclip = RotAverageNclip4(
            (SVECTOR *)&vertices[0],
            (SVECTOR *)&vertices[1],
            (SVECTOR *)&vertices[2],
            (SVECTOR *)&vertices[3],
            (uint32_t *)&poly->x0,
            (uint32_t *)&poly->x1,
            (uint32_t *)&poly->x2,
            (uint32_t *)&poly->x3,
            &otz);

        if((nclip > 0) && (otz > 0) && (otz < OT_LENGTH)) {
            sort_prim(poly, otz);
            increment_prim(sizeof(POLY_FT4));
        }
    }
}

void
screen_title_draw(void *d)
{   
    screen_title_data *data = (screen_title_data *)d;
    set_clear_color(LERPC(data->rgb_count, 56),
                    LERPC(data->rgb_count, 104),
                    LERPC(data->rgb_count, 200));

#ifdef ALLOW_DEBUG
    if(debug_mode) {
        char buffer[80];
        snprintf(buffer, 120,
                 "DBG %1u\n"
                 "%-29s\n"
                 "%4s %3d Hz\n"
                 "Build Date %11s %8s\n"
                 "CD Tracks: %d",
                 debug_mode,
                 GIT_COMMIT,
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
                 get_frame_rate(),
                 __DATE__, __TIME__,
                 sound_cdda_get_num_tracks());
        font_draw_sm(buffer, 8, 12);
        draw_quad(0, 0, SCREEN_XRES, 75,
                  0, 0, 0, 1,
                  OTZ_LAYER_LEVEL_FG_FRONT);
    }
#endif
    
    screen_title_drawtitle(data);
    screen_title_drawprl(data);
    screen_title_drawcld(data);
    /* render_model(&data->planet); */

    if(data->rgb_count >= 128) {
        const char *menutxt = title_text[data->menu_option];
        uint16_t txt_hsize = font_measurew_big(menutxt) >> 1;
        int16_t vx = CENTERX - txt_hsize;
        font_draw_big(menutxt, vx, 200);
        if(data->menu_option > 0) {
            font_set_color(0xc8, 0xc8, 0x00);
            if(data->menu_option > 1)
                font_draw_big("<", CENTERX - txt_hsize - 22, 200);
            if(data->menu_option < 4)
                font_draw_big(">", CENTERX + txt_hsize + 14, 200);
        }
        font_reset_color();
    }

    int16_t x;

    font_set_color(
        LERPC(data->rgb_count, 200),
        LERPC(data->rgb_count, 200),
        LERPC(data->rgb_count, 200));

    const char *text = GIT_VERSION;

    x = SCREEN_XRES - font_measurew_sm(text) - 8;
    font_draw_sm(GIT_VERSION, x, SCREEN_YRES - 21);

    text = "2025 luksamuk";
    x = SCREEN_XRES - font_measurew_sm(text) - 8;
    font_draw_sm(text, x, SCREEN_YRES - 14);
}

void
screen_title_reset_demo()
{
    demo_number = 6;
}

void
screen_title_cycle_demo()
{
    switch(demo_number) {
    /* case 0:  demo_number = 4;  break; */
    case 4:
        // Go to Surely Wood Zone 1, Sonic
        demo_character = CHARA_SONIC;
        demo_number = 6;
        break;
    case 6:
        // Go to Amazing Ocean Zone, Tails
        demo_character = CHARA_MILES;
        demo_number = 10;
        break;
    case 10:
        // Go to Green Hill Zone, Knuckles
        demo_character = CHARA_KNUCKLES;
        demo_number = 4;
        break;
    /* case 16: demo_number = 0;  break; */
    /* default: demo_number = 0;  break; */
    default:
        // Go to Surely Wood Zone 1, Sonic
        demo_character = CHARA_SONIC;
        demo_number = 6;
        break;
    }
}
