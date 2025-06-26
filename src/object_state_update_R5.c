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
#define OBJ_GATOR          (MIN_LEVEL_OBJ_GID + 2)

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

#define GATOR_PROXIMITY_RANGE_W   88
#define GATOR_PROXIMITY_RANGE_H   64


// Update functions
static void _bubblersmother_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _bubbler_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _gator_update(ObjectState *, ObjectTableEntry *, VECTOR *);

// Extern variables
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;


void
object_update_R5(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_BUBBLERSMOTHER: _bubblersmother_update(state, typedata, pos); break;
    case OBJ_BUBBLER:        _bubbler_update(state, typedata, pos);        break;
    case OBJ_GATOR:          _gator_update(state, typedata, pos);          break;
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

static void
_bubbler_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
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

static void
_gator_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // The Gator is a glorified motobug (See Green Hill, R2).
    // Its main difference from the motobug is that, when it is close to
    // the player, it opens its mouth, which creates a wide hurtbox, forcing
    // the player to work around it and hit it in its body.

    // Hide fragment on spawner
    if(typedata->has_fragment && state->freepos == NULL)
        state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;

    // Spawn, etc
    {
        PoolObject *self = NULL;
        switch(enemy_spawner_update(state, pos)) {
        default: return; // ???????
        case OBJECT_SPAWNER_CREATE_FREE:
            self = object_pool_create(OBJ_GATOR);
            state->parent = &self->state;
            self->state.parent = state;

            self->state.flipmask = state->flipmask;
            self->freepos.vx = pos->vx << 12;
            self->freepos.vy = pos->vy << 12;
            self->state.anim_state.animation = 0;
            self->state.timer = 0;
            self->state.freepos->spdx = ONE * ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
            state->props |= OBJ_FLAG_DESTROYED;
            return; // Nothing more to do
        case OBJECT_SPAWNER_ABORT_BEHAVIOUR:
            return; // Nothing to do
        case OBJECT_DESPAWN:
            // Reactivate parent
            if(state->parent) {
                state->parent->props &= ~OBJ_FLAG_DESTROYED;
                // Remove reference to this object
                state->parent->parent = NULL;
            }
            state->props |= OBJ_FLAG_DESTROYED;
            return; // Nothing more to do
        case OBJECT_UPDATE_AS_FREE: break; // Just go ahead
        }
    }

    // Motobug walk movement
    CollisionEvent grn = linecast(pos->vx, pos->vy - 16, CDIR_FLOOR, 16, CDIR_FLOOR);
    if(!grn.collided) {
        state->freepos->spdy += OBJ_GRAVITY;
    } else {
        state->freepos->spdy = 0;
        state->freepos->vy = grn.coord << 12;
        CollisionEvent llft = linecast(pos->vx - 8, pos->vy - 16, CDIR_FLOOR, 32, CDIR_FLOOR);
        CollisionEvent lrgt = linecast(pos->vx + 8, pos->vy - 16, CDIR_FLOOR, 32, CDIR_FLOOR);
        CollisionEvent wlft = linecast(pos->vx, pos->vy - 16, CDIR_LWALL, 20, CDIR_FLOOR);
        CollisionEvent wrgt = linecast(pos->vx, pos->vy - 16, CDIR_RWALL, 20, CDIR_FLOOR);
        if(state->timer > 0) state->timer--;
        if(state->freepos->spdx != 0) {
            if(((!llft.collided || wlft.collided) && (state->freepos->spdx < 0))
               || ((!lrgt.collided || wrgt.collided) && (state->freepos->spdx > 0))) {
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

    // Animation (fragment only)
    state->frag_anim_state->animation = (state->freepos->spdx == 0) ? 0 : 1;

    int32_t sign = ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);

    // Proximity box, open jaws when close to player
    RECT proximity_box = {
        .x = pos->vx - ((sign > 0) ? 24 : GATOR_PROXIMITY_RANGE_W - 24),
        .y = pos->vy - GATOR_PROXIMITY_RANGE_H + 8,
        .w = GATOR_PROXIMITY_RANGE_W,
        .h = GATOR_PROXIMITY_RANGE_H,
    };
    state->anim_state.animation =
        aabb_intersects(
           player_vx, player_vy, player_width, player_height,
           proximity_box.x, proximity_box.y, proximity_box.w, proximity_box.h);


    // Hitbox
    RECT hitbox = {
        .x = pos->vx - ((sign > 0) ? 28 : 12),
        .y = pos->vy - 24,
        .w = 40,
        .h = 24,
    };
    if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN) {
        return;
    }

    // When the jaws are open, add a hurtbox
    if(state->anim_state.animation == 1) {
        RECT hurtbox = {
            .x = pos->vx + ((sign > 0) ? 14 : -22),
            .y = pos->vy - 32,
            .w = 8,
            .h = 32,
        };
        hazard_player_interaction(&hurtbox, pos);
    }

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;
}
