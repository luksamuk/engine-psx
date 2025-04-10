#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "player.h"
#include "util.h"
#include "input.h"
#include "render.h"
#include "sound.h"
#include "camera.h"
#include "collision.h"
#include "basic_font.h"
#include "player_constants.h"

#define TMP_ANIM_SPD          7
#define ANIM_IDLE_TIMER_MAX 180

// Adler32 sums of animation names for ease of use
#define ANIM_STOPPED          0x08cd0220
#define ANIM_IDLE             0x02d1011f
#define ANIM_WALKING          0x0854020e
#define ANIM_RUNNING          0x08bf0222
#define ANIM_ROLLING          0x08890218
#define ANIM_SPINDASH         0x0acd025b
#define ANIM_SKIDDING         0x0a85024e
#define ANIM_PEELOUT          0x0849021f
#define ANIM_PUSHING          0x08b2021f
#define ANIM_CROUCHDOWN       0x104802fd
#define ANIM_LOOKUP           0x067001db
#define ANIM_SPRING           0x068e01d4
#define ANIM_HURT             0x031b0144
#define ANIM_DEATH            0x04200167
#define ANIM_DROWN            0x048a018b
#define ANIM_GASP             0x02d9012c
#define ANIM_WATERWALK        0x0da602b3 // SONIC ONLY
#define ANIM_DROP             0x02f80136
#define ANIM_BALANCELIGHT     0x156c035f
#define ANIM_BALANCEHEAVY     0x15570364

#define ANIM_FLYUP            0x04980191 // MILES ONLY
#define ANIM_FLYDOWN          0x086f0224 // MILES ONLY
#define ANIM_FLYTIRED         0x0aee0264 // MILES ONLY
#define ANIM_SWIMMING         0x0b2a026c // MILES ONLY
#define ANIM_SWIMTIRED        0x0e0502b9 // MILES ONLY
#define ANIM_TAILIDLE         0x0a6e0249 // MILES ONLY
#define ANIM_TAILMOVE         0x0ab30262 // MILES ONLY
#define ANIM_TAILFLY          0x08390216 // MILES ONLY

#define ANIM_CLIMBSTOP        0x0d1102ae // KNUCKLKES ONLY
#define ANIM_CLIMBUP          0x0805020d // KNUCKLKES ONLY
#define ANIM_CLIMBDOWN        0x0cd402a0 // KNUCKLKES ONLY
#define ANIM_CLIMBRISE        0x0ce9029b // KNUCKLKES ONLY
#define ANIM_CLIMBEND         0x0a22023f // KNUCKLKES ONLY
#define ANIM_GLIDE            0x04400166 // KNUCKLKES ONLY
#define ANIM_GLIDETURNA       0x100902f0 // KNUCKLKES ONLY
#define ANIM_GLIDETURNB       0x100a02f1 // KNUCKLKES ONLY
#define ANIM_GLIDECANCEL      0x1252030c // KNUCKLKES ONLY
#define ANIM_GLIDELAND        0x0cab0285 // KNUCKLKES ONLY
#define ANIM_GLIDERISE        0x0ce60299 // KNUCKLKES ONLY

extern int debug_mode;

SoundEffect sfx_jump   = { 0 };
SoundEffect sfx_skid   = { 0 };
SoundEffect sfx_roll   = { 0 };
SoundEffect sfx_dash   = { 0 };
SoundEffect sfx_relea  = { 0 };
SoundEffect sfx_dropd  = { 0 };
SoundEffect sfx_ring   = { 0 };
SoundEffect sfx_pop    = { 0 };
SoundEffect sfx_sprn   = { 0 };
SoundEffect sfx_chek   = { 0 };
SoundEffect sfx_death  = { 0 };
SoundEffect sfx_ringl  = { 0 };
SoundEffect sfx_shield = { 0 };
SoundEffect sfx_yea    = { 0 };
SoundEffect sfx_switch = { 0 };
SoundEffect sfx_splash = { 0 };
SoundEffect sfx_count  = { 0 };
SoundEffect sfx_bubble = { 0 };

// TODO: Maybe shouldn't be extern?
extern TileMap16  map16;
extern TileMap128 map128;
extern LevelData  leveldata;
extern Camera     camera;
extern uint8_t    level_ring_count;
extern int32_t    level_water_y;


/* GROUND SENSOR COLLISION ANGLES */
// As a rule of thumb, only floor and ceiling min/max
// angles are well-defined.
// Floor: floor left <= x OR x <= floor right
// R.wall: floor right < x < ceiling min
// Ceiling: ceiling min <= x <= ceiling max
// L.wall: ceiling max < x < floor left
#define GSMODE_ANGLE_FLOOR_RIGHT    0x01d5 // ~41° (original: 45°)
#define GSMODE_ANGLE_CEIL_MIN       0x0600 // 135°
#define GSMODE_ANGLE_CEIL_MAX       0x0a00 // 225°
#define GSMODE_ANGLE_FLOOR_LEFT     0x0e94 // ~318° (original: 315°)

/* PUSH SENSOR COLLISION ANGLES */
// As opposed to ground sensors, here the L.Wall and R.Wall modes are
// well-defined.
#define PSMODE_ANGLE_RWALL_MIN    0x014e // ~41°
#define PSMODE_ANGLE_RWALL_MAX    0x0579 // 135°
#define PSMODE_ANGLE_LWALL_MIN    0x0a87 // 225°
#define PSMODE_ANGLE_LWALL_MAX    0x0db9 // ~318°

/* LANDING SPEED TRANSFER ANGLES */
// Depending on these angle ranges, X and Y air speed transfer to
// ground speed in different ways
#define LANDING_ANGLE_FLAT_LEFT   0x0f11 // 339
#define LANDING_ANGLE_FLAT_RIGHT  0x0105 // 23
#define LANDING_ANGLE_SLOPE_LEFT  0x0e0b // 316
#define LANDING_ANGLE_SLOPE_RIGHT 0x0200 // 45

void
load_player(Player *player,
            PlayerCharacter character,
            const char *chara_filename,
            TIM_IMAGE  *sprites)
{
    player->character = character;
    player->input = (InputState){ 0 };
    load_chara(&player->chara, chara_filename, sprites);
    player->cur_anim = NULL;
    player->tail_cur_anim = NULL;
    player->cnst  = getconstants(character, PC_DEFAULT);
    player->pos   = (VECTOR){ 0 };
    player->vel   = (VECTOR){ 0 };
    player->angle = 0;
    player->spinrev = 0;
    player->ctrllock = 0;
    player->airdirlock = 0;
    player->framecount = 0;
    player->iframes = 0;
    player->shield = 0;
    player->underwater = 0;
    player->gsmode = CDIR_FLOOR;
    player->psmode = CDIR_FLOOR;
    player->remaining_air_frames = 1800; // 30 seconds
    player->speedshoes_frames = -1; // Start inactive
    player->glide_turn_dir = 0;

    player_set_animation_direct(player, ANIM_STOPPED);
    player->anim_frame = player->anim_timer = 0;
    player->anim_dir = 1;
    player->idle_timer = ANIM_IDLE_TIMER_MAX;
    player->grnd = player->ceil = player->push = 0;

    player->ev_grnd1 = (CollisionEvent){ 0 };
    player->ev_grnd2 = (CollisionEvent){ 0 };
    player->ev_left  = (CollisionEvent){ 0 };
    player->ev_right = (CollisionEvent){ 0 };
    player->ev_ceil1 = (CollisionEvent){ 0 };
    player->ev_ceil2 = (CollisionEvent){ 0 };
    player->col_ledge = 0;

    player->action = ACTION_NONE;

    if(sfx_jump.addr == 0)   sfx_jump    = sound_load_vag("\\SFX\\JUMP.VAG;1");
    if(sfx_skid.addr == 0)   sfx_skid    = sound_load_vag("\\SFX\\SKIDDING.VAG;1");
    if(sfx_roll.addr == 0)   sfx_roll    = sound_load_vag("\\SFX\\ROLL.VAG;1");
    if(sfx_dash.addr == 0)   sfx_dash    = sound_load_vag("\\SFX\\DASH.VAG;1");
    if(sfx_relea.addr == 0)  sfx_relea   = sound_load_vag("\\SFX\\RELEA.VAG;1");
    if(sfx_dropd.addr == 0)  sfx_dropd   = sound_load_vag("\\SFX\\DROPD.VAG;1");
    if(sfx_ring.addr == 0)   sfx_ring    = sound_load_vag("\\SFX\\RING.VAG;1");
    if(sfx_pop.addr == 0)    sfx_pop     = sound_load_vag("\\SFX\\POP.VAG;1");
    if(sfx_sprn.addr == 0)   sfx_sprn    = sound_load_vag("\\SFX\\SPRN.VAG;1");
    if(sfx_chek.addr == 0)   sfx_chek    = sound_load_vag("\\SFX\\CHEK.VAG;1");
    if(sfx_death.addr == 0)  sfx_death   = sound_load_vag("\\SFX\\DEATH.VAG;1");
    if(sfx_ringl.addr == 0)  sfx_ringl   = sound_load_vag("\\SFX\\RINGLOSS.VAG;1");
    if(sfx_shield.addr == 0) sfx_shield  = sound_load_vag("\\SFX\\SHIELD.VAG;1");
    if(sfx_yea.addr == 0)    sfx_yea     = sound_load_vag("\\SFX\\YEA.VAG;1");
    if(sfx_switch.addr == 0) sfx_switch  = sound_load_vag("\\SFX\\SWITCH.VAG;1");
    if(sfx_splash.addr == 0) sfx_splash  = sound_load_vag("\\SFX\\SPLASH.VAG;1");
    if(sfx_count.addr == 0)  sfx_count   = sound_load_vag("\\SFX\\COUNT.VAG;1");
    if(sfx_bubble.addr == 0) sfx_bubble  = sound_load_vag("\\SFX\\BUBBLE.VAG;1");
}

void
free_player(Player *player)
{
    free_chara(&player->chara);
}

uint32_t
player_get_current_animation_hash(Player *player)
{
    return player->cur_anim->hname;
}

CharaAnim *
player_get_animation(Player *player, uint32_t sum)
{
    for(uint16_t i = 0; i < player->chara.numanims; i++) {
        if(player->chara.anims[i].hname == sum)
            return &player->chara.anims[i];
    }
    return NULL;
}

CharaAnim *
player_get_animation_by_name(Player *player, const char *name)
{
    for(uint16_t i = 0; i < player->chara.numanims; i++) {
        if(strncmp(player->chara.anims[i].name, name, 16) == 0)
            return &player->chara.anims[i];
    }
    return NULL;
}

void
_player_set_tail_animation(Player *player, uint32_t anim_sum)
{ 
    if(player->character != CHARA_MILES) return;
    
    uint32_t tail_sum = ANIM_TAILMOVE;
    CharaAnim *tail_anim = NULL;

    switch(anim_sum) {
        /* Animations where the tail is idle */
    case ANIM_STOPPED:
    case ANIM_IDLE:
    case ANIM_CROUCHDOWN:
    case ANIM_LOOKUP:
        tail_sum = ANIM_TAILIDLE;
        tail_anim = player_get_animation(player, tail_sum);
        break;

        /* Animations where the tail is a buttcopter */
    case ANIM_FLYUP:
    case ANIM_FLYDOWN:
    case ANIM_FLYTIRED:
        tail_sum = ANIM_TAILFLY;
        tail_anim = player_get_animation(player, tail_sum);
        break;

        /* Animations where the tail is sideways */
    case ANIM_WALKING:
    case ANIM_ROLLING:
    case ANIM_SPINDASH:
    case ANIM_SKIDDING:
    case ANIM_PUSHING:
        tail_sum = ANIM_TAILMOVE; // (Redundant failsafe)
        tail_anim = player_get_animation(player, tail_sum);
        break;

        /* Any animation not described implies on not showing the tail */
    default: break;
    }

    if(!tail_anim) {
        player->tail_cur_anim = NULL;
        return;
    }
    
    if(tail_anim != player->tail_cur_anim) {
        player->tail_cur_anim = tail_anim;
        player->tail_anim_frame = tail_anim->start;
        player->tail_anim_timer = 7;
    }
}

void
_set_animation_underlying(Player *player, CharaAnim *anim)
{
    if(player->cur_anim == anim) return;
    player->cur_anim = anim;
    player->anim_frame = player->anim_timer = 0;
    player->frame_duration = 1; // Default
    player->loopback_frame = 0;
    if(anim) {
        player->anim_frame = anim->start;
        player->anim_timer = player->frame_duration;
    }
    _player_set_tail_animation(player, anim->hname);
}

void
player_set_animation(Player *player, const char *name)
{
    _set_animation_underlying(player, player_get_animation(player, adler32(name)));
}

void
player_set_animation_precise(Player *player, const char *name)
{
    _set_animation_underlying(player, player_get_animation_by_name(player, name));
}

void
player_set_animation_direct(Player *player, uint32_t sum)
{
    _set_animation_underlying(player, player_get_animation(player, sum));
    _player_set_tail_animation(player, sum);
}

void
player_set_frame_duration(Player *player, uint8_t duration)
{
    player->frame_duration = duration;
    if(player->anim_timer > duration)
        player->anim_timer = duration;
}

void
_draw_sensor(uint16_t anchorx, uint16_t anchory, LinecastDirection dir,
             uint16_t mag, uint8_t r, uint8_t g, uint8_t b)
{
    // Calculate ending point according to direction and magnitude
    uint16_t endx = anchorx, endy = anchory;
    switch(dir) {
    case CDIR_RWALL:
        endx += mag;
        break;
    case CDIR_LWALL:
        endx -= mag;
        break;
    case CDIR_CEILING:
        endy -= mag;
        break;
    case CDIR_FLOOR:
    default:
        endy += mag;
        break;
    };

    // Calculate positions relative to camera
    anchorx -= (camera.pos.vx >> 12) - CENTERX;
    anchory -= (camera.pos.vy >> 12) - CENTERY;
    endx -= (camera.pos.vx >> 12) - CENTERX;
    endy -= (camera.pos.vy >> 12) - CENTERY;

    LINE_F2 *line = get_next_prim();
    increment_prim(sizeof(LINE_F2));
    setLineF2(line);
    setRGB0(line, r, g, b);
    setXY2(line, anchorx, anchory, endx, endy);
    sort_prim(line, OTZ_LAYER_TOPMOST);
}

void
_player_update_collision_lr(Player *player)
{
    player->push = 0;

    // NOTE: Push sensors are ONLY used when in floor mode OR when
    // the angle in question is a multiple of 90 degrees.
    if((player->gsmode != CDIR_FLOOR) && ((player->angle % 0x400) != 0))
        return;

    /* Collider linecasts */
    uint16_t
        anchorx = (player->pos.vx >> 12),
        anchory = (player->pos.vy >> 12) - 8;

    // Adjust y anchor to y + 8 when on totally flat ground
    int32_t push_anchory = anchory
        + ((player->grnd && player->angle == 0) ? 8 : 0);

    uint16_t left_mag  = PUSH_RADIUS;
    uint16_t right_mag = PUSH_RADIUS;

    // Adjust modes
    LinecastDirection ldir = CDIR_LWALL;
    LinecastDirection rdir = CDIR_RWALL;

    switch(player->psmode) {
    case CDIR_RWALL:
        ldir = CDIR_FLOOR;
        rdir = CDIR_CEILING;
        break;
    case CDIR_LWALL:
        ldir = CDIR_CEILING;
        rdir = CDIR_FLOOR;
        break;
    case CDIR_CEILING:
        ldir = CDIR_RWALL;
        rdir = CDIR_LWALL;
        break;
    case CDIR_FLOOR:
    default:
        ldir = CDIR_LWALL;
        rdir = CDIR_RWALL;
        break;
    };

    // Push sensors
    uint8_t is_push_active;
    is_push_active = (!player->grnd && abs(player->vel.vx) > 0);
    is_push_active = is_push_active ||
        ((player->grnd && abs(player->vel.vz) > 0)
         && ((player->angle >= 0x0 && player->angle <= 0x400)
             || (player->angle >= 0xc00 && player->angle <= 0x1000)));

    int32_t vel_x = player->grnd ? player->vel.vz : player->vel.vx;

    if(is_push_active) {
        // "E" sensor
        if(!player->ev_left.collided) {
            if(vel_x < 0) {
                player->ev_left = linecast(&leveldata, &map128, &map16,
                                           anchorx, push_anchory,
                                           ldir, left_mag, player->gsmode);
            }
        }

        // "F" sensor
        if(!player->ev_right.collided) {
            if(vel_x > 0) {
                player->ev_right = linecast(&leveldata, &map128, &map16,
                                            anchorx, push_anchory,
                                            rdir, right_mag, player->gsmode);
            }
        }
    }

    // Draw sensors
    if(debug_mode > 1) {
        _draw_sensor(anchorx, push_anchory, ldir, left_mag, 0xff, 0x38, 0xff);
        _draw_sensor(anchorx, push_anchory, rdir, right_mag, 0xff, 0x54, 0x54);
    }


    /* HANDLE COLLISION */
    switch(player->psmode) {
    /* case CDIR_RWALL: */
    /*     if(player->ev_right.collided && vel_x > 0) { */
    /*         player->grnd = 0; */
    /*         player->angle = 0; */
    /*         player->vel.vz = 0; */
    /*         // TODO: Same as hitting the head. Adjust this to look like ceiling */
    /*         player->pos.vy = (player->ev_right.coord + 10) << 12; */
    /*     } */

    /*     if(player->ev_left.collided && vel_x < 0) { */
    /*         player->grnd = 00; */
    /*         player->angle = 0; */
    /*         player->vel.vz = 0; */
    /*         // TODO: Hit your ass down there, Adjust this to look like floor */
    /*         player->pos.vy = (player->ev_left.coord - 25) << 12; */
    /*     } */
    /*     break; */
    /* case CDIR_LWALL: */
    /*     if(player->ev_right.collided && vel_x > 0) { */
    /*         player->grnd = 0; */
    /*         player->angle = 0; */
    /*         player->vel.vz = 0; */
    /*         // TODO: Hit your ass down there, Adjust this to look like floor */
    /*         player->pos.vy = (player->ev_right.coord + 25) << 12; */
    /*     } */

    /*     if(player->ev_left.collided && vel_x < 0) { */
    /*         player->grnd = 00; */
    /*         player->angle = 0; */
    /*         player->vel.vz = 0; */
    /*         // TODO: Same as hitting the head. Adjust this to look like ceiling */
       /*         player->pos.vy = (player->ev_left.coord - 10) << 12; */
    /*     } */
    /*     break; */
    /* case CDIR_CEILING: */
    /*     if(player->ev_right.collided && vel_x > 0) { */
    /*         player->grnd = 0; */
    /*         player->angle = 0; */
    /*         player->vel.vz = 0; */
    /*         player->pos.vx = (player->ev_right.coord + 25) << 12; */
    /*     } */

    /*     if(player->ev_left.collided && vel_x > 0) { */
    /*         player->grnd = 0; */
    /*         player->angle = 0; */
    /*         player->vel.vz = 0; */
    /*         player->pos.vx = (player->ev_left.coord - 10) << 12; */
    /*     } */
    /*     break; */
    case CDIR_FLOOR:
        if(player->ev_right.collided && vel_x > 0) {
            if(player->grnd) player->vel.vz = 0;
            else player->vel.vx = 0;
            player->pos.vx = (player->ev_right.coord - 10) << 12;
            if(player->grnd) player->push = 1;
        }

        if(player->ev_left.collided && vel_x < 0) {
            if(player->grnd) player->vel.vz = 0;
            else player->vel.vx = 0;
            player->pos.vx = (player->ev_left.coord + 25) << 12;
            if(player->grnd) player->push = 1;
        }
        break;
    default: break;
    };
    
}

void
_player_update_collision_tb(Player *player)
{
    /* Collider linecasts */
    uint16_t
        anchorx = (player->pos.vx >> 12),
        anchory = (player->pos.vy >> 12);

    uint16_t grn_grnd_dist = WIDTH_RADIUS_NORMAL;
    uint16_t grn_mag   = HEIGHT_RADIUS_NORMAL;
    uint16_t ceil_mag  = HEIGHT_RADIUS_NORMAL;

    if(player->action == ACTION_JUMPING) {
        grn_grnd_dist = WIDTH_RADIUS_ROLLING;
        grn_mag = ceil_mag = HEIGHT_RADIUS_ROLLING;
    }

    uint16_t anchorx_left = anchorx,
        anchorx_right = anchorx,
        anchory_left = anchory,
        anchory_right = anchory;

    LinecastDirection grndir, ceildir;

    switch(player->gsmode) {
    case CDIR_RWALL:
        grndir = CDIR_RWALL;
        ceildir = CDIR_LWALL;
        anchory_left += grn_grnd_dist;
        anchory_right -= grn_grnd_dist - 1;
        break;
    case CDIR_LWALL:
        grndir = CDIR_LWALL;
        ceildir = CDIR_RWALL;
        anchory_left -= grn_grnd_dist - 1;
        anchory_right += grn_grnd_dist;
        break;
    case CDIR_CEILING:
        grndir = CDIR_CEILING;
        ceildir = CDIR_FLOOR;
        anchorx_left += grn_grnd_dist - 1;
        anchorx_right -= grn_grnd_dist;
        break;
    case CDIR_FLOOR:
    default:
        grndir = CDIR_FLOOR;
        ceildir = CDIR_CEILING;
        anchorx_left -= grn_grnd_dist;
        anchorx_right += grn_grnd_dist - 1;
        break;
    };

    // Ground sensors
    if(!player->ev_grnd1.collided) {
        player->ev_grnd1 = linecast(&leveldata, &map128, &map16,
                                    anchorx_left, anchory_left,
                                    grndir, grn_mag, player->gsmode);
    }
    if(!player->ev_grnd2.collided) {
        player->ev_grnd2 = linecast(&leveldata, &map128, &map16,
                                    anchorx_right, anchory_right,
                                    grndir, grn_mag, player->gsmode);
    }

    // Ledge sensor
    if((player->vel.vz == 0) && (player->gsmode == CDIR_FLOOR)) {
        CollisionEvent ev_ledge = linecast(&leveldata, &map128, &map16,
                                           anchorx, anchory_left,
                                           CDIR_FLOOR, LEDGE_SENSOR_MAGNITUDE,
                                           CDIR_FLOOR);
        player->col_ledge = ev_ledge.collided;
    }

    if(!player->grnd) {
        // Ceiling sensors
        if(!player->ev_ceil1.collided) {
            player->ev_ceil1 = linecast(&leveldata, &map128, &map16,
                                        anchorx_left, anchory_left,
                                        ceildir, ceil_mag, player->gsmode);
        }
        if(!player->ev_ceil2.collided) {
            player->ev_ceil2 = linecast(&leveldata, &map128, &map16,
                                        anchorx_right, anchory_right,
                                        ceildir, ceil_mag, player->gsmode);
        }
    }

    // Draw sensors
    if(debug_mode > 1) {
        // Ground sensors
        _draw_sensor(anchorx_left, anchory_left,
                     grndir, grn_mag,
                     0x00, 0xf0, 0x00);
        _draw_sensor(anchorx_right, anchory_right,
                     grndir, grn_mag,
                     0x38, 0xff, 0xa2);

        // Ceiling sensors
        _draw_sensor(anchorx_left, anchory_left,
                     ceildir, ceil_mag,
                     0x00, 0xae, 0xef);
        _draw_sensor(anchorx_right, anchory_right,
                     ceildir, ceil_mag,
                     0xff, 0xf2, 0x38);

        // Ledge sensor
        if((player->vel.vz == 0) && (player->gsmode == CDIR_FLOOR)) {
            _draw_sensor(anchorx, anchory_right,
                         CDIR_FLOOR, LEDGE_SENSOR_MAGNITUDE,
                         0x1c, 0xf7, 0x51);
        }
    }

    /* HANDLE COLLISION */
    if(!player->grnd) {
        player->angle = 0;
        player->gsmode = player->psmode = CDIR_FLOOR;

        // Landing on solid ground
        if((player->ev_grnd1.collided || player->ev_grnd2.collided) && (player->vel.vy >= 0)) {
            // Set angle according to movement
            if(player->ev_grnd1.collided && !player->ev_grnd2.collided)
                player->angle = player->ev_grnd1.angle;
            else if(!player->ev_grnd1.collided && player->ev_grnd2.collided)
                player->angle = player->ev_grnd2.angle;
            else {
                // In case both are available, get them based on gsp,
                // but if gsp is zero, favor left sensor
                player->angle =
                    (player->vel.vz <= 0)
                    ? player->ev_grnd1.angle
                    : player->ev_grnd2.angle;
            }

            if((player->angle >= LANDING_ANGLE_FLAT_LEFT)
               || (player->angle <= LANDING_ANGLE_FLAT_RIGHT)) {
                // Landed on very flat ground, conserve X
                player->vel.vz = player->vel.vx;
            } else if((player->angle <= LANDING_ANGLE_SLOPE_RIGHT)
                      || (player->angle >= LANDING_ANGLE_SLOPE_LEFT)) {
                // Slope ground, set to half vy
                player->vel.vz =
                    ((player->vel.vy * 2048) >> 12) * -SIGNUM(rsin(player->angle));
            } else {
                // Steep ground, set to full vy
                player->vel.vz =
                    player->vel.vy * -SIGNUM(rsin(player->angle));
            }

            int32_t new_coord = 0;
            if(player->ev_grnd1.collided) new_coord = player->ev_grnd1.coord;
            if((player->ev_grnd2.collided && (player->ev_grnd2.coord < new_coord))
               || (new_coord == 0))
                new_coord = player->ev_grnd2.coord;

            player->pos.vy = ((new_coord - 16) << 12);
            player->grnd = 1;

            if(player->action == ACTION_JUMPING
               || player->action == ACTION_ROLLING
               || player->action == ACTION_SPRING
               || player->action == ACTION_HURT) {
                if(player->action == ACTION_HURT) {
                    player->iframes = PLAYER_HURT_IFRAMES;
                    player->ctrllock = 0;
                }
                player_set_action(player, ACTION_NONE);
                player->airdirlock = 0;
            }
            else if(player->action == ACTION_DROPDASH) {
                // Perform drop dash
                player->framecount   = 0;
                player->holding_jump = 0;
                player_set_action(player, ACTION_ROLLING);
                // We're going to need the previous vel.vx as usual,
                // but we're going to manipulate gsp AFTER it has been calculated,
                // so this code MUST come after landing speed calculation
                uint8_t moving_backwards =
                    (player->vel.vx > 0 && player->anim_dir == -1)
                    || (player->vel.vx < 0 && player->anim_dir == 1);
                if(!moving_backwards) {
                    // gsp = (gsp / 4) + (drpspd * dir)
                    player->vel.vz = (player->vel.vz >> 2)
                        + (player->cnst->x_drpspd * player->anim_dir);
                    if(player->vel.vz > 0)
                        player->vel.vz = (player->vel.vz > player->cnst->x_drpmax)
                            ? player->cnst->x_drpmax
                            : player->vel.vz;
                    else player->vel.vz = (player->vel.vz < player->cnst->x_drpmax)
                             ? -player->cnst->x_drpmax : player->vel.vz;
                } else {
                    if(player->angle == 0)
                        player->vel.vz = player->cnst->x_drpspd * player->anim_dir;
                    else {
                        player->vel.vz = (player->vel.vz >> 1)
                            + (player->cnst->x_drpspd * player->anim_dir);
                        if(player->vel.vz > 0)
                            player->vel.vz = (player->vel.vz > player->cnst->x_drpmax)
                                ? player->cnst->x_drpmax
                                : player->vel.vz;
                        else player->vel.vz = (player->vel.vz < player->cnst->x_drpmax)
                                 ? -player->cnst->x_drpmax
                                 : player->vel.vz;
                    }
                }
                sound_play_vag(sfx_relea, 0);
                camera.lag = 0x8000 >> 12;
            } else if(player->action == ACTION_GLIDE) {
                // TODO: Slide
            } else if(player->action == ACTION_DROP) {
                player_set_action(player, ACTION_NONE);
                player->vel.vz = 0;
                player->airdirlock = 0;
            }
        }

        if((player->ev_ceil1.collided || player->ev_ceil2.collided) && (player->vel.vy < 0)) {
            player->vel.vy = 0;
            int32_t new_coord = 0;
            if(player->ev_ceil1.collided) new_coord = player->ev_ceil1.coord;
            if((player->ev_ceil2.collided && (player->ev_ceil2.coord < new_coord))
               || (new_coord == 0))
                new_coord = player->ev_ceil2.coord;
            
            player->pos.vy = (new_coord + 32) << 12;
            player->ceil = 1;
        } else player->ceil = 0;

        // Cancel drop dash if not holding jump
        if(player->action == ACTION_DROPDASH
           && !input_pressing(&player->input, PAD_CROSS)) {
            player_set_action(player, ACTION_JUMPING);
        }
    } else {
        if(!player->ev_ceil1.collided && !player->ev_ceil2.collided) {
            player->ceil = 0;
        }

        if(!player->ev_grnd1.collided && !player->ev_grnd2.collided) {
            player->grnd = 0;
            player->gsmode = player->psmode = CDIR_FLOOR;
        } else {
            // Set angle according to movement
            if(player->ev_grnd1.collided && !player->ev_grnd2.collided)
                player->angle = player->ev_grnd1.angle;
            else if(!player->ev_grnd1.collided && player->ev_grnd2.collided)
                player->angle = player->ev_grnd2.angle;
            // In case both are available, get the angle on the left.
            // This introduces certain collision bugs but let's leave it
            // like this for now
            else player->angle = player->ev_grnd1.angle;

            // Calculate which of the two coords we should use.
            int32_t new_coord = 0;
            
            // Positioning resolution according to collision mode
            switch(player->gsmode) {
            case CDIR_RWALL:
                if(player->ev_grnd1.collided) new_coord = player->ev_grnd1.coord;
                if((player->ev_grnd2.collided && (player->ev_grnd2.coord < new_coord))
                   || (new_coord == 0))
                    new_coord = player->ev_grnd2.coord;
                player->pos.vx = (new_coord - 16) << 12;
                break;
            case CDIR_LWALL:
                if(player->ev_grnd1.collided) new_coord = player->ev_grnd1.coord;
                if((player->ev_grnd2.collided && (player->ev_grnd2.coord > new_coord))
                   || (new_coord == 0))
                    new_coord = player->ev_grnd2.coord;
                player->pos.vx = (new_coord + 16) << 12;
                break;
            case CDIR_CEILING:
                if(player->ev_grnd1.collided) new_coord = player->ev_grnd1.coord;
                if((player->ev_grnd2.collided && (player->ev_grnd2.coord > new_coord))
                   || (new_coord == 0))
                    new_coord = player->ev_grnd2.coord;
                player->pos.vy = (new_coord + 16) << 12;
                break;
            case CDIR_FLOOR:
            default:
                if(player->ev_grnd1.collided) new_coord = player->ev_grnd1.coord;
                if((player->ev_grnd2.collided && (player->ev_grnd2.coord < new_coord))
                   || (new_coord == 0))
                    new_coord = player->ev_grnd2.coord;
                player->pos.vy = (new_coord - 16) << 12;
                break;
            };
        }
    }
}

void
_player_resolve_collision_modes(Player *player)
{
    // NOTE THAT PLAYER INPUT IS NOT UPDATED AUTOMATICALLY!
    // One must call input_get_state on player->input so that
    // player input is recognized. This is done in screen_level.c.

    // NOTE: Relative to the Sonic Physics Guide, angles are offset
    // by 43 units towards the direction that makes more sense for
    // that region, so angles may be increased or decreased depending
    // on convenience.
    int32_t p_angle = player->angle;

    /* GROUND SENSORS COLLISION MODES */
    if((p_angle >= GSMODE_ANGLE_FLOOR_LEFT) || (p_angle <= GSMODE_ANGLE_FLOOR_RIGHT))
        // floor
        player->gsmode = CDIR_FLOOR;
    else if((p_angle > GSMODE_ANGLE_FLOOR_RIGHT) && (p_angle < GSMODE_ANGLE_CEIL_MIN))
        // r.wall (l.wall if angle is negative)
        player->gsmode = (player->angle >= 0)
            ? CDIR_RWALL
            : CDIR_LWALL;
    else if((p_angle >= GSMODE_ANGLE_CEIL_MIN) && (p_angle <= GSMODE_ANGLE_CEIL_MAX))
        // ceiling
        player->gsmode = CDIR_CEILING;
    else if((p_angle > GSMODE_ANGLE_CEIL_MAX) && (p_angle < GSMODE_ANGLE_FLOOR_LEFT))
        // l.wall (r.wall if angle negative)
        player->gsmode = (player->angle >= 0)
            ? CDIR_LWALL
            : CDIR_RWALL;

    /* PUSH SENSORS COLLISION MODES */
    if((p_angle > PSMODE_ANGLE_LWALL_MAX) || (p_angle < PSMODE_ANGLE_RWALL_MIN))
        player->psmode = CDIR_FLOOR;
    else if((PSMODE_ANGLE_RWALL_MIN <= p_angle) && (p_angle <= PSMODE_ANGLE_RWALL_MAX))
        player->psmode = (player->angle >= 0)
            ? CDIR_RWALL
            : CDIR_LWALL;
    else if((p_angle > PSMODE_ANGLE_RWALL_MAX) && (p_angle < PSMODE_ANGLE_LWALL_MIN))
        player->psmode = CDIR_CEILING;
    else if((PSMODE_ANGLE_LWALL_MIN >= p_angle) && (p_angle <= PSMODE_ANGLE_LWALL_MAX))
        player->psmode = (player->angle >= 0)
            ? CDIR_LWALL
            : CDIR_RWALL;
}

void
player_update(Player *player)
{
    // Angle slope pattern in degrees: 3, 12, 30, 45, 60, 78, 87
    //_player_resolve_collision_modes(player);

    _player_update_collision_lr(player); // Push sensor collision detection
    _player_update_collision_tb(player); // Ground sensor collision detection

    // i-frames
    if(player->iframes > 0) player->iframes--;

    // Update control lock
    if(player->ctrllock > 0) {
        if((player->action == ACTION_GASP) || player->grnd) {
            player->ctrllock--;
        }
    }

    // X movement
    /* Ground movement */
    if(player->grnd) {
        if(player->action == ACTION_ROLLING) {
            // Rolling physics.
            // Slope factor
            int32_t angle_sin = rsin(player->angle);
            player->vel.vz -= (
                (SIGNUM(player->vel.vz) == SIGNUM(angle_sin)
                 ? player->cnst->x_slope_rollup
                 : player->cnst->x_slope_rolldown)
                * angle_sin) >> 12;

            // Deceleration on input
            if(player->vel.vz > 0 && input_pressing(&player->input, PAD_LEFT))
                player->vel.vz -=
                    player->cnst->x_roll_decel + player->cnst->x_roll_friction;
            else if(player->vel.vz < 0 && input_pressing(&player->input, PAD_RIGHT))
                player->vel.vz +=
                    player->cnst->x_roll_decel + player->cnst->x_roll_friction;
            else {
                // Apply roll friction
                player->vel.vz -=
                    (player->vel.vz > 0
                     ? player->cnst->x_roll_friction
                     : -player->cnst->x_roll_friction);
            }

            // Uncurl if too slow
            if(abs(player->vel.vz) < player->cnst->x_min_uncurl_spd)
                player_set_action(player, ACTION_NONE);
        } else if(player->action == ACTION_SPINDASH) {
            // Release
            if(!input_pressing(&player->input, PAD_DOWN)) {
                player->vel.vz +=
                    (0x8000 + (floor12(player->spinrev) >> 1)) * player->anim_dir;
                player_set_action(player, ACTION_ROLLING);
                camera.lag = (0x10000 - player->spinrev) >> 12;
                player->spinrev = 0;
                sound_play_vag(sfx_relea, 0);
            } else {
                if(player->spinrev > 0) {
                    player->spinrev -= (div12(player->spinrev, 0x200) << 12) / 0x100000;
                }
                if(input_pressed(&player->input, PAD_CROSS)) {
                    player->spinrev += 0x2000;
                    sound_play_vag(sfx_dash, 0);
                }
            }
        } else if(player->action == ACTION_PEELOUT) {
            // Release
            if(!input_pressing(&player->input, PAD_UP)) {
                player_set_action(player, ACTION_NONE);
                if(player->spinrev >= 30) { // Only properly release after 30 frames
                    player->vel.vz = (player->cnst->x_peelout_spd * player->anim_dir);
                    camera.lag = (0x10000 - player->spinrev) >> 12;
                    player->spinrev = 0;
                    sound_play_vag(sfx_relea, 0);
                }
            } else {
                if(player->spinrev < 30) {
                    player->spinrev++;
                }
            }
        } else if(player->action == ACTION_GASP) {
            // Ignore input while gasping. Release action when control
            // lock is over
            if(player->ctrllock <= 0) player_set_action(player, ACTION_NONE);
        } else {
            // Default physics
            player_set_action(player, ACTION_NONE);

            if(input_pressing(&player->input, PAD_RIGHT)
               && (player->ctrllock == 0)) {
                if(player->vel.vz < 0) {
                    player_set_action(player, ACTION_SKIDDING);
                    player->vel.vz += player->cnst->x_decel;
                } else {
                    if(player->vel.vz < player->cnst->x_top_spd)
                        player->vel.vz += player->cnst->x_accel;
                    player->anim_dir = 1;
                }
            } else if(input_pressing(&player->input, PAD_LEFT)
                      && (player->ctrllock == 0)) {
                if(player->vel.vz > 0) {
                    player_set_action(player, ACTION_SKIDDING);
                    player->vel.vz -= player->cnst->x_decel;
                } else {
                    if(player->vel.vz > -player->cnst->x_top_spd)
                        player->vel.vz -= player->cnst->x_accel;
                    player->anim_dir = -1;
                }
            } else {
                // Apply friction
                player->vel.vz -=
                    (player->vel.vz > 0
                     ? player->cnst->x_friction
                     : -player->cnst->x_friction);
                if(abs(player->vel.vz) <= player->cnst->x_friction)
                    player->vel.vz = 0;
            }

            // Slope factor application. TODO: Should only not work when in ceiling
            if(abs(player->vel.vz) >= player->cnst->x_slope_min_spd)
                player->vel.vz -= (player->cnst->x_slope_normal * rsin(player->angle)) >> 12;

            // Slip down slopes if they are too steep
            // TODO: FIX THIS!
            /* if((abs(player->vel.vz) < X_MAX_SLIP_SPD) */
            /*    && (abs(player->angle) > 0x1ec && abs(player->angle) < 0xe16) */
            /*    && player->ctrllock == 0) { */
            /*     player->ctrllock = 30; */
            /*     if(player->gsmode != CDIR_FLOOR) { */
            /*         player->grnd = 0; */
            /*         player->gsmode = CDIR_FLOOR; */
            /*         player->psmode = CDIR_FLOOR; */
            /*     } */
            /* } */

            /* Action changers */
            if(input_pressing(&player->input, PAD_DOWN)) {
                if(abs(player->vel.vz) >= player->cnst->x_min_roll_spd) { // Rolling
                    player_set_action(player, ACTION_ROLLING);
                    player_set_animation_direct(player, ANIM_ROLLING);
                    sound_play_vag(sfx_roll, 0);
                } else if(player->col_ledge
                          && player->vel.vz == 0
                          && input_pressed(&player->input, PAD_CROSS)) { // Spindash
                    player_set_action(player, ACTION_SPINDASH);
                    player_set_animation_direct(player, ANIM_SPINDASH);
                    player->spinrev = 0;
                    sound_play_vag(sfx_dash, 0);
                }
            } else if((player->character == CHARA_SONIC)
                && input_pressing(&player->input, PAD_UP)) {
                if(player->col_ledge
                   && player->vel.vz == 0
                   && input_pressed(&player->input, PAD_CROSS)) { // Peel-out
                    player_set_action(player, ACTION_PEELOUT);
                    player_set_animation_direct(player, ANIM_WALKING);
                    player->spinrev = 0;
                    sound_play_vag(sfx_dash, 0);
                }
            }
        }

        // Ground speed cap
        if(player->vel.vz > player->cnst->x_max_spd)
            player->vel.vz = player->cnst->x_max_spd;
        else if(player->vel.vz < -player->cnst->x_max_spd)
            player->vel.vz = -player->cnst->x_max_spd;

        // Distribute ground speed onto X and Y components
        player->vel.vx = (player->vel.vz * rcos(player->angle)) >> 12;
        player->vel.vy = (player->vel.vz * -rsin(player->angle)) >> 12;
    } else {
        // Air X movement
        if(player->action == ACTION_GLIDE) {
            // spinrev is an acceleration "angle" for turning while gliding.
            if(player->glide_turn_dir == -1) { // Turning right to left
                // Disable turn, fix angle
                if(player->spinrev >= (ONE >> 1)) {
                    player->glide_turn_dir = 0;
                    player->spinrev = (ONE >> 1);
                } else {
                    // Still turning? Setup X speed
                    player->spinrev += KNUX_GLIDE_TURN_STEP;
                    player->vel.vx = (player->vel.vz * rcos(player->spinrev)) >> 12;
                    // If angle is past 90 degrees, turn animation
                    if(player->spinrev >= (ONE >> 2)) player->anim_dir = -1;
                }
            } else if(player->glide_turn_dir == 1) { // Turning left to right
                // Disable turn, fix angle
                if(player->spinrev <= 0) {
                    player->glide_turn_dir = 0;
                    player->spinrev = 0;
                } else {
                    player->spinrev -= KNUX_GLIDE_TURN_STEP;
                    player->vel.vx = (player->vel.vz * -rcos(player->spinrev)) >> 12;
                    // If angle is past 90 degrees, turn animation
                    if(player->spinrev <= (ONE >> 2)) player->anim_dir = 1;
                }
            }

            // Turn to the other side
            if(input_pressed(&player->input, PAD_LEFT)
               && ((player->vel.vx > ONE) || (player->vel.vx == 0))) {
                player->vel.vz = player->vel.vx;
                player->glide_turn_dir = -1;
            } else if(input_pressed(&player->input, PAD_RIGHT)
                      && ((player->vel.vx < -ONE) || (player->vel.vx == 0))) {
                player->vel.vz = player->vel.vx;
                player->glide_turn_dir = 1;
            }

            // When not turning...
            if(player->glide_turn_dir == 0) {
                // Apply glide acceleration
                player->vel.vx += player->anim_dir * KNUX_GLIDE_X_ACCEL;
            }
        } else if(player->ctrllock == 0) {
            if(input_pressing(&player->input, PAD_RIGHT)) {
                if(player->vel.vx < player->cnst->x_top_spd)
                    player->vel.vx += player->cnst->x_air_accel;
                if(!player->airdirlock)
                    player->anim_dir = 1;
            } else if(input_pressing(&player->input, PAD_LEFT)) {
                if(player->vel.vx > -player->cnst->x_top_spd)
                    player->vel.vx -= player->cnst->x_air_accel;
                if(!player->airdirlock)
                    player->anim_dir = -1;
            }
        }

        // Air drag. Calculated before applying gravity.
        if((player->vel.vy < 0 && player->vel.vy > -player->cnst->y_min_jump)
           && (player->action != ACTION_HURT)
           && (player->action != ACTION_GLIDE)) {
            // xsp -= (xsp div 0.125) / 256
            int32_t air_drag = (div12(abs(player->vel.vx), 0x200) << 12) / 0x100000;
            if(player->vel.vx > 0)
                player->vel.vx -= air_drag;
            else if(player->vel.vx < 0)
                player->vel.vx += air_drag;
        }

        // Air speed cap
        if(player->vel.vx > player->cnst->x_max_spd)
            player->vel.vx = player->cnst->x_max_spd;
        else if(player->vel.vx < -player->cnst->x_max_spd)
            player->vel.vx = -player->cnst->x_max_spd;
    }

    // Y movement
    if(!player->grnd) {
        if(player->action == ACTION_JUMPING) {
            if(!input_pressing(&player->input, PAD_CROSS)) {
                // Short jump
                if(player->vel.vy < -player->cnst->y_min_jump)
                    player->vel.vy = -player->cnst->y_min_jump;
                player->holding_jump = 0;
            } else {
                if(!player->holding_jump) {
                    switch(player->character) {
                    case CHARA_SONIC:
                        // Drop dash charge wait
                        if(player->framecount < 20) {
                            player->framecount++;
                        } else {
                            sound_play_vag(sfx_dropd, 0);
                            player_set_action(player, ACTION_DROPDASH);
                        }
                        break;
                    case CHARA_MILES:
                        player_set_action(player, ACTION_FLY);
                        player->spinrev = 0;    // Do not move up for first time
                        player->framecount = 0; // Start counter for tiredness
                        break;
                    case CHARA_KNUCKLES:
                        player_set_action(player, ACTION_GLIDE);
                        player->vel.vx = (4 * player->anim_dir) << 12;
                        if(player->vel.vy < 0) player->vel.vy = 0;
                        // Glide "angle" (used when turning).
                        // Starts pointing at direction by default (0 for right,
                        // 0.5 for left)
                        player->spinrev = (player->anim_dir > 0) ? 0 : (ONE >> 1);
                        player->glide_turn_dir = 0;
                    default: break;
                    }
                }
            }
        } else if(player->action == ACTION_FLY) {
            if(player->framecount < PLAYER_FLY_MAXFRAMES) {
                if(player->vel.vy <= 0)
                    player->framecount++;

                // spinrev is a flight state. 1 is ascent, 0 is descent.
                // This state affects gravity, so we're OK with the speed here
                if(input_pressed(&player->input, PAD_CROSS)) {
                    player->spinrev = 1;
                }
            }

            // Ceiling collision
            if(player->ceil) player->spinrev = 0;
            
            // if ascending and ysp < -1, turn on descent again
            if(player->spinrev && (player->vel.vy < -ONE))
                player->spinrev = 0;
        } else if(player->action == ACTION_GLIDE) {
            if(!input_pressing(&player->input, PAD_CROSS)) {
                // Cancel gliding
                player_set_action(player, ACTION_DROP);
                player->vel.vx >>= 2; // times 0.25
                player->airdirlock = 1;
            }
        }

        // Apply gravity
        switch(player->action) {
        case ACTION_HURT:
            player->vel.vy += player->cnst->y_hurt_gravity;
            break;
        case ACTION_FLY:
            // Flying up (spinrev > 0) uses negative gravity
            if(player->spinrev) player->vel.vy -= MILES_GRAVITY_FLYUP;
            else player->vel.vy += MILES_GRAVITY_FLYDOWN;
            break;
        case ACTION_GLIDE:
            if(player->vel.vy < (ONE >> 1))
                player->vel.vy += KNUX_GLIDE_GRAVITY;
            else player->vel.vy -= KNUX_GLIDE_GRAVITY;
            break;
        default:
            player->vel.vy += player->cnst->y_gravity;
            break;
        }
    } else {
        if(input_pressed(&player->input, PAD_CROSS)
           && (player->action != ACTION_SPINDASH)
            && (player->action != ACTION_PEELOUT)) {
            // TODO: Review jump according to angle
            player->vel.vx -= (player->cnst->y_jump_strength * rsin(player->angle)) >> 12;
            player->vel.vy -= (player->cnst->y_jump_strength * rcos(player->angle)) >> 12;
            player->grnd = 0;
            player_set_animation_direct(player, ANIM_ROLLING);
            sound_play_vag(sfx_jump, 0);
            player_set_action(player, ACTION_JUMPING);
            player->holding_jump = 1;
        }
    }

    // Animation
    if(player->grnd) {
        if(player->push) {
            player_set_animation_direct(player, ANIM_PUSHING);
            player->idle_timer = ANIM_IDLE_TIMER_MAX;
        } else if(player->vel.vz == 0) {
            if(player->action == ACTION_SPINDASH) {
                player_set_animation_direct(player, ANIM_SPINDASH);
            } else if(player->action == ACTION_GASP) {
                player_set_animation_direct(player, ANIM_GASP);
            } else if(player->action == ACTION_PEELOUT) {
                // Use player->spinrev as a timer for when animations should
                // play. It builds up from walking to running to peel-out.
                if((player->spinrev >= 30) && !player->underwater)
                    player_set_animation_direct(player, ANIM_PEELOUT);
                else if(player->spinrev >= (player->underwater ? 15 : 10))
                    player_set_animation_direct(player, ANIM_RUNNING);
                else player_set_animation_direct(player, ANIM_WALKING);
            } else if(player->col_ledge && input_pressing(&player->input, PAD_UP)) {
                player_set_animation_direct(player, ANIM_LOOKUP);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
                player_set_action(player, ACTION_LOOKUP);
            } else if(player->col_ledge && input_pressing(&player->input, PAD_DOWN)) {
                player_set_animation_direct(player, ANIM_CROUCHDOWN);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
                player_set_action(player, ACTION_CROUCHDOWN);
            } else if(player->idle_timer == 0) {
                player_set_animation_direct(player, ANIM_IDLE);
                player->loopback_frame = 2;
            } else if (!input_pressing(&player->input, PAD_LEFT)
                       && !input_pressing(&player->input, PAD_RIGHT)) {
                // Balance on ledges
                if((player->ev_grnd1.collided ^ player->ev_grnd2.collided)
                   && !player->col_ledge) {
                    player->idle_timer = ANIM_IDLE_TIMER_MAX;
                    if(((player->anim_dir < 0) && player->ev_grnd1.collided)
                       || ((player->anim_dir > 0) && player->ev_grnd2.collided))
                        player_set_animation_direct(player, ANIM_BALANCEHEAVY);
                    else player_set_animation_direct(player, ANIM_BALANCELIGHT);
                } else {
                    player_set_animation_direct(player, ANIM_STOPPED);
                    if(player->idle_timer > 0) player->idle_timer--;
                }
            }
        } else {
            player->idle_timer = ANIM_IDLE_TIMER_MAX;
            if(player->action == ACTION_SKIDDING) {
                if(abs(player->vel.vz) >= (4 << 12)) {
                    if(player_get_current_animation_hash(player) != ANIM_SKIDDING) {
                        sound_play_vag(sfx_skid, 0);
                    }
                    player_set_animation_direct(player, ANIM_SKIDDING);
                    player->loopback_frame = 3;
                } if(player_get_current_animation_hash(player) != ANIM_SKIDDING) {
                    player_set_animation_direct(player, ANIM_WALKING);
                }
            } else if(player->action == ACTION_ROLLING
                      || player->action == ACTION_SPINDASH
                      || player->action == ACTION_DROPDASH) {
                 player_set_animation_direct(player, ANIM_ROLLING);
            } else if(abs(player->vel.vz) >= (10 << 12)) {
                player_set_animation_direct(player, ANIM_PEELOUT);
            } else if(abs(player->vel.vz) >= (6 << 12) - 0xff) {
                player_set_animation_direct(player, ANIM_RUNNING);
            } else if((player->character == CHARA_SONIC) &&
                (player->underwater && abs(player->vel.vz) >= (4 << 12))) {
                player_set_animation_direct(player, ANIM_WATERWALK);
            } else player_set_animation_direct(player, ANIM_WALKING);
        }
    } else {
        player->idle_timer = ANIM_IDLE_TIMER_MAX;
        if(player->action == ACTION_SPRING) {
            if(player->vel.vy < 0) {
                player_set_animation_direct(player, ANIM_SPRING);
            } else {
                player->airdirlock = 0;
                player_set_animation_direct(player, ANIM_DROP);
                /* if(abs(player->vel.vz) >= (10 << 12)) { */
                /*     player_set_animation_direct(player, ANIM_PEELOUT); */
                /* } else if(abs(player->vel.vz) >= (6 << 12)) { */
                /*     player_set_animation_direct(player, ANIM_RUNNING); */
                /* } else player_set_animation_direct(player, ANIM_WALKING); */
            }
        } else if(player->action == ACTION_HURT) {
            player_set_animation_direct(player, ANIM_HURT);
        } else if(player->action == ACTION_FLY) {
            if(player->framecount >= PLAYER_FLY_MAXFRAMES)
                player_set_animation_direct(
                    player,
                    (player->underwater ? ANIM_SWIMTIRED : ANIM_FLYTIRED));
            else player_set_animation_direct(
                player,
                player->underwater
                ? ANIM_SWIMMING
                : ((player->spinrev) || (player->vel.vy < 0)
                   ? ANIM_FLYUP
                   : ANIM_FLYDOWN));
        } else if(player->action == ACTION_GLIDE) {
            if(player->glide_turn_dir == 0) {
                player_set_animation_direct(player, ANIM_GLIDE);
            } else {
                if(abs(player->vel.vx) >= (1 << 12))
                    player_set_animation_direct(player, ANIM_GLIDETURNA);
                else player_set_animation_direct(player, ANIM_GLIDETURNB);
            }
        } else if(player->action == ACTION_DROP) {
            player_set_animation_direct(player, ANIM_GLIDECANCEL);
            player_set_frame_duration(player, 12);
            player->loopback_frame = 1;
            
        }
    }

    // Animation speed correction
    if(player->anim_timer == 0) {

        if(player->action == ACTION_PEELOUT) {
            // Play all animations with a duration of two game
            // frames per animation frame. But when underwater, make
            // it slightly slower
            player_set_frame_duration(player, 1);
        } else {
            uint32_t anim_hash = player_get_current_animation_hash(player);
            switch(anim_hash) {
            case ANIM_WALKING:
            case ANIM_WATERWALK:
            case ANIM_RUNNING:
                if((player->character == CHARA_KNUCKLES) && (anim_hash == ANIM_WALKING))
                    // Speed up Knuckles's walk animation, just a little bit
                    player_set_frame_duration(player, MAX(0, 7 - abs(player->vel.vz >> 12)));
                else player_set_frame_duration(player, MAX(0, 8 - abs(player->vel.vz >> 12)));
                break;

            case ANIM_SPINDASH:
                player_set_frame_duration(player, 0);
                break;
            case ANIM_ROLLING:
                if(player->action == ACTION_DROPDASH)
                    player_set_frame_duration(player, 0);
                else
                    player_set_frame_duration(player, MAX(0, 4 - abs(player->vel.vz >> 12)));
                break;
            case ANIM_PEELOUT:
                player_set_frame_duration(player, 1);
                break;

            case ANIM_PUSHING:
                player_set_frame_duration(
                    player,
                    (player->character == CHARA_KNUCKLES)
                    ? 6
                    : MAX(0, 8 - abs(player->vel.vz >> 12)) << 2);
                break;

            case ANIM_SPRING:
                player_set_frame_duration(player, 3);
                break;

            case ANIM_BALANCELIGHT:
            case ANIM_BALANCEHEAVY:
                player_set_frame_duration(player, 12);
                break;

            case ANIM_GLIDECANCEL:
                player_set_frame_duration(player, 12);
                break;

                // Single-frame animations
            case ANIM_STOPPED:
            case ANIM_IDLE:
            case ANIM_SKIDDING:
            case ANIM_CROUCHDOWN:
            case ANIM_LOOKUP:
            default:
                player_set_frame_duration(player, 6);
                break;
            };
        }
    }

    // Animation update
    if(player->cur_anim) {
        if(player->cur_anim->start == player->cur_anim->end)
            player->anim_frame = player->cur_anim->start;
        else if(player->anim_timer == 0) {
            player->anim_timer = player->frame_duration;
            player->anim_frame++;
            if(player->anim_frame > player->cur_anim->end)
                player->anim_frame = player->cur_anim->start + player->loopback_frame;
        } else player->anim_timer--;
    }

    if((player->character == CHARA_MILES) && player->tail_cur_anim) {
        if(player->tail_cur_anim->start >= player->tail_cur_anim->end)
            player->tail_anim_frame = player->tail_cur_anim->start;
        else if(player->tail_anim_timer == 0) {
            player->tail_anim_timer = 7; // Tail animation frame duration
            player->tail_anim_frame++;
            if(player->tail_anim_frame > player->tail_cur_anim->end) {
                player->tail_anim_frame = player->tail_cur_anim->start; // No loopback
            }
        } else player->tail_anim_timer--;
    }

    // Speed shoes
    // Constant values are modified by monitor behaviour.
    if(player->speedshoes_frames > 0)
        player->speedshoes_frames--;
    if(player->speedshoes_frames == 0) {
        // Reset constants.
        // Notice that speedshoes_frames is set to -1 by general level update.
        player->cnst = getconstants(player->character, player->underwater ? PC_UNDERWATER : PC_DEFAULT);
    }

    // Underwater / leaving water
    if(level_water_y >= 0) {
        // Underwater state update
        if(((player->pos.vy > level_water_y) && !player->underwater)
           || ((player->pos.vy < level_water_y) && player->underwater)) {
            player->underwater = !player->underwater;

            // Change constants accordingly!
            if(player->underwater) { // Entered underwater state
                player->cnst = getconstants(player->character, PC_UNDERWATER);
                // Halve player's X speed
                if(player->grnd) player->vel.vz = player->vel.vz >> 1;
                else player->vel.vx = player->vel.vx >> 1;
                // Quarter player's Y speed
                player->vel.vy = player->vel.vy >> 2;
            } else {
                player->cnst = getconstants(
                    player->character,
                    (player->speedshoes_frames > 0)
                    ? PC_SPEEDSHOES
                    : PC_DEFAULT);
                // Double Y speed, limiting it to -16.0 (0x00010000)
                player->vel.vy = MAX(player->vel.vy << 1, -0x10000);
            }
            
            PoolObject *explosion = object_pool_create(OBJ_EXPLOSION);
            explosion->freepos.vx = player->pos.vx;
            explosion->freepos.vy = level_water_y;
            explosion->state.anim_state.animation = 2; // Water splash
            sound_play_vag(sfx_splash, 0);
        }

        if(!player->underwater)
            // Reset to 30 secs of air
            player->remaining_air_frames = 1800;
        else {
            if(player->remaining_air_frames > 0) player->remaining_air_frames--;

            // Warning chime control
            switch(player->remaining_air_frames) {
            case (25 * 60):
            case (20 * 60):
            case (15 * 60):
                sound_play_vag(sfx_count, 0);
                break;
            default:
                // Start with a countdown of 8.
                // By now the player should've already seen the "5" and "4"
                // bubbles.
                if((player->remaining_air_frames > 0)
                   && (player->remaining_air_frames <= (8 * 60))
                   && !(player->remaining_air_frames % 60)) {
                    // Play sample every second.
                    // Notice that warning bubbles are actually emitted
                    // every two seconds.
                    sound_play_vag(sfx_count, 0);
                }
                break;
            }

            uint8_t emit_bubble = !((player->remaining_air_frames + 1) % 120)
                && (player->remaining_air_frames > 0);

            static uint8_t bubbletype = 0;

            switch(player->remaining_air_frames) {
            case (12 * 60): bubbletype = 3; break; // TODO: Warning bubble "5"
            case (10 * 60): bubbletype = 4; break; // TODO: Warning bubble "4"
            case (8 * 60):  bubbletype = 5; break; // TODO: Warning bubble "3"
            case (6 * 60):  bubbletype = 6; break; // TODO: Warning bubble "2"
            case (4 * 60):  bubbletype = 7; break; // TODO: Warning bubble "1"
            case (2 * 60):  bubbletype = 8; break; // TODO: Warning bubble "0"
            case 0: break; // TODO: DROWNED!
            }

            // Bubble emission
            if(emit_bubble) {
                // TODO: Either emit 1 or 2 bubbles. In case of 2,
                // First one has 1/4 chance of being a number bubble,
                // and the second is definitely a number bubble if not emitted
                // previously.
                // For now we emit a single small bubble.
                PoolObject *bubble = object_pool_create(OBJ_BUBBLE);
                if(bubble) {
                    bubble->state.anim_state.animation = bubbletype;
                    if(bubbletype > 0) bubbletype = 0;
                    bubble->freepos.vx = player->pos.vx + (0x6000 * player->anim_dir);
                    bubble->freepos.vy = player->pos.vy;
                }
            }
        }
    }

    // Screen transform
    player->pos.vx += player->vel.vx;
    player->pos.vy += player->vel.vy;

    // Print sensors state
    if(debug_mode > 1) {
        char buffer[255];
        snprintf(buffer, 255,
                 "COL %s%s.%s%s.%s.%s\n"
                 ,
                 // Collisions
                 player->ev_ceil1.collided ? "C" : " ",
                 player->ev_ceil2.collided ? "C" : " ",
                 player->ev_grnd1.collided ? "G" : " ",
                 player->ev_grnd2.collided ? "G" : " ",
                 player->ev_left.collided ? "L" : " ",
                 player->ev_right.collided ? "R" : " ");
        font_draw_sm(buffer, 8, 74);
    }

    // Reset sensors
    player->ev_left  = (CollisionEvent){ 0 };
    player->ev_right = (CollisionEvent){ 0 };
    player->ev_grnd1 = (CollisionEvent){ 0 };
    player->ev_grnd2 = (CollisionEvent){ 0 };
    player->ev_ceil1 = (CollisionEvent){ 0 };
    player->ev_ceil2 = (CollisionEvent){ 0 };
}

static int32_t
_snap_angle(int32_t angle)
{
    // Snap angle to nearest quarter.
    // These angles work somewhat like floor collision modes,
    // and floor/ceiling angles take precedence as well, but they're
    // a bit more complex since we also need to leverage diagonal angles.
    // NOTE: floor and ceiling angles add/subtract half a quarter (128)
    // units from their limits so the player doesn't rotate at every small
    // slope (e.g. 11.25 degrees or so)
    if((angle > 0x180) && (angle < 0x300))
        return 0x200;
    if((angle >= 0x300) && (angle <= 0x500))
        return 0x400;
    if((angle > 0x500) && (angle < 0x680))
        return 0x600;
    if((angle >= 0x680) && (angle <= 0x980))
        return 0x800;
    if((angle > 0x980) && (angle < 0xa38))
        return 0x938;
    if((angle >= 0xa38) && (angle <= 0xd00))
        return 0xc00;
    if((angle > 0xd00) && (angle < 0xe80))
        return 0xe00;
    return 0x00;
}

void
player_draw(Player *player, VECTOR *pos)
{
    uint8_t is_rolling_angle =
        (player_get_current_animation_hash(player) == ANIM_ROLLING);
    uint8_t is_rolling =
        is_rolling_angle
        || (player_get_current_animation_hash(player) == ANIM_SPINDASH);
    int32_t anim_angle = -_snap_angle(player->angle);
    uint8_t show_character = (((player->iframes >> 2) % 2) == 0);
    uint8_t facing_left = (player->anim_dir < 0);
    
    // if iframes, do not show for every 4 frames
    if(player->cur_anim && show_character) {
        chara_draw_gte(&player->chara,
                       player->anim_frame,
                       (int16_t)(pos->vx >> 12),
                       (int16_t)(pos->vy >> 12) + (is_rolling ? 4 : 0),
                       facing_left,
                       (is_rolling_angle ? 0 : anim_angle));
    }

    // Miles' tail
    if((player->character == CHARA_MILES)
       && player->tail_cur_anim
       && show_character) {
        
        int32_t tail_distance = (is_rolling ? 8 : 0) << 12;
        if(player->anim_dir < 0) tail_distance *= -1;

        uint8_t moving_towards_dir =
            (!player->grnd) || (SIGNUM(player->vel.vz) == player->anim_dir);
        
        int32_t tail_angle = anim_angle;
        if(((player->action == ACTION_JUMPING)
            || (player->action == ACTION_ROLLING))) {
                if(player->vel.vy < -(player->cnst->y_min_jump >> 1)) {
                    if(abs(player->vel.vx) <= player->cnst->x_min_roll_spd)
                        tail_angle = facing_left ? 0x400 : 0xc00;
                    else if(!moving_towards_dir) tail_angle = 0x000;
                    else tail_angle = facing_left ? 0x200 : 0xe00;
                } else if(player->vel.vy > (player->cnst->y_min_jump >> 1)) {
                    if(abs(player->vel.vx) <= player->cnst->x_min_roll_spd)
                        tail_angle = facing_left ? 0xc00 : 0x400;
                    else if(!moving_towards_dir) tail_angle = 0x000;
                    else tail_angle = facing_left ? 0xe00 : 0x200;
                } else tail_angle = 0x000;
        }

        int16_t tail_distance_x = (tail_distance * rcos(tail_angle)) >> 24;
        int16_t tail_distance_y = (tail_distance * rsin(tail_angle)) >> 24;
        
        chara_draw_gte(&player->chara,
                       player->tail_anim_frame,
                       (int16_t)(pos->vx >> 12) - tail_distance_x,
                       (int16_t)(pos->vy >> 12) - tail_distance_y,
                       facing_left,
                       tail_angle);
    }
}

void _player_set_hurt(Player *player, int32_t hazard_x);
void _player_set_ring_loss(Player *player, int32_t hazard_x, uint8_t num_rings);

void
player_do_damage(Player *player, int32_t hazard_x)
{
    // TODO: Missing death routine
    if((player->shield > 0)
       || (level_ring_count == 0)) { // TODO: Remove this in favor of a death
        player->shield = 0;
        _player_set_hurt(player, hazard_x);
        sound_play_vag(sfx_death, 0);
        return;
    }

    _player_set_ring_loss(player, hazard_x, level_ring_count);
    level_ring_count = 0;
    sound_play_vag(sfx_ringl, 0);
}

void
_player_set_hurt(Player *player, int32_t hazard_x)
{
    player_set_action(player, ACTION_HURT);
    player->grnd = 0;
    player->vel.vy = -player->cnst->y_hurt_force;
    int32_t a = SIGNUM(player->pos.vx - hazard_x);
    player->vel.vx = player->cnst->x_hurt_force * ((a == 0) ? 1 : a);
    player->ctrllock = 2;
}

// 101.25° = 0.28125 (normalized angle)
#define RING_STARTING_ANGLE 0x00000480
// 22.5° = 0.0625 (normalized angle)
#define RING_ANGLE_STEP     0x00000100
#define RING_START_SPD_DIRECT        4

void
_player_set_ring_loss(Player *player, int32_t hazard_x, uint8_t num_rings)
{
    _player_set_hurt(player, hazard_x);

    num_rings = num_rings > 32 ? 32 : num_rings;

    uint32_t ring_counter = 0;
    int32_t  ring_angle = RING_STARTING_ANGLE;
    uint8_t  ring_flip = 0;
    int32_t  ring_speed = RING_START_SPD_DIRECT;

    while(ring_counter < num_rings) {
        // Create ring object
        PoolObject *ring = object_pool_create(OBJ_RING);
        ring->freepos.vx = player->pos.vx;
        ring->freepos.vy = player->pos.vy;
        ring->props |= OBJ_FLAG_ANIM_LOCK;
        ring->props |= OBJ_FLAG_RING_MOVING;

        // Calculate ring speed.
        // Notice that the speed is attributed directly to 4 since
        // we would have to bit shift the result by 12 bits to the right anyway
        ring->freepos.spdx = ring_speed * rcos(ring_angle);
        ring->freepos.spdy = ring_speed * -rsin(ring_angle);

        if(ring_flip) {
            ring->freepos.spdx *= -1;
            ring_angle = (ring_angle + RING_ANGLE_STEP) % ONE;
        }
        ring_flip = !ring_flip;
        ring_counter++;

        if(ring_counter == 16) {
            ring_speed >>= 1; // Halve ring speed
            ring_angle = RING_STARTING_ANGLE;
        }
    }
}

void
player_set_action(Player *player, PlayerAction action)
{
    // When changing action from whence the player was hurt,
    // remove control lock and setup iframes
    if(player->action == ACTION_HURT) {
        player->ctrllock = 0;
        player->iframes = PLAYER_HURT_IFRAMES;
    }
    player->action = action;
}
