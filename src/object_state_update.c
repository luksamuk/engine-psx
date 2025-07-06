#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "object_state.h"

#include "player.h"
#include "collision.h"
#include "sound.h"
#include "camera.h"
#include "render.h"
#include "timer.h"

#include "screen.h"
#include "screens/level.h"
#include "screens/slide.h"

#define ANIM_WALKING          0x0854020e
#define ANIM_ROLLING          0x08890218
#define ANIM_GASP             0x02d9012c

// Extern elements
extern Player player;
extern Camera camera;
extern TileMap16  map16;
extern TileMap128 map128;
extern LevelData  leveldata;
extern SoundEffect sfx_ring;
extern SoundEffect sfx_pop;
extern SoundEffect sfx_sprn;
extern SoundEffect sfx_chek;
extern SoundEffect sfx_death;
extern SoundEffect sfx_ringl;
extern SoundEffect sfx_shield;
extern SoundEffect sfx_event;
extern SoundEffect sfx_switch;
extern SoundEffect sfx_bubble;
extern SoundEffect sfx_sign;

extern int debug_mode;

extern uint8_t  level_ring_count;
extern uint32_t level_score_count;
extern uint8_t  level_finished;
extern int32_t  level_water_y;


// Object-specific definitions
#define RING_GRAVITY 0x00000180


// Update functions
static void _ring_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _goal_sign_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _monitor_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _spring_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos, uint8_t is_red);
static void _spring_diagonal_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos, uint8_t is_red);
static void _checkpoint_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos);
static void _spikes_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos);
static void _explosion_update(ObjectState *state, ObjectTableEntry *, VECTOR *);
static void _monitor_image_update(ObjectState *state, ObjectTableEntry *, VECTOR *);
static void _shield_update(ObjectState *state, ObjectTableEntry *, VECTOR *);
static void _switch_update(ObjectState *state, ObjectTableEntry *, VECTOR *);
static void _bubble_patch_update(ObjectState *state, ObjectTableEntry *, VECTOR *);
static void _bubble_update(ObjectState *state, ObjectTableEntry *, VECTOR *);
static void _end_capsule_update(ObjectState *state, ObjectTableEntry *, VECTOR *);

// Level-specific update functions
extern void object_update_R0(ObjectState *, ObjectTableEntry *, VECTOR *);
extern void object_update_R2(ObjectState *, ObjectTableEntry *, VECTOR *);
extern void object_update_R3(ObjectState *, ObjectTableEntry *, VECTOR *);
extern void object_update_R5(ObjectState *, ObjectTableEntry *, VECTOR *);

// Player hitbox information. Calculated once per frame.
int32_t player_vx, player_vy; // Top left corner of player hitbox
uint8_t player_attacking;

// TODO: ADJUST ACCORDING TO CHARACTER
int32_t player_width = 16;
int32_t player_height = HEIGHT_RADIUS_NORMAL << 1;

int player_hitbox_shown;

/* void */
/* _draw_player_hitbox() */
/* { */
/*     if(player_hitbox_shown) return; */
/*     player_hitbox_shown = 1; */
/*     uint16_t */
/*         rel_vx = player_vx - (camera.pos.vx >> 12) + CENTERX, */
/*         rel_vy = player_vy - (camera.pos.vy >> 12) + CENTERY; */
/*     POLY_F4 *hitbox = get_next_prim(); */
/*     increment_prim(sizeof(POLY_F4)); */
/*     setPolyF4(hitbox); */
/*     setSemiTrans(hitbox, 1); */
/*     setXYWH(hitbox, rel_vx, rel_vy, 16, player_height); */
/*     setRGB0(hitbox, 0xfb, 0x94, 0xdc); */
/*     sort_prim(hitbox, OTZ_LAYER_OBJECTS); */
/* } */

void
object_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos, uint8_t round)
{
    if(state->props & OBJ_FLAG_DESTROYED) return;

    // Calculate top left corner of player AABB.
    // Note that player data is in fixed-point format!
    player_vx = (player.pos.vx >> 12) - 8;
    player_attacking = (player.action == ACTION_JUMPING ||
                        player.action == ACTION_ROLLING ||
                        player.action == ACTION_SPINDASH ||
                        player.action == ACTION_DROPDASH ||
                        player.action == ACTION_GLIDE);
    player_height = (player_attacking
                     ? HEIGHT_RADIUS_ROLLING
                     : HEIGHT_RADIUS_NORMAL) << 1;
    player_vy = (player.pos.vy >> 12) - (player_height >> 1) - 1;

    /* if(debug_mode > 1) { */
    /*     _draw_player_hitbox(); */
    /* } */

    if(typedata->is_level_specific) goto level_specific_update;

    switch(state->id) {
    default: break;
    case OBJ_RING:                   _ring_update(state, typedata, pos);               break;
    case OBJ_GOAL_SIGN:              _goal_sign_update(state, typedata, pos);          break;
    case OBJ_MONITOR:                _monitor_update(state, typedata, pos);            break;
    case OBJ_SPRING_YELLOW:          _spring_update(state, typedata, pos, 0);          break;
    case OBJ_SPRING_RED:             _spring_update(state, typedata, pos, 1);          break;
    case OBJ_SPRING_YELLOW_DIAGONAL: _spring_diagonal_update(state, typedata, pos, 0); break;
    case OBJ_SPRING_RED_DIAGONAL:    _spring_diagonal_update(state, typedata, pos, 1); break;
    case OBJ_CHECKPOINT:             _checkpoint_update(state, typedata, pos);         break;
    case OBJ_SPIKES:                 _spikes_update(state, typedata, pos);             break;
    case OBJ_EXPLOSION:              _explosion_update(state, typedata, pos);          break;
    case OBJ_MONITOR_IMAGE:          _monitor_image_update(state, typedata, pos);      break;
    case OBJ_SHIELD:                 _shield_update(state, typedata, pos);             break;
    case OBJ_SWITCH:                 _switch_update(state, typedata, pos);             break;
    case OBJ_BUBBLE_PATCH:           _bubble_patch_update(state, typedata, pos);       break;
    case OBJ_BUBBLE:                 _bubble_update(state, typedata, pos);             break;
    case OBJ_END_CAPSULE:            _end_capsule_update(state, typedata, pos);        break;
    }
    return;

level_specific_update:
    switch(round) {
    default: break;
    case 0: object_update_R0(state, typedata, pos); break; // Test Level
    case 2: object_update_R2(state, typedata, pos); break; // Green Hill
    case 3: object_update_R3(state, typedata, pos); break; // Surely Wood
    case 4: break;                                         // Dawn Canyon
    case 5: object_update_R5(state, typedata, pos); break; // Amazing Ocean
    case 6: break;                                         // R6
    case 7: break;                                         // R7
    case 8: break;                                         // Eggmanland
    case 9: break;                                         // Windmill Isle
    }
}

/* ======================== */
/*   OBJECT-SPECIFIC IMPL  */
/* ======================== */


static void
_ring_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    if(state->anim_state.animation == 0) {
        // Calculate actual top left corner of ring AABB
        pos->vx -= 8;    pos->vy -= (8 + 32);

        // Hey -- if this is a moving ring (ring loss behaviour), only
        // allow the player to collect it if its action is not ACTION_HURT
        if(!((state->props & OBJ_FLAG_RING_MOVING) && (player.action == ACTION_HURT))) {
            // Ring collision
            if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                               pos->vx, pos->vy, 16, 16))
            {
                state->anim_state.animation = 1;
                state->anim_state.frame = 0;
                state->props ^= OBJ_FLAG_ANIM_LOCK; // Unlock from global timer
                level_ring_count++;
                sound_play_vag(sfx_ring, 0);
                return;
            }
        }

        // If ring is moving, we will not proceed to move it!
        if(state->props & OBJ_FLAG_RING_MOVING) {
            state->timer++;
            
            // If the ring is way too far from camera, just destroy it
            // By "too far", I mean two screens apart.
            // Also, moving rings only live for 256 frames
            if((state->freepos->vx < camera.pos.vx - (SCREEN_XRES << 13))
               || (state->freepos->vx > camera.pos.vx + (SCREEN_XRES << 13))
               || (state->freepos->vy < camera.pos.vy - (SCREEN_YRES << 13))
               || (state->freepos->vy > camera.pos.vy + (SCREEN_YRES << 13))
               || (state->timer >= 256)) {
                state->props |= OBJ_FLAG_DESTROYED;
                return;
            }

            // Apply gravity
            state->freepos->spdy += RING_GRAVITY;

            /* In the original Sonic The Hedgehog for Sega Genesis, ring bouncing
             * only occurred every 4 frames and disregarded walls, all for
             * performance reasons. But we don't have to do this here, since
             * our MIPS processor is about 4.5 times faster than the Motorola 68k
             * (~7.6MHz vs ~33.87MHz).
             * So we're checking for ground and wall collisions every frame.
             */
            if(/*!(state->timer % 4) &&*/
                // Use Sonic's own linecast algorithm, since it is aware of
                // level geometry -- and oh, level data is stored in external
                // variables as well. Check the file header.
                ((state->freepos->spdy > 0) && linecast(pos->vx + 8, pos->vy + 8,
                                                        CDIR_FLOOR, 10, CDIR_FLOOR).collided)
                || ((state->freepos->spdy < 0) && linecast(pos->vx + 8, pos->vy + 8,
                                                           CDIR_CEILING, 10, CDIR_FLOOR).collided)) {
                // Multiply Y speed by -0.75
                state->freepos->spdy = (state->freepos->spdy * -0x00000c00) >> 12;
            }

            if(/*!(state->timer % 4) &&*/
                // Do the same thing; except for lateral collision
                ((state->freepos->spdx < 0) && linecast(pos->vx + 8, pos->vy + 8,
                                                        CDIR_LWALL, 10, CDIR_FLOOR).collided)
               || ((state->freepos->spdx > 0) && linecast(pos->vx + 8, pos->vy + 8,
                                                          CDIR_RWALL, 10, CDIR_FLOOR).collided))
                // Multiply X speed by -0.75
                state->freepos->spdx = (state->freepos->spdx * -0x00000c00) >> 12;

            // Transform ring position wrt. speed
            state->freepos->vx += state->freepos->spdx;
            state->freepos->vy += state->freepos->spdy;
        }
    } else if(state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION
              && !(state->props & OBJ_FLAG_DESTROYED))
    {
        state->props |= OBJ_FLAG_DESTROYED;
    }
}

void
_goal_sign_change_score()
{
    // TODO: A temporary score count. Change this later!
    level_score_count += level_ring_count * 100;

    uint32_t seconds = get_elapsed_frames() / 60;

    if(seconds <= 29)       level_score_count += 50000; // Under 0:30
    else if(seconds <= 44)  level_score_count += 10000; // Under 0:45
    else if(seconds <= 59)  level_score_count += 5000;  // Under 1:00
    else if(seconds <= 89)  level_score_count += 4000;  // Under 1:30
    else if(seconds <= 119) level_score_count += 3000;  // Under 2:00
    else if(seconds <= 179) level_score_count += 2000;  // Under 3:00
    else if(seconds <= 239) level_score_count += 1000;  // Under 4:00
    else if(seconds <= 299) level_score_count += 500;   // Under 5:00
    // Otherwise you get nothing
}

static void
_goal_sign_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    if(state->frag_anim_state->animation == 0) {
        if(pos->vx <= (player.pos.vx >> 12)) {
            state->frag_anim_state->animation = 1;
            state->frag_anim_state->frame = 0;
            camera_focus(&camera, ((pos->vx + 80) << 12), (pos->vy - (CENTERY >> 1)) << 12);
            state->timer = 180;
            level_finished = 1;
            pause_elapsed_frames();
            sound_play_vag(sfx_sign, 0);
        }
    } else if(state->frag_anim_state->animation == 1) {
        state->timer--;
        if(state->timer < 0) {
            state->timer = 360; // 6-seconds music

            switch(screen_level_getcharacter()) {
            default:             state->frag_anim_state->animation = 2; break;
            case CHARA_MILES:    state->frag_anim_state->animation = 3; break;
            case CHARA_KNUCKLES: state->frag_anim_state->animation = 4; break;
            }

            player_set_action(&player, ACTION_NONE);
            screen_level_setmode(LEVEL_MODE_FINISHED);
            _goal_sign_change_score();
        }
    } else if((state->frag_anim_state->animation > 1)
              && (player.pos.vx < camera.pos.vx + (CENTERX << 12))) {
        state->timer = 360; // 6-seconds music
    } else if(state->frag_anim_state->animation < OBJ_ANIMATION_NO_ANIMATION) {
        if(state->timer == 360) {
            // First frame
            sound_bgm_play(BGM_LEVELCLEAR);
        }
        state->timer--;

        if((state->timer < 0) && (screen_level_getstate() == 2)) {
            screen_level_setstate(3);
        } else if(screen_level_getstate() == 4) {
            // TODO: THIS NEEDS TO BE REFACTORED AND MOVED TO SOMEWHERE ELSE.
            uint8_t lvl = screen_level_getlevel();
            if(lvl == 2 || lvl == 3) {
                // Finished engine test
                scene_change(SCREEN_TITLE);
            } else if(lvl != 4) {
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
                screen_slide_set_next(SLIDE_COMINGSOON);
                scene_change(SCREEN_SLIDE);
            }
        }
    }
}

static void
_monitor_update(ObjectState *state, ObjectTableEntry *entry, VECTOR *pos)
{
    if(state->anim_state.animation == 0) {
        // Calculate solidity
        int32_t solidity_vx = pos->vx - 16;
        int32_t solidity_vy = pos->vy - 32; // Monitor is a 32x32 solid box

        int32_t hitbox_vx = pos->vx - 15;
        int32_t hitbox_vy = pos->vy - 32; // Monitor hitbox is a 28x32 solid box
        
        // Perform collision detection
        if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                           solidity_vx, solidity_vy, 32, 32))
        {
            if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                               hitbox_vx, hitbox_vy, 28, 32)
               && player_attacking) {
                state->anim_state.animation = 1;
                state->anim_state.frame = 0;
                state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;
                level_score_count += 10;
                sound_play_vag(sfx_pop, 0);

                // Create monitor image object.
                // Account for fragment offset as well.
                PoolObject *image = object_pool_create(OBJ_MONITOR_IMAGE);
                image->freepos.vx = (pos->vx << 12) + ((int32_t)entry->fragment->offsetx << 12);
                image->freepos.vy = (pos->vy << 12) + ((int32_t)entry->fragment->offsety << 12);
                uint16_t animation = ((MonitorExtra *)state->extra)->kind;
                if(animation == MONITOR_KIND_1UP) {
                    switch(player.character) {
                    default:
                    case CHARA_SONIC:    animation = 5; break;
                    case CHARA_MILES:    animation = 7; break;
                    case CHARA_KNUCKLES: animation = 8; break;
                    }
                }
                image->state.anim_state.animation = animation;

                // Create explosion effect
                PoolObject *explosion = object_pool_create(OBJ_EXPLOSION);
                explosion->freepos.vx = (pos->vx << 12);
                explosion->freepos.vy = (pos->vy << 12);
                explosion->state.anim_state.animation = 0; // Small explosion

                if(!player.grnd && player.vel.vy > 0) {
                    player.vel.vy *= -1;
                }
            } else {
                // Landing on top
                if(((player_vy + player_height) < solidity_vy + 16) &&
                   ((player_vx >= solidity_vx - 8) && ((player_vx + 8) <= solidity_vx + 32)))
                {
                    player.ev_grnd1.collided = player.ev_grnd2.collided = 1;
                    player.ev_grnd1.angle = player.ev_grnd2.angle = 0;
                    player.ev_grnd1.coord = player.ev_grnd2.coord = solidity_vy + 4;
                } else if((player_vy + 8) > solidity_vy) {
                    // Check for intersection on left/right
                    if((player_vx + 8) < pos->vx) {
                        player.ev_right.collided = 1;
                        player.ev_right.coord = (solidity_vx + 2);
                        player.ev_right.angle = 0;
                    } else {
                        player.ev_left.collided = 1;
                        player.ev_left.coord = solidity_vx + 16;
                        player.ev_right.angle = 0;
                    }
                }
            }
        }
    }
}

static void
_spring_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos, uint8_t is_red)
{
    if(state->anim_state.animation == 0) {
        // Spring is 32x16 solid
        RECT solidity = {
            .x = pos->vx - 16,
            .y = pos->vy - 16,
            .w = 32,
            .h = 16,
        };
        ObjectCollision bump_side = OBJ_SIDE_TOP;

        // Handle possible rotations
        if(state->flipmask & MASK_FLIP_ROTCW) {
            solidity.x = pos->vx - 32;
            solidity.y = pos->vy + 16;
            solidity.w  = 16;
            solidity.h  = 32;
            bump_side = OBJ_SIDE_RIGHT;
        } else if(state->flipmask & MASK_FLIP_ROTCT) {
            solidity.x = pos->vx - 48;
            solidity.y = pos->vy - 48;
            solidity.w  = 16;
            solidity.h  = 32;
            bump_side = OBJ_SIDE_LEFT;
        } else if(state->flipmask & MASK_FLIP_FLIPY) {
            solidity.y -= 48;
            bump_side = OBJ_SIDE_BOTTOM;
        }

        ObjectCollision collision_side =
            solid_object_player_interaction(state, &solidity);

        // The player's spring action relate to where the spring is pointing at.
        if(collision_side == OBJ_SIDE_NONE)
            return;
        else if(collision_side == bump_side) {
            switch(bump_side) {
            default: break; // ?????
            case OBJ_SIDE_TOP:
                player.grnd = 0;
                player.vel.vy = is_red ? -0x10000 : -0xa000;
                player.angle = 0;
                player.ctrllock = 0;
                player.over_object = NULL;
                player_set_action(&player, ACTION_SPRING);
                break;
            case OBJ_SIDE_BOTTOM:
                player.grnd = 0;
                player.vel.vy = is_red ? 0x10000 : 0xa000;
                player.angle = 0;
                player.ctrllock = 0;
                player_set_action(&player, ACTION_SPRING);
                break;
            case OBJ_SIDE_RIGHT:
                player.vel.vz = is_red ? 0x10000 : 0xa000;
                if(!player.grnd) player.vel.vx = is_red ? 0x10000 : 0xa000;
                player.ctrllock = 16;
                player.anim_dir = 1;
                if(player.action != ACTION_ROLLING)
                    player_set_action(&player, ACTION_NONE);
                break;
            case OBJ_SIDE_LEFT:
                player.vel.vz = is_red ? -0x10000 : -0xa000;
                if(!player.grnd) player.vel.vx = is_red ? -0x10000 : -0xa000;
                player.ctrllock = 16;
                player.anim_dir = -1;
                if(player.action != ACTION_ROLLING)
                    player_set_action(&player, ACTION_NONE);
                break;
            };
            state->anim_state.animation = 1;
            sound_play_vag(sfx_sprn, 0);
        }

        /* else if(state->flipmask & MASK_FLIP_ROTCT) { // Left-pointing spring */
        /*     //player.pos.vx = (solidity_vx - player_width) << 12; */
        /*     player.ev_right.collided = 0; // Detach player from right wall if needed */
        /*     player.vel.vz = is_red ? -0x10000 : -0xa000; */
        /*     if(!player.grnd) player.vel.vx = is_red ? -0x10000 : -0xa000; */
        /*     player.ctrllock = 16; */
        /*     player.anim_dir = -1; */
        /*     state->anim_state.animation = 1; */
        /*     sound_play_vag(sfx_sprn, 0); */
        /*     if(player.action != ACTION_ROLLING) */
        /*         player_set_action(&player, ACTION_NONE); */
        /* } else if(state->flipmask & MASK_FLIP_ROTCW) { // Right-pointing spring */
        /*     //player.pos.vx = (solidity_vx + solidity_w + player_width + 8) << 12; */
        /*     player.ev_left.collided = 0; // Detach player from left wall if needed */
        /*     player.vel.vz = is_red ? 0x10000 : 0xa000; */
        /*     if(!player.grnd) player.vel.vx = is_red ? 0x10000 : 0xa000; */
        /*     player.ctrllock = 16; */
        /*     player.anim_dir = 1; */
        /*     state->anim_state.animation = 1; */
        /*     sound_play_vag(sfx_sprn, 0); */
        /*     if(player.action != ACTION_ROLLING) */
        /*         player_set_action(&player, ACTION_NONE); */
        /* } else if(state->flipmask == 0) { // Top-pointing spring */
        /*     player.pos.vy = (solidity_vy - (player_height >> 1)) << 12; */
        /*     player.grnd = 0; */
        /*     player.vel.vy = is_red ? -0x10000 : -0xa000; */
        /*     player.angle = 0; */
        /*     player.ctrllock = 0; */
        /*     player_set_action(&player, ACTION_SPRING); */
        /*     state->anim_state.animation = 1; */
        /*     sound_play_vag(sfx_sprn, 0); */
        /* } else if(state->flipmask & MASK_FLIP_FLIPY) { // Bottom-pointing spring */
        /*     player.pos.vy = (solidity_vy + solidity_h + (player_height >> 1)) << 12; */
        /*     player.grnd = 0; */
        /*     player.vel.vy = is_red ? 0x10000 : 0xa000; */
        /*     player.angle = 0; */
        /*     player.ctrllock = 0; */
        /*     player_set_action(&player, ACTION_SPRING); */
        /*     state->anim_state.animation = 1; */
        /*     sound_play_vag(sfx_sprn, 0); */
        /* } */
    } else if(state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION) {
        state->anim_state.animation = 0;
        state->anim_state.frame = 0;
    }
}


static void
_checkpoint_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    if(!(state->props & OBJ_FLAG_CHECKPOINT_ACTIVE)) {
        int32_t hitbox_vx = pos->vx - 8;
        int32_t hitbox_vy = pos->vy - 48;

        if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                           hitbox_vx, hitbox_vy, 16, 48))
        {
            state->props |= OBJ_FLAG_CHECKPOINT_ACTIVE;
            state->frag_anim_state->animation = 1;
            state->frag_anim_state->frame = 0;
            player.respawnpos = (VECTOR){
                .vx = pos->vx << 12,
                .vy = (pos->vy - 8) << 12,
                .vz = 0
            };
            sound_play_vag(sfx_chek, 0);
        }
    }
}

#define SPRND_ST_R 0x0000b500 // 11.3125
#define SPRND_ST_Y 0x00007120 // 7.0703125

static void
_spring_diagonal_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos, uint8_t is_red)
{
    // For diagonal springs, interaction should occur if and only if the player
    // is colliding with its top.
    if(state->anim_state.animation == 0) {
        /* int32_t solidity_vx = pos->vx - 16; */
        /* int32_t solidity_vy = pos->vy - 32; // Spring is 32x32 solid */
        /* int32_t solidity_w  = 32; */
        /* int32_t solidity_h  = 32; */

        // Spring is 32x32 solid
        RECT solidity = {
            .x = pos->vx - 16,
            .y = pos->vy - 32,
            .w = 32,
            .h = 32,
        };
        ObjectCollision bump_side = OBJ_SIDE_TOP;

        // Spring hitbox is actually calculated relative to player's X position
        // within it.
        // The hitbox is always low on the bottom 10 pixels of the spring,
        // unless the player is at any given X coordinate of it; if so, the
        // hitbox height increases/decreases to give a diagonal effect.
        // This way, the player will always collide with a hitbox that looks
        // like a diagonal trapezium.

        int32_t shrink = 0;
        int32_t delta = 0;
        if(state->flipmask & MASK_FLIP_FLIPX) {
            delta = solidity.w - (player_vx - solidity.x);
        } else {
            delta = player_vx - solidity.x + 16;
        }

        if(delta > 10 && delta < 33) {
            shrink = delta - 10;
        } else shrink = 22;

        solidity.h -= shrink;
        
        if(state->flipmask & MASK_FLIP_FLIPY) {
            solidity.y -= 32;
            bump_side = OBJ_SIDE_BOTTOM;
        } else solidity.y += shrink;

        ObjectCollision collision_side =
            solid_object_player_interaction(state, &solidity);

        if(collision_side == OBJ_SIDE_NONE) return;
        else if(collision_side == bump_side) {
            player.grnd = 0;
            player.vel.vx = is_red ? SPRND_ST_R : SPRND_ST_Y;
            player.vel.vy = is_red ? SPRND_ST_R : SPRND_ST_Y;
            if(!(state->flipmask & MASK_FLIP_FLIPY)) player.vel.vy *= -1;
            if(state->flipmask & MASK_FLIP_FLIPX) {
                player.vel.vx *= -1;
                player.anim_dir = -1; // Flip on X: point player left
            } else player.anim_dir = 1; // No flip on X: point player right
            player.airdirlock = 1;
            player_set_action(&player, ACTION_SPRING);
            state->anim_state.animation = 1;
            sound_play_vag(sfx_sprn, 0);
            if(bump_side == OBJ_SIDE_TOP) {
                player.over_object = NULL;
            }
        }
    } else if(state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION) {
        state->anim_state.animation = 0;
        state->anim_state.frame = 0;
    }
    
}


static void
_spikes_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    // Spikes are a 32x32 solid box
    RECT solidity = {
        .x = pos->vx - 16,
        .y = pos->vy - 32,
        .w = 32,
        .h = 32,
    };
    ObjectCollision hurt_side = OBJ_SIDE_TOP;

    // Handle possible rotations
    if(state->flipmask & MASK_FLIP_ROTCW) {
        solidity.x = pos->vx - 32;
        solidity.y = pos->vy + 16;
        hurt_side = OBJ_SIDE_RIGHT;
    } else if(state->flipmask & MASK_FLIP_ROTCT) {
        solidity.x = pos->vx - 64;
        solidity.y = pos->vy - 48;
        hurt_side = OBJ_SIDE_LEFT;
    } else if(state->flipmask & MASK_FLIP_FLIPY) {
        solidity.y -= 32;
        hurt_side = OBJ_SIDE_BOTTOM;
    }

    ObjectCollision side = solid_object_player_interaction(state, &solidity);
    if(side == hurt_side) {
        if(player.action != ACTION_HURT && player.iframes == 0) {
            player_do_damage(&player, (solidity.x + 16) << 12);
            if(side == OBJ_SIDE_TOP)
                player.over_object = NULL;
            return;
        }
    }
}


static void
_explosion_update(ObjectState *state, ObjectTableEntry *, VECTOR *)
{
    // Explosions are simple particles: their animation is finished?
    // If so, destroy.
    if(state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION)
        state->props |= OBJ_FLAG_DESTROYED;
}


static void
_monitor_image_update(ObjectState *state, ObjectTableEntry *, VECTOR *)
{
    state->timer++;
    PoolObject *newobj;

    // Monitor images ascend for 15 frames, then stay still for 15 more
    if(state->timer <= 15) state->freepos->vy -= (ONE << 1);
    if(state->timer > 30) {
        switch(state->anim_state.animation) {
        case MONITOR_KIND_NONE:
            sound_play_vag(sfx_death, 0);
            break;
        case MONITOR_KIND_1UP:
        case 7: // !-up (Miles)
        case 8: // 1-up (Knuckles)
            sound_play_vag(sfx_event, 0);
            break;
        case MONITOR_KIND_RING:
            sound_play_vag(sfx_ring, 0);
            level_ring_count += 10;
            break;
        case MONITOR_KIND_SHIELD:
            if(player.shield != 1) {
                player.shield = 1;
                newobj = object_pool_create(OBJ_SHIELD);
                newobj->freepos.vx = player.pos.vx;
                newobj->freepos.vy = player.pos.vy + (20 << 12);
            }
            sound_play_vag(sfx_shield, 0);
            break;
        case MONITOR_KIND_SPEEDSHOES:
            // Start speed shoes count
            player.speedshoes_frames = 1200; // 20 seconds
            player.cnst = getconstants(player.character, PC_SPEEDSHOES);
            sound_bgm_play(BGM_SPEEDSHOES);
            break;
        default: break;
        }

        state->props |= OBJ_FLAG_DESTROYED;
    }
}


static void
_shield_update(ObjectState *state, ObjectTableEntry *, VECTOR *)
{
    // Just stay with the player and disappear if player gets hurt
    if(player.shield != 1)  {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    state->freepos->vx = player.pos.vx;
    state->freepos->vy = player.pos.vy + (16 << 12);

    if(player_get_current_animation_hash(&player) == ANIM_ROLLING) {
        state->freepos->vy += 4 << 12;
    }

    // Compensate position since it is drawn before player update
    state->freepos->vx += player.vel.vx;
    state->freepos->vy += player.vel.vy;
}


static void
_switch_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    // Switches are always solid at same size and change animation
    // while being upon.
    int32_t solidity_vx = pos->vx - 16;
    int32_t solidity_vy = pos->vy - 8;

    // Set as not pressed by default
    state->anim_state.animation = 0;

    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                           solidity_vx, solidity_vy, 32, 8))
    {
        // Check for intersection on left/right
        if((player_vy + 36) > solidity_vy
            && !((player_vx >= solidity_vx - 8) && ((player_vx + 8) <= solidity_vx + 32))) {

            if((player_vx + 8) < pos->vx) {
                player.ev_right.collided = 1;
                player.ev_right.coord = (solidity_vx + 2);
                player.ev_right.angle = 0;
            } else {
                player.ev_left.collided = 1;
                player.ev_left.coord = solidity_vx + 16;
                player.ev_right.angle = 0;
            }
        }
        // Landing on top; pressing
        else if(((player_vy + player_height) < solidity_vy + 16))
        {
            player.ev_grnd1.collided = player.ev_grnd2.collided = 1;
            player.ev_grnd1.angle = player.ev_grnd2.angle = 0;
            player.ev_grnd1.coord = player.ev_grnd2.coord = solidity_vy;
            state->anim_state.animation = 1;
            if(!(state->props & OBJ_FLAG_SWITCH_PRESSED)) {
                // Switch was just pressed; play "beep"
                sound_play_vag(sfx_switch, 0);
            }

            state->props |= OBJ_FLAG_SWITCH_PRESSED;
        }
    } else {
        // If button is pressed... un-press it
        state->props &= ~OBJ_FLAG_SWITCH_PRESSED;
    }
}


static void
_bubble_patch_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    // Fixed bubble patch sets
    static const uint8_t bubble_size_sets[4][6] = {
        {0, 0, 0, 0, 1, 0},
        {0, 0, 0, 1, 0, 0},
        {1, 0, 1, 0, 0, 0},
        {0, 1, 0, 0, 1, 0},
    };
    
    BubblePatchExtra *extra = state->extra;

    if(extra->timer > 0) extra->timer--;

    if(extra->timer == 0) {
        // Flip production state between idle/producing.
        // Since num_bubbles is always 0 when idle, we use this as shortcut
        if(extra->num_bubbles == 0) {
            extra->state = (extra->state + 1) % 2;
            if(extra->state == 0) {
                // Flipped from producing to idle. Pick a random delay [128; 255]
                extra->timer = 128 + (rand() % 128);
                extra->cycle = (extra->cycle + 1) % (extra->frequency + 1);
                extra->produced_big = 0;
            } else {
                // Flipped from idle to producing
                extra->num_bubbles = 1 + (rand() % 6); // Random [1; 6] bubbles
                extra->bubble_set = rand() % 4; // Random bubble set [0; 4]
                extra->bubble_idx = 0;
                extra->timer = rand() % 32; // Random [0; 31] frames
            }
        } else {
            // Produce a bubble according to constants
            PoolObject *bubble = object_pool_create(OBJ_BUBBLE);
            if(bubble) {
                bubble->state.anim_state.animation =
                    bubble_size_sets[extra->bubble_set][extra->bubble_idx++];
                bubble->freepos.vx = (pos->vx << 12)
                    - (8 << 12)
                    + ((rand() % 16) << 12);
                bubble->freepos.vy = (pos->vy << 12);

                // If this is a big bubble cycle, however, we may want to turn
                // the current bubble into a big one
                if((extra->cycle == extra->frequency) && !extra->produced_big) {
                    // Roll a dice with 1/4 of chance, but if this is a big
                    // bubble cycle and we're at the last bubble, make it big
                    // regardless!
                    if((extra->num_bubbles == 1) || !(rand() % 4)) {
                        extra->produced_big = 1;
                        bubble->state.anim_state.animation = 2;
                    }
                }
            }
            
            extra->timer = rand() % 32; // Random [0; 31] frames
            extra->num_bubbles--;
        }
    }

    
    
}

static void
_bubble_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    // NOTE: this object can only exist as a free object. Do not insist.
    if(state->freepos == NULL) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    // FIRST OFF: If way too far from camera, destroy it
    if((state->freepos->vx < camera.pos.vx - (SCREEN_XRES << 13))
       || (state->freepos->vx > camera.pos.vx + (SCREEN_XRES << 13))
       || (state->freepos->vy < camera.pos.vy - (SCREEN_YRES << 13))
       || (state->freepos->vy > camera.pos.vy + (SCREEN_YRES << 13))
       || (state->timer >= 256)) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    // INITIALIZATION: if the bubble has no X speed, initialize it and
    // set the timer
    if(state->freepos->spdx == 0) {
        // 8 pixels every 128 frames
        // 0.0625 pixel per frame
        state->freepos->spdx = 0x100;
        state->timer = 128;

        // Start with a 50% chance random direction
        state->timer *= ((rand() % 2) * 2) - 1;

        // If this is a number bubble, we need to initialize its position
        // relative to screen center, because it will hang around at the same
        // X and Y position on screen.
        if(state->anim_state.animation >= 3) {
            state->freepos->rx = state->freepos->vx - camera.pos.vx;
            state->freepos->ry = state->freepos->vy - camera.pos.vy;
        }
    }

    // A bubble can be of three diameters: small (8), medium (12) or big (32).
    // Animation dictates object diameter.
    int32_t diameter = 0;
    switch(state->anim_state.animation) {
    default:
    case 0: diameter = 8;  break; // small (breath)
    case 1: diameter = 12; break; // medium
    case 2: diameter = 32; break; // big
    }

    // The following movement logic should only work on common bubbles, and
    // on number bubbles before the actual number frames show up
    if(!((state->anim_state.animation >= 3) && (state->anim_state.frame >= 5))) {
        // Bubbles should always be ascending with a 0.5 speed (-0x800)
        if(state->anim_state.animation < 3)
            state->freepos->vy -= 0x800;
        else state->freepos->ry -= 0x800;

        // Bubbles also sway back-and-forth in a sine-like movement.
        // x = initial_x + 8 * sin(timer / 128.0)
        state->timer--;
        if(state->timer == 0) {
            state->freepos->spdx *= -1;
            state->timer = 128;
        }
        if(state->anim_state.animation < 3)
            state->freepos->vx += state->freepos->spdx;
        else state->freepos->rx += state->freepos->spdx;
    }

    // Again: if this is a stick-around bubble (number bubble), our free position
    // should be relative to camera center
    if(state->anim_state.animation >= 3) {
        state->freepos->vx = state->freepos->rx + camera.pos.vx;
        state->freepos->vy = state->freepos->ry + camera.pos.vy;
    }

    // When a normal bubble's top interact with water surface, destroy it.
    // When a number bubble finishes its animation, destroy it.
    if(((state->freepos->vy - (diameter << 12) <= level_water_y)
         && (state->anim_state.animation < 3))
       || (state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION)) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    
    // Bubbles can also only be interacted when big and on last animation frame.
    // There is no destruction animation because of VRAM constraints; we still
    // technically have a whole area available to add it, but I felt like I
    // shouldn't create a whole new texture this time just because of a single
    // bubble frame.
    if((state->anim_state.animation == 2) && (state->anim_state.frame == 5)) {
        // Bubble has an active trigger area of 32x16 at its bottom so we
        // always overlap Sonic's mouth.
        if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                           pos->vx - 16, pos->vy - 16, 32, 16)) {
            state->props |= OBJ_FLAG_DESTROYED;
            player.remaining_air_frames = 1800;
            // TODO: Cancel any drowning music.
            player_set_action(&player, ACTION_GASP);
            player_set_animation_direct(&player, ANIM_GASP);
            player.ctrllock = player.grnd ? 15 : 10;
            player.grnd = 0;
            player.vel.vx = player.vel.vy = player.vel.vz = 0;
            sound_play_vag(sfx_bubble, 0);
            return;
        }
    }
}

static void
_end_capsule_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    RECT solidity = {
        .x = pos->vx - 32,
        .y = pos->vy - 60,
        .w = 64,
        .h = 60,
    };

    solid_object_player_interaction(state, &solidity);
}
