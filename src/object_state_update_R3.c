#include "object.h"
#include "object_state.h"
#include "render.h"
#include "collision.h"
#include "player.h"
#include "camera.h"
#include <stdlib.h>

// Object constants
#define STEGWAY_SIGHT_DISTANCE_X     174
#define STEGWAY_SIGHT_DISTANCE_MIN_X 150
#define STEGWAY_SIGHT_DISTANCE_Y      64

// Object type enums
#define OBJ_STEGWAY (MIN_LEVEL_OBJ_GID + 0)

// Update functions
static void _stegway_update(ObjectState *, ObjectTableEntry *, VECTOR *);

// Extern variables
extern Player player;
extern Camera camera;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

void
object_update_R3(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_STEGWAY: _stegway_update(state, typedata, pos); break;
    }
}

static void
_stegway_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);

    // Most code here is kind of copied from R2's Motobug, except the Stegway
    // charges at you when you're near and doesn't turn on ledges

    {
        PoolObject *self = NULL;
        switch(enemy_spawner_update(state, pos)) {
        default: return; // ???????
        case OBJECT_SPAWNER_CREATE_FREE:
            self = object_pool_create(OBJ_STEGWAY);

            state->parent = &self->state;
            self->state.parent = state;

            self->state.flipmask = state->flipmask;
            self->freepos.vx = pos->vx << 12;
            self->freepos.vy = pos->vy << 12;
            self->state.timer = 0;

            state->props |= OBJ_FLAG_DESTROYED;
            self->state.freepos->spdx = ONE * ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
            return;
        case OBJECT_SPAWNER_ABORT_BEHAVIOUR: return;
        case OBJECT_DESPAWN:
            if(state->parent) {
                state->parent->props &= ~OBJ_FLAG_DESTROYED;
                state->parent->parent = NULL;
            }
            state->props |= OBJ_FLAG_DESTROYED;
            return;
        case OBJECT_UPDATE_AS_FREE: break;
        }
    }

    RECT hitbox = {
        .x = pos->vx - 24,
        .y = pos->vy - 32,
        .w = 48,
        .h = 32,
    };
    if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN)
        return;

    CollisionEvent grn = linecast(pos->vx, pos->vy - 16, CDIR_FLOOR, 16, CDIR_FLOOR);
    if(!grn.collided) {
        state->freepos->spdy += OBJ_GRAVITY;
    } else {
        state->freepos->spdy = 0;
        state->freepos->vy = grn.coord << 12;

        // Check for ledges while on ground.
        CollisionEvent llft = linecast(pos->vx - 8, pos->vy - 16, CDIR_FLOOR, 32, CDIR_FLOOR);
        CollisionEvent lrgt = linecast(pos->vx + 8, pos->vy - 16, CDIR_FLOOR, 32, CDIR_FLOOR);

        // Also check for walls
        CollisionEvent wlft = linecast(pos->vx, pos->vy - 16, CDIR_LWALL, 20, CDIR_FLOOR);
        CollisionEvent wrgt = linecast(pos->vx, pos->vy - 16, CDIR_RWALL, 20, CDIR_FLOOR);

        if(state->timer > 0) state->timer--;

        // Boundary checks.
        // If facing and walking towards a certain direction, and a ledge is detected,
        // stop, wait a few frames, then turn.
        if(state->freepos->spdx != 0) {
            if(((!llft.collided || wlft.collided) && (state->freepos->spdx < 0))
               || ((!lrgt.collided || wrgt.collided) && (state->freepos->spdx > 0))) {
                // Prepare to turn
                state->freepos->spdx = 0;
                state->timer = 60;
            }
        } else {
            if(state->timer == 0) {
                state->flipmask ^= MASK_FLIP_FLIPX;
                state->freepos->spdx = ONE * ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
            }
        }
    }

    if(state->timer == 0) {
        // Stegway sight range is a little ahead of itself but it can also see
        // its back half and a little bit, so he doesn't stop accelerating right
        // under the player. This behaviour allows it to escape being hit.
        // Also limit its sight on the Y direction to a default object size
        // from its bottom up
        int32_t sight_vx = (state->flipmask & MASK_FLIP_FLIPX) ?
            pos->vx - STEGWAY_SIGHT_DISTANCE_MIN_X
            : pos->vx - (STEGWAY_SIGHT_DISTANCE_X - STEGWAY_SIGHT_DISTANCE_MIN_X);
        int32_t sight_vy = pos->vy - STEGWAY_SIGHT_DISTANCE_Y;
        int32_t sign = ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);

        if(aabb_intersects(
               player_vx, player_vy, player_width, player_height,
               sight_vx, sight_vy, STEGWAY_SIGHT_DISTANCE_X, STEGWAY_SIGHT_DISTANCE_Y))
        {
            state->freepos->spdx = (3 << 12) * sign;
        } else {
            state->freepos->spdx = ONE * sign;
        }
    }

    if(state->freepos->spdx == 0)
        state->anim_state.animation = 0;
    else if(abs(state->freepos->spdx) < (3 << 12))
        state->anim_state.animation = 1;
    else state->anim_state.animation = 2;

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;
}
