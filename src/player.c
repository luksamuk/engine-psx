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
#define ANIM_STOPPED    0x08cd0220
#define ANIM_IDLE       0x02d1011f
#define ANIM_WALKING    0x0854020e
#define ANIM_RUNNING    0x08bf0222
#define ANIM_ROLLING    0x08890218
#define ANIM_SKIDDING   0x0a85024e
#define ANIM_PEELOUT    0x0849021f
#define ANIM_PUSHING    0x08b2021f
#define ANIM_CROUCHDOWN 0x104802fd
#define ANIM_LOOKUP     0x067001db

extern int debug_mode;

SoundEffect sfx_jump  = { 0 };
SoundEffect sfx_skid  = { 0 };
SoundEffect sfx_roll  = { 0 };
SoundEffect sfx_dash  = { 0 };
SoundEffect sfx_relea = { 0 };
SoundEffect sfx_dropd = { 0 };
SoundEffect sfx_ring  = { 0 };
SoundEffect sfx_pop   = { 0 };
SoundEffect sfx_sprn  = { 0 };
SoundEffect sfx_chek  = { 0 };

// TODO: Maybe shouldn't be extern?
extern TileMap16  map16;
extern TileMap128 map128;
extern LevelData  leveldata;
extern Camera     camera;

void
load_player(Player *player,
            const char *chara_filename,
            TIM_IMAGE  *sprites)
{
    load_chara(&player->chara, chara_filename, sprites);
    player->cur_anim = NULL;
    player->pos   = (VECTOR){ 0 };
    player->vel   = (VECTOR){ 0 };
    player->angle = 0;
    player->spinrev = 0;
    player->ctrllock = 0;
    player->framecount = 0;

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

    if(sfx_jump.addr == 0)  sfx_jump  = sound_load_vag("\\SFX\\JUMP.VAG;1");
    if(sfx_skid.addr == 0)  sfx_skid  = sound_load_vag("\\SFX\\SKIDDING.VAG;1");
    if(sfx_roll.addr == 0)  sfx_roll  = sound_load_vag("\\SFX\\ROLL.VAG;1");
    if(sfx_dash.addr == 0)  sfx_dash  = sound_load_vag("\\SFX\\DASH.VAG;1");
    if(sfx_relea.addr == 0) sfx_relea = sound_load_vag("\\SFX\\RELEA.VAG;1");
    if(sfx_dropd.addr == 0) sfx_dropd = sound_load_vag("\\SFX\\DROPD.VAG;1");
    if(sfx_ring.addr == 0)  sfx_ring  = sound_load_vag("\\SFX\\RING.VAG;1");
    if(sfx_pop.addr == 0)   sfx_pop   = sound_load_vag("\\SFX\\POP.VAG;1");
    if(sfx_sprn.addr == 0)  sfx_sprn  = sound_load_vag("\\SFX\\SPRN.VAG;1");
    if(sfx_chek.addr == 0)  sfx_chek  = sound_load_vag("\\SFX\\CHEK.VAG;1");
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
player_update_collision(Player *player)
{
    player->push = 0;

    /* Collider linecasts */
    uint16_t
        anchorx = (player->pos.vx >> 12),
        anchory = (player->pos.vy >> 12);

    // Adjust y anchor to y + 8 when on totally flat ground
    int32_t push_anchory = anchory
        - ((player->grnd && player->angle == 0) ? 8 : 0) - 8;

    uint16_t grn_ceil_dist = WIDTH_RADIUS_NORMAL;
    uint16_t grn_grnd_dist = WIDTH_RADIUS_NORMAL;
    uint16_t grn_mag   = HEIGHT_RADIUS_NORMAL;
    uint16_t ceil_mag  = HEIGHT_RADIUS_NORMAL;
    uint16_t left_mag  = PUSH_RADIUS;
    uint16_t right_mag = PUSH_RADIUS - 1;

    if(player->action == ACTION_JUMPING) {
        grn_ceil_dist = grn_grnd_dist = WIDTH_RADIUS_ROLLING;
        grn_mag = ceil_mag = HEIGHT_RADIUS_ROLLING;
    }

    // Reset sensors
    player->ev_grnd1 = (CollisionEvent){ 0 };
    player->ev_grnd2 = (CollisionEvent){ 0 };
    player->ev_ceil1 = (CollisionEvent){ 0 };
    player->ev_ceil2 = (CollisionEvent){ 0 };
    player->ev_left  = (CollisionEvent){ 0 };
    player->ev_right = (CollisionEvent){ 0 };

    // Ground sensors
    player->ev_grnd1 = linecast(&leveldata, &map128, &map16,
                                anchorx - grn_grnd_dist, anchory,
                                CDIR_FLOOR, grn_mag);
    player->ev_grnd2 = linecast(&leveldata, &map128, &map16,
                                anchorx + grn_grnd_dist - 1, anchory,
                                CDIR_FLOOR, grn_mag);

    if(!player->grnd) {
        // Ceiling sensors
        player->ev_ceil1 = linecast(&leveldata, &map128, &map16,
                                    anchorx - grn_ceil_dist, anchory,
                                    CDIR_CEILING, ceil_mag);
        player->ev_ceil2 = linecast(&leveldata, &map128, &map16,
                                    anchorx + grn_ceil_dist - 1, anchory,
                                    CDIR_CEILING, ceil_mag);
    }

    // Push sensors
    uint8_t is_push_active;
    is_push_active = (!player->grnd && abs(player->vel.vx) > 0);
    is_push_active = is_push_active ||
        ((player->grnd && abs(player->vel.vz) > 0)
         && ((player->angle >= 0x0 && player->angle <= 0x400)
             || (player->angle >= 0xc00 && player->angle <= 0x1000)));

    if(is_push_active) {
        // "E" sensor
        if(player->vel.vx < 0) {
            player->ev_left = linecast(&leveldata, &map128, &map16,
                                       anchorx, push_anchory,
                                       CDIR_LWALL, left_mag);
        }

        // "F" sensor
        if(player->vel.vx > 0) {
            player->ev_right = linecast(&leveldata, &map128, &map16,
                                        anchorx, push_anchory,
                                        CDIR_RWALL, right_mag);
        }
    }

    /* Draw Colliders */
    if(debug_mode > 1) {
        VECTOR player_canvas_pos = {
            player->pos.vx - camera.pos.vx + (CENTERX << 12),
            player->pos.vy - camera.pos.vy + (CENTERY << 12),
            0
        };
        uint16_t ax = (player_canvas_pos.vx >> 12),
            ay = (player_canvas_pos.vy >> 12);

        LINE_F2 *line;

        // Ground sensor left
        line = get_next_prim();
        increment_prim(sizeof(LINE_F2));
        setLineF2(line);
        if(player->ev_grnd1.collided) setRGB0(line, 255, 0, 0);
        else                  setRGB0(line, 0, 93, 0);
        setXY2(line, ax - grn_ceil_dist, ay, ax - grn_ceil_dist, ay + grn_mag);
        sort_prim(line, 0);
    
        // Ground sensor right
        line = get_next_prim();
        increment_prim(sizeof(LINE_F2));
        setLineF2(line);
        if(player->ev_grnd2.collided) setRGB0(line, 255, 0, 0);
        else                  setRGB0(line, 23, 99, 63);
        setXY2(line, ax + grn_ceil_dist, ay, ax + grn_ceil_dist, ay + grn_mag);
        sort_prim(line, 0);

        if(!player->grnd) {
            // Ceiling sensor left
            line = get_next_prim();
            increment_prim(sizeof(LINE_F2));
            setLineF2(line);
            if(player->ev_ceil1.collided) setRGB0(line, 255, 0, 0);
            else                  setRGB0(line, 0, 68, 93);
            setXY2(line, ax - grn_ceil_dist, ay - 8, ax - grn_ceil_dist, ay - 8 - ceil_mag);
            sort_prim(line, 0);
    
            // Ceiling sensor right
            line = get_next_prim();
            increment_prim(sizeof(LINE_F2));
            setLineF2(line);
            if(player->ev_ceil2.collided) setRGB0(line, 255, 0, 0);
            else                  setRGB0( line, 99, 94, 23);
            setXY2( line, ax + grn_ceil_dist, ay - 8, ax + grn_ceil_dist, ay - 8 - ceil_mag);
            sort_prim(line, 0);
        }

        if(is_push_active) {
            int32_t push_ay = ay + ((player->grnd && player->angle == 0) ? 8 : 0) + 8;
            // Left sensor
            if(player->vel.vx < 0) {
                line = get_next_prim();
                increment_prim(sizeof(LINE_F2));
                setLineF2(line);
                if(player->ev_left.collided) setRGB0(line, 255, 0, 0);
                else                 setRGB0(line, 99, 23, 99);
                setXY2(line, ax, push_ay, ax - left_mag, push_ay);
                sort_prim(line, 0);
            }
    
            // Right sensor
            if(player->vel.vx > 0) {
                line = get_next_prim();
                increment_prim(sizeof(LINE_F2));
                setLineF2(line);
                if(player->ev_right.collided) setRGB0(line, 255, 0, 0);
                else                  setRGB0(line, 99, 23, 99);
                setXY2(line, ax, push_ay, ax + right_mag, push_ay);
                sort_prim(line, 0);
            }
        }
    }
}

void
_player_handle_collision(Player *player)
{
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

    if(!player->grnd) {
        player->angle = 0;

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

            if(player->action == ACTION_JUMPING || player->action == ACTION_ROLLING)
                player->action = ACTION_NONE;
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

            int32_t new_coord = 0;
            if(player->ev_grnd1.collided) new_coord = player->ev_grnd1.coord;
            if((player->ev_grnd2.collided && (player->ev_grnd2.coord < new_coord))
               || (new_coord == 0))
                new_coord = player->ev_grnd2.coord;

            player->pos.vy = (new_coord - 16) << 12;
        }
    }
}

void
player_update(Player *player)
{
    _player_handle_collision(player);
    // X movement
    /* Ground movement */
    if(player->grnd) {
        if(player->ctrllock > 0) player->ctrllock--;

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
            if(player->vel.vz > 0 && pad_pressing(PAD_LEFT))
                player->vel.vz -= X_ROLL_DECEL + X_ROLL_FRICTION;
            else if(player->vel.vz < 0 && pad_pressing(PAD_RIGHT))
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
            if(!pad_pressing(PAD_DOWN)) {
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
                if(pad_pressed(PAD_CROSS)) {
                    player->spinrev += 0x2000;
                    sound_play_vag(sfx_dash, 0);
                }
            }
        } else {
            // Default physics
            player->action = ACTION_NONE;

            if(pad_pressing(PAD_RIGHT) && (player->ctrllock == 0)) {
                if(player->vel.vz < 0) {
                    player->action = ACTION_SKIDDING;
                    player->vel.vz += X_DECEL;
                } else {
                    if(player->vel.vz < X_TOP_SPD)
                        player->vel.vz += X_ACCEL;
                    player->anim_dir = 1;
                }
            } else if(pad_pressing(PAD_LEFT) && (player->ctrllock == 0)) {
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
            if((abs(player->vel.vz) < X_MAX_SLIP_SPD)
               && (player->angle > 0x1ec && player->angle < 0xe16)
               && player->ctrllock == 0) {
                player->ctrllock = 30;
                // if mode is not floor:
                //player->grnd = 0;
            }

            /* Action changers */
            if(pad_pressing(PAD_DOWN)) {
                if(abs(player->vel.vz) >= X_MIN_ROLL_SPD) { // Rolling
                    player->action = ACTION_ROLLING;
                    player_set_animation_direct(player, ANIM_ROLLING);
                    sound_play_vag(sfx_roll, 0);
                } else if(player->vel.vz == 0 && pad_pressed(PAD_CROSS)) { // Spindash
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
        if(pad_pressing(PAD_RIGHT)) {
            if(player->vel.vx < X_TOP_SPD)
                player->vel.vx += X_AIR_ACCEL;
            player->anim_dir = 1;
        } else if(pad_pressing(PAD_LEFT)) {
            if(player->vel.vx > -X_TOP_SPD)
                player->vel.vx -= X_AIR_ACCEL;
            player->anim_dir = -1;
        }

        // Air drag. Calculated before applying gravity.
        if(player->vel.vy < 0 && player->vel.vy > -Y_MIN_JUMP) {
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
            if(!pad_pressing(PAD_CROSS)) {
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
        player->vel.vy += Y_GRAVITY;
    } else {
        if(pad_pressed(PAD_CROSS) && player->action != ACTION_SPINDASH) {
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
            } else if(pad_pressing(PAD_UP)) {
                player_set_animation_direct(player, ANIM_LOOKUP);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
                player->action = ACTION_LOOKUP;
            } else if(pad_pressing(PAD_DOWN)) {
                player_set_animation_direct(player, ANIM_CROUCHDOWN);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
                player->action = ACTION_CROUCHDOWN;
            } else if(player->idle_timer == 0) {
                player_set_animation_direct(player, ANIM_IDLE);
                player->loopback_frame = 2;
            } else if (!pad_pressing(PAD_LEFT) && !pad_pressing(PAD_RIGHT)) {
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
    } else player->idle_timer = ANIM_IDLE_TIMER_MAX;

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
}

void
player_draw(Player *player, VECTOR *pos)
{
    if(player->cur_anim)
        chara_render_frame(&player->chara,
                           player->anim_frame,
                           (int16_t)(pos->vx >> 12),
                           (int16_t)(pos->vy >> 12) - 8,
                           player->anim_dir < 0);
}
