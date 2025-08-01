#include "screens/level.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
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
#include "boss.h"

extern int debug_mode;

extern SoundEffect sfx_ring;
extern SoundEffect sfx_switch;
extern SoundEffect sfx_kach;
extern SoundEffect sfx_event;

static uint8_t level = 0;
static PlayerCharacter level_character = CHARA_SONIC;

#define LEVEL_BONUS_SPD 12
#define ANIM_IDLE_TIMER_MAX 180 // Also defined in player.c

// Accessible in other source
Player      *player;
uint8_t     paused = 0;
uint8_t     paused_selection = 0;
TileMap16   *map16;
TileMap128  *map128;
LevelData   *leveldata;
Camera      *camera;
ObjectTable *obj_table_common;
ObjectTable *obj_table_level;
uint8_t     level_round; // Defined after load
uint8_t     level_act;   // Defined after load
uint8_t     level_fade;
uint16_t    level_ring_count;
uint16_t    level_ring_max;
uint32_t    level_score_count;
uint8_t     level_finished;
int32_t     level_water_y;
LEVELMODE   level_mode;
uint8_t     level_has_boss;
BossState   *boss;

typedef struct {
    LEVEL_TRANSITION level_transition;
    Parallax   parallax;
    uint8_t    parallax_tx_mode;
    int32_t    parallax_px;
    int32_t    parallax_py;
    int32_t    parallax_cx;
    int32_t    parallax_cy;
    const char *level_name;
    uint16_t   level_counter;
    uint8_t    boss_lock;
    uint8_t    ring_1up_mask;

    // Title card / End count variables
    uint8_t has_started;
    int16_t tc_ribbon_y;
    int16_t tc_title_x;
    int16_t tc_zone_x;
    int16_t tc_act_x;
    int16_t tc_ribbon_tgt_y;
    int16_t tc_title_tgt_x;
    int16_t tc_zone_tgt_x;
    int16_t tc_act_tgt_x;

    uint32_t time_bonus;
    uint32_t ring_bonus;
    uint32_t perfect_bonus;
    uint32_t total_bonus;
    uint8_t is_perfect;
    int16_t bonus_distance_threshold;

    // Water overlay primitives
    TILE     waterquad[2]; // Flat-shaded option
    /* POLY_G4  waterquad[2]; // Gouraud-shaded option */
    POLY_FT4 wavequad[2][5];
    uint8_t  waterbuffer;
    uint8_t  water_last_fade[2];
} screen_level_data;

// Forward function declarations
static void level_load_player(PlayerCharacter character);
static void level_load_level(screen_level_data *);
static void level_set_clearcolor();
static void prepare_titlecard(screen_level_data *data);

void
screen_level_load()
{
    screen_level_data *data = screen_alloc(sizeof(screen_level_data));
    player = screen_alloc(sizeof(Player));
    camera = screen_alloc(sizeof(Camera));
    map16 = screen_alloc(sizeof(TileMap16));
    map128 = screen_alloc(sizeof(TileMap128));
    leveldata = screen_alloc(sizeof(LevelData));
    obj_table_common = screen_alloc(sizeof(ObjectTable));
    obj_table_level = screen_alloc(sizeof(ObjectTable));

    data->level_transition = LEVEL_TRANS_TITLECARD;
    data->level_name = "";
    level_act  = 0;
    data->boss_lock = 0;
    data->ring_1up_mask = 0;
    data->has_started = 0;

    camera_init(camera);

    level_load_player(level_character);

    level_ring_max = 0;
    level_load_level(data);

    camera_set(camera, player->pos.vx, player->pos.vy);

    reset_elapsed_frames();
    pause_elapsed_frames();

    level_fade = 0;
    data->level_counter = 120;

    level_ring_count = 0;
    level_finished = 0;

    data->time_bonus = 0;
    data->ring_bonus = 0;
    data->perfect_bonus = 0;
    data->is_perfect = 0;
    data->bonus_distance_threshold = SCREEN_XRES + CENTERX;

    // Init water quads
    for(int i = 0; i < 2; i++) {
        // Flat-shaded option
        TILE *poly = &data->waterquad[i];
        setTile(poly);
        setSemiTrans(poly, 1);
        setRGB0(poly, 0, 0, 0);
        setXY0(poly, 0, 0);
        setWH(poly, SCREEN_XRES, 0);

        // Gouraud-shaded option
        /* POLY_G4 *poly = &data->waterquad[i]; */
        /* setPolyG4(poly); */
        /* setSemiTrans(poly, 1); */
        /* setRGB0(poly, 0, 0, 0); */
        /* setRGB1(poly, 0, 0, 0); */
        /* setRGB2(poly, 0, 0, 0); */
        /* setRGB3(poly, 0, 0, 0); */
        /* setXYWH(poly, 0, 0, SCREEN_XRES, 0); */

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

    demo_init();

    // init proper RNG per level
    srand(get_global_frames());

    // If it is a demo or we're recording, skip title card
    if(level_mode == LEVEL_MODE_DEMO || level_mode == LEVEL_MODE_RECORD) {
        data->level_transition = LEVEL_TRANS_FADEIN;
    }

    // Recover control if mode is "hold forward" or "do nothing"
    if(level_mode == LEVEL_MODE_FINISHED || level_mode == LEVEL_MODE_FINISHED2)
        level_mode = LEVEL_MODE_NORMAL;

    camera_follow_player(camera);
}

void
screen_level_unload(void *d)
{
    (void)(d);
    level_fade = 0;
    sound_cdda_stop();
    sound_reset_mem();
    screen_free();
}

static void
_calculate_level_bonus(screen_level_data *data)
{
    data->ring_bonus = level_ring_count * 100;

    // Time bonus
    uint32_t seconds = get_elapsed_frames() / 60;
    if(seconds <= 29)       data->time_bonus = 50000; // Under 0:30
    else if(seconds <= 44)  data->time_bonus = 10000; // Under 0:45
    else if(seconds <= 59)  data->time_bonus = 5000;  // Under 1:00
    else if(seconds <= 89)  data->time_bonus = 4000;  // Under 1:30
    else if(seconds <= 119) data->time_bonus = 3000;  // Under 2:00
    else if(seconds <= 179) data->time_bonus = 2000;  // Under 3:00
    else if(seconds <= 239) data->time_bonus = 1000;  // Under 4:00
    else if(seconds <= 299) data->time_bonus = 500;   // Under 5:00
    // Otherwise you get nothing

    // Perfect bonus
    data->is_perfect = (level_ring_max == 0);
    if(data->is_perfect)
        data->perfect_bonus = 50000;
}

static void
prepare_titlecard(screen_level_data *data)
{
    // Pre-calculate title card target X and Y positions
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

    data->level_counter = 120;
    level_fade = 0;
    data->level_transition = LEVEL_TRANS_TITLECARD;
}

void
screen_level_player_respawn()
{
    screen_level_data *data = screen_get_data();

    // TODO: Deaths on demo mode will cause problems!
    screen_level_setmode(LEVEL_MODE_NORMAL);

    // Show loading screen
    render_loading_logo();

    // Restore camera
    camera_init(camera);
    camera_follow_player(camera);

    // Restore player
    player->pos = player->respawnpos;
    camera->pos = camera->realpos = player->respawnpos;
    player->grnd = 0;
    player->anim_dir = 1;
    player->vel.vx = player->vel.vy = player->vel.vz = 0;
    player->psmode = player->gsmode = CDIR_FLOOR;
    player->cnst = getconstants(player->character, PC_DEFAULT);
    player->speedshoes_frames = (player->speedshoes_frames > 0) ? 0 : -1;
    player->shield = 0;
    player->iframes = 0;
    player->anim_frame = player->tail_anim_frame = 0;
    player->idle_timer = ANIM_IDLE_TIMER_MAX;
    player->action = ACTION_NONE;
    player->remaining_air_frames = 1800;
    player->death_type = 0;

    // Solve underwater behaviour
    player->underwater = (level_water_y >= 0) && (player->pos.vy > level_water_y);
    player->cnst =
        getconstants(player->character,
                     player->underwater ? PC_UNDERWATER : PC_DEFAULT);

    // Prepare titlecard
    prepare_titlecard(data);

    // Destroy ALL free objects (take special care in case of bosses with extra objects)
    object_pool_init();

    // Zero out the ring count
    level_ring_count = 0;

    // Restore any boss state
    if(level_has_boss) bzero(boss, sizeof(BossState));

    // UNLOAD ALL STATIC OBJECTS (except checkpoints)
    if(data->has_started)
        unload_object_placements(leveldata);

    // Stop music
    sound_cdda_stop();

    // RELOAD ALL OBJECTS (ignores checkpoints)
    // WARNING --
    // KNOWN MEMORY LEAK: Since Monitors have an EXTRA struct, every reload
    // of a level recreates this "EXTRA" struct, which is currently one byte,
    // so I'm going to ignore this because the cost is so low and the game
    // will reset the entire arena when skipping levels. Don't judge me.
    char basepath[255];
    char filename[255];
    snprintf(basepath, 255, "\\LEVELS\\R%1u", level_round);
    snprintf(filename, 255, "%s\\Z%1u.OMP;1", basepath, level_act + 1);
    load_object_placement(filename, leveldata, data->has_started);
    level_ring_max = count_emplaced_rings(leveldata);

    // Restart music
    data->boss_lock = 0;
    screen_level_play_music(level_round, level_act);
}

void
screen_level_update(void *d)
{
    screen_level_data *data = (screen_level_data *)d;

    // Debug mode cycling
#ifdef ALLOW_DEBUG
    {
        if(pad_pressing(PAD_L1) && pad_pressed(PAD_R1))
            debug_mode++;
        else if(pad_pressed(PAD_L1) && pad_pressing(PAD_R1))
            debug_mode--;
        if(debug_mode > 2) debug_mode = 0;
        else if(debug_mode < 0) debug_mode = 2;
    }
#endif

    level_set_clearcolor();

    // Manage level transitions/events
    if(data->level_transition == LEVEL_TRANS_TITLECARD) {
        data->level_counter--;
        if(data->level_counter == 0)
            data->level_transition = LEVEL_TRANS_FADEIN;
    } else if(data->level_transition == LEVEL_TRANS_FADEIN) {
        level_fade += 2;
        if(level_fade >= 128) {
            level_fade = 128;
            data->level_transition = LEVEL_TRANS_GAMEPLAY;

            // Start level timer
            if(!data->has_started) {
                data->has_started = 1;
                reset_elapsed_frames();
            } else resume_elapsed_frames();
        }
    }
    // 2: Gameplay
    else if(data->level_transition == LEVEL_TRANS_SCORE_IN) {
        // Move score count into screen
        data->bonus_distance_threshold =
            MAX(data->bonus_distance_threshold - LEVEL_BONUS_SPD, 0);

        data->level_counter--;
        if(data->level_counter == 0) {
            data->level_transition = LEVEL_TRANS_SCORE_COUNT;
        }
    } else if(data->level_transition == LEVEL_TRANS_SCORE_COUNT) {
        // If all counters are zero, move along
        if((data->time_bonus | data->ring_bonus | data->perfect_bonus) == 0) {
            data->level_counter = 120; // Next counter does 2 seconds
            data->level_transition = LEVEL_TRANS_SCORE_OUT;
            sound_play_vag(sfx_kach, 0);
        } else {
            if(data->level_counter > 0) data->level_counter--;
            else {
                uint32_t score_aggregate = 0;

#define INC_AGGREGATE(x)                        \
                if(x > 100) {                   \
                    score_aggregate += 100;     \
                    x -= 100;                   \
                } else {                        \
                    score_aggregate += x;       \
                    x = 0;                      \
                }

                INC_AGGREGATE(data->time_bonus);
                INC_AGGREGATE(data->ring_bonus);
                INC_AGGREGATE(data->perfect_bonus);

                data->total_bonus += score_aggregate;
                level_score_count += score_aggregate;
                sound_play_vag(sfx_switch, 0);
                data->level_counter = 2;

                // Shortcut to count everything instantly
                if(pad_pressing(PAD_START) || pad_pressing(PAD_CROSS)) {
                    score_aggregate = 0;
                    score_aggregate += data->time_bonus;
                    score_aggregate += data->ring_bonus;
                    score_aggregate += data->perfect_bonus;
                    data->time_bonus = data->ring_bonus = data->perfect_bonus = 0;
                    data->total_bonus += score_aggregate;
                    level_score_count += score_aggregate;
                }
            }
        }
    } else if(data->level_transition == LEVEL_TRANS_SCORE_OUT) {
        // Wait 2 secs then fade out
        if(data->level_counter > 0) data->level_counter--;
        else data->level_transition = LEVEL_TRANS_FADEOUT;
    } else if(data->level_transition == LEVEL_TRANS_FADEOUT) {
        // Move bonus text away
        data->bonus_distance_threshold =
            MIN(data->bonus_distance_threshold + LEVEL_BONUS_SPD,
                SCREEN_XRES + CENTERX);

        level_fade -= 2;
        if(level_fade == 0) {
            data->level_transition = LEVEL_TRANS_NEXT_LEVEL;
        }
    }
    else if(data->level_transition == LEVEL_TRANS_NEXT_LEVEL) {
        screen_level_transition_to_next();
        return;
    } else if(data->level_transition == LEVEL_TRANS_DEATH_WAIT) {
        // Countdown timer when player is out of the screen
        if(player->pos.vy <= camera->pos.vy + (CENTERY << 12)) {
            // Prepare to wait for 1 second
            data->level_counter = 60;
        }
        else if(data->level_counter > 0) data->level_counter--;
        else {
            // If you died during a demo, transition to "next level"
            // instead (which will actually redirect to the title screen)
            if(level_mode == LEVEL_MODE_DEMO)
                data->level_transition = LEVEL_TRANS_FADEOUT;
            else data->level_transition = LEVEL_TRANS_DEATH_FADEOUT;
        }
    } else if(data->level_transition == LEVEL_TRANS_DEATH_FADEOUT) {
        level_fade -= 2;
        if(level_fade == 0) {
            screen_level_player_respawn();
        }
    }

    // Manage title card depending on level transition
    {
        const uint16_t speed = 16;
        if(data->level_transition == LEVEL_TRANS_TITLECARD) {
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
        } else if(data->level_transition == LEVEL_TRANS_FADEIN) {
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
           && (data->level_transition == LEVEL_TRANS_GAMEPLAY)) {
            paused = !paused;
            if(paused) {
                paused_selection = 0;
                sound_cdda_set_mute(1);
            }
            else sound_cdda_set_mute(0);
        }
    } else {
        // If in demo mode, absolutely any button press will
        // trigger its end!
        uint32_t seconds = get_elapsed_frames() / 60;
        if((pad_pressed_any() || (seconds >= 30))
           && (data->level_transition == LEVEL_TRANS_GAMEPLAY)) {
            data->level_transition = LEVEL_TRANS_FADEOUT;
        }

        // Uncomment to test deaths on demo mode.
        /* if(player->death_type == 0 && seconds == 10) { */
        /*     level_ring_count = 0; */
        /*     player_do_damage(player, player->pos.vx); */
        /* } */

        if(data->level_transition == LEVEL_TRANS_NEXT_LEVEL) {
            // Go back to title screen
            scene_change(SCREEN_TITLE);
            return;
        }
    }
    
    if(paused) {
        if(debug_mode) {
            uint8_t updated = 0;
            if(pad_pressing(PAD_UP)) {
                player->pos.vy -= 40960;
                updated = 1;
            }

            if(pad_pressing(PAD_DOWN)) {
                player->pos.vy += 40960;
                updated = 1;
            }

            if(pad_pressing(PAD_LEFT)) {
                player->pos.vx -= 40960;
                updated = 1;
            }

            if(pad_pressing(PAD_RIGHT)) {
                player->pos.vx += 40960;
                updated = 1;
            }

            if(updated) {
                player->over_object = NULL;
                camera_update(camera, player);
            }

            if(pad_pressed(PAD_SELECT)) {
                scene_change(SCREEN_LEVELSELECT);
                return;
            }
        } else {
            if(pad_pressed(PAD_DOWN)) {
                if(paused_selection < 2) {
                    sound_play_vag(sfx_switch, 0);
                    paused_selection++;
                }
            }
            else if(pad_pressed(PAD_UP)) {
                if(paused_selection > 0) {
                    sound_play_vag(sfx_switch, 0);
                    paused_selection--;
                }
            }
            else if(
                // Let code above handle un-pausing with start,
                // since we may want to unpause during debug mode
                (pad_pressed(PAD_START) && (paused_selection > 0))
                || pad_pressed(PAD_CROSS)) {
                paused = 0;
                switch(paused_selection) {
                default: // Case 0 -- Continue
                    sound_cdda_set_mute(0);
                    break;
                case 1: // Restart
                    screen_level_player_respawn();
                    return;
                case 2: // Quit
                    scene_change(SCREEN_TITLE);
                    return;
                }
            }
        }
        
        return;
    }

    if(debug_mode > 0) {
        // Create a little falling ring
        if(pad_pressed(PAD_TRIANGLE)) {
            PoolObject *ring = object_pool_create(OBJ_RING);
            ring->freepos.vx = camera->pos.vx;
            ring->freepos.vy = camera->pos.vy - (CENTERY << 12) + (20 << 12);
            ring->props |= OBJ_FLAG_ANIM_LOCK;
            ring->props |= OBJ_FLAG_RING_MOVING;
        }

        // Respawn
        if(pad_pressed(PAD_SELECT) && !level_finished) {
            screen_level_player_respawn();
        }

        if(pad_pressed(PAD_CIRCLE)) {
            player_do_damage(player, player->pos.vx);
        }
    }

    // Input management according to level mode.
    if(player->death_type > 0) {
        // If dead, force no input, no X speed and face right
        player->input.current = player->input.old = 0x0000;
        player->vel.vx = 0;
        player->anim_dir = 1;
    } else {
        switch(level_mode) {
        case LEVEL_MODE_DEMO:
            demo_update_playback(level, &player->input);
            break;
        case LEVEL_MODE_RECORD:
            demo_record();
            input_get_state(&player->input);
            break;
        case LEVEL_MODE_FINISHED:
            player->input.current = player->input.old = 0x0020;
            break;
        case LEVEL_MODE_FINISHED2:
            player->input.current = player->input.old = 0x0000;
            break;
        default:
            input_get_state(&player->input);
            break;
        }
    }

    camera_update(camera, player);
    update_obj_window(camera->pos.vx, camera->pos.vy, level_round);
    object_pool_update(level_round);

    // Only update these if past fade in!
    if(data->level_transition > LEVEL_TRANS_TITLECARD) {
        player_update(player);
    }

    // Limit player left position
    if((player->pos.vx - (PUSH_RADIUS << 12)) < (camera->min_x - (CENTERX << 12))) {
        player->pos.vx = camera->min_x - (CENTERX << 12) + (PUSH_RADIUS << 12);
        if(player->vel.vx < 0) {
            if(player->grnd) player->vel.vz = 0;
            else player->vel.vx = 0;
        }
    }

    // Limit player top position
    if((player->pos.vy - (16 << 12) < 0) && (player->vel.vy < 0)) {
        player->pos.vy = (16 << 12);
        player->vel.vy = 0;
        if(player->action == ACTION_FLY) player->spinrev = 0;
    }

    // Limit player position when camera is fixed
    if(
        (!camera->follow_player)
        && (player->pos.vx - (PUSH_RADIUS << 12)) < (camera->pos.vx - (CENTERX << 12)))
    {
        player->pos.vx = camera->pos.vx - (CENTERX << 12) + (PUSH_RADIUS << 12);
        if(player->vel.vx < 0) {
            if(player->grnd) player->vel.vz = 0;
            else player->vel.vx = 0;
        }
    } else if(
        (level_finished != 1)
        && (!camera->follow_player)
        && (player->pos.vx + (PUSH_RADIUS << 12)) > (camera->pos.vx + (CENTERX << 12)))
    {
        player->pos.vx = camera->pos.vx + (CENTERX << 12) - (PUSH_RADIUS << 12);
        if(player->vel.vx > 0) {
            if(player->grnd) player->vel.vz = 0;
            else player->vel.vx = 0;
        }
    }

    // If speed shoes are finished, we use the player's values
    // as a flag to resume music playback.
    // Player constants are managed within player update
    if(player->speedshoes_frames == 0) {
        if(!level_finished)
            screen_level_play_music(level_round, level_act);
        player->speedshoes_frames = -1;
    }
}

void
_screen_level_draw_water(screen_level_data *data)
{
    if(level_water_y >= 0) {
        int32_t camera_bottom = camera->pos.vy + (CENTERY << 12);

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

                /* GOURAUD SHADE, STATIC BUFFERS */
                /* POLY_G4 *poly = &data->waterquad[data->waterbuffer]; */
                /* setXYWH(poly, 0, water_y, SCREEN_XRES, water_h); */
                /* if(data->water_last_fade[data->waterbuffer] != level_fade) { */
                /*     setRGB0(poly, */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0xb8)); */
                /*     setRGB1(poly, */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0xb8)); */
                /*     setRGB2(poly, */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0x18)); */
                /*     setRGB3(poly, */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0), */
                /*             LERPC(level_fade, 0x18)); */
                /* } */
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
                            LERPC(level_fade, 0x68));
                    setRGB0(&data->waterquad[data->waterbuffer ^ 1],
                            LERPC(level_fade, 0),
                            LERPC(level_fade, 0),
                            LERPC(level_fade, 0x68));
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
    char buffer[120] = { 0 };

    // As a rule of thumb, things are drawn in specific otz's.
    // When things are drawn on the same otz, anything drawn first
    // is shown on front, as the ordering table is drawn backwards.

    _screen_level_draw_water(data);

    // Draw player
    if(abs((player->pos.vx - camera->pos.vx) >> 12) <= SCREEN_XRES
       && abs((player->pos.vy - camera->pos.vy) >> 12) <= SCREEN_YRES) {
        VECTOR player_canvas_pos = {
            player->pos.vx - camera->pos.vx + (CENTERX << 12),
            player->pos.vy - camera->pos.vy + (CENTERY << 12),
            0
        };
        player_draw(player, &player_canvas_pos);
    }

    // Draw free objects
    object_pool_render(camera->pos.vx, camera->pos.vy);

    // Draw level and level objects
    render_lvl(camera->pos.vx, camera->pos.vy,
               level_round == 4); // Dawn Canyon: Draw in front

    // Draw background and parallax
    if(level_get_num_sprites() < 1312)
        parallax_draw(&data->parallax, camera);

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
    else if(level_round == 5) {
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
    if(paused && !debug_mode) {
        const char *line1 = "\awPaused\r";
        int16_t x = CENTERX - (font_measurew_big(line1) >> 1);
        font_draw_big(line1, x, CENTERY - 28);

        if(paused_selection == 0) font_set_color_yellow();
        else                      font_reset_color();
        line1 = "Continue\r";
        x = CENTERX - (font_measurew_sm(line1) >> 1);
        font_draw_sm(line1, x, CENTERY - 4);

        if(paused_selection == 1) font_set_color_yellow();
        else                      font_reset_color();
        line1 = "Restart\r";
        x = CENTERX - (font_measurew_sm(line1) >> 1);
        font_draw_sm(line1, x, CENTERY + 4);

        if(paused_selection == 2) font_set_color_yellow();
        else font_reset_color();
        line1 = "Quit\r";
        x = CENTERX - (font_measurew_sm(line1) >> 1);
        font_draw_sm(line1, x, CENTERY + 12);
    }

    // Title card
    if(data->level_transition <= LEVEL_TRANS_FADEIN) {
        font_reset_color();
        font_draw_hg(data->level_name, data->tc_title_x, 70);
        font_draw_hg("ZONE", data->tc_zone_x, 70 + GLYPH_HG_WHITE_HEIGHT + 5);

        // ACT card
        char buffer[5];
        uint8_t act_number = (level == 3) ? 2 : level_act;
        snprintf(buffer, 5, "*%d", act_number + 1);
        font_draw_hg(buffer, data->tc_act_x, 70 + GLYPH_HG_WHITE_HEIGHT + 40);

        // Game text
        //font_set_color(0xc8, 0xc8, 0x00);
        uint16_t wt = font_measurew_sm("SONIC XA");
        font_draw_sm("SONIC XA", 50 + ((80 - wt) >> 1), data->tc_ribbon_y + 180);
        font_reset_color();

        // Title card ribbon background
        {
            POLY_G4 *polyg = get_next_prim();
            increment_prim(sizeof(POLY_G4));
            setPolyG4(polyg);
            setRGB0(polyg, 0xc9, 0xc9, 0xc9);
            setRGB1(polyg, 0xc9, 0xc9, 0xc9);
            switch(level_character) {
            case CHARA_SONIC:
                setRGB2(polyg, 0x00, 0x24, 0xd8);
                setRGB3(polyg, 0x00, 0x24, 0xd8);
                break;
            case CHARA_MILES:
                setRGB2(polyg, 0xd8, 0x48, 0x00);
                setRGB3(polyg, 0xd8, 0x48, 0x00);
                break;
            case CHARA_KNUCKLES:
                setRGB2(polyg, 0xd0, 0x00, 0x40);
                setRGB3(polyg, 0xd0, 0x00, 0x40);
                break;
            default:
                setRGB2(polyg, 0x00, 0x24, 0xd8);
                setRGB3(polyg, 0x00, 0x24, 0xd8);
                break;
            }
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

    // Score count
    if(data->level_transition >= LEVEL_TRANS_SCORE_IN) {
        int16_t thrsh = data->bonus_distance_threshold;

        // TODO: Don't just display this! We gotta have a transition
        char buffer[20];
        const char *ctxt = "";
        switch(screen_level_getcharacter()) {
        default:             ctxt = "\asSONIC\r";    break;
        case CHARA_MILES:    ctxt = "\atTAILS\r";    break;
        case CHARA_KNUCKLES: ctxt = "\akKNUCKLES\r"; break;
        }
        snprintf(buffer, 20, "%s GOT", ctxt);

        const uint16_t text_base_y = 50;

        // Measure first part
        uint16_t textlen = font_measurew_md(buffer) >> 1;
        font_draw_md(buffer, CENTERX - textlen - thrsh, text_base_y);

        // Measure second part
        ctxt = "THROUGH";
        textlen = font_measurew_md(ctxt) >> 1;
        font_draw_md(ctxt, CENTERX - textlen + thrsh, text_base_y + GLYPH_MD_WHITE_HEIGHT);

        // Measure act
        uint8_t act_number = (level == 3) ? 2 : level_act;
        snprintf(buffer, 5, "*%d", act_number + 1);
        font_draw_hg(buffer, CENTERX + textlen - (GLYPH_HG_WHITE_WIDTH >> 1) + thrsh, text_base_y + 20);

        const uint16_t counters_base_y = CENTERY;

        // Score point
        uint16_t cty = counters_base_y;
        uint16_t txtx = (CENTERX >> 1) + (CENTERX >> 3);
        uint16_t ctx = SCREEN_XRES - (CENTERX >> 1);

        ctxt = "\ayTIME BONUS\r";
        textlen = font_measurew_big(ctxt) >> 1;
        font_draw_big(ctxt, txtx - textlen - thrsh, cty);
        snprintf(buffer, 20, "\aw%d\r", data->time_bonus);
        textlen = font_measurew_big(buffer);
        font_draw_big(buffer, ctx - textlen + thrsh, cty);
        cty += GLYPH_WHITE_HEIGHT + 2;

        ctxt = "\ayRING BONUS\r";
        textlen = font_measurew_big(ctxt) >> 1;
        font_draw_big(ctxt, txtx - textlen - thrsh, cty);
        snprintf(buffer, 20, "\aw%d\r", data->ring_bonus);
        textlen = font_measurew_big(buffer);
        font_draw_big(buffer, ctx - textlen + thrsh, cty);
        cty += GLYPH_WHITE_HEIGHT + 2;

        if(data->is_perfect) {
            ctxt = "\ayPERFECT BONUS\r";
            textlen = font_measurew_big(ctxt) >> 1;
            font_draw_big(ctxt, txtx - textlen - thrsh, cty);
            snprintf(buffer, 20, "\aw%d\r", data->perfect_bonus);
            textlen = font_measurew_big(buffer);
            font_draw_big(buffer, ctx - textlen + thrsh, cty);
        }
        cty += GLYPH_WHITE_HEIGHT + 4;

        ctxt = "\ayTOTAL\r";
        textlen = font_measurew_big(ctxt) >> 1;
        font_draw_big(ctxt, txtx - textlen - thrsh, cty);
        snprintf(buffer, 20, "\aw%d\r", data->total_bonus);
        textlen = font_measurew_big(buffer);
        font_draw_big(buffer, ctx - textlen + thrsh, cty);
    }

    // Demo HUD. Only when playing AutoDemo!
    /* if(level_mode == LEVEL_MODE_DEMO) { */
    /*     // Uses HUD layer to draw! */
    /*     font_draw_logo(20, SCREEN_YRES - 65, 120, 45); */
    /* } */

    // Heads-up display
    if((debug_mode <= 1) && (level_mode != LEVEL_MODE_DEMO)) {
        font_set_color(
            LERPC(level_fade, 0xc8),
            LERPC(level_fade, 0xc8),
            0);
        font_draw_big("SCORE", 10, 10);
        font_draw_big("TIME",  10, 24);

        // Flash red every 8 frames
        if(!elapsed_frames_paused()
           && (level_ring_count == 0)
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

        snprintf(buffer, 120, "%8d", level_score_count);
        font_draw_big(buffer, 60, 10);

        {
            uint32_t seconds = get_elapsed_frames() / 60;
            snprintf(buffer, 120, "%2d:%02d", seconds / 60, seconds % 60);
            font_draw_big(buffer, 54, 24);
        }

        snprintf(buffer, 120, "%3d", level_ring_count);
        font_draw_big(buffer, 60, 38);

        font_reset_color();
    }

    if(debug_mode) {
        font_set_color(0xc8, 0xc8, 0xc8);

        // Video debug
        snprintf(buffer, 120,
                 "%4s %3d",
                 GetVideoMode() == MODE_PAL ? "PAL" : "NTSC", get_frame_rate());
        font_draw_sm(buffer, 248, 12);

        // Free object debug
        snprintf(buffer, 120, "SPR  %3d", object_pool_get_count());
        font_draw_sm(buffer, 248, 20);

        // Rings, time and air for convenience
        snprintf(buffer, 120, "RING %03d", level_ring_count);
        font_draw_sm(buffer, 248, 28);

        snprintf(buffer, 120, "TIME %03d", (get_elapsed_frames() / 60));
        font_draw_sm(buffer, 248, 36);

        snprintf(buffer, 120, "AIR   %02d", player->remaining_air_frames / 60);
        font_draw_sm(buffer, 248, 44);

        snprintf(buffer, 120, "TILE%4d", level_get_num_sprites());
        font_draw_sm(buffer, 248, 52);

        snprintf(buffer, 120, "FRA%5d", player->framecount);
        font_draw_sm(buffer, 248, 60);

        snprintf(buffer, 120, "PFT %4d", level_ring_max);
        font_draw_sm(buffer, 248, 68);

        // Player debug
        if(debug_mode > 1) {
            snprintf(buffer, 255,
                     "GSP %08x\n"
                     "SPD %08x %08x\n"
                     "ANG %08x G.P %s %s %3d\n"
                     "POS %08x %08x\n"
                     "ACT %02u\n"
                     "GRN CEI %01u %01u\n"
                     ,
                     player->vel.vz,
                     player->vel.vx, player->vel.vy,
                     player->angle,
                     (player->gsmode == CDIR_FLOOR)
                     ? "FL"
                     : (player->gsmode == CDIR_RWALL)
                     ? "RW"
                     : (player->gsmode == CDIR_LWALL)
                     ? "LW"
                     : (player->gsmode == CDIR_CEILING)
                     ? "CE"
                     : "  ",
                     (player->psmode == CDIR_FLOOR)
                     ? "FL"
                     : (player->psmode == CDIR_RWALL)
                     ? "RW"
                     : (player->psmode == CDIR_LWALL)
                     ? "LW"
                     : (player->psmode == CDIR_CEILING)
                     ? "CE"
                     : "  ",
                     (int32_t)(((int32_t)player->angle * (int32_t)(360 << 12)) >> 24), // angle in deg
                     player->pos.vx, player->pos.vy,
                     player->action,
                     player->grnd, player->ceil
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
level_load_player(PlayerCharacter character)
{
    const char *chara_file = "\\SPRITES\\SONIC.CHARA;1";
    const char *tim_file = "\\SPRITES\\SONIC.TIM;1";

    switch(character) {
    case CHARA_MILES:
        chara_file = "\\SPRITES\\MILES.CHARA;1";
        tim_file = "\\SPRITES\\MILES.TIM;1";
        break;
    case CHARA_KNUCKLES:
        chara_file = "\\SPRITES\\KNUX.CHARA;1";
        tim_file = "\\SPRITES\\KNUX.TIM;1";
        break;
    case CHARA_SONIC:
    default: break;
    }
    
    uint32_t filelength;
    TIM_IMAGE tim;
    uint8_t *timfile = file_read(tim_file, &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }

    load_player(player, character, chara_file, &tim);
    player->startpos = (VECTOR){ 250 << 12, CENTERY << 12, 0 };
    player->pos = player->startpos;
}

static void
level_load_level(screen_level_data *data)
{
    paused = 0;
    level_has_boss = 0;

    // Negative water means no water
    level_water_y = -1;

    data->level_name = "PLACEHOLDER";

    switch(level) {
    case 0: case 1: case 2: case 3: // Test level
        data->level_name = "TEST LEVEL";
        level_round = 0;
        // Act 4 is Knuckles act 3
        level_act = level;
        if(level == 2) {
            level_water_y = 0x00c43401;
        }
        break;
    case 4: case 5:
        data->level_name = "GREEN HILL";
        level_round = 2;
        level_act = level - 4;
        break;
    case 6: case 7:
        data->level_name = "SURELY WOOD";
        level_round = 3;
        level_act = level - 6;
        break;
    case 8: case 9:
        data->level_name = "DAWN CANYON";
        level_round = 4;
        level_act = level - 8;
        break;
    case 10: case 11:
        data->level_name = "AMAZING OCEAN";
        level_round = 5;
        level_act = level - 10;
        level_water_y = 0x002c0000;
        break;
    case 12: case 13:
        /* data->level_name = "R6"; */
        level_round = 6;
        level_act = level - 12;
        break;
    case 14: case 15:
        /* data->level_name = "R7"; */
        level_round = 7;
        level_act = level - 14;
        break;
    case 16: case 17: case 18:
        data->level_name = "EGGMANLAND";
        level_round = 8;
        level_act = level - 16;
        break;
    case 19:
        data->level_name = "WINDMILL ISLE";
        level_round = 9;
        level_act = level - 19;
        break;
    default:
        data->level_name = "TEST LEVEL";
        level_round = 0xff;
        level_act = 0;
        break;
    }

    char basepath[255];
    char filename0[255], filename1[255];

    level_set_clearcolor();

    snprintf(basepath, 255, "\\LEVELS\\R%1u", level_round);

    TIM_IMAGE tim;
    uint32_t filelength;



    /* === LEVEL TILES === */
    // Load level tiles
    snprintf(filename0, 255, "%s\\TILES.TIM;1", basepath);
    printf("Loading %s...\n", filename0);
    uint8_t *timfile = file_read(filename0, &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        leveldata->clutmode = tim.mode;
        free(timfile);
    } else {
        // If not single "TILES.TIM" was found, then perharps try a
        // "TILES0.TIM" and a "TILES1.TIM".
        snprintf(filename0, 255, "%s\\TILES0.TIM;1", basepath);
        printf("Loading %s...\n", filename0);
        timfile = file_read(filename0, &filelength);
        if(timfile) {
            load_texture(timfile, &tim);
            leveldata->clutmode = tim.mode; // Use CLUT mode from 1st texture
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
    load_map16(map16, filename0, filename1);
    snprintf(filename0, 255, "%s\\MAP128.MAP;1", basepath);
    printf("Loading %s...\n", filename0);
    load_map128(map128, filename0);



    /* === LEVEL LAYOUT === */
    snprintf(filename0, 255, "%s\\Z%1u.LVL;1", basepath, level_act + 1);
    printf("Loading %s...\n", filename0);
    load_lvl(leveldata, filename0);



    /* === OBJECTS === */
    // Load common objects
    printf("Loading common object texture...\n");
    timfile = file_read("\\LEVELS\\COMMON\\OBJ.TIM;1", &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    }
    printf("Loading common object table...\n");
    load_object_table("\\LEVELS\\COMMON\\OBJ.OTD;1", obj_table_common);

    // Load level objects
    snprintf(filename0, 255, "%s\\OBJ.TIM;1", basepath);
    printf("Loading level object texture...\n");
    timfile = file_read(filename0, &filelength);
    if(timfile) {
        load_texture(timfile, &tim);
        free(timfile);
    } else printf("Warning: No level object texture found, skipping\n");

    // Load level boss object, if existing.
    // Warning: This supersedes the second half of level object textures!
    if(((level_round == 0) && (level_act >= 2))
       || (level_act >= 1)) {
        printf("Loading level boss...\n");
        snprintf(filename0, 255, "%s\\BOSS.TIM;1", basepath);
        timfile = file_read(filename0, &filelength);
        if(timfile) {
            level_has_boss = 1;
            load_texture(timfile, &tim);
            /* clut_print_all_colors(&tim); */
            // Setup glowing color palette and reupload it right below
            // the original boss palette
            tim.crect->y += 1;
            clut_set_glow_color(&tim, 0xd3, 0xd3, 0xd3);
            load_clut_only(&tim);

            free(timfile);
        } else printf("Warning: No level boss texture found, skipping\n");

        // Init boss structure
        boss = screen_alloc(sizeof(BossState));
    }

    printf("Loading level object table...\n");
    snprintf(filename0, 255, "%s\\OBJ.OTD;1", basepath);
    load_object_table(filename0, obj_table_level);

    // Load object positioning on level.
    // Always do this AFTER loading object definitions!
    snprintf(filename0, 255, "%s\\Z%1u.OMP;1", basepath, level_act + 1);
    load_object_placement(filename0, leveldata, 0);

    // Load number of rings on level
    level_ring_max = count_emplaced_rings(leveldata);


    /* === OBJECT POOL / FREE OBJECTS === */
    object_pool_init();


    /* === RENDERING PREPARATION === */
    // Pre-allocate and initialize level primitive buffer
    prepare_renderer();

    screen_debrief();

    printf("Number of level layers: %d\n", leveldata->num_layers);

    // Start playback after we don't need the CD anymore.
    screen_level_play_music(level_round, level_act);

    prepare_titlecard(data);
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
    else if(level == 19) // R9 (WIZ)
        set_clear_color(0x00, LERPC(level_fade, 0xa0), LERPC(level_fade, 0xe0));
    // R0
    else set_clear_color(LERPC(level_fade, 63),
                         LERPC(level_fade, 0),
                         LERPC(level_fade, 127));
}

LEVEL_TRANSITION
screen_level_getstate()
{
    screen_level_data *data = screen_get_data();
    return data->level_transition;
}

uint16_t
screen_level_get_counter()
{
    screen_level_data *data = screen_get_data();
    return data->level_counter;
}


void
screen_level_setmode(LEVELMODE mode)
{
    level_mode = mode;
}

LEVELMODE
screen_level_getmode()
{
    return level_mode;
}

void
screen_level_setcharacter(PlayerCharacter character)
{
    level_character = character;
}

PlayerCharacter
screen_level_getcharacter()
{
    return level_character;
}


void
screen_level_play_music(uint8_t round, uint8_t act)
{
    screen_level_data *data = screen_get_data();
    if(data->boss_lock) return;
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
    /* case 4: sound_bgm_play(BGM_DAWNCANYON);      break; */
    case 5: sound_bgm_play(BGM_AMAZINGOCEAN);    break;
    case 6: break; // TODO
    case 7: break; // TODO
    /* case 8: sound_bgm_play(BGM_EGGMANLAND);      break; */
    /* case 9: sound_bgm_play(BGM_WINDMILLISLE);    break; */
    default: break;
    }
}

#include <screens/slide.h>

void
screen_level_transition_to_next()
{
    // WARNING: WHEN CALLING THIS, RETURN IMMEDIATELY SO YOU DON'T
    // OVERWRITE THE NEXT SCREEN WITH JUNK.
    uint8_t lvl = screen_level_getlevel();
    if(lvl == 2 || lvl == 3) {
        // Finished engine test
        scene_change(SCREEN_TITLE);
    } else if(lvl != 5) {
        // If on test level 2 and our character is Knuckles...
        // Go to test level 4 (also an act 3)
        if(lvl == 1) {
            if(screen_level_getcharacter() == CHARA_KNUCKLES) {
                screen_level_setlevel(3);
            } else screen_level_setlevel(2);
        } else if(lvl == 6) {
            // Transition from SWZ1 to AOZ1
            // TODO: THIS IS TEMPORARY
            screen_level_setlevel(10);
        } else if(lvl == 10) {
            // Transition from AOZ1 to GHZ1
            // TODO: THIS IS TEMPORARY
            screen_level_setlevel(4);
        } else screen_level_setlevel(lvl + 1);
        scene_change(SCREEN_LEVEL);
    } else {
        /* screen_slide_set_next(SLIDE_COMINGSOON); */
        screen_slide_set_next(SLIDE_THANKS);
        scene_change(SCREEN_SLIDE);
    }
}

void
screen_level_transition_start_timer()
{
    screen_level_data *data = screen_get_data();
    data->level_counter = 360; // 6 seconds of music
    data->level_transition = LEVEL_TRANS_SCORE_IN;
    _calculate_level_bonus(data);
    sound_bgm_play(BGM_LEVELCLEAR);
}

void
screen_level_transition_death()
{
    screen_level_data *data = screen_get_data();
    data->level_transition = LEVEL_TRANS_DEATH_WAIT;
}

void
screen_level_boss_lock(uint8_t state)
{
    screen_level_data *data = screen_get_data();
    data->boss_lock = state;
}

uint8_t
screen_level_give_1up(int8_t ring_cent)
{
    if(ring_cent < 0) goto give_1up;
    if(ring_cent == 0) return 0;
    ring_cent--;

    screen_level_data *data = screen_get_data();
    uint8_t mask = 1 << ring_cent;
    if((~data->ring_1up_mask) & mask) {
        data->ring_1up_mask |= mask;
        goto give_1up;
    }
    return 0;
 give_1up:
    // TODO
    sound_play_vag(sfx_event, 0);
    return 1;
}

void
screen_level_give_rings(uint16_t amount)
{
    level_ring_count += amount;

    // Give 1-up's at every cent
    if(!screen_level_give_1up(level_ring_count / 100)) {
        sound_play_vag(sfx_ring, 0);
    }
    
}
