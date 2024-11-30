#ifndef COLLISION_H
#define COLLISION_H

#include "level.h"

typedef struct {
    uint8_t collided;
    int32_t coord;
    int32_t angle;
} CollisionEvent;

typedef enum {
    CDIR_FLOOR   = 0,
    CDIR_RWALL   = 1,
    CDIR_CEILING = 2,
    CDIR_LWALL   = 3,
} LinecastDirection;

typedef enum {
    OBJ_SIDE_NONE   = 0,
    OBJ_SIDE_LEFT   = 1,
    OBJ_SIDE_RIGHT  = 2,
    OBJ_SIDE_TOP    = 3,
    OBJ_SIDE_BOTTOM = 4,
} ObjectCollision;

CollisionEvent linecast(LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
                        int32_t vx, int32_t vy, LinecastDirection direction,
                        uint8_t magnitude, LinecastDirection floor_direction);


/* Simpler collision detection algorithms */

// Intersection between two axis-aligned bounding boxes (AABBs).
int
aabb_intersects(int32_t a_vx, int32_t a_vy, int32_t aw, int32_t ah,
                int32_t b_vx, int32_t b_vy, int32_t bw, int32_t bh);

// Intersection between player and object.
// Object collision side is always given with respect to the second informed
// hitbox (this assumes that the first hitbox is the player's).
ObjectCollision
hitbox_collision(int32_t p_vx, int32_t p_vy, int32_t pw, int32_t ph,
                 int32_t o_vx, int32_t o_vy, int32_t ow, int32_t oh);

#endif
