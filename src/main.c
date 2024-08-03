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

// Timer-related
volatile int timer_counter = 0;
volatile int frame_counter = 0;
volatile int frame_rate = 0;

static int debug_mode = 0;

/* #define SPRTSZ 56 */

/* static int  x = (SPRTSZ >> 1),  y = (SPRTSZ >> 1); */
/* static int dx = 1,  dy = 1; */

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
static VECTOR  scale    = { ONE, ONE, ONE };
static MATRIX  world    = { 0 };

static Player     player;
static TileMap16  map16;
static TileMap128 map128;
static LevelData  leveldata;

#define BGM001_NUM_CHANNELS 2
#define BGM001_LOOP_SECTOR  7100
#define BGM002_NUM_CHANNELS 7
#define BGM002_LOOP_SECTOR  870

static uint8_t cur_bgm = 0;
static uint8_t music_num_channels = BGM001_NUM_CHANNELS;
static uint8_t music_channel = 1;

static VECTOR cam_pos = { 0 };

static CollisionEvent ev_grnd1 = { 0 };
static CollisionEvent ev_grnd2 = { 0 };
static CollisionEvent ev_left  = { 0 };
static CollisionEvent ev_right = { 0 };
static CollisionEvent ev_ceil1 = { 0 };
static CollisionEvent ev_ceil2 = { 0 };

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
    cam_pos.vx = CENTERX * ONE;
    cam_pos.vy = CENTERY * ONE;

    // Start playback after we don't need the CD anymore.
    sound_stop_xa();
    music_num_channels = BGM001_NUM_CHANNELS;
    music_channel = 0;
    cur_bgm = 0;
    sound_play_xa("\\BGM\\BGM001.XA;1", 0, music_channel, BGM001_LOOP_SECTOR);
}

void
engine_update()
{
    sound_update();
    pad_update();

    /* if(x < (SPRTSZ >> 1) || x > (SCREEN_XRES - 32)) dx = -dx; */
    /* if(y < (SPRTSZ >> 1) || y > (SCREEN_YRES - 32)) dy = -dy; */

    /* x += dx; */
    /* y += dy; */

    rotation.vx += 6;
    rotation.vy -= 8;
    rotation.vz -= 12;

    // Change music
    if(pad_pressed(PAD_L2) && cur_bgm != 0) {
        sound_stop_xa();
        music_num_channels = BGM001_NUM_CHANNELS;
        music_channel = 0;
        cur_bgm = 0;
        sound_play_xa("\\BGM\\BGM001.XA;1", 0, music_channel, BGM001_LOOP_SECTOR);
    } else if(pad_pressed(PAD_R2) && cur_bgm != 1) {
        sound_stop_xa();
        music_num_channels = BGM002_NUM_CHANNELS;
        music_channel = 0;
        cur_bgm = 1;
        sound_play_xa("\\BGM\\BGM002.XA;1", 1, music_channel, BGM002_LOOP_SECTOR);
    }
    
    // Change music channel
    if(pad_pressed(PAD_R1) && (music_channel < music_num_channels - 1)) {
        music_channel++;
        sound_xa_set_channel(music_channel);
    }

    if(pad_pressed(PAD_L1) && (music_channel > 0)) {
        music_channel--;
        sound_xa_set_channel(music_channel);
    }

    if(pad_pressed(PAD_SELECT)) {
        debug_mode = !debug_mode;
    }

/* #define SPD (8 * ONE) */

/*     if(pad_pressing(PAD_RIGHT)) { */
/*         cam_pos.vx += SPD; */
/*     } */

/*     if(pad_pressing(PAD_LEFT)) { */
/*         cam_pos.vx -= SPD; */
/*     } */

/*     if(pad_pressing(PAD_UP)) { */
/*         cam_pos.vy -= SPD; */
/*     } */

/*     if(pad_pressing(PAD_DOWN)) { */
/*         cam_pos.vy += SPD; */
/*     } */

    cam_pos = player.pos;

    

    /* Collider linecasts */
    uint16_t
        anchorx = (player.pos.vx >> 12),
        anchory = (player.pos.vy >> 12) + 4;

    uint16_t grn_ceil_dist = 8;
    uint16_t grn_mag   = 14;
    uint16_t ceil_mag  = 14;
    uint16_t left_mag  = 12;
    uint16_t right_mag = 12;
    
    ev_grnd1 = linecast(&leveldata, &map128, &map16,
                        anchorx - grn_ceil_dist, anchory + 8,
                        1, grn_mag);
    ev_grnd2 = linecast(&leveldata, &map128, &map16,
                        anchorx + grn_ceil_dist, anchory + 8,
                        1, grn_mag);

    ev_ceil1 = linecast(&leveldata, &map128, &map16,
                        anchorx - grn_ceil_dist, anchory - 8,
                        1, -ceil_mag);
    ev_ceil2 = linecast(&leveldata, &map128, &map16,
                        anchorx + grn_ceil_dist, anchory - 8,
                        1, -ceil_mag);

    // 16px horizontal to the left or to the right
    ev_left = linecast(&leveldata, &map128, &map16,
                       anchorx, anchory,
                       0, -left_mag);
    ev_right = linecast(&leveldata, &map128, &map16,
                        anchorx, anchory,
                        0, right_mag);

    /* Draw Colliders */
    uint16_t
        ax = anchorx - (cam_pos.vx >> 12) + CENTERX,
        ay = anchory - (cam_pos.vy >> 12) + CENTERY;

    if(debug_mode) {
        LINE_F2 line;
        setLineF2(&line);

        // Ground sensor left
        setRGB0(&line, 0, 93, 0);
        setXY2(&line, ax - grn_ceil_dist, ay + 8, ax - grn_ceil_dist, ay + 8 + grn_mag);
        DrawPrim((void *)&line);
        // (dot)
        if(ev_grnd1.collided) setRGB0(&line, 255, 0, 0);
        else                  setRGB0(&line, 255, 255, 255);
        setXY2(&line, ax - grn_ceil_dist, ay + 8 + grn_mag, ax - grn_ceil_dist, ay + 8 + grn_mag);
        DrawPrim((void *)&line);
    
        // Ground sensor right
        setRGB0(&line, 23, 99, 63);
        setXY2(&line, ax + grn_ceil_dist, ay + 8, ax + grn_ceil_dist, ay + 8 + grn_mag);
        DrawPrim((void *)&line);
        // (dot)
        if(ev_grnd2.collided) setRGB0(&line, 255, 0, 0);
        else                  setRGB0(&line, 255, 255, 255);
        setXY2(&line, ax + grn_ceil_dist, ay + 8 + grn_mag, ax + grn_ceil_dist, ay + 8 + grn_mag);
        DrawPrim((void *)&line);
    
        // Ceiling sensor left
        setRGB0(&line, 0, 68, 93);
        setXY2(&line, ax - grn_ceil_dist, ay - 8, ax - grn_ceil_dist, ay - 8 - ceil_mag);
        DrawPrim((void *)&line);
        // (dot)
        if(ev_ceil1.collided) setRGB0(&line, 255, 0, 0);
        else                  setRGB0(&line, 255, 255, 255);
        setXY2(&line, ax - grn_ceil_dist, ay - 8 - ceil_mag, ax - grn_ceil_dist, ay - 8 - ceil_mag);
        DrawPrim((void *)&line);
    
        // Ceiling sensor right
        setRGB0(&line, 99, 94, 23);
        setXY2(&line, ax + grn_ceil_dist, ay - 8, ax + grn_ceil_dist, ay - 8 - ceil_mag);
        DrawPrim((void *)&line);
        // (dot)
        if(ev_ceil2.collided) setRGB0(&line, 255, 0, 0);
        else                  setRGB0(&line, 255, 255, 255);
        setXY2(&line, ax + grn_ceil_dist, ay - 8 - ceil_mag, ax + grn_ceil_dist, ay - 8 - ceil_mag);
        DrawPrim((void *)&line);
    
        // Left sensor
        setRGB0(&line, 99, 23, 99);
        setXY2(&line, ax, ay, ax - left_mag, ay);
        DrawPrim((void *)&line);
        // (dot)
        if(ev_left.collided) setRGB0(&line, 255, 0, 0);
        else                 setRGB0(&line, 255, 255, 255);
        setXY2(&line, ax - left_mag, ay, ax - left_mag, ay);
        DrawPrim((void *)&line);
    
        // Right sensor
        setRGB0(&line, 99, 23, 99);
        setXY2(&line, ax, ay, ax + right_mag, ay);
        DrawPrim((void *)&line);
        // (dot)
        if(ev_right.collided) setRGB0(&line, 255, 0, 0);
        else                  setRGB0(&line, 255, 255, 255);
        setXY2(&line, ax + right_mag, ay, ax + right_mag, ay);
        DrawPrim((void *)&line);

        // Player center (dot)
        setRGB0(&line, 255, 255, 255);
        setXY2(&line, ax, ay, ax, ay);
        DrawPrim((void *)&line);
    }

    /* Player collision detection */
    if(ev_right.collided && player.vel.vx > 0) {
        player.vel.vx = 0;
        player.pos.vx = ((player.pos.vx >> 12) - (int32_t)(ev_right.pushback)) << 12;
    }

    if(ev_left.collided && player.vel.vx < 0) {
        player.vel.vx = 0;
        player.pos.vx = ((player.pos.vx >> 12) + (int32_t)(ev_left.pushback) - 4) << 12;
    }

    if(!player.grnd) {
        if((ev_grnd1.collided || ev_grnd2.collided) && (player.vel.vy > 0)) {
            player.vel.vy = 0;
            int32_t pushback =
                (ev_grnd1.pushback > ev_grnd2.pushback)
                ? ev_grnd1.pushback
                : ev_grnd2.pushback;
            player.pos.vy = ((player.pos.vy >> 12) - pushback) << 12;
            player.grnd = 1;
            player.jmp = 0;
        }
    } else {
        if(!ev_grnd1.collided && !ev_grnd2.collided) {
            player.grnd = 0;
        }
    }
    
    player_update(&player);
}

// Text
static char buffer[255] = { 0 };

void
engine_draw()
{
    //chara_render_test(&player.chara);
    player_draw(&player, &(VECTOR){CENTERX << 12, CENTERY << 12, 0});

    render_lvl(&leveldata, &map128, &map16, cam_pos.vx, cam_pos.vy);

    // Gouraud-shaded SQUARE
    /* POLY_G4 *poly = (POLY_G4 *) get_next_prim(); */
    /* setPolyG4(poly); */
    /* setXY4(poly, */
    /*        x - (SPRTSZ >> 1), y - (SPRTSZ >> 1), */
    /*        x + (SPRTSZ >> 1), y - (SPRTSZ >> 1), */
    /*        x - (SPRTSZ >> 1), y + (SPRTSZ >> 1), */
    /*        x + (SPRTSZ >> 1), y + (SPRTSZ >> 1)); */
    /* setRGB0(poly, 255, 0, 0); */
    /* setRGB1(poly, 0, 255, 0); */
    /* setRGB2(poly, 0, 0, 255); */
    /* setRGB3(poly, 255, 255, 0); */
    /* sort_prim(poly, 1); */
    /* increment_prim(sizeof(POLY_G4)); */

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

    
    
    // Sound debug
    snprintf(buffer, 255,
             "%4s %3d\n"
             /* "FPS %4d\n" */
             /* "VMD %4s\n" */
             "MUS %2u/2\n"
             "CHN  %1u/%1u\n"
             "%08u",
             GetVideoMode() == MODE_PAL ? "PAL" : "NTSC",
             get_frame_rate(),
             cur_bgm + 1,
             music_channel + 1,
             music_num_channels,
             elapsed_sectors);
    draw_text(248, 12, 0, buffer);

    // Player debug
    snprintf(buffer, 255,
             /* "CAM %08x %08x\n" */
             "POS %08x %08x\n"
             "VEL %08x %08x\n"
             "GSP %08x\n"
             "DIR %c\n"
             "GRN %1x %2d // %1x %2d\n"
             "CEI %1x %2d // %1x %2d\n"
             "LEF %1x %2d\n"
             "RIG %1x %2d\n",
             /* cam_pos.vx, cam_pos.vy, */
             player.pos.vx, player.pos.vy,
             player.vel.vx, player.vel.vy,
             player.vel.vz,
             player.anim_dir >= 0 ? 'R' : 'L',
             ev_grnd1.collided, ev_grnd1.pushback,
             ev_grnd2.collided, ev_grnd2.pushback,
             ev_ceil1.collided, ev_ceil1.pushback,
             ev_ceil2.collided, ev_ceil2.pushback,
             ev_left.collided, ev_left.pushback,
             ev_right.collided, ev_right.pushback);
    draw_text(8, 12, 0, buffer);
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
