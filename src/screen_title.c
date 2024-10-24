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
/* #include "model.h" */
#include "timer.h"

#include "screens/fmv.h"
#include "screens/level.h"

extern int debug_mode;

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

static const int16_t txt_data[] = {
    /* W H u0 v0 */
    // PRESS START
    86, 13, 0, 110,
    // CONTINUE
    60, 13, 86, 110,
    // NEW GAME
    65, 13, 146, 110,
    // LEVEL SELECT
    87, 13, 0, 123,
    // ARROW LEFT
    8, 13, 211, 110,
    // ARROW RIGHT
    8, 13, 219, 110,
};

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

    /* Model planet; */
} screen_title_data;

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

    printf("Commit: %s:%s\n", GIT_SHA1, GIT_REFSPEC);

    sound_play_xa("\\BGM\\MNU001.XA;1", 0, 1, 0);

    set_clear_color(0, 0, 0);
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
    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }

    screen_title_data *data = (screen_title_data *)d;

    uint32_t elapsed_sectors;
    sound_xa_get_elapsed_sectors(&elapsed_sectors);
    if(elapsed_sectors >= 950) sound_stop_xa();

    /* data->planet.rot.vz -= 24; */

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
    sort_prim(poly, 1);
}

static void
screen_title_drawtxt(screen_title_data *data, uint8_t idx, int16_t cx, int16_t cy)
{
    int16_t w  = txt_data[(idx << 2)];
    int16_t h  = txt_data[(idx << 2) + 1];
    int16_t u0 = txt_data[(idx << 2) + 2];
    int16_t v0 = txt_data[(idx << 2) + 3];

    int16_t x = cx - (w >> 1);
    int16_t y = cy - (h >> 1);

    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, 128, 128, 128);
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
           u0,         v0,
           u0 + w , v0,
           u0,         v0 + h,
           u0 + w , v0 + h);
    sort_prim(poly, 0);
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
            sort_prim(poly, 1);

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

    char buffer[255] = { 0 };

    if(debug_mode) {
        uint32_t elapsed_sectors;
        sound_xa_get_elapsed_sectors(&elapsed_sectors);
        FntPrint(-1, "%-29s %4s %3d\n",
                 GIT_COMMIT,
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
                 get_frame_rate());
        FntPrint(-1, "%11s %8s          %08u\n",
                 __DATE__, __TIME__, elapsed_sectors);
        FntFlush(-1);
    }


    
    screen_title_drawtitle(data);
    screen_title_drawprl(data);
    screen_title_drawcld(data);
    /* render_model(&data->planet); */

    if(data->rgb_count >= 128) {
        screen_title_drawtxt(data, data->menu_option, CENTERX, 208);
        if(data->menu_option > 0) {
            if(data->menu_option > 1)
                screen_title_drawtxt(data, 4, CENTERX - 60, 208);
            if(data->menu_option < 3)
                screen_title_drawtxt(data, 5, CENTERX + 60, 208);
        }

        int16_t x;

        snprintf(buffer, 255, "v0.1 Beta");
        x = SCREEN_XRES - (strlen(buffer) * 8) - 8;
        draw_text(x, SCREEN_YRES - 14, 0, buffer);
    }
}
