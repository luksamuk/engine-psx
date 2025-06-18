#include "object.h"
#include "object_state.h"
#include "collision.h"
#include "player.h"
#include "sound.h"
#include "camera.h"
#include "render.h"
#include "boss.h"
#include "timer.h"

// Extern elements
extern Player player;
extern Camera camera;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

extern uint32_t level_score_count;

extern SoundEffect sfx_pop;
extern SoundEffect sfx_bomb;

extern TileMap16  map16;
extern TileMap128 map128;
extern LevelData  leveldata;



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
        ((player.pos.vx >> 12) <= pos->vx)
        ? MASK_FLIP_FLIPX
        : 0;

    // Calculate hitbox
    int32_t hitbox_vx = pos->vx - 12;
    int32_t hitbox_vy = pos->vy - 40;
    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                       hitbox_vx, hitbox_vy, 24, 40))
    {
        if(player_attacking) {
            state->props |= OBJ_FLAG_DESTROYED;
            level_score_count += 100;
            sound_play_vag(sfx_pop, 0);

            // Explosion
            PoolObject *explosion = object_pool_create(OBJ_EXPLOSION);
            explosion->freepos.vx = (pos->vx << 12);
            explosion->freepos.vy = (pos->vy << 12);
            explosion->state.anim_state.animation = 0; // Small explosion
            if(!player.grnd && player.vel.vy > 0) {
                player.vel.vy *= -1;
            }
        } else {
            if(player.action != ACTION_HURT && player.iframes == 0) {
                player_do_damage(&player, pos->vx << 12);
            }
        }
    }

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

    // Setup the timer to explode at double the time a Ballhog needs to
    // create a new bomb. This way we'll have a max of two bombs on screen
    state->timer--;
    if(state->timer == 0) goto explode;

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
        goto explode;
    }

    // When colliding with Sonic, explode and do some damage
    int32_t hitbox_vx = pos->vx - 8;
    int32_t hitbox_vy = pos->vy - 16;
    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                       hitbox_vx, hitbox_vy, 16, 16))
    {
        if(player.action != ACTION_HURT && player.iframes == 0) {
            player_do_damage(&player, pos->vx << 12);
        }
        goto explode;
    }

    // Despawn if too far from camera
    if((state->freepos->vx < camera.pos.vx - (SCREEN_XRES << 12))
       || (state->freepos->vx > camera.pos.vx + (SCREEN_XRES << 12))
       || (state->freepos->vy < camera.pos.vy - (SCREEN_YRES << 12))
       || (state->freepos->vy > camera.pos.vy + (SCREEN_YRES << 12))) {
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
    explosion->state.anim_state.animation = 1; // Big explosion
    state->props |= OBJ_FLAG_DESTROYED;
    sound_play_vag(sfx_bomb, 0);
}

// ============================
//      BOSS - EGG MOBILE
// ============================

// These variables relate to the boss only.
extern BossState *boss;
#define BOSS_STATE_INIT       0
#define BOSS_STATE_WALKBACK   1
#define BOSS_STATE_WALKFRONT  2
#define BOSS_STATE_SWINGBACK  3
#define BOSS_STATE_SWINGFRONT 4

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

static void
_boss_spawner_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    // Create boss once camera position fits
    if((camera.pos.vx >> 12) >= (pos->vx - (CENTERX >> 1))) {
        state->props |= OBJ_FLAG_DESTROYED;
        PoolObject *boss_obj = object_pool_create(OBJ_BOSS);
        boss_obj->freepos.vx = ((pos->vx + 128) << 12);
        boss_obj->freepos.vy = ((pos->vy - 320) << 12);
        boss_obj->state.anim_state.animation = 0;

        // Setup boss state
        boss->state = BOSS_STATE_INIT;
        boss->health = 8;
        boss->anchor.vx = (pos->vx << 12);
        boss->anchor.vy = (pos->vy << 12);
        boss->counter4 = boss_obj->freepos.vy;
        boss->counter6 = BOSS_BOMB_INTERVAL;

        camera_focus(&camera, boss->anchor.vx, boss->anchor.vy - (100 << 12));

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

    boss->counter5 += BOSS_WOBBLE_SPEED;

    if((boss->state < BOSS_STATE_SWINGBACK)
       && boss->counter6 > 0) boss->counter6--;

    switch(boss->state) {
    default: break;
    case BOSS_STATE_INIT:
        boss->counter6 = BOSS_BOMB_INTERVAL;
        if(boss->counter4 < (boss->anchor.vy - (128 << 12))) {
            state->flipmask = MASK_FLIP_FLIPX;
            state->freepos->spdy = BOSS_DESCENT_SPEED;
        } else {
            boss->counter4 = boss->anchor.vy - (128 << 12);
            state->freepos->spdy = 0;
            boss->counter1 = 0;
            boss->state = BOSS_STATE_WALKBACK;
            state->flipmask = MASK_FLIP_FLIPX;
            boss->counter3 = 0;
            boss->counter2 = 60;
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
    }

    state->freepos->vx += state->freepos->spdx;
    boss->counter4 += state->freepos->spdy;
    state->freepos->vy = boss->counter4 + (rsin(boss->counter5 >> 1) * 8);
}
