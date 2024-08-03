#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include <psxgpu.h>

/* TODO: ALL LEVEL DATA SHOULD PROBABLY BE USING A LEVEL-WIDE ARENA ALLOCATOR. */

typedef struct {
    uint8_t floor[8];
    uint8_t rwall[8];
    uint8_t ceiling[8];
    uint8_t lwall[8];
} Collision;

typedef struct {
    uint16_t tile_width;
    uint16_t num_tiles;
    uint16_t frame_side;
    uint16_t *frames;

    Collision **collision;
} TileMapping;

typedef TileMapping TileMap16;
typedef TileMapping TileMap128;

typedef struct {
    uint8_t width;
    uint8_t height;
    uint16_t *tiles;
} LevelLayerData;

typedef struct {
    uint8_t num_layers;
    uint8_t _unused0;
    LevelLayerData *layers;

    uint16_t crectx, crecty;
    uint16_t prectx, precty;
    uint16_t clutmode, _unused1;
} LevelData;

typedef struct {
    uint8_t collided;
    uint8_t direction; // horizontal/vertical
    int16_t pushback;
} CollisionEvent;


void load_map(TileMapping *mapping, const char *filename, const char *collision_filename);
void free_map(TileMapping *mapping);

void load_lvl(LevelData *lvl, const char *filename);
void free_lvl(LevelData *lvl);

void render_lvl(
    LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
    int32_t cam_x, int32_t cam_y);


CollisionEvent linecast(LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
                        int32_t vx, int32_t vy, uint8_t direction,
                        int32_t magnitude);

#endif
