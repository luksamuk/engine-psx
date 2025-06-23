#include "object.h"
#include "object_state.h"
#include "camera.h"
#include "render.h"

#define OBJ_MIN_SPAWN_DIST_X (CENTERX + (CENTERX >> 1))
#define OBJ_MIN_SPAWN_DIST_Y (CENTERY + (CENTERY >> 1))

extern Camera camera;

ObjectBehaviour
enemy_spawner_update(ObjectState *state, VECTOR *pos)
{
    // An object with a spawner is an object such that, when updating as
    // a static object, nullifies its own behaviour into checking whether
    // it is on range for spawning, if not spawned already. But if said
    // object is a free object, it behaves as it should.
    // This function abstracts the spawner behaviour of an object.

    // Whenever the spawner is outside of the screen, it destroys itself and
    // spawns a free-walking Self.
    // (Notice that being out of the screen also relies on the fact that this
    // object will not be updated if it is too far away, since it will not fit
    // the static object update window)

    if(state->freepos == NULL) {
        state->anim_state.animation = OBJ_ANIMATION_NO_ANIMATION;

        // Spawn free object WHEN too close to camera,
        // but still far away from  play area itself!
        if((state->parent == NULL) && (
               // Within outside boundary, and...
               (pos->vx > (camera.pos.vx >> 12) - SCREEN_XRES)
               && (pos->vx < (camera.pos.vx >> 12) + SCREEN_XRES)
               && (pos->vy > (camera.pos.vy >> 12) - SCREEN_YRES)
               && (pos->vy < (camera.pos.vy >> 12) + SCREEN_YRES)
            ) && (
                // Outside center screen
                (pos->vx < (camera.pos.vx >> 12) - OBJ_MIN_SPAWN_DIST_X)
                || (pos->vx > (camera.pos.vx >> 12) + OBJ_MIN_SPAWN_DIST_X)
                || (pos->vy < (camera.pos.vy >> 12) - OBJ_MIN_SPAWN_DIST_Y)
                || (pos->vy > (camera.pos.vy >> 12) + OBJ_MIN_SPAWN_DIST_Y)
                )
            )
        {
            return OBJECT_SPAWNER_CREATE_FREE;
        }
        return OBJECT_SPAWNER_ABORT_BEHAVIOUR;
    }

    // Despawn if too far from camera. Use a greater range to compensate
    // the spawner
    if((state->freepos->vx < camera.pos.vx - (SCREEN_XRES << 12))
       || (state->freepos->vx > camera.pos.vx + (SCREEN_XRES << 12))
       || (state->freepos->vy < camera.pos.vy - (SCREEN_YRES << 12))
       || (state->freepos->vy > camera.pos.vy + (SCREEN_YRES << 12))) {
        return OBJECT_DESPAWN;
    }

    // Otherwise this is a free object that should be updated
    return OBJECT_UPDATE_AS_FREE;
}
