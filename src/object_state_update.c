#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "object_state.h"

#include "player.h"
#include "collision.h"
#include "sound.h"
#include "camera.h"
#include "render.h"

#include "screen.h"
#include "screens/level.h"

// Adler32 sums of player animation names
#define ANIM_WALKING    0x0854020e

// Extern elements
extern Player player;
extern Camera camera;
extern SoundEffect sfx_ring;
extern SoundEffect sfx_pop;
extern SoundEffect sfx_sprn;
extern int debug_mode;

// Update functions
static void _ring_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _goal_sign_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _monitor_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _spring_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos, uint8_t is_red);

// Player hitbox information. Calculated once per frame.
static int32_t player_vx, player_vy; // Top left corner of player hitbox
static int32_t player_width = 16;
static int32_t player_height = HEIGHT_RADIUS_NORMAL << 1;
static uint8_t player_attacking;

void
_draw_player_hitbox()
{
    uint16_t
        rel_vx = player_vx - (camera.pos.vx >> 12) + CENTERX,
        rel_vy = player_vy - (camera.pos.vy >> 12) + CENTERY;
    POLY_F4 *hitbox = get_next_prim();
    increment_prim(sizeof(POLY_F4));
    setPolyF4(hitbox);
    setXYWH(hitbox, rel_vx, rel_vy, 16, player_height);
    setRGB0(hitbox, 0xfb, 0x94, 0xdc);
    sort_prim(hitbox, 5);
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
    player_vy = (player.pos.vy >> 12) - (player_height >> 1);

    if(debug_mode > 1) {
        _draw_player_hitbox();
    }

    switch(state->id) {
    default: break;
    case OBJ_RING:          _ring_update(state, typedata, pos);          break;
    case OBJ_GOAL_SIGN:     _goal_sign_update(state, typedata, pos);     break;
    case OBJ_MONITOR:       _monitor_update(state, typedata, pos);       break;
    case OBJ_SPRING_YELLOW: _spring_update(state, typedata, pos, 0);     break;
    case OBJ_SPRING_RED:    _spring_update(state, typedata, pos, 1);     break;
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

        if(aabb_intersects(pos->vx, pos->vy, 16, 16,
                           player_vx, player_vy, player_width, player_height))
        {
            state->anim_state.animation = 1;
            state->anim_state.frame = 0;
            state->props ^= OBJ_FLAG_ANIM_LOCK; // Unlock from global timer
            sound_play_vag(sfx_ring, 0);
        }
    } else if(state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION
              && !(state->props & OBJ_FLAG_DESTROYED))
    {
        state->props |= OBJ_FLAG_DESTROYED;
    }
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
        }
    } else if(state->anim_state.animation == 1) {
        state->timer--;
        if(state->timer < 0) {
            // Set animation according to character
            state->anim_state.animation = 2;
            state->timer = 360; // 6-seconds music
            sound_play_xa("\\BGM\\EVENT001.XA;1", 0, 0, 0);
        }
    } else if(state->anim_state.animation < OBJ_ANIMATION_NO_ANIMATION) {
        state->timer--;
        if((state->timer < 0) && (screen_level_getstate() == 1)) {
            screen_level_setstate(2);
        } else if(screen_level_getstate() == 3) {
            uint8_t lvl = screen_level_getlevel();
            // TODO: This is temporary and goes only upto R2Z1
            if(lvl >= 4) {
                scene_change(SCREEN_COMINGSOON);
            } else {
                screen_level_setlevel(lvl + 1);
                scene_change(SCREEN_LEVEL);
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

        int32_t hitbox_vx = pos->vx - 12;
        int32_t hitbox_vy = pos->vy - 32; // Monitor hitbox is a 28x32 solid box
        
        // Perform collision detection
        if(aabb_intersects(solidity_vx, solidity_vy, 32, 32,
                           player_vx, player_vy, player_width, player_height))
        {
            if(aabb_intersects(hitbox_vx, hitbox_vy, 28, 32,
                               player_vx, player_vy, player_width, player_height)
               && player_attacking) {
                state->anim_state.animation = 1;
                state->anim_state.frame = 0;
                state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;
                sound_play_vag(sfx_pop, 0);

                if(!player.grnd && player.vel.vy > 0) {
                    player.vel.vy *= -1;
                }
            } else {
                // Landing on top
                if((player_vy - solidity_vy < 16) &&
                   ((player_vx >= solidity_vx - 4) && (player_vx <= solidity_vx + 32 - 4)))
                {
                    player.ev_grnd1.collided = player.ev_grnd2.collided = 1;
                    player.ev_grnd1.angle = player.ev_grnd2.angle = 0;
                    player.ev_grnd1.coord = player.ev_grnd2.coord = solidity_vy;
                } else {
                    // Check for intersection on left/right
                    if(player_vx < pos->vx) {
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
    // Only allow simple cases (normal and Y-flipped)
    if(state->flipmask & (MASK_FLIP_ROTCW | MASK_FLIP_ROTCT)) return;

    if(state->anim_state.animation == 0) {
        int32_t solidity_vx = pos->vx - 16;
        int32_t solidity_vy = pos->vy - 16; // Spring is 32x16 solid

        if(state->flipmask & MASK_FLIP_FLIPY) {
            solidity_vy -= 48;
        }

        ObjectCollision collision_side =
            hitbox_collision(player_vx, player_vy, player_width, player_height,
                             solidity_vx, solidity_vy, 32, 16);

        switch(collision_side) {
        default: return;
        case OBJ_SIDE_LEFT:
            player.ev_right = (CollisionEvent) {
                .collided = 1,
                .coord = solidity_vx + 2,
                .angle = 0
            };
            break;
        case OBJ_SIDE_RIGHT:
            player.ev_left = (CollisionEvent) {
                .collided = 1,
                .coord = (solidity_vx + 15),
                .angle = 0
            };
            break;
        case OBJ_SIDE_TOP:
        case OBJ_SIDE_BOTTOM:
            /* player.ev_grnd1 = player.ev_grnd2 = (CollisionEvent) { */
            /*     .collided = 1, */
            /*     .coord = solidity_vy, */
            /*     .angle = 0 */
            /* }; */
            if(player.vel.vy > 0) {
                player.grnd = 0;

                if(state->flipmask & MASK_FLIP_FLIPY) {
                    player.vel.vy = is_red ? 0x10000 : 0xa000;
                } else player.vel.vy = is_red ? -0x10000 : -0xa000;

                player.angle = 0;
                player.action = 0;
                state->anim_state.animation = 1;
                player_set_animation_direct(&player, ANIM_WALKING);
                sound_play_vag(sfx_sprn, 0);
            }
            break;
        }
    } else if(state->anim_state.animation == OBJ_ANIMATION_NO_ANIMATION) {
        state->anim_state.animation = 0;
        state->anim_state.frame = 0;
    }
}
