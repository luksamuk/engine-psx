#include <stdlib.h>
#include "object.h"
#include "object_state.h"
#include "camera.h"
#include "render.h"
#include "sound.h"

#define OBJ_MIN_SPAWN_DIST_X (CENTERX + (CENTERX >> 1))
#define OBJ_MIN_SPAWN_DIST_Y (CENTERY + (CENTERY >> 1))

extern Player *player;
extern Camera *camera;
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
    // Despawn if too far from camera->
    return
        ((state->freepos->vx < camera->pos.vx - (SCREEN_XRES << 12))
         || (state->freepos->vx > camera->pos.vx + (SCREEN_XRES << 12))
         || (state->freepos->vy < camera->pos.vy - (SCREEN_YRES << 12))
         || (state->freepos->vy > camera->pos.vy + (SCREEN_YRES << 12)));
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
               (pos->vx > (camera->pos.vx >> 12) - SCREEN_XRES)
               && (pos->vx < (camera->pos.vx >> 12) + SCREEN_XRES)
               && (pos->vy > (camera->pos.vy >> 12) - SCREEN_YRES)
               && (pos->vy < (camera->pos.vy >> 12) + SCREEN_YRES)
            ) && (
                // Outside center screen
                (pos->vx < (camera->pos.vx >> 12) - OBJ_MIN_SPAWN_DIST_X)
                || (pos->vx > (camera->pos.vx >> 12) + OBJ_MIN_SPAWN_DIST_X)
                || (pos->vy < (camera->pos.vy >> 12) - OBJ_MIN_SPAWN_DIST_Y)
                || (pos->vy > (camera->pos.vy >> 12) + OBJ_MIN_SPAWN_DIST_Y)
                )
            )
        {
            return OBJECT_SPAWNER_CREATE_FREE;
        }
        return OBJECT_SPAWNER_ABORT_BEHAVIOUR;
    }

    // Despawn if too far from camera->
    if(object_should_despawn(state)) {
        return OBJECT_DESPAWN;
    }

    // Otherwise this is a free object that should be updated
    return OBJECT_UPDATE_AS_FREE;
}

static void
_enemy_do_destruction(ObjectState *state, VECTOR *pos)
{
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

     // Animal
     PoolObject *animal = object_pool_create(OBJ_ANIMAL);
     animal->state.subtype = rand() % 4;
     animal->freepos.vx = (pos->vx << 12);
     animal->freepos.vy = (pos->vy << 12);
     animal->state.flipmask |= MASK_FLIP_FLIPX;
     animal->freepos.spdy = -0x05000;
}

RECT
player_get_extra_hitbox(uint8_t *exists)
{
    RECT hitbox = { 0 };
    *exists = 0;
    if((player->action == ACTION_PIKOPIKO) && (player->framecount < 11))
    {
        *exists = 1;
        hitbox = (RECT){
            .x = player_vx + ((player->anim_dir > 0) ? 5 : -20),
            .y = player_vy - 12,
            .w = 30,
            .h = 42,
        };
    } else if(player->action == ACTION_PIKOSPIN) {
        *exists = 1;
        hitbox = (RECT){
            .x = player_vx - 12,
            .y = player_vy - 14,
            .w = 40,
            .h = 40,
        };
    }
    return hitbox;
}

ObjectBehaviour
enemy_player_interaction(ObjectState *state, RECT *hitbox, VECTOR *pos)
{
    if(player->death_type > 0) {
        return OBJECT_DO_NOTHING;
    }

    if(aabb_intersects(player_vx, player_vy, player_width, player_height,
                       hitbox->x, hitbox->y, hitbox->w, hitbox->h))
    {
        if(player_attacking) {
            _enemy_do_destruction(state, pos);

            if(!player->grnd && player->vel.vy > 0) {
                player->vel.vy *= -1;
            }
            return OBJECT_DESPAWN;
        } else {
            if(player->action != ACTION_HURT && player->iframes == 0) {
                player_do_damage(player, pos->vx << 12);
            }
        }
    }

    // Check extra hitbox intersection.
    // This is where we configure hitboxes such as Amy Rose's hammer
    uint8_t extra_check = 0;
    RECT extra_hitbox = player_get_extra_hitbox(&extra_check);

    if(extra_check) {
        if(debug_mode > 1)
            draw_collision_hitbox(
                extra_hitbox.x,
                extra_hitbox.y,
                extra_hitbox.w,
                extra_hitbox.h);

        if(aabb_intersects(extra_hitbox.x, extra_hitbox.y,
                           extra_hitbox.w, extra_hitbox.h,
                           hitbox->x, hitbox->y, hitbox->w, hitbox->h))
        {
            _enemy_do_destruction(state, pos);
            if(player->action == ACTION_PIKOSPIN && player->vel.vy > 0) {
                player->vel.vy *= -1;
            }
            return OBJECT_DESPAWN;
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
        if(player->action != ACTION_HURT && player->iframes == 0) {
            player_do_damage(player, pos->vx << 12);
        }
    }
}

ObjectCollision
solid_object_player_interaction(ObjectState *obj, FRECT *box, uint8_t is_platform)
{
    if(player->death_type > 0) {
        player->pushed_object = player->over_object = NULL;
        return OBJ_SIDE_NONE;
    }

    if(debug_mode > 1) draw_collision_hitbox(box->x >> 12, box->y >> 12, box->w >> 12, box->h >> 12);

    // Player center calculated more or less like in object_update
    VECTOR player_center = {
        .vx = player->pos.vx,
        .vy = player->pos.vy,
    };

    VECTOR object_center = {
        .vx = box->x + (box->w >> 1),
        .vy = box->y + (box->h >> 1),
    };

    int32_t combined_x_radius = (box->w >> 1) + ((PUSH_RADIUS + 1) << 12);
    int32_t combined_y_radius = (box->h >> 1) + (player_height << 11);
    int32_t combined_x_diameter = (combined_x_radius << 1);
    int32_t combined_y_diameter = (combined_y_radius << 1);


    // If we're standing over the current object, do something about this
    if(player->over_object == obj) {
        int32_t x_left_distance = (player_center.vx - object_center.vx) + combined_x_radius;
        if((x_left_distance < 0) || (x_left_distance >= combined_x_diameter)) {
            player->over_object = NULL;
            player->grnd = 0;
            return OBJ_SIDE_NONE;
        } else {
            // Balance on ledges
            int32_t left_difference = (player_center.vx - object_center.vx) + combined_x_radius;
            if(player_center.vx > object_center.vx) {
                left_difference = left_difference - combined_x_diameter;
            }
            player->col_ledge = (abs(left_difference) >= (12 << 12));
            if(!player->col_ledge) {
                if(left_difference < 0)
                    player->ev_grnd1.collided = 1;
                else player->ev_grnd2.collided = 1;
            }
        }
        return OBJ_SIDE_TOP;
    }


    // Horizontal overlap
    int32_t left_difference = (player_center.vx - object_center.vx) + combined_x_radius;
    // Cancel if player is too far to the left or the right to be touching object
    if((left_difference < 0) || (left_difference > combined_x_diameter)) {
        if(player->pushed_object == obj) player->pushed_object = NULL;
        return OBJ_SIDE_NONE;
    }

    // Vertical overlap
    int32_t top_difference = (player_center.vy - object_center.vy) + (4 << 12) + combined_y_radius;
    // Cancel if player is too far to the top or bottom to be touching object
    if((top_difference < 0) || (top_difference > combined_y_diameter))
        return OBJ_SIDE_NONE;

    
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
        y_distance = top_difference - (4 << 12) - combined_y_diameter;
    } else {
        // Player is on top
        y_distance = top_difference;
    }

    if((abs(x_distance) > abs(y_distance)) || (abs(y_distance) <= (8 << 12)) || is_platform) {
        // Collide vertically.

        // Pop downwards
        if(!is_platform && (y_distance < 0)) {
            // If not moving vertically and standing on the ground,
            // get crushed
            if((player->vel.vy == 0) && (player->grnd)) {
                player_do_die(player, PLAYER_DEATH_NORMAL);
                return OBJ_SIDE_BOTTOM;
            }
            if(player->vel.vy >= 0) return OBJ_SIDE_NONE;
            player->pos.vy -= y_distance;
            player->vel.vy = 0;
            return OBJ_SIDE_BOTTOM;
        } else {
            // Popped upwards: Land on object
            // y_distance must not be larger or equal to 16
            if(y_distance >= (16 << 12)) return OBJ_SIDE_NONE;
            y_distance -= (4 << 12); // Subtract 4px added earlier

            // Forget the combined_x_radius; use the actual width radius of
            // the object, not combined with anything at all
            combined_x_radius = box->w >> 1;
            combined_x_diameter = box->w;

            // Get a distance from the player's X position to the object's
            // right edge
            int32_t x_comparison = object_center.vx - player_center.vx + combined_x_radius;

            // If the player is too far to the right; or...
            // If the player is too far to the left...
            if((x_comparison < 0) || (x_comparison >= combined_x_diameter))
                return OBJ_SIDE_NONE;

            // If player is moving upwards, cancel too
            if(player->vel.vy < 0) return OBJ_SIDE_NONE;

            // Land over object
            if(!is_platform)
                player->pos.vy -= y_distance + ONE;
            else
                player->pos.vy = object_center.vy - (box->h >> 1) - (player_height << 11);
            player->grnd = 1;
            player->vel.vy = 0;
            player->angle = 0;
            player->over_object = obj;
            player->vel.vz = player->vel.vx;
            player_do_dropdash(player);
            return OBJ_SIDE_TOP;
        }
    } else {
        // Collide horizontally
        // Do not affect speed if x_distance is zero.
        if((x_distance != 0)
           && (((x_distance > 0) && (player->vel.vx > 0))
               || ((x_distance < 0) && (player->vel.vx < 0))))
        {
            player->vel.vx = player->vel.vz = 0;
            if(player->grnd) player->pushed_object = obj;
        }
        player->pos.vx -= x_distance;
        return (x_distance < 0) ? OBJ_SIDE_RIGHT : OBJ_SIDE_LEFT;
    }
}
