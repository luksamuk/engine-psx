#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "object_state.h"

#include "player.h"
#include "collision.h"
#include "sound.h"

// Extern elements
extern Player player;
extern SoundEffect sfx_ring;
extern SoundEffect sfx_pop;

// Update functions
static void _ring_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _goal_sign_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _monitor_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);

// Player hitbox information. Calculated once per frame.
static int32_t player_vx, player_vy; // Top left corner of player hitbox
static int32_t player_width = 16;
static int32_t player_height = HEIGHT_RADIUS_NORMAL << 1;
static uint8_t player_attacking;

void
object_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    if(state->props & OBJ_FLAG_DESTROYED) return;

    // Calculate top left corner of player AABB.
    // Note that player data is in fixed-point format!
    player_vx = (player.pos.vx >> 12) - 8;
    player_vy = (player.pos.vy >> 12) - HEIGHT_RADIUS_NORMAL;

    player_attacking = (player.action == ACTION_JUMPING ||
                        player.action == ACTION_ROLLING ||
                        player.action == ACTION_SPINDASH ||
                        player.action == ACTION_DROPDASH);

    switch(state->id) {
    default: break;
    case OBJ_RING:      _ring_update(state, typedata, pos);               break;
    case OBJ_GOAL_SIGN: _goal_sign_update(state, typedata, pos);          break;
    case OBJ_MONITOR:   _monitor_update(state, typedata, pos);            break;
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
    if(state->anim_state.animation == 0 && (pos->vx <= (player.pos.vx >> 12))) {
        state->anim_state.animation = 1;
        state->anim_state.frame = 0;
    }
}

static void
_monitor_update(ObjectState *state, ObjectTableEntry *, VECTOR *pos)
{
    if(state->anim_state.animation == 0) {
        // Calculate solidity
        int32_t solidity_vx = pos->vx - 16;
        int32_t solidity_vy = pos->vy - 32; // Monitor is a 32x32 solid box
        
        // Perform collision detection
        if(aabb_intersects(solidity_vx, solidity_vy, 32, 32,
                           player_vx, player_vy, player_width, player_height))
        {
            if(player_attacking) {
                if((player.grnd && (abs(player.vel.vx) != 0)) ||
                   (!player.grnd &&
                    (((player_vy + player_height < pos->vy) && player.vel.vy > 0)
                     || (player_vy + (player_height >> 1) > pos->vy)))) {
                    state->anim_state.animation = 1;
                    state->anim_state.frame = 0;
                    state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;
                    sound_play_vag(sfx_pop, 0);

                    if(!player.grnd && player.vel.vy > 0) {
                        player.vel.vy *= -1;
                    }
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
