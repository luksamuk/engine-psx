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
#define BUBBLERSMOTHER_SWIM_SPEED           0x00800
#define BUBBLERSMOTHER_WOBBLE_SPEED         0x00040
#define BUBBLERSMOTHER_WOBBLE_RADIUS              8
#define BUBBLERSMOTHER_FIRST_BUBBLER_INTERVAL    30
#define BUBBLERSMOTHER_BUBBLER_INTERVAL          90


#define BUBBLER_ANIM_SEED           0
#define BUBBLER_ANIM_BUBBLING       1
#define BUBBLER_ANIM_POISON_GAS     2
#define BUBBLER_ANIM_DISSOLVE       3

#define BUBBLER_SEED_WOBBLE_SPEED    0x00030
#define BUBBLER_SEED_WOBBLE_RADIUS         4
#define BUBBLER_SEED_FALL_SPEED      0x00800
#define BUBBLER_BUBBLING_MAX_FRAME         5
#define BUBBLER_POISONGAS_RISE_SPEED 0x00800
#define BUBBLER_POISONGAS_INTERVAL        60
#define BUBBLER_DISSOLVE_INTERVAL          7


// Update functions
static void _bubblersmother_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _bubbler_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R5(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_BUBBLERSMOTHER: _bubblersmother_update(state, typedata, pos); break;
    case OBJ_BUBBLER:        _bubbler_update(state, typedata, pos);        break;
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
        
        // Drop a bubbler
        PoolObject *bubbler = object_pool_create(OBJ_BUBBLER);
        bubbler->freepos.rx = bubbler->freepos.vx = (sign > 0)
            ? state->freepos->vx - (6 << 12)
            : state->freepos->vx + (6 << 12);
        bubbler->freepos.ry = bubbler->freepos.vy =
            state->freepos->vy - (7 << 12);
        bubbler->state.anim_state.animation = BUBBLER_ANIM_SEED;
        bubbler->state.flipmask = state->flipmask;
        bubbler->state.timer = 0;
    }

    // Wobble using a senoid
    state->timer2 += BUBBLERSMOTHER_WOBBLE_SPEED;
    int32_t wobble = rsin(state->timer2 >> 1) * BUBBLERSMOTHER_WOBBLE_RADIUS;

    // Player interaction
    RECT hitbox = {
        .x = pos->vx - 16,
        .y = pos->vy - 4 - 24,
        .w = 32,
        .h = 24,
    };
    if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN) {
        return;
    }

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy = state->freepos->ry + wobble;
}
#include <stdio.h>
static void
_bubbler_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    // Bubblers are usually not created on their own, so we'll just go ahead
    // and destroy any statically-placed bubbler
    if((state->freepos == NULL) || object_should_despawn(state)) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    if(state->anim_state.animation == BUBBLER_ANIM_SEED) {
        // Fall at constant speed until reaching the ocean bed
        state->timer += BUBBLER_SEED_WOBBLE_SPEED;
        int32_t wobble = rsin(state->timer >> 1) * BUBBLER_SEED_WOBBLE_RADIUS;
        state->freepos->vx = state->freepos->rx  + wobble;
        state->freepos->spdy = BUBBLER_SEED_FALL_SPEED;
        state->freepos->vy += state->freepos->spdy;

        CollisionEvent grn = linecast(pos->vx, pos->vy - 4, CDIR_FLOOR, 4, CDIR_FLOOR);
        if(grn.collided) {
            // Move on to bubbling phase
            state->freepos->vy = grn.coord << 12;
            state->freepos->spdy = 0;
            state->timer = 0;
            state->anim_state.animation = BUBBLER_ANIM_BUBBLING;
            state->anim_state.frame = 0;
        }
    }
    
    else if(state->anim_state.animation == BUBBLER_ANIM_BUBBLING) {
        // Bubbler can only be killed when bubbling
        RECT hitbox = {
            .x = pos->vx - 8,
            .y = pos->vy - 16,
            .w = 16,
            .h = 16,
        };
        if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN) {
            return;
        }
        
        if(state->anim_state.frame == BUBBLER_BUBBLING_MAX_FRAME) {
            state->anim_state.animation = BUBBLER_ANIM_POISON_GAS;
            state->anim_state.frame = 0;
            state->timer = BUBBLER_POISONGAS_INTERVAL;
        }
    }

    else if(state->anim_state.animation == BUBBLER_ANIM_POISON_GAS) {
        state->freepos->spdy = -BUBBLER_POISONGAS_RISE_SPEED;
        state->freepos->vy += state->freepos->spdy;

        // Hurt player, like a hazard
        RECT hitbox = {
            .x = pos->vx - 12,
            .y = pos->vy - 16,
            .w = 24,
            .h = 16,
        };
        hazard_player_interaction(&hitbox, pos);
        
        if(state->timer > 0) state->timer--;
        else {
            state->anim_state.animation = BUBBLER_ANIM_DISSOLVE;
            state->anim_state.frame = 0;
            state->timer = BUBBLER_DISSOLVE_INTERVAL;
        }
    }

    else if(state->anim_state.animation == BUBBLER_ANIM_DISSOLVE) {
        // Same as poison gas but doesn't hurt
        state->freepos->spdy = -BUBBLER_POISONGAS_RISE_SPEED;
        state->freepos->vy += state->freepos->spdy;
        if(state->timer > 0) state->timer--;
        else state->props |= OBJ_FLAG_DESTROYED;
        
    }
}
