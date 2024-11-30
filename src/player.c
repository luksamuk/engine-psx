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

#define TMP_ANIM_SPD          7
#define ANIM_IDLE_TIMER_MAX 180

// Adler32 sums of animation names for ease of use
#define ANIM_STOPPED          0x08cd0220
#define ANIM_IDLE             0x02d1011f
#define ANIM_WALKING          0x0854020e
#define ANIM_RUNNING          0x08bf0222
#define ANIM_ROLLING          0x08890218
#define ANIM_SKIDDING         0x0a85024e
#define ANIM_PEELOUT          0x0849021f
#define ANIM_PUSHING          0x08b2021f
#define ANIM_CROUCHDOWN       0x104802fd
#define ANIM_LOOKUP           0x067001db
#define ANIM_SPRING           0x068e01d4
#define ANIM_HURT             0x031b0144
#define ANIM_DEATH            0x04200167

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

// TODO: Maybe shouldn't be extern?
extern TileMap16  map16;
extern TileMap128 map128;
extern LevelData  leveldata;
extern Camera     camera;
extern uint8_t    level_ring_count;

void
load_player(Player *player,
            const char *chara_filename,
            TIM_IMAGE  *sprites)
{
    player->input = (InputState){ 0 };
    load_chara(&player->chara, chara_filename, sprites);
    player->cur_anim = NULL;
    player->pos   = (VECTOR){ 0 };
    player->vel   = (VECTOR){ 0 };
    player->angle = 0;
    player->spinrev = 0;
    player->ctrllock = 0;
    player->airdirlock = 0;
    player->framecount = 0;
    player->iframes = 0;
    player->shield = 0;
    player->gsmode = CDIR_FLOOR;
    player->psmode = CDIR_FLOOR;

    player_set_animation_direct(player, ANIM_STOPPED);
    player->anim_frame = player->anim_timer = 0;
    player->anim_dir = 1;
    player->idle_timer = ANIM_IDLE_TIMER_MAX;
    player->grnd = player->push = 0;

    player->ev_grnd1 = (CollisionEvent){ 0 };
    player->ev_grnd2 = (CollisionEvent){ 0 };
    player->ev_left  = (CollisionEvent){ 0 };
    player->ev_right = (CollisionEvent){ 0 };
    player->ev_ceil1 = (CollisionEvent){ 0 };
    player->ev_ceil2 = (CollisionEvent){ 0 };

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

    /* Collider linecasts */
    uint16_t
        anchorx = (player->pos.vx >> 12),
        anchory = (player->pos.vy >> 12);

    // Adjust y anchor to y + 8 when on totally flat ground
    int32_t push_anchory = anchory
        - ((player->grnd && player->angle == 0) ? 8 : 0) - 8;

    uint16_t left_mag  = PUSH_RADIUS;
    uint16_t right_mag = PUSH_RADIUS - 1;

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

    if(is_push_active) {
        // "E" sensor
        if(!player->ev_left.collided) {
            if(player->vel.vx < 0) {
                player->ev_left = linecast(&leveldata, &map128, &map16,
                                           anchorx, push_anchory,
                                           ldir, left_mag, player->gsmode);
            }
        }

        // "F" sensor
        if(!player->ev_right.collided) {
            if(player->vel.vx > 0) {
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
    case CDIR_RWALL:
        if(player->ev_right.collided && player->vel.vz > 0) {
            player->grnd = 0;
            player->angle = 0;
            player->vel.vz = 0;
            // TODO: Same as hitting the head. Adjust this to look like ceiling
            player->pos.vy = (player->ev_right.coord + 10) << 12;
        }

        if(player->ev_left.collided && player->vel.vz < 0) {
            player->grnd = 00;
            player->angle = 0;
            player->vel.vz = 0;
            // TODO: Hit your ass down there, Adjust this to look like floor
            player->pos.vy = (player->ev_left.coord - 25) << 12;
        }
        break;
    case CDIR_LWALL:
        if(player->ev_right.collided && player->vel.vz > 0) {
            player->grnd = 0;
            player->angle = 0;
            player->vel.vz = 0;
            // TODO: Hit your ass down there, Adjust this to look like floor
            player->pos.vy = (player->ev_right.coord + 25) << 12;
        }

        if(player->ev_left.collided && player->vel.vz < 0) {
            player->grnd = 00;
            player->angle = 0;
            player->vel.vz = 0;
            // TODO: Same as hitting the head. Adjust this to look like ceiling
            player->pos.vy = (player->ev_left.coord - 10) << 12;
        }
        break;
    case CDIR_CEILING:
        if(player->ev_right.collided && player->vel.vz > 0) {
            player->grnd = 0;
            player->angle = 0;
            player->vel.vz = 0;
            player->pos.vx = (player->ev_right.coord + 25) << 12;
        }

        if(player->ev_left.collided && player->vel.vz > 0) {
            player->grnd = 0;
            player->angle = 0;
            player->vel.vz = 0;
            player->pos.vx = (player->ev_left.coord - 10) << 12;
        }
        break;
    case CDIR_FLOOR:
    default:
        if(player->ev_right.collided && player->vel.vx > 0) {
            if(player->grnd) player->vel.vz = 0;
            else player->vel.vx = 0;
            player->pos.vx = (player->ev_right.coord - 10) << 12;
            if(player->grnd) player->push = 1;
        }

        if(player->ev_left.collided && player->vel.vx < 0) {
            if(player->grnd) player->vel.vz = 0;
            else player->vel.vx = 0;
            player->pos.vx = (player->ev_left.coord + 25) << 12;
            if(player->grnd) player->push = 1;
        }
        break;
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
        anchory_left += grn_grnd_dist + 1;
        anchory_right -= grn_grnd_dist - 1;
        break;
    case CDIR_LWALL:
        grndir = CDIR_LWALL;
        ceildir = CDIR_RWALL;
        anchory_left -= grn_grnd_dist - 1;
        anchory_right += grn_grnd_dist + 1;
        break;
    case CDIR_CEILING:
        grndir = CDIR_CEILING;
        ceildir = CDIR_FLOOR;
        anchorx_left += grn_grnd_dist - 1;
        anchorx_right -= grn_grnd_dist + 1;
        break;
    case CDIR_FLOOR:
    default:
        grndir = CDIR_FLOOR;
        ceildir = CDIR_CEILING;
        anchorx_left -= grn_grnd_dist + 1;
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
            // In case both are available, get the angle on the left.
            // This introduces certain collision bugs but let's leave it
            // like this for now
            else player->angle = player->ev_grnd1.angle;

            int32_t deg = (player->angle * (360 << 12) >> 24);

            // Set ground speed according to X and Y velocity,
            // and plaform angle
            if((deg <= 23) || (deg >= 339))
                // Landed on very flat ground, conserve X
                player->vel.vz = player->vel.vx;
            else if((deg <= 45) || (deg >= 316))
                // Slope ground, set to half vy
                player->vel.vz =
                    ((player->vel.vy * 2048) >> 12) * -SIGNUM(rsin(player->angle));
            else
                // Steep ground, set to full vy
                player->vel.vz =
                    player->vel.vy * -SIGNUM(rsin(player->angle));

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
                player->action = ACTION_NONE;
                player->airdirlock = 0;
            }
            else if(player->action == ACTION_DROPDASH) {
                // Perform drop dash
                player->framecount   = 0;
                player->holding_jump = 0;
                player->action = ACTION_ROLLING;
                // We're going to need the previous vel.vx as usual,
                // but we're going to manipulate gsp AFTER it has been calculated,
                // so this code MUST come after landing speed calculation
                uint8_t moving_backwards =
                    (player->vel.vx > 0 && player->anim_dir == -1)
                    || (player->vel.vx < 0 && player->anim_dir == 1);
                if(!moving_backwards) {
                    // gsp = (gsp / 4) + (drpspd * dir)
                    player->vel.vz = (player->vel.vz >> 2) + (X_DRPSPD * player->anim_dir);
                    if(player->vel.vz > 0)
                        player->vel.vz = (player->vel.vz > X_DRPMAX) ? X_DRPMAX : player->vel.vz;
                    else player->vel.vz = (player->vel.vz < X_DRPMAX) ? -X_DRPMAX : player->vel.vz;
                } else {
                    if(player->angle == 0) player->vel.vz = X_DRPSPD * player->anim_dir;
                    else {
                        // gsp = (gsp / 2) + (drpspd * dir)
                        player->vel.vz = (player->vel.vz >> 1) + (X_DRPSPD * player->anim_dir);
                        if(player->vel.vz > 0)
                            player->vel.vz = (player->vel.vz > X_DRPMAX) ? X_DRPMAX : player->vel.vz;
                        else player->vel.vz = (player->vel.vz < X_DRPMAX) ? -X_DRPMAX : player->vel.vz;
                    }
                }
                sound_play_vag(sfx_relea, 0);
                camera.lag = 0x8000 >> 12;
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
        }
    } else {
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
                player->pos.vx = (new_coord) << 12;
                break;
            case CDIR_CEILING:
                if(player->ev_grnd1.collided) new_coord = player->ev_grnd1.coord;
                if((player->ev_grnd2.collided && (player->ev_grnd2.coord > new_coord))
                   || (new_coord == 0))
                    new_coord = player->ev_grnd2.coord;
                player->pos.vy = (new_coord) << 12;
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
player_update(Player *player)
{
    // NOTE THAT PLAYER INPUT IS NOT UPDATED AUTOMATICALLY!
    // One must call input_get_state on player->input so that
    // player input is recognized. This is done in screen_level.c.

    uint32_t p_angle = abs(player->angle);

    /* GROUND SENSORS COLLISION MODES */
    if((p_angle > 0x0e2d) || (p_angle <= 0x01bb))
            player->gsmode = CDIR_FLOOR;
    else if((p_angle > 0x01bb) && (p_angle <= 0x05d2))
        player->gsmode = (player->angle >= 0)
            ? CDIR_RWALL
            : CDIR_LWALL;
    else if((p_angle > 0x05d2) && (p_angle <= 0x0a22))
        player->gsmode = CDIR_CEILING;
    else if((p_angle > 0x0a22) && (p_angle <= 0x0e2d))
        player->gsmode = (player->angle >= 0)
            ? CDIR_LWALL
            : CDIR_RWALL;

    /* PUSH SENSORS COLLISION MODES */
    // The logic here is basically subtract #x010 from each angle.
    // Push sensors are supposed to turn EARLIER than ground sensors
    /* if((p_angle > 0x0ec8) || (p_angle <= 0x0110)) */
    /*     player->psmode = CDIR_FLOOR; */
    /* else if((p_angle > 0x0110) && (p_angle <= 0x0527)) */
    /*     player->psmode = (player->angle >= 0) */
    /*         ? CDIR_RWALL */
    /*         : CDIR_LWALL; */
    /* else if((p_angle > 0x0527) && (p_angle <= 0x0acd)) */
    /*     player->psmode = CDIR_CEILING; */
    /* else if((p_angle > 0x0acd) && (p_angle <= 0x0ec8)) */
    /*     player->psmode = (player->angle >= 0) */
    /*         ? CDIR_LWALL */
    /*         : CDIR_RWALL; */
    player->psmode = player->gsmode;

    _player_update_collision_lr(player); // Push sensor collision detection
    _player_update_collision_tb(player); // Ground sensor collision detection

    // i-frames
    if(player->iframes > 0) player->iframes--;

    // X movement
    /* Ground movement */
    if(player->grnd) {
        if(player->ctrllock > 0) {
            player->ctrllock--;
        }

        if(player->action == ACTION_ROLLING) {
            // Rolling physics.
            // Slope factor
            int32_t angle_sin = rsin(player->angle);
            player->vel.vz -= (
                (SIGNUM(player->vel.vz) == SIGNUM(angle_sin)
                 ? X_SLOPE_ROLLUP
                 : X_SLOPE_ROLLDOWN)
                * angle_sin) >> 12;

            // Deceleration on input
            if(player->vel.vz > 0 && input_pressing(&player->input, PAD_LEFT))
                player->vel.vz -= X_ROLL_DECEL + X_ROLL_FRICTION;
            else if(player->vel.vz < 0 && input_pressing(&player->input, PAD_RIGHT))
                player->vel.vz += X_ROLL_DECEL + X_ROLL_FRICTION;
            else {
                // Apply roll friction
                player->vel.vz -= (player->vel.vz > 0 ? X_ROLL_FRICTION : -X_ROLL_FRICTION);
            }

            // Uncurl if too slow
            if(abs(player->vel.vz) < X_MIN_UNCURL_SPD)
                player->action = ACTION_NONE;
        } else if(player->action == ACTION_SPINDASH) {
            // Release
            if(!input_pressing(&player->input, PAD_DOWN)) {
                player->vel.vz +=
                    (0x8000 + (floor12(player->spinrev) >> 1)) * player->anim_dir;
                player->action = ACTION_ROLLING;
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
        } else {
            // Default physics
            player->action = ACTION_NONE;

            if(input_pressing(&player->input, PAD_RIGHT)
               && (player->ctrllock == 0)) {
                if(player->vel.vz < 0) {
                    player->action = ACTION_SKIDDING;
                    player->vel.vz += X_DECEL;
                } else {
                    if(player->vel.vz < X_TOP_SPD)
                        player->vel.vz += X_ACCEL;
                    player->anim_dir = 1;
                }
            } else if(input_pressing(&player->input, PAD_LEFT)
                      && (player->ctrllock == 0)) {
                if(player->vel.vz > 0) {
                    player->action = ACTION_SKIDDING;
                    player->vel.vz -= X_DECEL;
                } else {
                    if(player->vel.vz > -X_TOP_SPD)
                        player->vel.vz -= X_ACCEL;
                    player->anim_dir = -1;
                }
            } else {
                // Apply friction
                player->vel.vz -= (player->vel.vz > 0 ? X_FRICTION : -X_FRICTION);
                if(abs(player->vel.vz) <= X_FRICTION) player->vel.vz = 0;
            }

            // Slope factor application. Should only not work when in ceiling
            if(abs(player->vel.vz) >= X_SLOPE_MIN_SPD)
                player->vel.vz -= (X_SLOPE_NORMAL * rsin(player->angle)) >> 12;

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
                if(abs(player->vel.vz) >= X_MIN_ROLL_SPD) { // Rolling
                    player->action = ACTION_ROLLING;
                    player_set_animation_direct(player, ANIM_ROLLING);
                    sound_play_vag(sfx_roll, 0);
                } else if(player->vel.vz == 0
                          && input_pressed(&player->input, PAD_CROSS)) { // Spindash
                    player->action = ACTION_SPINDASH;
                    player_set_animation_direct(player, ANIM_ROLLING);
                    player->spinrev = 0;
                    sound_play_vag(sfx_dash, 0);
                }
                
            }
        }

        // Ground speed cap
        if(player->vel.vz > X_MAX_SPD) player->vel.vz = X_MAX_SPD;
        else if(player->vel.vz < -X_MAX_SPD) player->vel.vz = -X_MAX_SPD;

        // Distribute ground speed onto X and Y components
        player->vel.vx = (player->vel.vz * rcos(player->angle)) >> 12;
        player->vel.vy = (player->vel.vz * -rsin(player->angle)) >> 12;
    } else {
        // Air X movement
        if(input_pressing(&player->input, PAD_RIGHT)
           && (player->ctrllock == 0)) {
            if(player->vel.vx < X_TOP_SPD)
                player->vel.vx += X_AIR_ACCEL;
            if(!player->airdirlock)
                player->anim_dir = 1;
        } else if(input_pressing(&player->input, PAD_LEFT)
                  && (player->ctrllock == 0)) {
            if(player->vel.vx > -X_TOP_SPD)
                player->vel.vx -= X_AIR_ACCEL;
            if(!player->airdirlock)
                player->anim_dir = -1;
        }

        // Air drag. Calculated before applying gravity.
        if((player->vel.vy < 0 && player->vel.vy > -Y_MIN_JUMP)
           && (player->action != ACTION_HURT)) {
            // xsp -= (xsp div 0.125) / 256
            int32_t air_drag = (div12(abs(player->vel.vx), 0x200) << 12) / 0x100000;
            if(player->vel.vx > 0)
                player->vel.vx -= air_drag;
            else if(player->vel.vx < 0)
                player->vel.vx += air_drag;
        }

        // Air speed cap
        if(player->vel.vx > X_MAX_SPD) player->vel.vx = X_MAX_SPD;
        else if(player->vel.vx < -X_MAX_SPD) player->vel.vx = -X_MAX_SPD;
    }

    // Y movement
    if(!player->grnd) {
        if(player->action == ACTION_JUMPING) {
            if(!input_pressing(&player->input, PAD_CROSS)) {
                // Short jump
                if(player->vel.vy < -Y_MIN_JUMP)
                    player->vel.vy = -Y_MIN_JUMP;
                player->holding_jump = 0;
            } else {
                // Drop dash charge wait
                if(!player->holding_jump) {
                    if(player->framecount < 20) {
                        player->framecount++;
                    } else {
                        sound_play_vag(sfx_dropd, 0);
                        player->action = ACTION_DROPDASH;
                    }
                }
            }
        }
        player->vel.vy += (player->action == ACTION_HURT)
            ? Y_HURT_GRAVITY
            : Y_GRAVITY;
    } else {
        if(input_pressed(&player->input, PAD_CROSS) && player->action != ACTION_SPINDASH) {
            player->vel.vx -= (Y_JUMP_STRENGTH * rsin(player->angle)) >> 12;
            player->vel.vy -= (Y_JUMP_STRENGTH * rcos(player->angle)) >> 12;
            player->grnd = 0;
            player_set_animation_direct(player, ANIM_ROLLING);
            sound_play_vag(sfx_jump, 0);
            player->action = ACTION_JUMPING;
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
                player_set_animation_direct(player, ANIM_ROLLING);
            } else if(input_pressing(&player->input, PAD_UP)) {
                player_set_animation_direct(player, ANIM_LOOKUP);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
                player->action = ACTION_LOOKUP;
            } else if(input_pressing(&player->input, PAD_DOWN)) {
                player_set_animation_direct(player, ANIM_CROUCHDOWN);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
                player->action = ACTION_CROUCHDOWN;
            } else if(player->idle_timer == 0) {
                player_set_animation_direct(player, ANIM_IDLE);
                player->loopback_frame = 2;
            } else if (!input_pressing(&player->input, PAD_LEFT)
                       && !input_pressing(&player->input, PAD_RIGHT)) {
                player_set_animation_direct(player, ANIM_STOPPED);
                if(player->idle_timer > 0) player->idle_timer--;
            }
        } else {
            player->idle_timer = ANIM_IDLE_TIMER_MAX;
            if(player->action == ACTION_SKIDDING) {
                if(abs(player->vel.vz) >= (4 << 12)) {
                    if(player_get_current_animation_hash(player) != ANIM_SKIDDING) {
                        sound_play_vag(sfx_skid, 0);
                    }
                    player_set_animation_direct(player, ANIM_SKIDDING);
                } if(player_get_current_animation_hash(player) != ANIM_SKIDDING) {
                      player_set_animation_direct(player, ANIM_WALKING);
                }
            } else if(player->action == ACTION_ROLLING
                      || player->action == ACTION_SPINDASH
                      || player->action == ACTION_DROPDASH) {
                 player_set_animation_direct(player, ANIM_ROLLING);
            } else if(abs(player->vel.vz) >= (10 << 12)) {
                player_set_animation_direct(player, ANIM_PEELOUT);
            } else if(abs(player->vel.vz) >= (6 << 12)) {
                player_set_animation_direct(player, ANIM_RUNNING);
            } else player_set_animation_direct(player, ANIM_WALKING);
        }
    } else {
        player->idle_timer = ANIM_IDLE_TIMER_MAX;
        if(player->action == ACTION_SPRING) {
            if(player->vel.vy < 0) {
                player_set_animation_direct(player, ANIM_SPRING);
            } else {
                player->airdirlock = 0;
                if(abs(player->vel.vz) >= (10 << 12)) {
                    player_set_animation_direct(player, ANIM_PEELOUT);
                } else if(abs(player->vel.vz) >= (6 << 12)) {
                    player_set_animation_direct(player, ANIM_RUNNING);
                } else player_set_animation_direct(player, ANIM_WALKING);
            }
        } else if(player->action == ACTION_HURT) {
            player_set_animation_direct(player, ANIM_HURT);
        }
    }

    // Animation speed correction
    if(player->anim_timer == 0) {
        switch(player_get_current_animation_hash(player)) {
        case ANIM_WALKING:
        case ANIM_RUNNING:
            player_set_frame_duration(player, MAX(0, 8 - abs(player->vel.vz >> 12)));
            break;

        case ANIM_ROLLING:
            if(player->action == ACTION_SPINDASH || player->action == ACTION_DROPDASH)
                player_set_frame_duration(player, 0);
            else
                player_set_frame_duration(player, MAX(0, 4 - abs(player->vel.vz >> 12)));
            break;
        case ANIM_PEELOUT:
            break;

        case ANIM_PUSHING:
            player_set_frame_duration(player, MAX(0, 8 - abs(player->vel.vz >> 12)) << 2);
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

    // Screen transform
    player->pos.vx += player->vel.vx;
    player->pos.vy += player->vel.vy;

    // Reset sensors
    player->ev_left  = (CollisionEvent){ 0 };
    player->ev_right = (CollisionEvent){ 0 };
    player->ev_grnd1 = (CollisionEvent){ 0 };
    player->ev_grnd2 = (CollisionEvent){ 0 };
    player->ev_ceil1 = (CollisionEvent){ 0 };
    player->ev_ceil2 = (CollisionEvent){ 0 };

    
}

void
player_draw(Player *player, VECTOR *pos)
{
    // if iframes, do not show for every 4 frames
    if(player->cur_anim && ((player->iframes >> 2) % 2) == 0)
        chara_render_frame(&player->chara,
                           player->anim_frame,
                           (int16_t)(pos->vx >> 12),
                           (int16_t)(pos->vy >> 12) - 9,
                           player->anim_dir < 0);
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
    player->action = ACTION_HURT;
    player->grnd = 0;
    player->vel.vy = -Y_HURT_FORCE;
    int32_t a = SIGNUM(player->pos.vx - hazard_x);
    player->vel.vx = X_HURT_FORCE * ((a == 0) ? 1 : a);
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
