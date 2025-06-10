#include "object.h"
#include "object_state.h"
#include "collision.h"
#include "player.h"
#include "sound.h"

// Extern elements
extern Player player;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

extern uint32_t level_score_count;

extern SoundEffect sfx_pop;



// Object type enums
#define OBJ_BALLHOG  (MIN_LEVEL_OBJ_GID + 0)

// Update functions
static void _ballhog_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R0(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_BALLHOG: _ballhog_update(state, typedata, pos); break;
    }
}

static void
_ballhog_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
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
                       
}
