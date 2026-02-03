// Level data loader interface

#ifndef LEVEL_LOADER_H
#define LEVEL_LOADER_H

#include "ps1_types.h"

// Level file structure
typedef struct {
    char name[64];
    char description[256];
    uint16_t* tile_data;
    uint32_t width_tiles;
    uint32_t height_tiles;
    uint64_t file_size;
} LevelData;

// Load level file
LevelData* level_load(const char* level_path);

// Free level data
void level_free(LevelData* level);

// Get tile at specific position
uint16_t level_get_tile(LevelData* level, int tile_x, int tile_y);

#endif // LEVEL_LOADER_H