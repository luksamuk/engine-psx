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
#include "demo.h"

extern int debug_mode;
extern PlayerConstants CNST_DEFAULT;

static uint8_t level = 0;

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
int32_t     level_water_y;
LEVELMODE   level_mode;


// Forward function declarations
static void level_load_player();
static void level_load_level();
static void level_set_clearcolor();
static void level_play_music(uint8_t round, uint8_t act);

typedef struct {
    uint8_t    level_transition;
    Parallax   parallax;
    uint8_t    parallax_tx_mode;
    int32_t    parallax_px;
    int32_t    parallax_py;
    int32_t    parallax_cx;
    int32_t    parallax_cy;
    const char *level_name;
    uint8_t    level_round;
    uint8_t    level_act;
    uint16_t   level_counter;

    // Title card variables
    int16_t tc_ribbon_y;
    int16_t tc_title_x;
    int16_t tc_zone_x;
    int16_t tc_act_x;

    int16_t tc_ribbon_tgt_y;
    int16_t tc_title_tgt_x;
    int16_t tc_zone_tgt_x;
    int16_t tc_act_tgt_x;

    // Water overlay primitives
    TILE     waterquad[2];
    POLY_FT4 wavequad[2][5];
    uint8_t  waterbuffer;
    uint8_t  water_last_fade[2];
} screen_level_data;

void
screen_level_load()
{
    screen_level_data *data = screen_alloc(sizeof(screen_level_data));
    data->level_transition = 0;
    data->level_name = "PLAYGROUND";
    data->level_act  = 0;

    camera_init(&camera);

    level_load_player();
    level_load_level(data);
    camera_set(&camera, player.pos.vx, player.pos.vy);

    reset_elapsed_frames();
    pause_elapsed_frames();

    level_fade = 0;
    data->level_counter = 120;

    level_ring_count = 0;
    level_finished = 0;

    // Init water quads
    for(int i = 0; i < 2; i++) {
        TILE *poly = &data->waterquad[i];
        setTile(poly);
        setSemiTrans(poly, 1);
        setRGB0(poly, 0, 0, 0);
        setXY0(poly, 0, 0);
        setWH(poly, SCREEN_XRES, 0);

        for(int j = 0; j < 5; j++) {
            POLY_FT4 *tx = &data->wavequad[i][j];
            setPolyFT4(tx);
            setSemiTrans(tx, 1);
            tx->tpage = getTPage(1, 0, 576, 0);
            tx->clut = getClut(0, 481);
            setRGB0(tx, 0, 0, 0);
            setXYWH(tx, j * 64, 0, 64, 9);
        }
    }
    data->waterbuffer = 0;
    data->water_last_fade[0] = 0;
    data->water_last_fade[1] = 0;

    // Init water wave quads

    demo_init();

    // init proper RNG per level
    srand(get_global_frames());

    // If it is a demo or we're recording, skip title card
    if(level_mode == LEVEL_MODE_DEMO
       || level_mode == LEVEL_MODE_RECORD) {
        data->level_transition = 1;
    }

    // Recover control if mode is "hold forward"
    if(level_mode == LEVEL_MODE_FINISHED)
        level_mode = LEVEL_MODE_NORMAL;
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

    // Debug mode cycling
    {
        if(pad_pressing(PAD_L1) && pad_pressed(PAD_R1))
            debug_mode++;
        else if(pad_pressed(PAD_L1) && pad_pressing(PAD_R1))
            debug_mode--;
        if(debug_mode > 2) debug_mode = 0;
        else if(debug_mode < 0) debug_mode = 2;
    }

    level_set_clearcolor();

    // Manage fade in and fade out.
    // States:
    // 0: Showing title card
    // 1: Fade in
    // 2: Gameplay (differentiate if level is finished with level_finished global)
    // 3: Fade out
    // 4: Go to next level (managed by goal sign object)
    if(data->level_transition == 0) { // Show title card
        data->level_counter--;
        if(data->level_counter == 0)
            data->level_transition = 1;
    } else if(data->level_transition == 1) { // Fade in
        level_fade += 2;
        if(level_fade >= 128) {
            level_fade = 128;
            data->level_transition = 2;

            // Start level timer
            reset_elapsed_frames();
        }
    } else if(data->level_transition == 3) { // Fade out
        level_fade -= 2;
        if(level_fade == 0) {
            data->level_transition = 4;
        }
    }

    // Manage title card depending on level transition
    {
        const uint16_t speed = 16;
        if(data->level_transition == 0) {
            data->tc_ribbon_y += speed;
            data->tc_title_x -= speed;
            data->tc_zone_x -= speed;
            data->tc_act_x -= speed;

            if(data->tc_ribbon_y > data->tc_ribbon_tgt_y)
                data->tc_ribbon_y = data->tc_ribbon_tgt_y;
            if(data->tc_title_x < data->tc_title_tgt_x)
                data->tc_title_x = data->tc_title_tgt_x;
            if(data->tc_zone_x < data->tc_zone_tgt_x)
                data->tc_zone_x = data->tc_zone_tgt_x;
            if(data->tc_act_x < data->tc_act_tgt_x)
                data->tc_act_x = data->tc_act_tgt_x;
        } else if(data->level_transition == 1) {
            data->tc_ribbon_y -= speed;
            data->tc_title_x += speed;
            data->tc_zone_x += speed;
            data->tc_act_x += speed;
        }
    }

    // Toggle pause. But only if not playing a demo!
    if(level_mode != LEVEL_MODE_DEMO) {
        if(pad_pressed(PAD_START)
           && !level_finished
           && (data->level_transition == 2)) {
            paused = !paused;
            if(paused) sound_xa_set_volume(0x00);
            else sound_xa_set_volume(XA_DEFAULT_VOLUME);
        }
    } else {
        // If in demo mode, absolutely any button press will
        // trigger its end!
        uint32_t seconds = get_elapsed_frames() / 60;
        if((pad_pressed_any() || (seconds >= 30))
           && (screen_level_getstate() == 2)) {
            screen_level_setstate(3);
        }

        if(screen_level_getstate() == 4) {
            // Go back to title screen
            scene_change(SCREEN_TITLE);
        }
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

    if(debug_mode > 0) {
        // Create a little falling ring
        if(pad_pressed(PAD_TRIANGLE)) {
            PoolObject *ring = object_pool_create(OBJ_RING);
            ring->freepos.vx = camera.pos.vx;
            ring->freepos.vy = camera.pos.vy - (CENTERY << 12) + (20 << 12);
            ring->props |= OBJ_FLAG_ANIM_LOCK;
            ring->props |= OBJ_FLAG_RING_MOVING;
        }

        // Respawn
        if(pad_pressed(PAD_SELECT) && !level_finished) {
            player.pos = player.respawnpos;
            camera.pos = camera.realpos = player.respawnpos;
            player.grnd = 0;
            player.anim_dir = 1;
            player.vel.vx = player.vel.vy = player.vel.vz = 0;
            player.psmode = player.gsmode = CDIR_FLOOR;
            player.underwater = 0;
            player.cnst = &CNST_DEFAULT;
            player.speedshoes_frames = (player.speedshoes_frames > 0) ? 0 : -1;
        }

        if(pad_pressed(PAD_CIRCLE)) {
            player_do_damage(&player, player.pos.vx);
        }
    }

    // Record level demo. Uncomment to print.
    switch(level_mode) {
    case LEVEL_MODE_DEMO:
        demo_update_playback(level, &player.input);
        break;
    case LEVEL_MODE_RECORD:
        demo_record();
        input_get_state(&player.input);
        break;
    case LEVEL_MODE_FINISHED:
        player.input.current = player.input.old = 0x0020;
        break;
    default:
        input_get_state(&player.input);
        break;
    }

    camera_update(&camera, &player);
    update_obj_window(&leveldata, &obj_table_common, camera.pos.vx, camera.pos.vy);
    object_pool_update(&obj_table_common);

    // Limit player left position
    if((player.pos.vx - (PUSH_RADIUS << 12)) < (camera.min_x - (CENTERX << 12))) {
        player.pos.vx = camera.min_x - (CENTERX << 12) + (PUSH_RADIUS << 12);
        if(player.vel.vx < 0) {
            if(player.grnd) player.vel.vz = 0;
            else player.vel.vx = 0;
        }
    }

    // Only update these if past fade in!
    if(data->level_transition > 0) {
        player_update(&player);
    }

    // If speed shoes are finished, we use the player's values
    // as a flag to resume music playback.
    // Player constants are managed within player update
    if(player.speedshoes_frames == 0) {
        if(!level_finished)
            level_play_music(data->level_round, data->level_act);
        player.speedshoes_frames = -1;
    }
}

void
_screen_level_draw_water(screen_level_data *data)
{
    if(level_water_y >= 0) {
        int32_t camera_bottom = camera.pos.vy + (CENTERY << 12);

        if(camera_bottom > level_water_y) {
            int32_t water_vh = camera_bottom - level_water_y;
            uint16_t water_rh = water_vh >> 12;
            uint16_t water_h = MIN(water_rh, SCREEN_YRES);
            int16_t water_y = MAX(0, SCREEN_YRES - water_h);
            int16_t water_ry = SCREEN_YRES - water_rh;

            // Draw water overlay
            {
                /* GOURAUD SHADE */
                /* POLY_G4 *poly = get_next_prim(); */
                /* increment_prim(sizeof(POLY_G4)); */
                /* setPolyG4(poly); */
                /* setXYWH(poly, 0, water_y, SCREEN_XRES, water_h); */
                /* setSemiTrans(poly, 1); */
                /* setRGB0(poly, */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0xb8)); */
                /* setRGB1(poly, */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0xb8)); */
                /* setRGB2(poly, */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0x18)); */
                /* setRGB3(poly, */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0), */
                /*         LERPC(level_fade, 0x18)); */
                /* sort_prim(poly, OTZ_LAYER_LEVEL_FG_FRONT); */

                /* FLAT SHADE, STATIC BUFFERS */
                TILE *poly = &data->waterquad[data->waterbuffer];
                poly->y0 = water_y;
                poly->h = water_h;

                if(data->water_last_fade[data->waterbuffer] != level_fade) {
                    data->water_last_fade[data->waterbuffer] = level_fade;
                    setRGB0(poly,
                            LERPC(level_fade, 0),
                            LERPC(level_fade, 0),
                            LERPC(level_fade, 0xb8));
                    setRGB0(&data->waterquad[data->waterbuffer ^ 1],
                            LERPC(level_fade, 0),
                            LERPC(level_fade, 0),
                            LERPC(level_fade, 0xb8));
                }
                sort_prim(poly, OTZ_LAYER_LEVEL_FG_FRONT);
            }

            // Draw water waves
            {
                static uint8_t wave_dir = 0;
                static uint8_t wave_visible = 1;

                uint32_t frame = get_global_frames();

                if(!(frame % 6)) wave_dir = !wave_dir;
                if(!(frame % 3)) wave_visible = !wave_visible;

                for(uint8_t i = 0; i < 5; i++) {
                    uint8_t current_visible = (i + wave_visible) % 2;
                    POLY_FT4 *poly = &data->wavequad[data->waterbuffer][i];
                    if(poly->r0 != level_fade)
                        setRGB0(poly, level_fade, level_fade, level_fade);
                    if(!current_visible)
                        setUVWH(poly, 0, 0, 0, 0);
                    else if(wave_dir)
                        setUVWH(poly, 146, 183, 64, 9);
                    else setUV4(poly,
                                146 + 63, 183,
                                146, 183,
                                146 + 63, 183 + 9,
                                146, 183 + 9);
                    poly->y0 = poly->y1 = water_ry - 6;
                    poly->y2 = poly->y3 = water_ry - 6 + 9;
                    sort_prim(poly, OTZ_LAYER_LEVEL_FG_FRONT);
                }
            }

            // Flip buffers
            data->waterbuffer ^= 1;
        }
        
    }
}

void
screen_level_draw(void *d)
{
    screen_level_data *data = (screen_level_data *)d;
    char buffer[255] = { 0 };

    // As a rule of thumb, things are drawn in specific otz's.
    // When things are drawn on the same otz, anything drawn first
    // is shown on front, as the ordering table is drawn backwards.

    _screen_level_draw_water(data);

    // Draw player
    if(abs((player.pos.vx - camera.pos.vx) >> 12) <= SCREEN_XRES
       && abs((player.pos.vy - camera.pos.vy) >> 12) <= SCREEN_YRES) {
        VECTOR player_canvas_pos = {
            player.pos.vx - camera.pos.vx + (CENTERX << 12),
            player.pos.vy - camera.pos.vy + (CENTERY << 12),
            0
        };
        player_draw(&player, &player_canvas_pos);
    }

    // Draw free objects
    object_pool_render(&obj_table_common, camera.pos.vx, camera.pos.vy);

    // Draw level and level objects
    render_lvl(&leveldata, &map128, &map16, &obj_table_common, camera.pos.vx, camera.pos.vy);

    // Draw background and parallax
    if(level_get_num_sprites() < 1312)
        parallax_draw(&data->parallax, &camera);

    // If we're in R4, draw a gradient on the background.
    if(level == 8 || level == 9) {
        POLY_G4 *poly = get_next_prim();
        increment_prim(sizeof(POLY_G4));
        setPolyG4(poly);
        setXYWH(poly, 0, 0, SCREEN_XRES, CENTERY);
        setRGB0(poly,
                LERPC(level_fade, 0x13),
                LERPC(level_fade, 0x12),
                LERPC(level_fade, 0x3c));
        setRGB1(poly,
                LERPC(level_fade, 0x13),
                LERPC(level_fade, 0x12),
                LERPC(level_fade, 0x3c));
        setRGB2(poly,
                LERPC(level_fade, 0x30),
                LERPC(level_fade, 0x66),
                LERPC(level_fade, 0xb5));
        setRGB3(poly,
                LERPC(level_fade, 0x30),
                LERPC(level_fade, 0x66),
                LERPC(level_fade, 0xb5));
        sort_prim(poly, OTZ_LAYER_LEVEL_BG);
    }
    else if(data->level_round == 5) {
        // If we're in R5, draw a dark gradient on 
    }
    // If we're in R8, draw a gradient as well, but at a lower position.
    else if(level == 16 || level == 17 || level == 18) {
        POLY_G4 *poly = get_next_prim();
        increment_prim(sizeof(POLY_G4));
        setPolyG4(poly);
        setXYWH(poly, 0, 120, SCREEN_XRES, SCREEN_YRES - 120);
        setRGB0(poly, LERPC(level_fade, 0x21), 0x00, 0x00);
        setRGB1(poly, LERPC(level_fade, 0x21), 0x00, 0x00);
        setRGB2(poly, LERPC(level_fade, 0xbd), 0x00, LERPC(level_fade, 0xbd));
        setRGB3(poly, LERPC(level_fade, 0xbd), 0x00, LERPC(level_fade, 0xbd));
        sort_prim(poly, OTZ_LAYER_LEVEL_BG);
    }

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

    // Title card
    if(data->level_transition <= 1) {
        font_reset_color();
        font_draw_hg(data->level_name, data->tc_title_x, 70);
        font_draw_hg("ZONE", data->tc_zone_x, 70 + GLYPH_HG_WHITE_HEIGHT + 5);

        // ACT card
        char buffer[5];
        snprintf(buffer, 5, "*%d", data->level_act + 1);
        font_draw_hg(buffer, data->tc_act_x, 70 + GLYPH_HG_WHITE_HEIGHT + 40);

        // Game text
        font_set_color(0xc8, 0xc8, 0x00);
        uint16_t wt = font_measurew_sm("SONIC XA");
        font_draw_sm("SONIC XA", 50 + ((80 - wt) >> 1), data->tc_ribbon_y + 180);
        font_reset_color();

        // Title card ribbon background
        {
            POLY_G4 *polyg = get_next_prim();
            increment_prim(sizeof(POLY_G4));
            setPolyG4(polyg);
            setRGB0(polyg, 0x48, 0x48, 0xfc);
            setRGB1(polyg, 0x48, 0x48, 0xfc);
            setRGB2(polyg, 0xf0, 0xf0, 0xf0);
            setRGB3(polyg, 0xf0, 0xf0, 0xf0);
            setXYWH(polyg, 50, data->tc_ribbon_y, 80, 200);
            sort_prim(polyg, OTZ_LAYER_HUD);

            POLY_F4 *polyf = get_next_prim();
            increment_prim(sizeof(POLY_F4));
            setPolyF4(polyf);
            setRGB0(polyf, 0x1c, 0x1a, 0x1a);
            setXYWH(polyf, 55, data->tc_ribbon_y, 80, 205);
            sort_prim(polyf, OTZ_LAYER_HUD);
        }
    }

    // Demo HUD. Only when playing AutoDemo!
    if(level_mode == LEVEL_MODE_DEMO) {
        // Uses HUD layer to draw!
        font_draw_logo(20, SCREEN_YRES - 65, 120, 45);
    }

    // Heads-up display
    if((debug_mode <= 1) && (level_mode != LEVEL_MODE_DEMO)) {
        font_set_color(
            LERPC(level_fade, 0xc8),
            LERPC(level_fade, 0xc8),
            0);
        font_draw_big("SCORE", 10, 10);
        font_draw_big("TIME",  10, 24);

        // Flash red every 8 frames
        if(level_ring_count == 0
           && ((get_elapsed_frames() >> 3) % 2 == 1)) {
            font_set_color(
                LERPC(level_fade, 0xc8),
                0,
                0);
        } else {
            font_set_color(
                LERPC(level_fade, 0xc8),
                LERPC(level_fade, 0xc8),
                0);
        }
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
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC", get_frame_rate());
        font_draw_sm(buffer, 248, 12);

        // Sound debug
        uint32_t elapsed_sectors;
        sound_xa_get_elapsed_sectors(&elapsed_sectors);
        snprintf(buffer, 255, "%08u", elapsed_sectors);
        font_draw_sm(buffer, 248, 20);

        // Free object debug
        snprintf(buffer, 255, "OBJS %3d", object_pool_get_count());
        font_draw_sm(buffer, 248, 28);

        // Rings, time and air for convenience
        snprintf(buffer, 255, "RING %03d", level_ring_count);
        font_draw_sm(buffer, 248, 36);

        snprintf(buffer, 255, "TIME %03d", (get_elapsed_frames() / 60));
        font_draw_sm(buffer, 248, 44);

        snprintf(buffer, 255, "AIR   %02d", player.remaining_air_frames / 60);
        font_draw_sm(buffer, 248, 52);

        snprintf(buffer, 255, "SPR %4d", level_get_num_sprites());
        font_draw_sm(buffer, 248, 60);

        // Player debug
        if(debug_mode > 1) {
            snprintf(buffer, 255,
                     "GSP %08x\n"
                     "SPD %08x %08x\n"
                     "ANG %08x G.P %s %s %3d\n"
                     "POS %08x %08x\n"
                     "ACT %02u\n"
                     ,
                     player.vel.vz,
                     player.vel.vx, player.vel.vy,
                     player.angle,
                     (player.gsmode == CDIR_FLOOR)
                     ? "FL"
                     : (player.gsmode == CDIR_RWALL)
                     ? "RW"
                     : (player.gsmode == CDIR_LWALL)
                     ? "LW"
                     : (player.gsmode == CDIR_CEILING)
                     ? "CE"
                     : "  ",
                     (player.psmode == CDIR_FLOOR)
                     ? "FL"
                     : (player.psmode == CDIR_RWALL)
                     ? "RW"
                     : (player.psmode == CDIR_LWALL)
                     ? "LW"
                     : (player.psmode == CDIR_CEILING)
                     ? "CE"
                     : "  ",
                     (int32_t)(((int32_t)player.angle * (int32_t)(360 << 12)) >> 24), // angle in deg
                     player.pos.vx, player.pos.vy,
                     player.action
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

    // Negative water means no water
    level_water_y = -1;

    switch(level) {
    case 0: case 1: case 2: case 3: // Playground
        data->level_name = "PLAYGROUND";
        data->level_round = 0;
        data->level_act = level;
        break;
    case 4: case 5:
        data->level_name = "GREEN HILL";
        data->level_round = 2;
        data->level_act = level - 4;
        break;
    case 6: case 7:
        data->level_name = "SURELY WOOD";
        data->level_round = 3;
        data->level_act = level - 6;
        break;
    case 8: case 9:
        data->level_name = "DAWN CANYON";
        data->level_round = 4;
        data->level_act = level - 8;
        break;
    case 10: case 11:
        data->level_name = "AMAZING OCEAN";
        data->level_round = 5;
        data->level_act = level - 10;
        level_water_y = 0x2c0000;
        break;
    case 12: case 13:
        data->level_name = "BLAZING FORCE";
        data->level_round = 6;
        data->level_act = level - 12;
        break;
    case 14: case 15:
        data->level_name = "RADIANT TOMB";
        data->level_round = 7;
        data->level_act = level - 14;
        break;
    case 16: case 17: case 18:
        data->level_name = "EGGMANLAND";
        data->level_round = 8;
        data->level_act = level - 16;
        break;
    default:
        data->level_name = "TEST LEVEL";
        data->level_round = 0xff;
        data->level_act = 0;
        break;
    }

    char basepath[255];
    char filename0[255], filename1[255];

    level_set_clearcolor();

    snprintf(basepath, 255, "\\LEVELS\\R%1u", data->level_round);

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


    /* === TILE MAPPINGS === */
    snprintf(filename0, 255, "%s\\MAP16.MAP;1", basepath);
    snprintf(filename1, 255, "%s\\MAP16.COL;1", basepath);
    printf("Loading %s and %s...\n", filename0, filename1);
    load_map16(&map16, filename0, filename1);
    snprintf(filename0, 255, "%s\\MAP128.MAP;1", basepath);
    printf("Loading %s...\n", filename0);
    load_map128(&map128, filename0);



    /* === LEVEL LAYOUT === */
    snprintf(filename0, 255, "%s\\Z%1u.LVL;1", basepath, data->level_act + 1);
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
    snprintf(filename0, 255, "%s\\Z%1u.OMP;1", basepath, data->level_act + 1);
    load_object_placement(filename0, &leveldata);


    /* === OBJECT POOL / FREE OBJECTS === */
    object_pool_init();


    /* === RENDERING PREPARATION === */
    // Pre-allocate and initialize level primitive buffer
    prepare_renderer(&leveldata);

    level_debrief();

    printf("Number of level layers: %d\n", leveldata.num_layers);

    // Start playback after we don't need the CD anymore.
    level_play_music(data->level_round, data->level_act);

    // Pre-calculate title card target X and Y positions
    {
        uint16_t wt = font_measurew_hg(data->level_name);
        uint16_t wz = font_measurew_hg("ZONE");
        uint16_t vx = CENTERX - (wt >> 1) + 20;

        data->tc_ribbon_tgt_y = 0;
        data->tc_title_tgt_x = vx;
        data->tc_zone_tgt_x = vx + wt - wz;
        data->tc_act_tgt_x = vx + wt - 40;

        data->tc_ribbon_y = -200;
        data->tc_title_x = SCREEN_XRES + wt;
        data->tc_zone_x  = SCREEN_XRES + wt;
        data->tc_act_x   = SCREEN_XRES + wt;
    }
}


static void
level_set_clearcolor()
{
    if(level == 2 || level == 3) // R1
        set_clear_color(LERPC(level_fade, 26),
                        LERPC(level_fade, 104),
                        LERPC(level_fade, 200));
    else if(level == 4 || level == 5) // R2 (GHZ)
        set_clear_color(LERPC(level_fade, 36),
                        LERPC(level_fade, 0),
                        LERPC(level_fade, 180));
    else if(level == 6 || level == 7) // R3 (SWZ)
        set_clear_color(0, 0, 0);
    else if(level == 10 || level == 11) // R5 (AOZ)
        set_clear_color(LERPC(level_fade, 56),
                        LERPC(level_fade, 104),
                        LERPC(level_fade, 200));
    else if(level == 16 || level == 17 || level == 18) // R8 (EZ)
        set_clear_color(LERPC(level_fade, 0x21), 0x00, 0x00);
    // R0
    else set_clear_color(LERPC(level_fade, 63),
                         LERPC(level_fade, 0),
                         LERPC(level_fade, 127));
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


void
screen_level_setmode(LEVELMODE mode)
{
    level_mode = mode;
}


static void
level_play_music(uint8_t round, uint8_t act)
{
    switch(round) {
    case 0:
        switch(act) {
        case 0: sound_bgm_play(BGM_PLAYGROUND1); break;
        case 1: sound_bgm_play(BGM_PLAYGROUND2); break;
        case 2: sound_bgm_play(BGM_PLAYGROUND3); break;
        case 3: sound_bgm_play(BGM_PLAYGROUND4); break;
        };
        break;
    case 2: sound_bgm_play(BGM_GREENHILL);       break;
    case 3: sound_bgm_play(BGM_SURELYWOOD);      break;
    case 4: sound_bgm_play(BGM_DAWNCANYON);      break;
    case 5: sound_bgm_play(BGM_AMAZINGOCEAN);    break;
    case 6: break; // TODO
    case 7: break; // TODO
    case 8: sound_bgm_play(BGM_EGGMANLAND);      break;
    default: break;
    }
}
