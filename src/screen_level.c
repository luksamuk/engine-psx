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

extern int debug_mode;

static uint8_t level = 0;
static uint8_t paused = 0;
static uint8_t music_channel = 0;

static Player player;

TileMap16  map16;
TileMap128 map128;
LevelData  leveldata;
Camera     camera;

#define CHANNELS_PER_BGM    3
static uint32_t bgm_loop_sectors[] = {
    4000,
    4800,
    3230,

    11000,
};


/* Related to rotating cube on background */
static int dx = 1,  dy = 1;

static SVECTOR vertices[] = {
    { -64, -64, -64, 0 },
    {  64, -64, -64, 0 },
    {  64, -64,  64, 0 },
    { -64, -64,  64, 0 },
    { -64,  64, -64, 0 },
    {  64,  64, -64, 0 },
    {  64,  64,  64, 0 },
    { -64,  64,  64, 0 }
};

static short faces[] = {
    2, 1, 3, 0, // top
    1, 5, 0, 4, // front
    5, 6, 4, 7, // bottomn
    2, 6, 1, 5, // right
    7, 6, 3, 2, // back
    7, 3, 4, 0  // left
};

static SVECTOR rotation = { 0 };
static VECTOR  pos      = { 0, 0, 450 };
static VECTOR  scale    = { ONE >> 1, ONE >> 1, ONE >> 1 };
static MATRIX  world    = { 0 };
/* -------------------------------------- */


// Forward function declarations
static void level_load_player();
static void level_load_level();

void screen_level_load()
{
    level_load_player();
    level_load_level();
    camera_set(&camera, player.pos.vx, player.pos.vy);
}

void screen_level_unload()
{
    sound_stop_xa();
    level_reset();
    sound_reset_mem();
}

void screen_level_update()
{
    if(pad_pressed(PAD_START)) paused = !paused;
 
    if(paused) {
        if(pad_pressed(PAD_SELECT)) {
            scene_change(SCREEN_LEVELSELECT);
        }
        return;
    }

    /* Rotating cube */
    if(pos.vx < -CENTERX || pos.vx > CENTERX) dx = -dx;
    if(pos.vy < -CENTERY || pos.vy > CENTERY) dy = -dy;
    pos.vx += dx;
    pos.vy += dy;

    rotation.vx += 6;
    rotation.vy -= 8;
    rotation.vz -= 12;
    /* --------- */

    if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
       (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
        debug_mode = (debug_mode + 1) % 3;
    }

    if(pad_pressed(PAD_SELECT)) {
        player.pos = (VECTOR){ 250 << 12, CENTERY << 12, 0 };
        player.grnd = 0;
        player.anim_dir = 1;
        player.vel.vx = player.vel.vy = player.vel.vz = 0;
    }

    camera_update(&camera, &player);

    player_update(&player);
}

void screen_level_draw()
{
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

    render_lvl(&leveldata, &map128, &map16, camera.pos.vx, camera.pos.vy);

    // Gouraud-shaded cube
    RotMatrix(&rotation, &world);
    TransMatrix(&world, &pos);
    ScaleMatrix(&world, &scale);
    gte_SetRotMatrix(&world);
    gte_SetTransMatrix(&world);

    for(int i = 0; i < 24; i += 4) {
        int nclip, otz;
        POLY_G4 *poly = (POLY_G4 *) get_next_prim();
        setPolyG4(poly);
        setRGB0(poly, 96,   0,   0);
        setRGB1(poly,   0, 96,   0);
        setRGB2(poly,   0,   0, 96);
        setRGB3(poly, 96, 96,   0);

        nclip = RotAverageNclip4(
            &vertices[faces[i]],
            &vertices[faces[i + 1]],
            &vertices[faces[i + 2]],
            &vertices[faces[i + 3]],
            (uint32_t *)&poly->x0,
            (uint32_t *)&poly->x1,
            (uint32_t *)&poly->x2,
            (uint32_t *)&poly->x3,
            &otz);

        if((nclip > 0) && (otz > 0) && (otz < OT_LENGTH)) {
            sort_prim(poly, otz);
            increment_prim(sizeof(POLY_G4));
        }
    }

    // Pause text
    if(paused) {
        const char *line1 = "Paused";
        int16_t x = CENTERX - (strlen(line1) * 4);
        draw_text(x, CENTERY - 12, 0, line1);
    }

    // Sound and video debug
    //uint32_t elapsed_sectors;
    //sound_xa_get_elapsed_sectors(&elapsed_sectors);
    snprintf(buffer, 255,
             "%4s %3d\n"
             //"%08u"
             ,
             GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
             get_frame_rate()
             //elapsed_sectors
        );
    draw_text(248, 12, 0, buffer);

    // Player debug
    if(debug_mode) {
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
    set_clear_color(63, 0, 127);
    paused = 0;

    char basepath[255];
    char filename0[255], filename1[255];

    uint8_t round = level >> 1;

    snprintf(basepath, 255, "\\LEVELS\\R%1u", round);

    TIM_IMAGE tim;
    uint32_t filelength;

    snprintf(filename0, 255, "%s\\TILES.TIM;1", basepath);

    printf("Loading %s...\n", filename0);
    uint8_t *timfile = file_read(filename0, &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    } else {
        // If not single "TILES.TIM" was found, then perharps try a
        // "TILES0.TIM" and a "TILES1.TIM".
        snprintf(filename0, 255, "%s\\TILES0.TIM;1", basepath);
        printf("Loading %s...\n", filename0);
        timfile = file_read(filename0, &filelength);
        if(timfile) {
            load_texture(timfile, &tim);
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
    load_map(&map16, filename0, filename1);
    snprintf(filename0, 255, "%s\\MAP128.MAP;1", basepath);
    printf("Loading %s...\n", filename0);
    load_map(&map128, filename0, NULL);

    snprintf(filename0, 255, "%s\\Z%1u.LVL;1", basepath, (level & 0x01) + 1);
    printf("Loading %s...\n", filename0);
    load_lvl(&leveldata, filename0);

    level_debrief();

    camera_init(&camera);

    // Level 0 Zone 2 actually has background sound effects
    /* if(level == 1) { */
    /*     SoundEffect sfx_rain = sound_load_vag("\\SFX\\RAIN.VAG;1"); */
    /*     sound_play_vag(sfx_rain, 1); */
    /* } */

    // Start playback after we don't need the CD anymore.
    sound_stop_xa();

    snprintf(filename0, 255, "\\BGM\\BGM%03u.XA;1", (level / CHANNELS_PER_BGM) + 1);
    music_channel = level % CHANNELS_PER_BGM;

    sound_play_xa(filename0, 0, music_channel, bgm_loop_sectors[level]);
}
