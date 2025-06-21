#include "object.h"
#include "object_state.h"
#include "collision.h"
#include "player.h"
#include "sound.h"
#include "camera.h"
#include "render.h"
#include <stdio.h>

// Extern elements
extern Player player;
extern Camera camera;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

extern uint32_t   level_score_count;
extern uint8_t    level_round;
extern uint8_t    level_act;

// Object constants
#define OBJ_GRAVITY 0x00380

// Object type enums
#define OBJ_MOTOBUG (MIN_LEVEL_OBJ_GID + 0)

// Update functions
static void _motobug_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R2(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_MOTOBUG: _motobug_update(state, typedata, pos); break;
    }
}

static void
_motobug_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // Motobug has two stages.
    // The first stage is the Spawner stage, which only happens when Motobug
    // IS NOT a free object, so it wouldn't be able to walk around the stage.
    // Whenever the spawner is outside of the screen, it destroys itself and
    // spawns a free-walking Motobug.
    // (Notice that being out of the screen also relies on the fact that this
    // object will not be updated if it is too far away, since it will not fit
    // the static object update window)
    if(state->freepos == NULL) {
        state->anim_state.animation = OBJ_ANIMATION_NO_ANIMATION;
        // Spawn free object WHEN too close to camera,
        // but still far away from  play area itself!
        if((state->parent == NULL) && // Spawned motobug is not on screen, and...
            (
                // Within outside boundary, and...
                (pos->vx > (camera.pos.vx >> 12) - SCREEN_XRES)
                && (pos->vx < (camera.pos.vx >> 12) + SCREEN_XRES)
                && (pos->vy > (camera.pos.vy >> 12) - SCREEN_YRES)
                && (pos->vy < (camera.pos.vy >> 12) + SCREEN_YRES)
            ) && (
                // Outside center screen
                (pos->vx < (camera.pos.vx >> 12) - CENTERX)
                || (pos->vx > (camera.pos.vx >> 12) + CENTERX)
                || (pos->vy < (camera.pos.vy >> 12) - CENTERY)
                || (pos->vy > (camera.pos.vy >> 12) + CENTERY)
                )
            )
        {
            printf("Spawned free motobug\n");
            PoolObject *self = object_pool_create(OBJ_MOTOBUG);
            self->freepos.vx = pos->vx << 12;
            self->freepos.vy = pos->vy << 12;
            self->state.anim_state.animation = 0;
            self->state.flipmask = state->flipmask;
            self->state.timer = 0;
            self->state.freepos->spdx = ONE * ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
            // Save a reference to this spawner on new object. The "moving"
            // motobug, when de-spawned, recreates this spawner since the
            // Motobug hasn't been destroyed yet
            self->state.parent = state;

            // Save a reference to the new motobug on this spawner.
            // Finally, deactivate spawner
            state->parent = &self->state;
            state->props |= OBJ_FLAG_DESTROYED;
        }
        return;
    }

    // Despawn if too far from camera
    if((state->freepos->vx < camera.pos.vx - (SCREEN_XRES << 12))
       || (state->freepos->vx > camera.pos.vx + (SCREEN_XRES << 12))
       || (state->freepos->vy < camera.pos.vy - (SCREEN_YRES << 12))
       || (state->freepos->vy > camera.pos.vy + (SCREEN_YRES << 12))) {
        state->props |= OBJ_FLAG_DESTROYED;
        // Reactivate parent
        printf("Reactivated parent\n");
        if(state->parent) {
            state->parent->props &= ~OBJ_FLAG_DESTROYED;
            // Remove reference to this object
            state->parent->parent = NULL;
        }
        return;
    }

    // Collision
    // Motobug walks on ground, so we make a linecast downwards to check
    // where the ground is. We also have helpers at its left and its right.
    CollisionEvent grn = linecast(pos->vx, pos->vy - 16, CDIR_FLOOR, 16, CDIR_FLOOR);

    // Do ground collision and apply gravity if necessary
    if(!grn.collided) {
        state->freepos->spdy += OBJ_GRAVITY;
    } else {
        state->freepos->spdy = 0;
        state->freepos->vy = grn.coord << 12;

        CollisionEvent lft = linecast(pos->vx - 8, pos->vy - 16, CDIR_FLOOR, 64, CDIR_FLOOR);
        CollisionEvent rgt = linecast(pos->vx + 8, pos->vy - 16, CDIR_FLOOR, 64, CDIR_FLOOR);

        if(state->timer > 0) state->timer--;
        
        // Boundary checks.
        // If facing and walking towards a certain direction, and a ledge is detected,
        // stop, wait a few frames, then turn.
        if(state->freepos->spdx != 0) {
            if(((!lft.collided) && (state->freepos->spdx < 0))
               || ((!rgt.collided) && (state->freepos->spdx > 0))) {
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

    // Animation
    state->anim_state.animation = (state->freepos->spdx == 0) ? 0 : 1;

    // Transform position
    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;
}
