#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgte.h>
#include <psxcd.h>
#include <inline_c.h>
#include <hwregs_c.h>
#include <string.h>

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

static uint8_t cur_bgm = 0;
static uint8_t music_num_channels = BGM001_NUM_CHANNELS;
static uint8_t music_channel = 0;

Camera camera;

static uint8_t current_scene = 0;
static uint8_t menu_choice = 0;

void
engine_load_player()
{
    uint32_t filelength;
    TIM_IMAGE tim;
    uint8_t *timfile = file_read("\\SPRITES\\SONIC.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }

    load_player(&player, "\\SPRITES\\SONIC.CHARA;1", &tim);
    player.pos = (VECTOR){ CENTERX << 12, CENTERY << 12, 0 };
}

void
engine_load_level(uint8_t level)
{
    set_clear_color(63, 0, 127);

    TIM_IMAGE tim;
    uint32_t filelength;
    uint8_t *timfile = file_read("\\LEVELS\\R0\\TILES.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }
    load_map(&map16,
             "\\LEVELS\\R0\\MAP16.MAP;1",
             "\\LEVELS\\R0\\R0.COLLISION;1");
    load_map(&map128,
             "\\LEVELS\\R0\\MAP128.MAP;1",
             NULL);

    if(level == 0)
        load_lvl(&leveldata, "\\LEVELS\\R0\\Z1.LVL;1");
    else load_lvl(&leveldata, "\\LEVELS\\R0\\Z2.LVL;1");

    level_debrief();

    camera_init(&camera);

    // Start playback after we don't need the CD anymore.
    sound_stop_xa();
    music_num_channels = BGM001_NUM_CHANNELS;
    music_channel = (level == 0) ? 0 : 1;
    cur_bgm = 0;
    sound_play_xa("\\BGM\\BGM001.XA;1", 0, music_channel, BGM001_LOOP_SECTOR);
}

void
engine_unload_level()
{
    sound_stop_xa();
    level_reset();
    sound_reset_mem();
}

void
engine_load_menu()
{
    set_clear_color(0, 0, 0);
    render_loading_text();
    current_scene = 0;
    menu_choice = 0;
}

void
engine_init()
{
    setup_context();
    sound_init();
    CdInit();
    pad_init();
    timer_init();
    level_init();

    set_clear_color(0, 0, 0);
    render_loading_text();
    set_clear_color(63, 0, 127);

    engine_load_menu();
}

void
engine_update()
{
    sound_update();
    pad_update();

    if(current_scene == 0) {
        if(pad_pressed(PAD_DOWN))
            menu_choice++;
        else if(pad_pressed(PAD_UP)) {
            if(menu_choice == 0) menu_choice = 1;
            else menu_choice--;
        }

        menu_choice = (menu_choice % 2);

        if(pad_pressed(PAD_START) || pad_pressed(PAD_CROSS)) {
            set_clear_color(0, 0, 0);
            render_loading_text();

            engine_load_player();
            engine_load_level(menu_choice);
            current_scene = 1;
        }
    } else if(current_scene == 1) {
        if(pos.vx < -CENTERX || pos.vx > CENTERX) dx = -dx;
        if(pos.vy < -CENTERY || pos.vy > CENTERY) dy = -dy;
        pos.vx += dx;
        pos.vy += dy;

        rotation.vx += 6;
        rotation.vy -= 8;
        rotation.vz -= 12;

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
}

// Text
static char buffer[255] = { 0 };

void
engine_draw()
{
    if(current_scene == 0) {
        int16_t x;
        const char *title = "*** engine-psx ***";
        x = CENTERX - (strlen(title) * 4);
        draw_text(x, 12, 0, title);

        snprintf(buffer, 255, "Built %s %s", __DATE__, __TIME__);
        x = CENTERX - (strlen(buffer) * 4);
        draw_text(x, 24, 0, buffer);

        snprintf(buffer, 255,
                 "%c Round 0 Zone 1\n"
                 "%c Round 0 Zone 2\n",
                 (menu_choice == 0) ? '>' : ' ',
                 (menu_choice == 1) ? '>' : ' '
            );
        draw_text(8, 60, 0, buffer);
    } else if(current_scene == 1) {
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
