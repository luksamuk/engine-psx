#include "object.h"
#include "object_state.h"
#include "render.h"
#include "collision.h"
#include "player.h"
#include "camera.h"
#include <stdlib.h>

// Object type enums
#define OBJ_STEGWAY    (MIN_LEVEL_OBJ_GID + 0)
#define OBJ_BUZZBOMBER (MIN_LEVEL_OBJ_GID + 1)
#define OBJ_PROJECTILE (MIN_LEVEL_OBJ_GID + 2)


// Object constants
#define STEGWAY_SIGHT_DISTANCE_X      96
#define STEGWAY_SIGHT_DISTANCE_MIN_X  (STEGWAY_SIGHT_DISTANCE_X - 24)
#define STEGWAY_SIGHT_DISTANCE_Y      64
#define STEGWAY_BASE_SPEED            ONE
#define STEGWAY_RUN_SPEED             0x2800

#define BUZZBOMBER_PATROL_RADIUS   (128 << 12)
#define BUZZBOMBER_AIMING_FRAMES   60
#define BUZZBOMBER_SHOOT_COOLDOWN  180
#define BUZZBOMBER_SHOOT_RADIUS    (96 << 12)
#define BUZZBOMBER_FLIGHT_SPEED    (2 << 12)
#define BUZZBOMBER_PROJECTILE_SPD  (2 << 12)

// Update functions
static void _stegway_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _buzzbomber_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _projectile_update(ObjectState *, ObjectTableEntry *, VECTOR *);

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
    case OBJ_STEGWAY:    _stegway_update(state, typedata, pos);    break;
    case OBJ_BUZZBOMBER: _buzzbomber_update(state, typedata, pos); break;
    case OBJ_PROJECTILE: _projectile_update(state, typedata, pos); break;
    }
}

static void
_stegway_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);

    if(typedata->has_fragment && state->freepos == NULL)
        state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;

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
    if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN) {
        return;
    }

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
            state->freepos->spdx = STEGWAY_RUN_SPEED * sign;
        } else {
            state->freepos->spdx = STEGWAY_BASE_SPEED * sign;
        }
    }

    if(state->freepos->spdx == 0) {
        state->anim_state.animation = 0;
        state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;
    }
    else if(abs(state->freepos->spdx) < STEGWAY_RUN_SPEED) {
        state->anim_state.animation = 1;
        state->frag_anim_state->animation = OBJ_ANIMATION_NO_ANIMATION;
    }
    else {
        state->anim_state.animation = 2;
        state->frag_anim_state->animation = 0;
    }

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;
}


static void
_buzzbomber_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
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
    if(state->timer == 3) {
        PoolObject *projectile = object_pool_create(OBJ_PROJECTILE);
        projectile->freepos.vx = (sign > 0)
            ? state->freepos->vx - (10 << 12)
            : state->freepos->vx + (10 << 12);
        projectile->freepos.vy = state->freepos->vy;
        projectile->freepos.spdx = (2 * sign) << 12;
        projectile->freepos.spdy = 2 << 12;
        projectile->state.flipmask = state->flipmask;
        
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
    if(state->timer == 0 || state->timer > 30) {
        state->anim_state.animation = 0;
    } else state->anim_state.animation = 1;
}

static void
_projectile_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // A generic level projectile.
    // This object should always be a free object and it also expects
    // its speed to be well-defined.

    if((state->freepos == NULL) || object_should_despawn(state)) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    // Setup player interaction hitbox.
    // Default size is 16x16, and then handle edge cases.
    RECT hitbox = {
        .x = pos->vx - 8,
        .y = pos->vy - 16,
        .w = 16,
        .h = 16,
    };

    // Edge case: buzzbomber
    if(state->anim_state.animation == 0) {
        int32_t sign = ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1);
        if(sign > 0) hitbox.x = pos->vx;
        hitbox.y = pos->vy - 8;
        hitbox.w = hitbox.h = 8;
    }

    hazard_player_interaction(&hitbox, pos);

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;
}
