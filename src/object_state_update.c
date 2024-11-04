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

// Extern elements
extern Player player;
extern Camera camera;
extern SoundEffect sfx_ring;
extern SoundEffect sfx_pop;
extern SoundEffect sfx_sprn;
extern SoundEffect sfx_chek;
extern SoundEffect sfx_death;
extern int debug_mode;

extern uint8_t  level_ring_count;
extern uint32_t level_score_count;
extern uint8_t  level_finished;

// Update functions
static void _ring_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _goal_sign_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _monitor_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _spring_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos, uint8_t is_red);
static void _spring_diagonal_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos, uint8_t is_red);
static void _checkpoint_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos);
static void _spikes_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos);

// Player hitbox information. Calculated once per frame.
static int32_t player_vx, player_vy; // Top left corner of player hitbox
static int32_t player_width = 16;
static int32_t player_height = HEIGHT_RADIUS_NORMAL << 1;
static uint8_t player_attacking;

int player_hitbox_shown;

void
_draw_player_hitbox()
{
    if(player_hitbox_shown) return;
    player_hitbox_shown = 1;
    uint16_t
        rel_vx = player_vx - (camera.pos.vx >> 12) + CENTERX,
        rel_vy = player_vy - (camera.pos.vy >> 12) + CENTERY;
    POLY_F4 *hitbox = get_next_prim();
    increment_prim(sizeof(POLY_F4));
    setPolyF4(hitbox);
    setSemiTrans(hitbox, 1);
    setXYWH(hitbox, rel_vx, rel_vy, 16, player_height);
    setRGB0(hitbox, 0xfb, 0x94, 0xdc);
    sort_prim(hitbox, OTZ_LAYER_OBJECTS);
}

void
object_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    if(state->props & OBJ_FLAG_DESTROYED) return;

    // Calculate top left corner of player AABB.
    // Note that player data is in fixed-point format!
    player_vx = (player.pos.vx >> 12) - 8;
    player_attacking = (player.action == ACTION_JUMPING ||
                        player.action == ACTION_ROLLING ||
                        player.action == ACTION_SPINDASH ||
                        player.action == ACTION_DROPDASH);
    player_height = (player_attacking
                     ? HEIGHT_RADIUS_ROLLING
                     : HEIGHT_RADIUS_NORMAL) << 1;
    player_vy = (player.pos.vy >> 12) - (player_height >> 1) - 1;

    if(debug_mode > 1) {
        _draw_player_hitbox();
    }

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

        if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                           pos->vx, pos->vy, 16, 16))
        {
            state->anim_state.animation = 1;
            state->anim_state.frame = 0;
            state->props ^= OBJ_FLAG_ANIM_LOCK; // Unlock from global timer
            level_ring_count++;
            sound_play_vag(sfx_ring, 0);
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
    if(state->anim_state.animation == 0) {
        if(pos->vx <= (player.pos.vx >> 12)) {
            state->anim_state.animation = 1;
            state->anim_state.frame = 0;
            camera_set_left_bound(&camera, ((pos->vx + 80) << 12));
            state->timer = 180;
            level_finished = 1;
            pause_elapsed_frames();
        }
    } else if(state->anim_state.animation == 1) {
        state->timer--;
        if(state->timer < 0) {
            // Set animation according to character
            state->anim_state.animation = 2;
            state->timer = 360; // 6-seconds music
            sound_play_xa("\\BGM\\EVENT001.XA;1", 0, 0, 0);
            _goal_sign_change_score();
        }
    } else if(state->anim_state.animation < OBJ_ANIMATION_NO_ANIMATION) {
        state->timer--;
        if((state->timer < 0) && (screen_level_getstate() == 1)) {
            screen_level_setstate(2);
        } else if(screen_level_getstate() == 3) {
            uint8_t lvl = screen_level_getlevel();
            // TODO: This is temporary and goes only upto R2Z1
            if(lvl < 6) {
                if(lvl == 4) {
                    // TODO: Transition from GHZ1 to SWZ1
                    // THIS IS TEMPORARY
                    screen_level_setlevel(6);
                } else screen_level_setlevel(lvl + 1);
                scene_change(SCREEN_LEVEL);
            } else {
                scene_change(SCREEN_COMINGSOON);
            }
        }
    }
}

static void
_monitor_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
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

                switch(((MonitorExtra *)state->extra)->kind) {
                case MONITOR_KIND_RING:
                    level_ring_count += 10;
                    sound_play_vag(sfx_ring, 0);
                    break;
                default: break;
                }

                if(!player.grnd && player.vel.vy > 0) {
                    player.vel.vy *= -1;
                }
            } else {
                // Landing on top
                if(((player_vy + 8) - solidity_vy < 16) &&
                   ((player_vx >= solidity_vx - 4) && ((player_vx + 8) <= solidity_vx + 32 - 4)))
                {
                    player.ev_grnd1.collided = player.ev_grnd2.collided = 1;
                    player.ev_grnd1.angle = player.ev_grnd2.angle = 0;
                    player.ev_grnd1.coord = player.ev_grnd2.coord = solidity_vy;
                } else {
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
        int32_t solidity_vx = pos->vx - 16;
        int32_t solidity_vy = pos->vy - 16; // Spring is 32x16 solid
        int32_t solidity_w  = 32;
        int32_t solidity_h  = 16;
        
        if(state->flipmask & MASK_FLIP_ROTCW) {
            solidity_vx = pos->vx - 32;
            solidity_vy = pos->vy + 16;
            solidity_w  = 16;
            solidity_h  = 32;
        } else if(state->flipmask & MASK_FLIP_ROTCT) {
            solidity_vx = pos->vx - 48;
            solidity_vy = pos->vy - 48;
            solidity_w  = 16;
            solidity_h  = 32;
        } else if(state->flipmask & MASK_FLIP_FLIPY) {
            solidity_vy -= 48;
        }

        ObjectCollision collision_side =
            hitbox_collision(player_vx, player_vy, player_width, player_height,
                             solidity_vx, solidity_vy, solidity_w, solidity_h);

        // Simple spring collision.
        // In this case, springs are not solid, and the player's spring action
        // relate to where the spring is pointing at.
        if(collision_side == OBJ_SIDE_NONE)
            return;
        else if(state->flipmask & MASK_FLIP_ROTCT) { // Left-pointing spring
            player.pos.vx = (solidity_vx - player_width) << 12;
            player.ev_right.collided = 0; // Detach player from right wall if needed
            if(player.grnd) player.vel.vz = is_red ? -0x10000 : -0xa000;
            else player.vel.vx = is_red ? -0x10000 : -0xa000;
            player.ctrllock = 16;
            player.anim_dir = -1;
            state->anim_state.animation = 1;
            sound_play_vag(sfx_sprn, 0);
        } else if(state->flipmask & MASK_FLIP_ROTCW) { // Right-pointing spring
            player.pos.vx = (solidity_vx + solidity_w + player_width + 8) << 12;
            player.ev_left.collided = 0; // Detach player from left wall if needed
            if(player.grnd) player.vel.vz = is_red ? 0x10000 : 0xa000;
            else player.vel.vx = is_red ? 0x10000 : 0xa000;
            player.ctrllock = 16;
            player.anim_dir = 1;
            state->anim_state.animation = 1;
            sound_play_vag(sfx_sprn, 0);
        } else if(state->flipmask == 0) { // Top-pointing spring
            player.pos.vy = (solidity_vy - (player_height >> 1)) << 12;
            player.grnd = 0;
            player.vel.vy = is_red ? -0x10000 : -0xa000;
            player.angle = 0;
            player.action = ACTION_SPRING;
            state->anim_state.animation = 1;
            sound_play_vag(sfx_sprn, 0);
        } else if(state->flipmask & MASK_FLIP_FLIPY) { // Bottom-pointing spring
            player.pos.vy = (solidity_vy + solidity_h + (player_height >> 1)) << 12;
            player.grnd = 0;
            player.vel.vy = is_red ? 0x10000 : 0xa000;
            player.angle = 0;
            player.action = ACTION_SPRING;
            state->anim_state.animation = 1;
            sound_play_vag(sfx_sprn, 0);
        }
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
        int32_t solidity_vx = pos->vx - 16;
        int32_t solidity_vy = pos->vy - 32; // Spring is 32x32 solid
        int32_t solidity_w  = 32;
        int32_t solidity_h  = 32;

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
            delta = solidity_w - (player_vx - solidity_vx);
        } else {
            delta = player_vx - solidity_vx + 16;
        }

        if(delta > 10 && delta < 33) {
            shrink = delta - 10;
        } else shrink = 22;

        solidity_h -= shrink;
        
        if(state->flipmask & MASK_FLIP_FLIPY) {
            solidity_vy -= 32;
        } else solidity_vy += shrink;

        ObjectCollision collision_side =
            hitbox_collision(player_vx, player_vy, player_width, player_height,
                             solidity_vx, solidity_vy, solidity_w, solidity_h);

        if(collision_side == OBJ_SIDE_NONE) return;
        
        player.grnd = 0;
        player.vel.vx = is_red ? SPRND_ST_R : SPRND_ST_Y;
        player.vel.vy = is_red ? SPRND_ST_R : SPRND_ST_Y;
        if(!(state->flipmask & MASK_FLIP_FLIPY)) player.vel.vy *= -1;
        if(state->flipmask & MASK_FLIP_FLIPX) {
            player.vel.vx *= -1;
            player.anim_dir = -1; // Flip on X: point player left
        } else player.anim_dir = 1; // No flip on X: point player right
        player.angle = 0;
        player.airdirlock = 1;
        player.action = ACTION_SPRING;
        state->anim_state.animation = 1;
        sound_play_vag(sfx_sprn, 0);
        
    } else if(state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION) {
        state->anim_state.animation = 0;
        state->anim_state.frame = 0;
    }
    
}


static void
_spikes_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    // TODO: For now, only spikes pointing upwards check for collision
    if(state->flipmask != 0) return;

    // Collision logic is very similar to monitors
    // Calculate solidity
    int32_t solidity_vx = pos->vx - 16;
    int32_t solidity_vy = pos->vy - 32; // Spikes are a 32x32 solid box
        
    // Perform collision detection
    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                       solidity_vx, solidity_vy, 32, 32))
    {
       
        // Landing on top
        if(((player_vy + 8) - solidity_vy < 16) &&
           ((player_vx >= solidity_vx - 4) && ((player_vx + 8) <= solidity_vx + 32 - 4)))
        {
            if(player.action != ACTION_HURT && player.iframes == 0) {
                player_set_hurt(&player, (solidity_vx + 16) << 12);
                sound_play_vag(sfx_death, 0); // TODO: SFX changes depending on situation
                return;
            }

            player.ev_grnd1.collided = player.ev_grnd2.collided = 1;
            player.ev_grnd1.angle = player.ev_grnd2.angle = 0;
            player.ev_grnd1.coord = player.ev_grnd2.coord = solidity_vy;
        } else {
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
