#include "screens/level.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inline_c.h>
#include "util.h"
#include "player.h"
#include "camera.h"
#include "render.h"
#include "sound.h"
#include "input.h"
#include "screen.h"
#include "level.h"
#include "timer.h"
#include "model.h"
#include "object.h"
#include "parallax.h"
#include "basic_font.h"

extern int debug_mode;

static uint8_t level = 0;
static uint8_t music_channel = 0;

// Accessible in other source
Player      player;
uint8_t     paused = 0;
TileMap16   map16;
TileMap128  map128;
LevelData   leveldata;
Camera      camera;
ObjectTable obj_table_common;
uint8_t     level_fade;
uint8_t     level_ring_count;
uint32_t    level_score_count;
uint8_t     level_finished;

#define CHANNELS_PER_BGM    3
static uint32_t bgm_loop_sectors[] = {
    4000, // R0Z1
    4800, // R0Z2
    3230, // R1Z1
    4050, // R1Z2
    4050, // GHZ1
    4050, // GHZ2
    3250, // SWZ1
};


/* Related to rotating cube on background */
/* static int dx = 1,  dy = 1; */

/* static SVECTOR vertices[] = { */
/*     { -64, -64, -64, 0 }, */
/*     {  64, -64, -64, 0 }, */
/*     {  64, -64,  64, 0 }, */
/*     { -64, -64,  64, 0 }, */
/*     { -64,  64, -64, 0 }, */
/*     {  64,  64, -64, 0 }, */
/*     {  64,  64,  64, 0 }, */
/*     { -64,  64,  64, 0 } */
/* }; */

/* static short faces[] = { */
/*     2, 1, 3, 0, // top */
/*     1, 5, 0, 4, // front */
/*     5, 6, 4, 7, // bottomn */
/*     2, 6, 1, 5, // right */
/*     7, 6, 3, 2, // back */
/*     7, 3, 4, 0  // left */
/* }; */

/* static SVECTOR rotation = { 0 }; */
/* static VECTOR  pos      = { 0, 0, 450 }; */
/* static VECTOR  scale    = { ONE >> 1, ONE >> 1, ONE >> 1 }; */
/* static MATRIX  world    = { 0 }; */
/* -------------------------------------- */


// Forward function declarations
static void level_load_player();
static void level_load_level();
static void level_set_clearcolor();

typedef struct {
    uint8_t level_transition;
    Parallax parallax;
    uint8_t parallax_tx_mode;
    int32_t parallax_px;
    int32_t parallax_py;
    int32_t parallax_cx;
    int32_t parallax_cy;
} screen_level_data;

void
screen_level_load()
{
    screen_level_data *data = screen_alloc(sizeof(screen_level_data));
    data->level_transition = 0;
    /* load_model(&data->ring, "\\MODELS\\COMMON\\RING.MDL"); */

    /* data->ring.pos.vz = 0x12c0; */
    /* data->ring.rot.vx = 0x478; */
    /* data->ring.scl.vx = data->ring.scl.vy = data->ring.scl.vz = 0x200; */

    camera_init(&camera);

    level_load_player();
    level_load_level(data);
    camera_set(&camera, player.pos.vx, player.pos.vy);

    reset_elapsed_frames();

    level_fade = 0;

    level_ring_count = 0;
    /* level_score_count = 0; */
    level_finished = 0;
}

void
screen_level_unload(void *)
{
    level_fade = 0;
    sound_stop_xa();
    level_reset();
    sound_reset_mem();
    screen_free();
}

void
screen_level_update(void *d)
{
    screen_level_data *data = (screen_level_data *)d;

    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }

    level_set_clearcolor();

    // Manage fade in and fade out
    if(data->level_transition == 0) { // Fade in
        level_fade += 2;
        if(level_fade >= 128) {
            level_fade = 128;
            data->level_transition = 1;
        }
    } else if(data->level_transition == 2) { // Fade out
        level_fade -= 2;
        if(level_fade == 0) {
            data->level_transition = 3;
        }
    }

    if(pad_pressed(PAD_START) && !level_finished) {
        paused = !paused;
        if(paused) sound_xa_set_volume(0x00);
        else sound_xa_set_volume(XA_DEFAULT_VOLUME);
    }
    
    if(paused) {
        if(pad_pressed(PAD_SELECT)) {
            scene_change(SCREEN_LEVELSELECT);
        }

        if(debug_mode) {
            uint8_t updated = 0;
            if(pad_pressing(PAD_UP)) {
                player.pos.vy -= 40960;
                updated = 1;
            }

            if(pad_pressing(PAD_DOWN)) {
                player.pos.vy += 40960;
                updated = 1;
            }

            if(pad_pressing(PAD_LEFT)) {
                player.pos.vx -= 40960;
                updated = 1;
            }

            if(pad_pressing(PAD_RIGHT)) {
                player.pos.vx += 40960;
                updated = 1;
            }

            if(updated) {
                camera_update(&camera, &player);
            }
        }
        
        return;
    }

    /* data->ring.rot.vz += 36; */
    
    /* Rotating cube */
    /* if(pos.vx < -CENTERX || pos.vx > CENTERX) dx = -dx; */
    /* if(pos.vy < -CENTERY || pos.vy > CENTERY) dy = -dy; */
    /* pos.vx += dx; */
    /* pos.vy += dy; */

    /* rotation.vx += 6; */
    /* rotation.vy -= 8; */
    /* rotation.vz -= 12; */
    /* --------- */

    if(pad_pressed(PAD_SELECT) && !level_finished) {
        player.pos = player.startpos;
        player.grnd = 0;
        player.anim_dir = 1;
        player.vel.vx = player.vel.vy = player.vel.vz = 0;
    }

    camera_update(&camera, &player);
    update_obj_window(&leveldata, &obj_table_common, camera.pos.vx, camera.pos.vy);
    player_update(&player);


    /* // FAKE LEVEL TRANSITION!!! */
    /* if(level < 4) { */
    /*     if(player.pos.vx >= camera.max_x + (CENTERX << 12)) { */
    /*         screen_level_setlevel(level + 1); */
    /*         scene_change(SCREEN_LEVEL); */
    /*     } */
    /* } */
}

void
screen_level_draw(void *d)
{
    screen_level_data *data = (screen_level_data *)d;
    char buffer[255] = { 0 };

    if(abs((player.pos.vx - camera.pos.vx) >> 12) <= SCREEN_XRES
       && abs((player.pos.vy - camera.pos.vy) >> 12) <= SCREEN_YRES) {
        VECTOR player_canvas_pos = {
            player.pos.vx - camera.pos.vx + (CENTERX << 12),
            player.pos.vy - camera.pos.vy + (CENTERY << 12),
            0
        };
        player_draw(&player, &player_canvas_pos);
    }

        
    /* render_model(&data->ring); */

    render_lvl(&leveldata, &map128, &map16, &obj_table_common, camera.pos.vx, camera.pos.vy);
    parallax_draw(&data->parallax, &camera);

    // Gouraud-shaded cube
    /* RotMatrix(&rotation, &world); */
    /* TransMatrix(&world, &pos); */
    /* ScaleMatrix(&world, &scale); */
    /* gte_SetRotMatrix(&world); */
    /* gte_SetTransMatrix(&world); */

    /* for(int i = 0; i < 24; i += 4) { */
    /*     int nclip, otz; */
    /*     POLY_G4 *poly = (POLY_G4 *) get_next_prim(); */
    /*     setPolyG4(poly); */
    /*     setRGB0(poly, 96,   0,   0); */
    /*     setRGB1(poly,   0, 96,   0); */
    /*     setRGB2(poly,   0,   0, 96); */
    /*     setRGB3(poly, 96, 96,   0); */

    /*     nclip = RotAverageNclip4( */
    /*         &vertices[faces[i]], */
    /*         &vertices[faces[i + 1]], */
    /*         &vertices[faces[i + 2]], */
    /*         &vertices[faces[i + 3]], */
    /*         (uint32_t *)&poly->x0, */
    /*         (uint32_t *)&poly->x1, */
    /*         (uint32_t *)&poly->x2, */
    /*         (uint32_t *)&poly->x3, */
    /*         &otz); */

    /*     if((nclip > 0) && (otz > 0) && (otz < OT_LENGTH)) { */
    /*         sort_prim(poly, otz); */
    /*         increment_prim(sizeof(POLY_G4)); */
    /*     } */
    /* } */

    // Pause text
    if(paused) {
        const char *line1 = "Paused";
        int16_t x = CENTERX - (strlen(line1) * 4);
        font_set_color(
            LERPC(level_fade, 0xc8),
            LERPC(level_fade, 0xc8),
            LERPC(level_fade, 0xc8));
        font_draw_big(line1, x, CENTERY - 12);
    }

    // Heads-up display
    if(debug_mode <= 1) {
        font_set_color(
            LERPC(level_fade, 0xc8),
            LERPC(level_fade, 0xc8),
            0);
        font_draw_big("SCORE", 10, 10);
        font_draw_big("TIME",  10, 24);
        font_draw_big("RINGS", 10, 38);
        font_set_color(
            LERPC(level_fade, 0xc8),
            LERPC(level_fade, 0xc8),
            LERPC(level_fade, 0xc8));

        snprintf(buffer, 255, "%8d", level_score_count);
        font_draw_big(buffer, 60, 10);

        {
            uint32_t seconds = get_elapsed_frames() / 60;
            snprintf(buffer, 255, "%2d:%02d", seconds / 60, seconds % 60);
            font_draw_big(buffer, 54, 24);
        }

        snprintf(buffer, 255, "%3d", level_ring_count);
        font_draw_big(buffer, 60, 38);

        font_reset_color();
    }

    if(debug_mode) {
        font_set_color(0xc8, 0xc8, 0xc8);

        // Video debug
        snprintf(buffer, 255,
                 "%4s %3d",
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
                 get_frame_rate());
        font_draw_sm(buffer, 248, 12);
        /* draw_text(248, 12, 0, buffer); */

        if(debug_mode > 1) {
            // Sound debug
            uint32_t elapsed_sectors;
            sound_xa_get_elapsed_sectors(&elapsed_sectors);
            snprintf(buffer, 255, "%08u", elapsed_sectors);
            font_draw_sm(buffer, 248, 20);
            /* draw_text(248, 20, 0, buffer); */

            // Player debug
            snprintf(buffer, 255,
                     "GSP %08x\n"
                     "SPD %08x %08x\n"
                     "ANG %04x\n"
                     "POS %08x %08x\n"
                     "ACT %02u\n"
                     "REV %08x\n"
                     ,
                     player.vel.vz,
                     player.vel.vx, player.vel.vy,
                     player.angle,
                     player.pos.vx, player.pos.vy,
                     player.action,
                     player.spinrev
                );
            font_draw_sm(buffer, 8, 12);
        }
    }
}

/* ============================== */

void
screen_level_setlevel(uint8_t menuchoice)
{
    level = menuchoice;
}

uint8_t
screen_level_getlevel(void)
{
    return level;
}


/* ============================== */

static void
level_load_player()
{
    uint32_t filelength;
    TIM_IMAGE tim;
    uint8_t *timfile = file_read("\\SPRITES\\SONIC.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }

    load_player(&player, "\\SPRITES\\SONIC.CHARA;1", &tim);
    player.startpos = (VECTOR){ 250 << 12, CENTERY << 12, 0 };
    player.pos = player.startpos;
}

static void
level_load_level(screen_level_data *data)
{
    paused = 0;

    char basepath[255];
    char filename0[255], filename1[255];

    uint8_t round = level >> 1;

    level_set_clearcolor();

    snprintf(basepath, 255, "\\LEVELS\\R%1u", round);

    TIM_IMAGE tim;
    uint32_t filelength;

    /* === LEVEL TILES === */
    // Load level tiles
    snprintf(filename0, 255, "%s\\TILES.TIM;1", basepath);
    printf("Loading %s...\n", filename0);
    uint8_t *timfile = file_read(filename0, &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        leveldata.clutmode = tim.mode;
        free(timfile);
    } else {
        // If not single "TILES.TIM" was found, then perharps try a
        // "TILES0.TIM" and a "TILES1.TIM".
        snprintf(filename0, 255, "%s\\TILES0.TIM;1", basepath);
        printf("Loading %s...\n", filename0);
        timfile = file_read(filename0, &filelength);
        if(timfile) {
            load_texture(timfile, &tim);
            leveldata.clutmode = tim.mode; // Use CLUT mode from 1st texture
            free(timfile);
        }

        snprintf(filename0, 255, "%s\\TILES1.TIM;1", basepath);
        printf("Loading %s...\n", filename0);
        timfile = file_read(filename0, &filelength);
        if(timfile) {
            load_texture(timfile, &tim);
            free(timfile);
        }
    }

    /* === PARALLAX === */
    // Load level parallax textures
    snprintf(filename0, 255, "%s\\BG0.TIM;1", basepath);
    printf("Loading %s...\n", filename0);
    timfile = file_read(filename0, &filelength);
    if(timfile) {
        load_texture(timfile, &tim);

        // Background compression must be the same for both background
        // images. Also, they must be one texture page apart (64 bytes)
        // horizontally, and their clut must be vertically aligned
        data->parallax_tx_mode = tim.mode;
        data->parallax_px = 448;
        data->parallax_py = 256;
        data->parallax_cx = 0;
        data->parallax_cy = 483;
        
        free(timfile);
    } else printf("Warning: Level BG0 not found, ignoring\n");

    snprintf(filename0, 255, "%s\\BG1.TIM;1", basepath);
    printf("Loading %s...\n", filename0);
    timfile = file_read(filename0, &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    } else printf("Warning: Level BG1 not found, ignoring\n");

    // Load level parallax data
    snprintf(filename0, 255, "%s\\PRL.PRL;1", basepath);
    printf("Loading parallax data...\n");
    load_parallax(&data->parallax, filename0,
                  data->parallax_tx_mode,
                  data->parallax_px, data->parallax_py,
                  data->parallax_cx, data->parallax_cy);
    printf("Loaded parallax strips: %d\n", data->parallax.num_strips);

    snprintf(filename0, 255, "%s\\MAP16.MAP;1", basepath);
    snprintf(filename1, 255, "%s\\MAP16.COL;1", basepath);
    printf("Loading %s and %s...\n", filename0, filename1);
    load_map16(&map16, filename0, filename1);
    snprintf(filename0, 255, "%s\\MAP128.MAP;1", basepath);
    printf("Loading %s...\n", filename0);
    load_map128(&map128, filename0);

    snprintf(filename0, 255, "%s\\Z%1u.LVL;1", basepath, (level & 0x01) + 1);
    printf("Loading %s...\n", filename0);
    load_lvl(&leveldata, filename0);

    /* === OBJECTS === */
    // Load common objects
    printf("Loading common object texture...\n");
    timfile = file_read("\\LEVELS\\COMMON\\OBJ.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }

    printf("Loading common object table...\n");
    load_object_table("\\LEVELS\\COMMON\\OBJ.OTD;1", &obj_table_common);

    // Load level objects
    // TODO

    // Load object positioning on level
    snprintf(filename0, 255, "%s\\Z%1u.OMP;1", basepath, (level & 0x01) + 1);
    load_object_placement(filename0, &leveldata);


    /* === RENDERING PREPARATION === */
    // Pre-allocate and initialize level primitive buffer
    prepare_renderer(&leveldata);

    level_debrief();

    // Start playback after we don't need the CD anymore.
    if(level == 0) { // R0Z1
        snprintf(filename0, 255, "\\BGM\\BGM001.XA;1");
        music_channel = 0;
    } else if(level == 1) { // R0Z2
        snprintf(filename0, 255, "\\BGM\\BGM001.XA;1");
        music_channel = 1;
    } else if(level == 2) { // R1Z1
        snprintf(filename0, 255, "\\BGM\\BGM001.XA;1");
        music_channel = 2;
    } else if(level == 3) { // R1Z2
        snprintf(filename0, 255, "\\BGM\\BGM002.XA;1");
        music_channel = 0;
    } else if(level == 4 || level == 5) { // GHZ1 / GHZ2
        snprintf(filename0, 255, "\\BGM\\BGM002.XA;1");
        music_channel = 1;
    } else if(level == 6) { // SWZ1
        // El Gato Battle 2 Vortex Remake by pkVortex
        // https://www.youtube.com/watch?v=ZU-MGiM5YlA
        snprintf(filename0, 255, "\\BGM\\BGM002.XA;1");
        music_channel = 2;
    } else {
        // Do not play anything
        return;
    }

    printf("Number of level layers: %d\n", leveldata.num_layers);

    sound_play_xa(filename0, 0, music_channel, bgm_loop_sectors[level]);
}


static void
level_set_clearcolor()
{
    if(level == 2 || level == 3) // R1
        set_clear_color(LERPC(level_fade, 26), LERPC(level_fade, 104), LERPC(level_fade, 200));
    else if(level == 4 || level == 5) // R2 (GHZ)
        set_clear_color(LERPC(level_fade, 36), LERPC(level_fade, 0), LERPC(level_fade, 180));
    else if(level == 6 || level == 7)
        set_clear_color(0, 0, 0);
    // R0
    else set_clear_color(LERPC(level_fade, 63), LERPC(level_fade, 0), LERPC(level_fade, 127));
}

void
screen_level_setstate(uint8_t state)
{
    screen_level_data *data = screen_get_data();
    data->level_transition = state;
}

uint8_t
screen_level_getstate()
{
    screen_level_data *data = screen_get_data();
    return data->level_transition;
}
