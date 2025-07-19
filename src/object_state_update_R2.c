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

extern SoundEffect sfx_pop;

extern uint32_t   level_score_count;
extern uint8_t    level_round;
extern uint8_t    level_act;


// Object type enums
#define OBJ_MOTOBUG    (MIN_LEVEL_OBJ_GID + 0)
#define OBJ_BUZZBOMBER (MIN_LEVEL_OBJ_GID + 1)
#define OBJ_PROJECTILE (MIN_LEVEL_OBJ_GID + 2)


#define BUZZBOMBER_PATROL_RADIUS   (192 << 12)
#define BUZZBOMBER_AIMING_FRAMES   60
#define BUZZBOMBER_SHOOT_COOLDOWN  180
#define BUZZBOMBER_SHOOT_RADIUS    (80 << 12)
#define BUZZBOMBER_FLIGHT_SPEED    (4 << 12)
#define BUZZBOMBER_PROJECTILE_SPD  (2 << 12)

// Update functions
static void _motobug_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _buzzbomber_ghz_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _projectile_ghz_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R2(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_MOTOBUG: _motobug_update(state, typedata, pos); break;
    case OBJ_BUZZBOMBER: _buzzbomber_ghz_update(state, typedata, pos); break;
    case OBJ_PROJECTILE: _projectile_ghz_update(state, typedata, pos); break;
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

    // Common behaviour
    {
        PoolObject *self = NULL;
        switch(enemy_spawner_update(state, pos)) {
        default: return; // ???????
        case OBJECT_SPAWNER_CREATE_FREE:
            self = object_pool_create(OBJ_MOTOBUG);

            // Save a reference to the new motobug on this spawner.
            state->parent = &self->state;

            // Save a reference to this spawner on new object. The "moving"
            // motobug, when de-spawned, recreates this spawner since the
            // Motobug hasn't been destroyed yet
            self->state.parent = state;

            // Initialize new motobug
            self->state.flipmask = state->flipmask;
            self->freepos.vx = pos->vx << 12;
            self->freepos.vy = pos->vy << 12;
            self->state.anim_state.animation = 0;
            self->state.timer = 0;
            self->state.freepos->spdx = ONE * ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);

            // Finally, deactivate spawner
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

    // Collision
    // Motobug walks on ground, so we make a linecast downwards to check
    // where the ground is.
    CollisionEvent grn = linecast(pos->vx, pos->vy - 16, CDIR_FLOOR, 16, CDIR_FLOOR);

    // Do ground collision and apply gravity if necessary
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

    // Animation
    state->anim_state.animation = (state->freepos->spdx == 0) ? 0 : 1;

    // Transform position
    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;

    // Hitbox and interaction with player
    RECT hitbox = {
        .x = pos->vx - 20,
        .y = pos->vy - 30,
        .w = 40,
        .h = 30
    };
    if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN)
        return;
}

static void
_buzzbomber_ghz_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);

    if(typedata->has_fragment && state->freepos == NULL)
        state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;

    // Spawner behaviour
    {
        PoolObject *self = NULL;
        switch(enemy_spawner_update(state, pos)) {
        default: return; // ???????
        case OBJECT_SPAWNER_CREATE_FREE:
            self = object_pool_create(OBJ_BUZZBOMBER);

            state->parent = &self->state;
            self->state.parent = state;

            self->state.flipmask = state->flipmask;
            self->freepos.vx = pos->vx << 12;
            self->freepos.vy = pos->vy << 12;
            self->freepos.rx = pos->vx << 12;
            self->freepos.ry = pos->vy << 12;
            self->state.timer = 0;
            self->state.timer2 = 0;

            state->props |= OBJ_FLAG_DESTROYED;
            self->state.freepos->spdx = BUZZBOMBER_FLIGHT_SPEED * ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
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

    // Patrol movement.
    // Move left and right at constant speed. Distance is calculated from
    // relative position, which is treated as anchor position (rx and ry).
    if(state->timer == 0) {
        if(((sign < 0) && (state->freepos->vx < state->freepos->rx - BUZZBOMBER_PATROL_RADIUS))
           || ((sign > 0) && (state->freepos->vx > state->freepos->rx + BUZZBOMBER_PATROL_RADIUS))) {
            state->flipmask ^= MASK_FLIP_FLIPX;
            sign *= -1;
        }
        state->freepos->spdx = BUZZBOMBER_FLIGHT_SPEED * sign;

        state->freepos->vx += state->freepos->spdx;
        state->freepos->vy += state->freepos->spdy;
    }

    // Shooting at the player.
    // Shooting is always diagonal, and we have two timers.
    // "state->timer" is a cooldown where the buzzbomber will shoot at the end.
    // "state->timer2" is a cooldown for shooting in general, so we don't have
    // too many particles on screen.
    if(state->timer > 0) state->timer--;

    if(state->timer2 > 0) {
        state->timer2--;
    } else if(state->timer == 0) {
        // Check for player and aim
        int32_t shoot_min_x, shoot_max_x;
        if(sign < 0) {
            shoot_min_x = state->freepos->vx - BUZZBOMBER_SHOOT_RADIUS;
            shoot_max_x = state->freepos->vx;
        } else {
            shoot_min_x = state->freepos->vx;
            shoot_max_x = state->freepos->vx + BUZZBOMBER_SHOOT_RADIUS;
        }

        if(player.pos.vx >= shoot_min_x && player.pos.vx <= shoot_max_x) {
            state->timer = BUZZBOMBER_AIMING_FRAMES;
        }
    }

    // If finishing aiming, shoot.
    if(state->timer == 20) {
        PoolObject *projectile = object_pool_create(OBJ_PROJECTILE);
        projectile->freepos.vx = (sign > 0)
            ? state->freepos->vx + (28 << 12)
            : state->freepos->vx - (28 << 12);
        projectile->freepos.vy = state->freepos->vy + (18 << 12);
        projectile->state.flipmask = state->flipmask;
    } else if(state->timer == 3) {
        state->timer2 = BUZZBOMBER_SHOOT_COOLDOWN;
    }
    

    // Hitbox is 48x16 when flying
    // But is 40x32 when attacking
    RECT hitbox = {
        .x = pos->vx - 24,
        .y = pos->vy - 32,
        .w = 48,
        .h = 16,
    };
    if(state->anim_state.animation == 1) {
        hitbox.h = 24;
    }
    if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN) {
        return;
    }

    // Animation
    if(state->timer == 0 || state->timer > 35) {
        state->anim_state.animation = 0;
    } else state->anim_state.animation = 1;
}

static void
_projectile_ghz_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // A generic level projectile.
    // This object should always be a free object and it also expects
    // its speed to be well-defined.

    if((state->freepos == NULL) || object_should_despawn(state)) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    // Edge case: Buzzbomber charge
    if(state->anim_state.animation == 0) {
        if(state->anim_state.frame == 3) {
            // Set speed, etc
            int32_t sign = ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
            state->freepos->spdx = (2 * sign) << 12;
            state->freepos->spdy = 2 << 12;
            state->anim_state.animation = 1;
            state->anim_state.frame = 0;
        } else return;
    }

    // Setup player interaction hitbox.
    // Default size is 16x16, and then handle edge cases.
    RECT hitbox = {
        .x = pos->vx - 8,
        .y = pos->vy - 16,
        .w = 16,
        .h = 16,
    };

    if(state->anim_state.animation == 1) {
        int32_t sign = ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
        if(sign > 0) hitbox.x = pos->vx;
        hitbox.y = pos->vy - 8;
        hitbox.w = hitbox.h = 8;
    }

    hazard_player_interaction(&hitbox, pos);

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;
}
