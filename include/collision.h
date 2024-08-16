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

CollisionEvent linecast(LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
                        int32_t vx, int32_t vy, LinecastDirection direction,
                        uint8_t magnitude);

#endif
