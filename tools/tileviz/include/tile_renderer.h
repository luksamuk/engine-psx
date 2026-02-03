// Tile renderer interface

#ifndef TILE_RENDERER_H
#define TILE_RENDERER_H

#include "tim_types.h"
#include "ps1_types.h"

// Tile rendering configuration
typedef struct {
    int width;
    int height;
    int tile_size;
    TIMFile* texture;
    PS1CLUT* palette;
    bool use_clut;
    bool dither_enable;
} TileRenderer;

// Create tile renderer
TileRenderer* tr_create(int width, int height, int tile_size);

// Set texture for rendering
void tr_set_texture(TileRenderer* renderer, TIMFile* texture);

// Set palette
void tr_set_palette(TileRenderer* renderer, PS1CLUT* palette);

// Render single tile to surface
void tr_render_tile(TileRenderer* renderer, uint16_t tile_index, int x, int y);

// Render entire level
void tr_render_level(TileRenderer* renderer, uint16_t* tile_data, int level_width, int level_height);

// Get tile size
int tr_get_tile_size(TileRenderer* renderer);

// Cleanup renderer
void tr_cleanup(TileRenderer* renderer);

#endif // TILE_RENDERER_H