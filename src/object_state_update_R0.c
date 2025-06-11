#include "object.h"
#include "object_state.h"
#include "collision.h"
#include "player.h"
#include "sound.h"
#include "camera.h"
#include "render.h"

// Extern elements
extern Player player;
extern Camera camera;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

extern uint32_t level_score_count;

extern SoundEffect sfx_pop;

extern TileMap16  map16;
extern TileMap128 map128;
extern LevelData  leveldata;



// Object constants
#define OBJ_GRAVITY 0x00380

// Object type enums
#define OBJ_BALLHOG    (MIN_LEVEL_OBJ_GID + 0)
#define OBJ_BOUNCEBOMB (MIN_LEVEL_OBJ_GID + 1)

// Update functions
static void _ballhog_update(ObjectState *, ObjectTableEntry *, VECTOR *);
static void _bouncebomb_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R0(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_BALLHOG:    _ballhog_update(state, typedata, pos);    break;
    case OBJ_BOUNCEBOMB: _bouncebomb_update(state, typedata, pos); break;
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
    if(linecast(&leveldata, &map128, &map16,
                pos->vx, pos->vy - 8,
                CDIR_FLOOR, 8, CDIR_FLOOR).collided) {
        state->freepos->spdy = -0x3800; // -2
    }

    // When colliding with a wall, explode
    if(((state->freepos->spdx < 0) && linecast(&leveldata, &map128, &map16,
                                               pos->vx, pos->vy - 8,
                                               CDIR_LWALL, 10, CDIR_FLOOR).collided)
       || ((state->freepos->spdx > 0) && linecast(&leveldata, &map128, &map16,
                                                  pos->vx, pos->vy - 8,
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
    // TODO: Add sound effect
}
