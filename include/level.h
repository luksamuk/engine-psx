#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include <psxgpu.h>

#include "object_state.h"

#define LEVEL_MAX_X_CHUNKS   255
#define LEVEL_MAX_Y_CHUNKS    31
#define LEVEL_ARENA_SIZE  131072

typedef struct {
    uint8_t floor[8];
    uint8_t rwall[8];
    uint8_t ceiling[8];
    uint8_t lwall[8];

    int32_t floor_angle;
    int32_t rwall_angle;
    int32_t ceiling_angle;
    int32_t lwall_angle;
} Collision;

typedef struct {
    uint16_t tile_width;
    uint16_t num_tiles;
    uint16_t frame_side;
    uint16_t *frames;
    Collision **collision;
} TileMap16;

typedef struct {
    uint16_t index;
    uint8_t  props;
    uint8_t  _unused;
} Frame128;

typedef struct {
    uint16_t tile_width;
    uint16_t num_tiles;
    uint16_t frame_side;
    Frame128 *frames;
} TileMap128;

#define MAP128_PROP_SOLID  0
#define MAP128_PROP_ONEWAY 1
#define MAP128_PROP_NONE   2

typedef struct {
    uint8_t width;
    uint8_t height;
    uint16_t *tiles;
} LevelLayerData;

#define MAX_OBJECTS_PER_CHUNK 15

typedef struct {
    uint8_t num_objects;
    ObjectState objects[MAX_OBJECTS_PER_CHUNK];
} ChunkObjectData;

typedef struct {
    uint8_t num_layers;
    uint8_t _unused0;
    LevelLayerData *layers;
    ChunkObjectData **objects;

    uint16_t crectx, crecty;
    uint16_t prectx, precty;
    uint16_t clutmode, _unused1;
} LevelData;

void level_init();
void level_reset();
void level_debrief();

void load_map16(TileMap16 *mapping, const char *filename, const char *collision_filename);
void load_map128(TileMap128 *mapping, const char *filename);
void load_lvl(LevelData *lvl, const char *filename);

void prepare_renderer(LevelData *lvl);
void render_lvl(
    LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
    int32_t cam_x, int32_t cam_y);


// Object-related. These are defined in object_state.c
void load_object_placement(const char *filename, LevelData *lvl);

#endif
