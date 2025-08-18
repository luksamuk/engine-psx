#include "object.h"
#include "object_state.h"
#include "collision.h"
#include "player.h"
#include "sound.h"
#include "camera.h"
#include "render.h"
#include "boss.h"
#include "timer.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

#include "screens/level.h"

// Extern elements
extern Player *player;
extern Camera *camera;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

extern uint32_t level_score_count;

extern SoundEffect sfx_pop;
extern SoundEffect sfx_bomb;

extern uint8_t    level_round;
extern uint8_t    level_act;



// Object constants
#define OBJ_GRAVITY 0x00380

// Object type enums
#define OBJ_BALLHOG      (MIN_LEVEL_OBJ_GID + 0)
#define OBJ_BOUNCEBOMB   (MIN_LEVEL_OBJ_GID + 1)
#define OBJ_BOSS_SPAWNER (MIN_LEVEL_OBJ_GID + 2)
#define OBJ_BOSS         (MIN_LEVEL_OBJ_GID + 3)

// Update functions
static void _ballhog_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _bouncebomb_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _boss_spawner_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _boss_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R0(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_BALLHOG:      _ballhog_update(state, typedata, pos);      break;
    case OBJ_BOUNCEBOMB:   _bouncebomb_update(state, typedata, pos);   break;
    case OBJ_BOSS_SPAWNER: _boss_spawner_update(state, typedata, pos); break;
    case OBJ_BOSS:         _boss_update(state, typedata, pos);         break;
    }
}

static void
_ballhog_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // Face the player
    state->flipmask =
        ((player->pos.vx >> 12) <= pos->vx)
        ? MASK_FLIP_FLIPX
        : 0;

    // Player interaction
    RECT hitbox = {
        .x = pos->vx - 12,
        .y = pos->vy - 40,
        .w = 24,
        .h = 40,
    };
    if(enemy_player_interaction(state, &hitbox, pos) == OBJECT_DESPAWN)
        return;

    // Ballhog should throw a bomb if player is close enough
    // Further,
    if(state->anim_state.animation == 0) {
        // First bomb happens after 180 frames
        if(state->timer == 0) state->timer = 180;
        else {
            if(state->timer == 1) {
                // Switch to throw bomb animation
                state->anim_state.animation = 1;
                state->anim_state.frame = 0;
            }
            state->timer--;
        }
    } else if(state->anim_state.animation == 1) {
        if(state->anim_state.frame == 2) {
            // Create bomb object at direction
            PoolObject *bomb = object_pool_create(OBJ_BOUNCEBOMB);
            bomb->freepos.vx = (pos->vx << 12);
            bomb->freepos.vy = ((pos->vy - 6) << 12);
            bomb->state.anim_state.animation = 0;
            bomb->freepos.spdx = (1 * ((state->flipmask & MASK_FLIP_FLIPX) ? -1 : 1)) << 12;
            bomb->freepos.spdy = 0;
            bomb->state.timer = 480;
            
            // Reset animation
            state->anim_state.animation = 0;
            state->anim_state.frame = 0;

            // Next bomb takes a shorter time
            state->timer = 135;
        }
    }
}

static void
_bouncebomb_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    PoolObject *explosion;
    // This is a free object, always.
    // Object is assumed to have an X speed at startup
    if(state->freepos == NULL) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    // Setup the timer to explode at double the time a Ballhog needs to
    // create a new bomb. This way we'll have a max of two bombs on screen
    state->timer--;
    if(state->timer == 0) {
        sound_play_vag(sfx_pop, 0);
        goto explode;
    }

    // When colliding with ground, bounce at a specific speed
    if(linecast(pos->vx, pos->vy - 8,
                CDIR_FLOOR, 8, CDIR_FLOOR).collided) {
        state->freepos->spdy = -0x3800; // -2
    }

    // When colliding with a wall, explode
    if(((state->freepos->spdx < 0) && linecast(pos->vx, pos->vy - 8,
                                               CDIR_LWALL, 10, CDIR_FLOOR).collided)
       || ((state->freepos->spdx > 0) && linecast(pos->vx, pos->vy - 8,
                                                  CDIR_RWALL, 10, CDIR_FLOOR).collided)) {
        sound_play_vag(sfx_pop, 0);
        goto explode;
    }

    /* Player interaction and hitboxes */
    RECT hitbox = {
        .x = pos->vx - 8,
        .y = pos->vy - 16,
        .w = 16,
        .h = 16,
    };

    // When colliding with Amy's hammer, explode and do no harm (Piko Piko only)
    if(pikopiko_object_interaction(state, pos, &hitbox)) return;

    // When colliding with player, explode and do some damage
    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                       hitbox.x, hitbox.y, 16, 16))
    {
        if(player->action != ACTION_HURT && player->iframes == 0) {
            player_do_damage(player, pos->vx << 12);
            goto explode;
        }
    }

    // Despawn if too far from camera
    if((state->freepos->vx < camera->pos.vx - (SCREEN_XRES << 12))
       || (state->freepos->vx > camera->pos.vx + (SCREEN_XRES << 12))
       || (state->freepos->vy < camera->pos.vy - (SCREEN_YRES << 12))
       || (state->freepos->vy > camera->pos.vy + (SCREEN_YRES << 12))) {
        state->props |= OBJ_FLAG_DESTROYED;
        return;
    }

    // Apply gravity
    state->freepos->spdy += OBJ_GRAVITY;
    state->freepos->spdy = state->freepos->spdy > 0x10000 ? 0x10000 : state->freepos->spdy;
    
    state->freepos->vx += state->freepos->spdx;
    state->freepos->vy += state->freepos->spdy;
    return;

explode:
    // Create explosion
    explosion = object_pool_create(OBJ_EXPLOSION);
    explosion->freepos.vx = (pos->vx << 12);
    explosion->freepos.vy = (pos->vy << 12);
    explosion->state.anim_state.animation = 0; // Small explosion
    state->props |= OBJ_FLAG_DESTROYED;
}

// ===================================
//      BOSS - EGG BOMB DISPENSER
// ===================================

// These variables relate to the boss only.
extern BossState *boss;

#define BOSS_ANIM_NORMAL      0
#define BOSS_ANIM_LOOKDOWN    1
#define BOSS_ANIM_HIT         2
#define BOSS_ANIM_DEAD        3
#define BOSS_ANIM_RECOVERING  4
#define BOSS_ANIM_DROPPING    5
#define BOSS_ANIM_SMILING     6

#define BOSS_STATE_INIT       0
#define BOSS_STATE_WALKBACK   1
#define BOSS_STATE_WALKFRONT  2
#define BOSS_STATE_SWINGBACK  3
#define BOSS_STATE_SWINGFRONT 4
#define BOSS_STATE_DEAD       5
#define BOSS_STATE_DROPPING   6
#define BOSS_STATE_RECOVERING 7
#define BOSS_STATE_FLEEING    8

#define BOSS_TOSWING_COUNT    3
#define BOSS_DESCENT_SPEED    0x01200
#define BOSS_WALK_SPEED       0x01200
#define BOSS_SWING_SPEED      0x00020
#define BOSS_WOBBLE_SPEED     0x00040
#define BOSS_SWING_HEIGHT     120
#define BOSS_TURN_DELAY       30
#define BOSS_SWING_DELAY      60
#define BOSS_BOMB_INTERVAL    75
#define BOSS_BOMB_LIFETIME    160
#define BOSS_BOMB_XDELTA      8
#define BOSS_BOMB_XSPD        0x00480
#define BOSS_FLEE_XSPD        0x03600
#define BOSS_FLEE_YSPD        0x01200
#define BOSS_FLEE_GRAVITY     0x00380
#define BOSS_RISING_SPEED     0x02400

static void
_boss_spawner_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // Create boss once camera position fits
    if((player->pos.vx >> 12) >= (pos->vx - (CENTERX >> 1))) {
        state->props |= OBJ_FLAG_DESTROYED;
        PoolObject *boss_obj = object_pool_create(OBJ_BOSS);
        boss_obj->freepos.vx = ((pos->vx + 128) << 12);
        boss_obj->freepos.vy = ((pos->vy - 320) << 12);
        boss_obj->state.anim_state.animation = 0;
        boss_obj->state.flipmask = state->flipmask;

        // Setup boss state
        boss->state = BOSS_STATE_INIT;
        boss->health = 8;
        boss->anchor.vx = (pos->vx << 12);
        boss->anchor.vy = (pos->vy << 12);
        boss->counter4 = boss_obj->freepos.vy;
        boss->counter6 = BOSS_BOMB_INTERVAL;

        camera_focus(camera, boss->anchor.vx, boss->anchor.vy - (100 << 12));

        screen_level_boss_lock(1);
        sound_bgm_play(BGM_BOSS);
    }
}

static void
_boss_spawn_bomb(ObjectState *state, VECTOR *pos)
{
    if(boss->counter6 == 0) {
        uint8_t facing_left = (state->flipmask & MASK_FLIP_FLIPX);
        PoolObject *bomb = object_pool_create(OBJ_BOUNCEBOMB);
        int32_t deltax = 0;

        if(state->freepos->spdx != 0) {
            deltax = facing_left ? -BOSS_BOMB_XDELTA : BOSS_BOMB_XDELTA;
            bomb->freepos.spdx = facing_left ? -BOSS_BOMB_XSPD : BOSS_BOMB_XSPD;
        } else {
            bomb->freepos.spdx = 0;
            deltax = 0;
        }
        bomb->freepos.vx = ((pos->vx + deltax) << 12);
        bomb->freepos.vy = ((pos->vy + 8) << 12);
        bomb->state.anim_state.animation = 0;
        bomb->freepos.spdy = 0;
        bomb->state.timer = BOSS_BOMB_LIFETIME;
        boss->counter6 = BOSS_BOMB_INTERVAL;
    }
}

static void
_boss_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    // Have a sinusoid describe the swing behaviour.
    // We're not looking for a full cycle here, just the first crest!
    // (Remember that (1 << 12) == (2*pi))
    // f(x) = top_y + (amplitude * sin(x/2))

    // counter1: Lap counter
    // counter2: Frame wait before swing
    // counter3: Used on angle operations (swing)
    // counter4: Stores calculated Y position (without wobble effect)
    // counter5: Used on angle operations (wobble)
    // counter6: Countdown to next bomb

    if(boss->state != BOSS_STATE_DEAD)
        boss->counter5 += BOSS_WOBBLE_SPEED;

    if((boss->state < BOSS_STATE_SWINGBACK) && boss->counter6 > 0)
        boss->counter6--;

    switch(boss->state) {
    default: break;
    case BOSS_STATE_INIT:
        state->frag_anim_state->animation = BOSS_ANIM_SMILING;
        boss->counter6 = BOSS_BOMB_INTERVAL;
        if(boss->counter4 < (boss->anchor.vy - (128 << 12))) {
            state->flipmask = MASK_FLIP_FLIPX;
            state->freepos->spdy = BOSS_DESCENT_SPEED;
            boss->counter2 = 60;
        } else if(boss->counter2 > 0) {
            state->freepos->spdy = 0;
            boss->counter2--;
        } else {
            boss->counter4 = boss->anchor.vy - (128 << 12);
            state->freepos->spdy = 0;
            boss->counter1 = 0;
            boss->state = BOSS_STATE_WALKBACK;
            state->flipmask = MASK_FLIP_FLIPX;
            boss->counter3 = 0;
        }
        break;
    case BOSS_STATE_WALKBACK:
        state->flipmask = MASK_FLIP_FLIPX;
        if(boss->counter2 > 0) {
            boss->counter2--;
            _boss_spawn_bomb(state, pos);
        } else if(state->freepos->vx > (boss->anchor.vx - (128 << 12))) {
            state->freepos->spdx = -BOSS_WALK_SPEED;
            _boss_spawn_bomb(state, pos);
        } else {
            state->freepos->vx = boss->anchor.vx - (128 << 12);
            boss->counter1++;
            // Either flip or descent; it depends.
            if(boss->counter1 == BOSS_TOSWING_COUNT) {
                boss->counter1 = 0;
                boss->counter3 = 0;
                state->freepos->spdx = 0;
                boss->counter2 = BOSS_SWING_DELAY;
                boss->state = BOSS_STATE_SWINGFRONT;
            } else {
                boss->counter2 = BOSS_TURN_DELAY;
                boss->state = BOSS_STATE_WALKFRONT;
                state->freepos->spdx = 0;
            }
        }
        break;
    case BOSS_STATE_WALKFRONT:
        state->flipmask = 0;
        if(boss->counter2 > 0) {
            boss->counter2--;
            _boss_spawn_bomb(state, pos);
        } else if(state->freepos->vx < (boss->anchor.vx + (128 << 12))) {
            state->freepos->spdx = BOSS_WALK_SPEED;
            _boss_spawn_bomb(state, pos);
        } else {
            state->freepos->vx = boss->anchor.vx + (128 << 12);
            boss->counter1++;
            // Either flip or descent; it depends.
            if(boss->counter1 == BOSS_TOSWING_COUNT) {
                boss->counter1 = 0;
                boss->counter3 = ONE;
                state->freepos->spdx = 0;
                boss->counter2 = BOSS_SWING_DELAY;
                boss->state = BOSS_STATE_SWINGBACK;
            } else {
                boss->counter2 = BOSS_TURN_DELAY;
                boss->state = BOSS_STATE_WALKBACK;
                state->freepos->spdx = 0;
            }
        }
        break;
    case BOSS_STATE_SWINGBACK:
        state->flipmask = MASK_FLIP_FLIPX;
        if(state->freepos->vx > (boss->anchor.vx - (128 << 12))) {
            if(boss->counter2 > 0) boss->counter2--;
            else boss->counter3 -= BOSS_SWING_SPEED;
            state->freepos->vx =
                (boss->anchor.vx - (128 << 12)) + (boss->counter3 * 0x100);
            boss->counter4 =
                (boss->anchor.vy - (128 << 12)) +
                (rsin(boss->counter3 >> 1) * BOSS_SWING_HEIGHT);
        } else {
            state->freepos->vx = boss->anchor.vx - (128 << 12);
            state->flipmask = 0;
            boss->state = BOSS_STATE_WALKFRONT;
            boss->counter1++;
            boss->counter3 = 0;
            boss->counter2 = 30;
            boss->counter6 = 0;

            // Spawn an extra bomb
            _boss_spawn_bomb(state, pos);
        }
        break;       
    case BOSS_STATE_SWINGFRONT:
        state->flipmask = 0;
        if(state->freepos->vx < (boss->anchor.vx + (128 << 12))) {
            if(boss->counter2 > 0) boss->counter2--;
            else boss->counter3 += BOSS_SWING_SPEED;
            state->freepos->vx =
                (boss->anchor.vx - (128 << 12)) + (boss->counter3 * 0x100);
            boss->counter4 =
                (boss->anchor.vy - (128 << 12)) +
                (rsin(boss->counter3 >> 1) * BOSS_SWING_HEIGHT);
        } else {
            state->freepos->vx = boss->anchor.vx + (128 << 12);
            state->flipmask = MASK_FLIP_FLIPX;
            boss->state = BOSS_STATE_WALKBACK;
            boss->counter1++;
            boss->counter3 = 0;
            boss->counter2 = 30;
            boss->counter6 = 0;

            // Spawn an extra bomb
            _boss_spawn_bomb(state, pos);
        }
        break;
    case BOSS_STATE_DEAD:
        state->frag_anim_state->animation = BOSS_ANIM_DEAD;
        break;
    case BOSS_STATE_DROPPING:
        state->frag_anim_state->animation = BOSS_ANIM_DROPPING;
        if(boss->counter4 < boss->anchor.vy + (CENTERY << 11)) {
            state->freepos->spdy += BOSS_FLEE_GRAVITY;
            boss->counter3 = 0;
        } else boss->state = BOSS_STATE_RECOVERING;
        break;
    case BOSS_STATE_RECOVERING:
        state->frag_anim_state->animation = BOSS_ANIM_RECOVERING;
        state->flipmask = 0;
        if(boss->counter4 > boss->anchor.vy - (CENTERY << 11)) {
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
        state->frag_anim_state->animation = BOSS_ANIM_LOOKDOWN;
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

    int32_t wobble_sin = (rsin(boss->counter5 >> 1) * 8);
    state->freepos->vx += state->freepos->spdx;
    boss->counter4 += state->freepos->spdy;
    state->freepos->vy = boss->counter4 + wobble_sin;


    // Collision and hitbox
    // Hitbox is a lot smaller than it looks, being 52x32, positioned at 9x17
    // (relative to top left corner of entire eggmobile which is 64x64).
    // Coordinates are calculated relative to bottom center.
    if(boss->health > 0) {
        if(boss->hit_cooldown > 0) {
            boss->hit_cooldown--;
        } else {
            RECT hitbox = {
                .x = pos->vx - 23,
                .y = pos->vy - 47,
                .w = 52,
                .h = 32,
            };
            if(player_boss_interaction(pos, &hitbox)) {
                // Boss got hit
                boss->counter2 = 15; // Stop for half the cooldown time
            }
        }
    } else {
        if(boss->state < BOSS_STATE_DEAD) {
            boss->state = BOSS_STATE_DEAD;
            state->freepos->spdx = 0;
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
                explosion->freepos.vy = ((pos->vy - 32) << 12) + rvy + wobble_sin;
                explosion->state.anim_state.animation = 1; // Big explosion
                sound_play_vag(sfx_bomb, 0);
            } else boss->counter6--;
        } else if(boss->state == BOSS_STATE_DEAD) {
            state->freepos->spdy = 0;
            boss->state = BOSS_STATE_DROPPING;
        }
    }

    // Animation control before defeat
    if((boss->state > BOSS_STATE_INIT) && (boss->state < BOSS_STATE_DEAD)) {
        if(boss->hit_cooldown > 0)
            state->frag_anim_state->animation = BOSS_ANIM_HIT;
        else state->frag_anim_state->animation =
                 ((player->action == ACTION_HURT) || (player->death_type > 0))
                 ? BOSS_ANIM_SMILING
                 : BOSS_ANIM_NORMAL;
    }
}
