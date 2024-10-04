#include <stdio.h>

#include "object.h"
#include "object_state.h"

#include "player.h"
#include "collision.h"
#include "sound.h"

// Extern elements
extern Player player;
extern SoundEffect sfx_ring;

// Update functions
static void _ring_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);
static void _goal_sign_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);

void
object_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    if(state->props & OBJ_FLAG_DESTROYED) return;

    switch(state->id) {
    default: break;
    case OBJ_RING: _ring_update(state, typedata, pos);                    break;
    case OBJ_GOAL_SIGN: _goal_sign_update(state, typedata, pos);          break;
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

        // Calculate top left corner of player AABB.
        // Note that player data is in fixed-point format!
        int32_t player_vx = (player.pos.vx >> 12) - 8;
        int32_t player_vy = (player.pos.vy >> 12) - HEIGHT_RADIUS_NORMAL;

        if(aabb_intersects(pos->vx, pos->vy, 16, 16,
                           player_vx, player_vy, 16, HEIGHT_RADIUS_NORMAL << 1))
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
