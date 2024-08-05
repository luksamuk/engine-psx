#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>
#include <psxcd.h>
#include <inline_c.h>
#include <hwregs_c.h>

#include "render.h"
#include "util.h"
#include "chara.h"
#include "sound.h"
#include "input.h"
#include "player.h"
#include "level.h"
#include "timer.h"
#include "camera.h"

int debug_mode = 0;

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

static Player     player;
TileMap16  map16;
TileMap128 map128;
LevelData  leveldata;

#define BGM001_NUM_CHANNELS 2
#define BGM001_LOOP_SECTOR  7100
#define BGM002_NUM_CHANNELS 7
#define BGM002_LOOP_SECTOR  870

static uint8_t cur_bgm = 0;
static uint8_t music_num_channels = BGM001_NUM_CHANNELS;
static uint8_t music_channel = 0;

Camera camera;

void
engine_init()
{
    setup_context();
    sound_init();
    CdInit();
    pad_init();
    timer_init();

    // Present a "Now Loading..." text
    swap_buffers();
    draw_text(CENTERX - 52, CENTERY - 4, 0, "Now Loading...");
    swap_buffers();
    swap_buffers();

    set_clear_color(63, 0, 127);

    uint32_t filelength;
    TIM_IMAGE tim;
    uint8_t *timfile = file_read("\\SPRITES\\SONIC.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }

    load_player(&player, "\\SPRITES\\SONIC.CHARA;1", &tim);
    player.pos = (VECTOR){ CENTERX << 12, CENTERY << 12, 0 };

    // Load level data
    timfile = file_read("\\LEVELS\\R0\\TILES.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }
    
    load_map(&map16, "\\LEVELS\\R0\\MAP16.MAP;1", "\\LEVELS\\R0\\R0.COLLISION;1");
    load_map(&map128, "\\LEVELS\\R0\\MAP128.MAP;1", NULL);
    load_lvl(&leveldata, "\\LEVELS\\R0\\Z1.LVL;1");

    //cam_pos.vx = 1216 * ONE;
    //cam_pos.vy = 448 * ONE;
    //cam_pos.vx = CENTERX * ONE;
    //cam_pos.vy = CENTERY * ONE;

    camera_init(&camera);

    // Start playback after we don't need the CD anymore.
    sound_stop_xa();
    music_num_channels = BGM001_NUM_CHANNELS;
    music_channel = 1;
    cur_bgm = 0;
    sound_play_xa("\\BGM\\BGM001.XA;1", 0, music_channel, BGM001_LOOP_SECTOR);
}

void
engine_update()
{
    sound_update();
    pad_update();

    if(pos.vx < -CENTERX || pos.vx > CENTERX) dx = -dx;
    if(pos.vy < -CENTERY || pos.vy > CENTERY) dy = -dy;
    pos.vx += dx;
    pos.vy += dy;

    rotation.vx += 6;
    rotation.vy -= 8;
    rotation.vz -= 12;

    /* // Change music */
    /* if(pad_pressed(PAD_L2) && cur_bgm != 0) { */
    /*     sound_stop_xa(); */
    /*     music_num_channels = BGM001_NUM_CHANNELS; */
    /*     music_channel = 0; */
    /*     cur_bgm = 0; */
    /*     sound_play_xa("\\BGM\\BGM001.XA;1", 0, music_channel, BGM001_LOOP_SECTOR); */
    /* } else if(pad_pressed(PAD_R2) && cur_bgm != 1) { */
    /*     sound_stop_xa(); */
    /*     music_num_channels = BGM002_NUM_CHANNELS; */
    /*     music_channel = 0; */
    /*     cur_bgm = 1; */
    /*     sound_play_xa("\\BGM\\BGM002.XA;1", 1, music_channel, BGM002_LOOP_SECTOR); */
    /* } */
    
    /* // Change music channel */
    /* if(pad_pressed(PAD_R1) && (music_channel < music_num_channels - 1)) { */
    /*     music_channel++; */
    /*     sound_xa_set_channel(music_channel); */
    /* } */

    /* if(pad_pressed(PAD_L1) && (music_channel > 0)) { */
    /*     music_channel--; */
    /*     sound_xa_set_channel(music_channel); */
    /* } */

    if(pad_pressed(PAD_SELECT)) {
        debug_mode = !debug_mode;
    }

    if(pad_pressed(PAD_START)) {
        player.pos = (VECTOR){ CENTERX << 12, CENTERY << 12, 0 };
        player.grnd = 0;
        player.vel.vx = player.vel.vy = player.vel.vz = 0;
    }

    camera_update(&camera, &player);

    player_update(&player);
}

// Text
static char buffer[255] = { 0 };

void
engine_draw()
{
    VECTOR player_canvas_pos = {
        player.pos.vx - camera.pos.vx + (CENTERX << 12),
        player.pos.vy - camera.pos.vy + (CENTERY << 12),
        0
    };
    player_draw(&player, &player_canvas_pos);
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

    uint32_t elapsed_sectors;
    sound_xa_get_elapsed_sectors(&elapsed_sectors);

    
    
    // Sound and video debug
    snprintf(buffer, 255,
             "%4s %3d\n",
             /* "FPS %4d\n" */
             /* "VMD %4s\n" */
             /* "MUS %2u/2\n" */
             /* "CHN  %1u/%1u\n" */
             /* "%08u", */
             GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
             get_frame_rate()
             /* cur_bgm + 1, */
             /* music_channel + 1, */
             /* music_num_channels, */
             /* elapsed_sectors */
             );
    draw_text(248, 12, 0, buffer);

    // Player debug
    if(debug_mode) {
        snprintf(buffer, 255,
                 /* "CAM %08x %08x\n" */
                 "POS %08x %08x\n"
                 "VEL %08x %08x\n"
                 "GSP %08x\n"
                 "DIR %c\n"
                 "GRN %c(%2d) // %c(%2d)\n"
                 "CEI %c(%2d) // %c(%2d)\n"
                 "LEF %c(%2d)\n"
                 "RIG %c(%2d)\n",
                 /* cam_pos.vx, cam_pos.vy, */
                 player.pos.vx, player.pos.vy,
                 player.vel.vx, player.vel.vy,
                 player.vel.vz,
                 player.anim_dir >= 0 ? 'R' : 'L',
                 player.ev_grnd1.collided ? 'Y' : 'N', player.ev_grnd1.pushback,
                 player.ev_grnd2.collided ? 'Y' : 'N', player.ev_grnd2.pushback,
                 player.ev_ceil1.collided ? 'Y' : 'N', player.ev_ceil1.pushback,
                 player.ev_ceil2.collided ? 'Y' : 'N', player.ev_ceil2.pushback,
                 player.ev_left.collided ? 'Y' : 'N', player.ev_left.pushback,
                 player.ev_right.collided ? 'Y' : 'N', player.ev_right.pushback);
        draw_text(8, 12, 0, buffer);
    }
}

int
main(void)
{
    engine_init();

    while(1) {
        engine_update();
        engine_draw();
        timer_update();
        swap_buffers();
    }

    return 0;
}
