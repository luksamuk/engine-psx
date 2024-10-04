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

#define CHANNELS_PER_BGM    3
static uint32_t bgm_loop_sectors[] = {
    4000,
    4800,
    3230,

    7300,
    7300,
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

// TODO!!!
typedef struct {
    /* Model ring; */
} screen_level_data;

void
screen_level_load()
{
    screen_level_data *data = screen_alloc(sizeof(screen_level_data));
    /* load_model(&data->ring, "\\MODELS\\COMMON\\RING.MDL"); */

    /* data->ring.pos.vz = 0x12c0; */
    /* data->ring.rot.vx = 0x478; */
    /* data->ring.scl.vx = data->ring.scl.vy = data->ring.scl.vz = 0x200; */

    camera_init(&camera);

    level_load_player();
    level_load_level();
    camera_set(&camera, player.pos.vx, player.pos.vy);

    reset_elapsed_frames();
}

void
screen_level_unload(void *)
{
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

    if(pad_pressed(PAD_START)) paused = !paused;
    
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

    if(pad_pressed(PAD_SELECT)) {
        player.pos = (VECTOR){ 250 << 12, CENTERY << 12, 0 };
        player.grnd = 0;
        player.anim_dir = 1;
        player.vel.vx = player.vel.vy = player.vel.vz = 0;
    }

    camera_update(&camera, &player);
    player_update(&player);
    update_obj_window(&leveldata, &obj_table_common, camera.pos.vx, camera.pos.vy);


    // FAKE LEVEL TRANSITION!!!
    if(level < 4) {
        if(player.pos.vx >= camera.max_x + (CENTERX << 12)) {
            screen_level_setlevel(level + 1);
            scene_change(SCREEN_LEVEL);
        }
    }
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
        draw_text(x, CENTERY - 12, 0, line1);
    }

    if(debug_mode) {
        // Video debug
        snprintf(buffer, 255,
                 "%4s %3d",
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
                 get_frame_rate());
        draw_text(248, 12, 0, buffer);

        if(debug_mode > 1) {
            // Sound debug
            uint32_t elapsed_sectors;
            sound_xa_get_elapsed_sectors(&elapsed_sectors);
            snprintf(buffer, 255, "%08u", elapsed_sectors);
            draw_text(248, 20, 0, buffer);

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
            draw_text(8, 12, 0, buffer);
        }
    }
}

/* ============================== */

void
screen_level_setlevel(uint8_t menuchoice)
{
    level = menuchoice;
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
    player.pos = (VECTOR){ 250 << 12, CENTERY << 12, 0 };
}

static void
level_load_level()
{
    paused = 0;

    char basepath[255];
    char filename0[255], filename1[255];

    uint8_t round = level >> 1;

    if(level == 4 || level == 5)
        set_clear_color(26, 104, 200); // GHZ placeholder
    else set_clear_color(63, 0, 127);

    snprintf(basepath, 255, "\\LEVELS\\R%1u", round);

    TIM_IMAGE tim;
    uint32_t filelength;

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

    // Load common objects
    printf("Loading common object texture...\n");
    file_read("\\LEVELS\\COMMON\\OBJ.TIM;1", &filelength);
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


    // Pre-allocate and initialize level primitive buffer
    prepare_renderer(&leveldata);

    level_debrief();

    // Level 0 Zone 2 actually has background sound effects
    /* if(level == 1) { */
    /*     SoundEffect sfx_rain = sound_load_vag("\\SFX\\RAIN.VAG;1"); */
    /*     sound_play_vag(sfx_rain, 1); */
    /* } */

    // Start playback after we don't need the CD anymore.
    snprintf(filename0, 255, "\\BGM\\BGM%03u.XA;1", (level / CHANNELS_PER_BGM) + 1);
    music_channel = level % CHANNELS_PER_BGM;

    sound_play_xa(filename0, 0, music_channel, bgm_loop_sectors[level]);
}
