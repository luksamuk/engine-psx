#include "object.h"
#include "object_state.h"
#include "collision.h"
#include "player.h"
#include "sound.h"
#include "camera.h"
#include "render.h"
#include "boss.h"
#include <stdio.h>
#include <stdlib.h>

#include "screens/level.h"

// Extern elements
extern Player *player;
extern Camera *camera;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

extern SoundEffect sfx_pop;
extern SoundEffect sfx_bomb;

extern uint32_t   level_score_count;
extern uint8_t    level_round;
extern uint8_t    level_act;


// Object type enums
#define OBJ_MOTOBUG      (MIN_LEVEL_OBJ_GID + 0)
#define OBJ_BUZZBOMBER   (MIN_LEVEL_OBJ_GID + 1)
#define OBJ_PROJECTILE   (MIN_LEVEL_OBJ_GID + 2)
#define OBJ_CHOPPER      (MIN_LEVEL_OBJ_GID + 3) // Unused
#define OBJ_BOSS_SPAWNER (MIN_LEVEL_OBJ_GID + 4)
#define OBJ_BOSS         (MIN_LEVEL_OBJ_GID + 5)
#define OBJ_BOSS_EXTRAS  (MIN_LEVEL_OBJ_GID + 6) // Wrecking ball and chain
#define OBJ_ROCK         (MIN_LEVEL_OBJ_GID + 7)
#define OBJ_PLATFORM     (MIN_LEVEL_OBJ_GID + 8)


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
static void _boss_spawner_ghz_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _boss_ghz_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _boss_extras_ghz_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _rock_ghz_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _platform_ghz_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R2(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_MOTOBUG:      _motobug_update(state, typedata, pos);          break;
    case OBJ_BUZZBOMBER:   _buzzbomber_ghz_update(state, typedata, pos);   break;
    case OBJ_PROJECTILE:   _projectile_ghz_update(state, typedata, pos);   break;
    case OBJ_CHOPPER: break; // Unused
    case OBJ_BOSS_SPAWNER: _boss_spawner_ghz_update(state, typedata, pos); break;
    case OBJ_BOSS:         _boss_ghz_update(state, typedata, pos);         break;
    case OBJ_BOSS_EXTRAS:  _boss_extras_ghz_update(state, typedata, pos);  break;
    case OBJ_ROCK:         _rock_ghz_update(state, typedata, pos);         break;
    case OBJ_PLATFORM:     _platform_ghz_update(state, typedata, pos);     break;
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

    // Shooting at the player->
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

        if(player->pos.vx >= shoot_min_x && player->pos.vx <= shoot_max_x) {
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

static void
_rock_ghz_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);

    FRECT solidity = {
        .x = ((pos->vx - 16) << 12) - (ONE >> 1),
        .y = (pos->vy - 32) << 12,
        .w = 33 << 12,
        .h = 32 << 12,
    };
    solid_object_player_interaction(state, &solidity, 0);
}

static void
_platform_ghz_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);

    FRECT solidity = {
        .x = (pos->vx - 30) << 12,
        .y = (pos->vy - 26) << 12,
        .w = 60 << 12,
        .h = 13 << 12,
    };
    solid_object_player_interaction(state, &solidity, 1);
}

// ===================================
//      BOSS - EGG WRECKER
// ===================================

extern BossState *boss;

// State enumeration
#define BOSS_STATE_INIT       0
#define BOSS_STATE_MOCKPLAYER 1
#define BOSS_STATE_GO_LEFT    2
#define BOSS_STATE_GO_RIGHT   3
#define BOSS_STATE_DEAD       4
#define BOSS_STATE_DROPPING   5
#define BOSS_STATE_RECOVERING 6
#define BOSS_STATE_FLEEING    7

// Properties and physics
#define BOSS_NUM_CHAINS                  4
#define BOSS_DESCENT_SPEED         0x01200
#define BOSS_WALK_SPEED            0x01200
#define BOSS_WALK_SPEED_SWING      0x00680
#define BOSS_WALK_COOLDOWN              60
#define BOSS_BALL_DESCENT_SPEED    0x01200
#define WRECKINGBALL_SWING_RADIUS      116
#define WRECKINGBALL_SWING_SPEED   0x00010
#define WRECKINGBALL_MAX_ANGLE     0x002aa
#define BOSS_DROP_GRAVITY          0x00380
#define BOSS_RISING_SPEED          0x02400
#define BOSS_FLEE_XSPD             0x03600
#define BOSS_FLEE_YSPD             0x01200

// Animations
#define BOSS_ANIM_STOP       0
#define BOSS_ANIM_MOVE       1
#define BOSS_ANIM_LAUGH      2
#define BOSS_ANIM_HIT        3
#define BOSS_ANIM_RECOVERING 4
#define BOSS_ANIM_DEAD       5
#define BOSS_ANIM_DROPPING   6

// Extra object animations
#define BOSS_EXTRA_WRECKINGBALL 0
#define BOSS_EXTRA_CHAIN        1

static void
_boss_spawner_ghz_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // Create boss once camera position fits
    if((player->pos.vx >> 12) >= (pos->vx - (CENTERX >> 1))) {
        state->props |= OBJ_FLAG_DESTROYED;
        PoolObject *boss_obj = object_pool_create(OBJ_BOSS);
        boss_obj->freepos.vx = ((pos->vx + 128) << 12);
        boss_obj->freepos.vy = ((pos->vy - 320) << 12);
        boss_obj->state.anim_state.animation = 0;
        boss_obj->state.flipmask = state->flipmask; // Face left

        // Setup boss state
        boss->state = BOSS_STATE_INIT;
        boss->health = 8;
        boss->anchor.vx = (pos->vx << 12);
        boss->anchor.vy = (pos->vy << 12);
        boss->counter4 = boss_obj->freepos.vy;
        /* boss->counter6 = BOSS_BOMB_INTERVAL; */

        camera_focus(camera, boss->anchor.vx + (8 << 12), boss->anchor.vy - (100 << 12));

        screen_level_boss_lock(1);
        sound_bgm_play(BGM_BOSS);
    }
}

static ObjectState *
_boss_ghz_spawn_wrecking_ball(ObjectState *state)
{
    // Chain links
    PoolObject *chain[BOSS_NUM_CHAINS];
    PoolObject *ball;

    // Create wrecking ball
    ball = object_pool_create(OBJ_BOSS_EXTRAS);
    //ball->freepos.vx = state->freepos->vx + (8 << 12);
    ball->freepos.vx = state->freepos->vx;
    ball->freepos.vy = state->freepos->vy - (14 << 12);
    ball->state.anim_state.animation = BOSS_EXTRA_WRECKINGBALL;
    ball->freepos.rx = ball->freepos.vx;
    ball->freepos.ry = ball->freepos.vy;
    ball->state.parent = state; // Point to boss state

    // As for the chains, 
    for(int i = 0; i < BOSS_NUM_CHAINS; i++) {
        chain[i] = object_pool_create(OBJ_BOSS_EXTRAS);
        chain[i]->freepos.vx = ball->freepos.vx;
        chain[i]->freepos.vy = ball->freepos.vy;
        chain[i]->state.anim_state.animation = BOSS_EXTRA_CHAIN;
    }

    // Link chain parts
    chain[0]->state.parent = state; // Point to boss
    chain[0]->state.next   = &chain[1]->state;
    for(int i = 1; i < BOSS_NUM_CHAINS-1; i++) {
        // Link middle chain parts
        chain[i]->state.parent = &chain[i-1]->state;
        chain[i]->state.next   = &chain[i+1]->state;
    }
    chain[BOSS_NUM_CHAINS-1]->state.parent = &chain[BOSS_NUM_CHAINS-2]->state;
    chain[BOSS_NUM_CHAINS-1]->state.next   = &ball->state; // Point to ball

    return &chain[0]->state;
}

static void
_boss_ghz_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    (void)(pos);

    // counter1: Walk turn counter
    // counter2: Death explosion countdown
    // counter3: Wrecking ball swing angle // After death: stopped timer after recover
    // counter4: Wrecking ball swing auxiliary (angle range)
    // counter6: In-between explosions cooldown

    switch(boss->state) {
    default: break;
    case BOSS_STATE_INIT:
        // Center boss atop the stage
        if(state->freepos->vy < (boss->anchor.vy - (128 << 12))) {
            // Descent
            state->flipmask = MASK_FLIP_FLIPX;
            state->freepos->spdy = BOSS_DESCENT_SPEED;
        } else if(state->freepos->vx > boss->anchor.vx) {
            // Move left
            state->flipmask = MASK_FLIP_FLIPX;
            state->freepos->spdy = 0;
            state->freepos->spdx = -BOSS_WALK_SPEED;
        } else {
            state->freepos->spdy = 0;
            state->freepos->spdx = 0;
            state->next = _boss_ghz_spawn_wrecking_ball(state);
            boss->state = BOSS_STATE_MOCKPLAYER;
        }
        break;
    case BOSS_STATE_MOCKPLAYER:
        // Wrecking ball object takes boss out of this state.
        state->freepos->rx = state->freepos->vx;
        break;
    case BOSS_STATE_GO_LEFT:
        if(state->freepos->vx > state->freepos->rx - (CENTERX << 12) + (WRECKINGBALL_SWING_RADIUS << 12)) {
            boss->counter1 = 1;
            state->freepos->spdx = -BOSS_WALK_SPEED;
        } else {
            state->freepos->spdx = 0;
            if(boss->counter1 == 1) {
                state->freepos->vx -= 8 << 12;
                state->flipmask = 0; // Turn back
                boss->counter1 = 0;
            }
        }
        // Wrecking ball object moves boss right
        break;
    case BOSS_STATE_GO_RIGHT:
        if(state->freepos->vx < state->freepos->rx + (CENTERX << 12) - (WRECKINGBALL_SWING_RADIUS << 12) - (8 << 12)) {
            boss->counter1 = 1;
            state->freepos->spdx = BOSS_WALK_SPEED;
        }
        else {
            state->freepos->spdx = 0;
            if(boss->counter1 == 1) {
                state->freepos->vx += 8 << 12;
                state->flipmask = MASK_FLIP_FLIPX; // Turn back
                boss->counter1 = 0;
            }
        }
        // Wrecking ball object moves boss left
        break;
    case BOSS_STATE_DEAD:
        state->freepos->spdx = 0;
        break;
    case BOSS_STATE_DROPPING:
        if(state->freepos->vy < boss->anchor.vy + (CENTERY << 11)) {
            state->freepos->spdy += BOSS_DROP_GRAVITY;
            boss->counter3 = 0;
        } else {
            boss->state = BOSS_STATE_RECOVERING;
        }
        break;
    case BOSS_STATE_RECOVERING:
        state->flipmask = 0;
        if(state->freepos->vy > boss->anchor.vy - (CENTERY << 11)) {
            state->freepos->spdy = -BOSS_RISING_SPEED;
            boss->counter3 = 30;
        } else {
            if(boss->counter3 > 0) {
                state->freepos->spdy = 0;
                boss->counter3--;
            } else {
                screen_level_boss_lock(0);
                camera_set_left_bound(camera, boss->anchor.vx);
                camera_follow_player(camera);
                screen_level_play_music(level_round, level_act);
                boss->state = BOSS_STATE_FLEEING;
            }
        }
        break;
    case BOSS_STATE_FLEEING:
        state->freepos->spdx = BOSS_FLEE_XSPD;
        state->freepos->spdy = -BOSS_FLEE_YSPD;
        state->flipmask = 0;

        // Despawn if too far!
        if((state->freepos->vx < camera->pos.vx - (SCREEN_XRES << 12))
           || (state->freepos->vx > camera->pos.vx + (SCREEN_XRES << 12))
           || (state->freepos->vy < camera->pos.vy - (SCREEN_YRES << 12))
           || (state->freepos->vy > camera->pos.vy + (SCREEN_YRES << 12))) {
            state->props |= OBJ_FLAG_DESTROYED;
            return;
        }
        break;
    }

    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;

    // Collision and hitbox
    if(boss->health > 0) {
        if(boss->hit_cooldown > 0) {
            boss->hit_cooldown--;
        } else if(boss->state > BOSS_STATE_MOCKPLAYER) {
            int32_t hitbox_vx = pos->vx - 23;
            int32_t hitbox_vy = pos->vy - 47;
            uint8_t extra_check;
            RECT extra = player_get_extra_hitbox(&extra_check);
            if((extra_check
                && aabb_intersects(extra.x, extra.y, extra.w, extra.h,
                                   hitbox_vx, hitbox_vy, 52, 32))
               || aabb_intersects(player_vx, player_vy, player_width, player_height,
                                  hitbox_vx, hitbox_vy, 52, 32)) {
                if(player_attacking) {
                    boss->health--;
                    boss->hit_cooldown = 30;
                    sound_play_vag(sfx_bomb, 0);

                    // Rebound Sonic
                    if(!player->grnd) {
                        player->vel.vy = -(player->vel.vy >> 1);
                        if(player->action == ACTION_GLIDE) {
                            player_set_action(player, ACTION_DROP);
                            player->airdirlock = 1;
                            // To be a little more fair with Knuckles,
                            // rebound him on the X axis by double the distance
                            // otherwise Knuckles will always get hurt when
                            // gliding onto the boss
                            player->vel.vx = -player->vel.vx;
                        } else player->vel.vx = -(player->vel.vx >> 1);
                    } else player->vel.vz = -(player->vel.vz >> 1);
                } else {
                    if(player->action != ACTION_HURT && player->iframes == 0) {
                        player_do_damage(player, pos->vx << 12);
                    }
                }
            }
        }
    } else {
        if(boss->state < BOSS_STATE_DEAD) {
            boss->state = BOSS_STATE_DEAD;
            boss->counter6 = 0;
            boss->counter2 = 180; // 3 seconds exploding
            level_score_count += 1000;
        }

        if(boss->counter2 > 0) {
            boss->counter2--;
            // When boss is dead, setup random explosions around the egg mobile
            if(boss->counter6 == 0) {
                boss->counter6 = 10;
                int32_t rvx = (rand() % 54) << 12;
                int32_t rvy = (rand() % 37) << 12;

                PoolObject *explosion = object_pool_create(OBJ_EXPLOSION);
                explosion->freepos.vx = ((pos->vx - 25) << 12) + rvx;
                explosion->freepos.vy = ((pos->vy - 32) << 12) + rvy;
                explosion->state.anim_state.animation = 1; // Big explosion

                // Go down the chain creating explosions on each link
                // and the wrecking ball
                ObjectState *link = state;
                do {
                    link = link->next;
                    explosion = object_pool_create(OBJ_EXPLOSION);
                    explosion->state.anim_state.animation = 1; // Big explosion
                    int32_t rangex = 16;
                    int32_t rangey = 16;
                    if(link->anim_state.animation == BOSS_EXTRA_WRECKINGBALL) {
                        rangex = rangey = 48;
                    }

                    rvx = (rand() % (rangex + 5)) << 12;
                    rvy = (rand() % (rangey + 5)) << 12;
                    explosion->freepos.vx = link->freepos->vx - (rangex << 11) + rvx;
                    explosion->freepos.vy = link->freepos->vy - (rangey << 12) + rvy;
                } while(link->anim_state.animation != BOSS_EXTRA_WRECKINGBALL);
                
                sound_play_vag(sfx_bomb, 0);
            } else boss->counter6--;
        } else if(state->next != NULL) {
            // Destroy all chain links
            ObjectState *link = state;
            do {
                link = link->next;
                link->props |= OBJ_FLAG_DESTROYED;
            } while(link->anim_state.animation != BOSS_EXTRA_WRECKINGBALL);
            boss->state = BOSS_STATE_DROPPING;
            state->next = NULL;
        }
    }

    // Update chain positions according to boss position itself
    // TODO

    // Animation
    if(boss->state == BOSS_STATE_MOCKPLAYER)
        state->frag_anim_state->animation = BOSS_ANIM_LAUGH;
    else if(boss->state == BOSS_STATE_DEAD)
        state->frag_anim_state->animation = BOSS_ANIM_DEAD;
    else if(boss->state == BOSS_STATE_DROPPING)
        state->frag_anim_state->animation = BOSS_ANIM_DROPPING;
    else if(boss->state == BOSS_STATE_RECOVERING)
        state->frag_anim_state->animation = BOSS_ANIM_RECOVERING;
    else if(boss->state == BOSS_STATE_FLEEING)
        state->frag_anim_state->animation = BOSS_ANIM_MOVE;
    else if(boss->hit_cooldown > 0)
        state->frag_anim_state->animation = BOSS_ANIM_HIT;
    else state->frag_anim_state->animation =
             ((player->action == ACTION_HURT) || (player->death_type > 0))
             ? BOSS_ANIM_LAUGH
             : ((state->freepos->spdx != 0)
                ? BOSS_ANIM_MOVE
                : BOSS_ANIM_STOP);
    
}

static void
_boss_extras_ghz_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // This object is supposed to always spawn as a free object in the first
    // place, so we'll make this quick and dirty.

    // The boss state's next pointer points to the first chain link.

    // A CHAIN always has a parent and a left pointers, pointing to the next
    // link and finally to the wrecking ball.

    // A WRECKING BALL points on *parent to the boss's state.
    if(state->anim_state.animation == BOSS_EXTRA_WRECKINGBALL) {
        // Setup relative Y position first. We need to position it a few
        // places lower than the Boss's anchor.
        // There are four chain links and the boss has 1px padding underneath,
        // so its final position should be (16 * 4) + 48 - 1.
        if(state->freepos->ry < state->parent->freepos->vy +
           (WRECKINGBALL_SWING_RADIUS << 12)) {
            state->freepos->vy += BOSS_BALL_DESCENT_SPEED;
            state->freepos->ry = state->freepos->vy;
        } else {
            if(boss->state == BOSS_STATE_MOCKPLAYER) {
                boss->state = BOSS_STATE_GO_LEFT;
            }
            
            // Swing
            if(boss->health > 0) {
                boss->counter3 += WRECKINGBALL_SWING_SPEED;
                boss->counter4 = rsin(boss->counter3) / 6;

                // Hit player
                RECT hitbox = {
                    .x = pos->vx - 18,
                    .y = pos->vy - 42,
                    .w = 36,
                    .h = 36,
                };
                hazard_player_interaction(&hitbox, pos);
            }

            state->freepos->vx = state->parent->freepos->vx +
                (rsin(boss->counter4) * WRECKINGBALL_SWING_RADIUS);
            state->freepos->vy = state->parent->freepos->vy +
                (rcos(boss->counter4) * WRECKINGBALL_SWING_RADIUS);

            // Move boss left/right when ball reaches max height
            if((boss->counter4 == -WRECKINGBALL_MAX_ANGLE)
               && (boss->state == BOSS_STATE_GO_LEFT))
                boss->state = BOSS_STATE_GO_RIGHT;
            else if((boss->counter4 == WRECKINGBALL_MAX_ANGLE)
                    && (boss->state == BOSS_STATE_GO_RIGHT))
                boss->state = BOSS_STATE_GO_LEFT;
        }

        return;
    }

    if(state->anim_state.animation == BOSS_EXTRA_CHAIN) {
        int32_t next_vy = state->next->freepos->vy;
        // If "next" is actually a wrecking ball, subtract its size
        // (We take advantage here of the fact that "next" only points to
        // an object of the current kind, and never to a boss or something
        if(state->next->anim_state.animation == BOSS_EXTRA_WRECKINGBALL) {
            next_vy -= (32 << 12);
        }
        
        state->freepos->vx =
            (state->parent->freepos->vx + state->next->freepos->vx) >> 1;
        state->freepos->vy =
            (state->parent->freepos->vy + next_vy) >> 1;

        return;
    }
}
