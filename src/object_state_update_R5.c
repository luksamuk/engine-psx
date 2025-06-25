#include "object.h"
#include "object_state.h"
#include "render.h"
#include "collision.h"
#include "player.h"
#include "camera.h"
#include <stdlib.h>

// Object type enums
#define OBJ_BUBBLERSMOTHER (MIN_LEVEL_OBJ_GID + 0)
#define OBJ_BUBBLER        (MIN_LEVEL_OBJ_GID + 1)

// Object constants
#define BUBBLERSMOTHER_PATROL_RADIUS    (128 << 12)
#define BUBBLERSMOTHER_SWIM_SPEED             0x800
#define BUBBLERSMOTHER_WOBBLE_SPEED         0x00040
#define BUBBLERSMOTHER_WOBBLE_RADIUS              8
#define BUBBLERSMOTHER_FIRST_BUBBLER_INTERVAL    30
#define BUBBLERSMOTHER_BUBBLER_INTERVAL          90

// Update functions
static void _bubblersmother_update(ObjectState *, ObjectTableEntry *, VECTOR *);
//static void _bubbler_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R5(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_BUBBLERSMOTHER: _bubblersmother_update(state, typedata, pos); break;
    }
}

static void
_bubblersmother_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    // Bubblers mother does a patrol behaviour and drops bubblers from time to
    // time. It borrows some logic from the buzzbomber (R3) but is much much
    // simpler.

    if(typedata->has_fragment && state->freepos == NULL)
        state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;

    // Spawner behaviour
    {
        PoolObject *self = NULL;
        switch(enemy_spawner_update(state, pos)) {
        default: return; // ???????
        case OBJECT_SPAWNER_CREATE_FREE:
            self = object_pool_create(OBJ_BUBBLERSMOTHER);

            state->parent = &self->state;
            self->state.parent = state;

            self->state.flipmask = state->flipmask;
            self->freepos.vx = pos->vx << 12;
            self->freepos.vy = pos->vy << 12;
            self->freepos.rx = pos->vx << 12;
            self->freepos.ry = pos->vy << 12;
            self->state.timer = BUBBLERSMOTHER_FIRST_BUBBLER_INTERVAL;
            self->state.timer2 = 0;

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

    int32_t sign = ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);

    // Patrol behaviour. No delay when turning
    if(((sign < 0) && (state->freepos->vx < state->freepos->rx - BUBBLERSMOTHER_PATROL_RADIUS))
       || ((sign > 0) && (state->freepos->vx > state->freepos->rx + BUBBLERSMOTHER_PATROL_RADIUS))) {
        state->flipmask ^= MASK_FLIP_FLIPX;
        sign *= -1;
    }
    state->freepos->spdx = BUBBLERSMOTHER_SWIM_SPEED * sign;

    // Drop a bubbler every now and then
    if(state->timer > 0) state->timer--;
    else {
        state->timer = BUBBLERSMOTHER_BUBBLER_INTERVAL;
        // TODO: Drop a bubbler
    }

    // Wobble using a senoid
    state->timer2 += BUBBLERSMOTHER_WOBBLE_SPEED;
    int32_t wobble = rsin(state->timer2 >> 1) * BUBBLERSMOTHER_WOBBLE_RADIUS;

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy = state->freepos->ry + wobble;
}
