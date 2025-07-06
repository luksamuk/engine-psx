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

CollisionEvent linecast(int32_t vx, int32_t vy, LinecastDirection direction,
                        uint8_t magnitude, LinecastDirection floor_direction);


/* Simpler collision detection algorithms */

void draw_collision_hitbox(int32_t vx, int32_t vy, int32_t w, int32_t h);

// Intersection between two axis-aligned bounding boxes (AABBs).
int
aabb_intersects(int32_t a_vx, int32_t a_vy, int32_t aw, int32_t ah,
                int32_t b_vx, int32_t b_vy, int32_t bw, int32_t bh);

#endif
