#include <stdlib.h>
#include "object.h"
#include "object_state.h"
#include "camera.h"
#include "render.h"
#include "sound.h"

#define OBJ_MIN_SPAWN_DIST_X (CENTERX + (CENTERX >> 1))
#define OBJ_MIN_SPAWN_DIST_Y (CENTERY + (CENTERY >> 1))

extern Player player;
extern Camera camera;
extern int32_t player_vx, player_vy; // Top left corner of player hitbox
extern uint8_t player_attacking;
extern int32_t player_width;
extern int32_t player_height;

extern uint32_t level_score_count;
extern int      debug_mode;

extern SoundEffect sfx_pop;

uint8_t
object_should_despawn(ObjectState *state)
{
    // Despawn if too far from camera.
    return
        ((state->freepos->vx < camera.pos.vx - (SCREEN_XRES << 12))
         || (state->freepos->vx > camera.pos.vx + (SCREEN_XRES << 12))
         || (state->freepos->vy < camera.pos.vy - (SCREEN_YRES << 12))
         || (state->freepos->vy > camera.pos.vy + (SCREEN_YRES << 12)));
}

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

    // Despawn if too far from camera.
    if(object_should_despawn(state)) {
        return OBJECT_DESPAWN;
    }

    // Otherwise this is a free object that should be updated
    return OBJECT_UPDATE_AS_FREE;
}


ObjectBehaviour
enemy_player_interaction(ObjectState *state, RECT *hitbox, VECTOR *pos)
{
    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                       hitbox->x, hitbox->y, hitbox->w, hitbox->h))
    {
        if(player_attacking) {
            state->props |= OBJ_FLAG_DESTROYED;
            if(state->parent) {
                state->parent->props |= OBJ_FLAG_DESTROYED;
                state->parent->parent = NULL;
            }
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
            return OBJECT_DESPAWN;
        } else {
            if(player.action != ACTION_HURT && player.iframes == 0) {
                player_do_damage(&player, pos->vx << 12);
            }
        }
    }
    return OBJECT_DO_NOTHING;
}

void
hazard_player_interaction(RECT *hitbox, VECTOR *pos)
{
    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                       hitbox->x, hitbox->y, hitbox->w, hitbox->h))
    {
        if(player.action != ACTION_HURT && player.iframes == 0) {
            player_do_damage(&player, pos->vx << 12);
        }
    }
}

void
solid_object_player_interaction(RECT *solidity)
{
    if(debug_mode > 1) draw_collision_hitbox(solidity->x, solidity->y, solidity->w, solidity->h);

    VECTOR player_center = {
        .vx = player_vx + (player_width >> 1),
        .vy = player_vy + (player_height >> 1),
    };

    VECTOR object_center = {
        .vx = solidity->x + (solidity->w >> 1),
        .vy = solidity->y + (solidity->h >> 1),
    };

    int32_t combined_x_radius = (solidity->w >> 1) + PUSH_RADIUS + 1;
    int32_t combined_y_radius = (solidity->h >> 1) + (player_height >> 1);
    int32_t combined_x_diameter = (combined_x_radius << 1);
    int32_t combined_y_diameter = (combined_y_radius << 1);

    // 1. Check if standing on top of object.
    if(player.over_object) {
        int32_t x_left_distance = (player_center.vx - object_center.vx) + combined_x_radius;
        if((x_left_distance < 0) || (x_left_distance > combined_x_diameter)) {
            player.over_object = 0;
            player.grnd = 0;
        }
        return;
    }


    // Horizontal overlap
    int32_t left_difference = (player_center.vx - object_center.vx) + combined_x_radius;

    // The player is too far to the left to be touching? or...
    // the player is too far to the right to be touching?
    if((left_difference < 0) || (left_difference > combined_x_diameter))
       return;
    // The player is overlapping on X axis, and it will continue.

    // Vertical overlap
    int32_t top_difference = (player_center.vy - object_center.vy) + 4 + combined_y_radius;
    
    // Is the player too far above to be touching? or...
    // is the player too far down to be touching?
    if((top_difference < 0) || (top_difference > combined_y_diameter))
        return;
        
    // Find direction of collision.
    // Directions will be known through the signs of x_distance and y_distance.

    // Horizontal edge distance
    int32_t x_distance;
    if(player_center.vx > object_center.vx) {
        // Player is on the right: flipped, will be a negative number
        x_distance = left_difference - combined_x_diameter;
    } else {
        // Player is on the left
        x_distance = left_difference;
    }

    // Vertical edge distance
    int32_t y_distance;
    if(player_center.vy > object_center.vy) {
        // Player is on the bottom; flipped, will be a negative number, minus extra 4px
        y_distance = top_difference - 4 - combined_y_diameter;
    } else {
        // Player is on top
        y_distance = top_difference;
    }

    if((abs(x_distance) > abs(y_distance)) || (abs(y_distance) <= 4)) {
        // Collide vertically.

        // Pop downwards
        if(y_distance < 0) {
            // If not moving vertically and standing on the ground,
            // get crushed
            //if((player.vel.vy == 0) && (player.grnd)) {} // TODO
            // Else...
            if(player.vel.vy >= 0) return;
            player.pos.vy -= (y_distance << 12);
            player.vel.vy = 0;
        } else if(y_distance > 0){
            // Popped upwards: Land on object
            // y_distance must not be larger or equal to 16
            if(y_distance >= 16) return;
            y_distance -= 4; // Subtract 4px added earlier

            // Forget the combined_x_radius; use the actual width radius of
            // the object, not combined with anything at all
            combined_x_radius = solidity->w >> 1;
            combined_x_diameter = solidity->w;

            // Get a distance from the player's X position to the object's
            // right edge
            int32_t x_comparison = object_center.vx - player_center.vx + combined_x_radius;

            // If the player is too far to the right; or...
            // If the player is too far to the left...
            if((x_comparison < 0) || (x_comparison >= combined_x_diameter))
                return;

            // If player is moving upwards, cancel too
            if(player.vel.vy < 0) return;

            // Land over object
            player.pos.vy -= (y_distance + 1) << 12;
            player.grnd = 1;
            player.vel.vy = 0;
            player.angle = 0;
            player.over_object = 1;
            player.vel.vz = player.vel.vx;
        }
    } else {
        // Collide horizontally
        // Do not affect speed if x_distance is zero.
        if((x_distance != 0)
           && (((x_distance > 0) && (player.vel.vx > 0))
               || ((x_distance < 0) && (player.vel.vx < 0))))
        {
            player.vel.vx = player.vel.vz = 0;
            if(player.grnd) player.push = 1;
        }
        player.pos.vx -= (x_distance << 12);
    }
}
