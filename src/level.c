#include "level.h"
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "render.h"

void
load_map(TileMapping *mapping, const char *filename)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading MAP file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    mapping->tile_width = get_short_be(bytes, &b);
    mapping->num_tiles  = get_short_be(bytes, &b);
    mapping->frame_side = get_short_be(bytes, &b);

    uint32_t frames_per_tile = mapping->frame_side * mapping->frame_side;
    uint32_t total_frames = frames_per_tile * mapping->num_tiles;

    mapping->frames = malloc(total_frames * sizeof(uint16_t));
    printf("Total allocated frames: %lu B\n", total_frames * sizeof(uint16_t));
    for(uint32_t i = 0; i < total_frames; i++) {
        mapping->frames[i] = get_short_be(bytes, &b);
    }

    free(bytes);
}

void
free_map(TileMapping *mapping)
{
    free(mapping->frames);
}

void
load_lvl(LevelData *lvl, const char *filename)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading LVL file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    lvl->num_layers = get_byte(bytes, &b);
    lvl->_unused0 = 0; b += 1;

    lvl->layers = malloc(lvl->num_layers * sizeof(LevelLayerData));
    printf("Allocated layers table: %lu B\n", lvl->num_layers * sizeof(LevelLayerData));
    for(uint8_t n_layer = 0; n_layer < lvl->num_layers; n_layer++) {
        LevelLayerData *layer = &lvl->layers[n_layer];
        layer->width = get_byte(bytes, &b);
        layer->height = get_byte(bytes, &b);
        uint16_t num_tiles = (uint16_t)layer->width * (uint16_t)layer->height;
        layer->tiles = malloc(num_tiles * sizeof(uint16_t));
        printf("Allocated tiles for layer: %lu B\n", num_tiles * sizeof(uint16_t));
        for(uint16_t i = 0; i < num_tiles; i++) {
            layer->tiles[i] = get_short_be(bytes, &b);
        }
    }

    lvl->prectx   = 448;
    lvl->precty   = 0;
    lvl->crectx   = 448;
    lvl->crecty   = 256;
    lvl->clutmode = 0; // 4-bit CLUT
    lvl->_unused1 = 0;

    free(bytes);
}

void
free_lvl(LevelData *lvl)
{
    for(uint8_t i = 0; i < lvl->num_layers; i++) {
        free(lvl->layers[i].tiles);
    }
    free(lvl->layers);
}


void
_render_8(
    LevelData *lvl,
    int16_t vx, int16_t vy, int16_t otz,
    uint16_t frame)
{
    uint16_t
        v0_idx = frame >> 5,
        u0_idx = frame - (v0_idx << 5);
    uint8_t
        u0 = (u0_idx << 3),
        v0 = (v0_idx << 3);

    SPRT_8 *sprt = get_next_prim();
    increment_prim(sizeof(SPRT_8));
    setSprt8(sprt);
    setXY0(sprt, vx, vy);
    setRGB0(sprt, 128, 128, 128);
    setUV0(sprt, u0, v0);
    setClut(sprt, lvl->crectx, lvl->crecty);
    sort_prim(sprt, otz);
}

void
_render_16(
    LevelData *lvl, TileMap16 *map16,
    int16_t vx, int16_t vy, int16_t otz,
    uint16_t frame)
{
    // Frames per tile: 2 * 2 = 4
    uint16_t *tileframes = &map16->frames[frame << 2];
    for(int16_t idx = 0; idx < 4; idx++) {

        if(tileframes[idx] == 0) continue;


        int16_t
            deltax = (idx % 2),
            deltay = (idx >> 1);

        _render_8(
            lvl,
            vx + (deltax << 3),
            vy + (deltay << 3),
            otz,
            tileframes[idx]);
    }

    // Draw debug lines
    /* LINE_F2 line; */
    /* setLineF2(&line); */
    /* setXY2(&line, vx, vy, vx, vy + 16); */
    /* setRGB0(&line, 255, 255, 0); */
    /* DrawPrim((void *)&line); */
    /* setXY2(&line, vx, vy, vx + 16, vy); */
    /* DrawPrim((void *)&line); */
    /* setXY2(&line, vx + 16, vy, vx + 16, vy + 16); */
    /* DrawPrim((void *)&line); */
    /* setXY2(&line, vx, vy + 16, vx + 16, vy + 16); */
    /* DrawPrim((void *)&line); */
}

void
_render_128(
    LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
    int16_t vx, int16_t vy, int16_t otz,
    uint16_t frame)
{
    // 128x128 has 8x8 tiles of 16x16.
    // Since this is a constant, we will then write the optimized code
    // just like _render_16.
    // Frames per tile: 8 * 8 = 64 => rshift << 6
    uint16_t *tileframes = &map128->frames[frame << 6];
    for(int16_t idx = 0; idx < 64; idx++) {
        if(tileframes[idx] == 0) continue;

        int16_t
            deltax = (idx % 8),
            deltay = (idx >> 3);
        _render_16(lvl, map16,
                   vx + (deltax << 4),
                   vy + (deltay << 4),
                   otz,
                   tileframes[idx]);
    }

    // Draw debug lines
    /* LINE_F2 line; */
    /* setLineF2(&line); */
    /* setRGB0(&line, 0, 255, 255); */
    /* setXY2(&line, vx, vy, vx, vy + 128); */
    /* DrawPrim((void *)&line); */
    /* setXY2(&line, vx, vy, vx + 128, vy); */
    /* DrawPrim((void *)&line); */
    /* setXY2(&line, vx + 128, vy, vx + 128, vy + 128); */
    /* DrawPrim((void *)&line); */
    /* setXY2(&line, vx, vy + 128, vx + 128, vy + 128); */
    /* DrawPrim((void *)&line); */
}

#define CLAMP_SUM(X, N, MAX) ((X + N) > MAX ? MAX : (X + N))

void
_render_layer(
    LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
    int16_t vx, int16_t vy, int16_t otz,
    uint8_t layer)
{
    LevelLayerData *l = &lvl->layers[layer];
    // vx and vy are the camera center.
    // We need to use these values to calculate:
    // - what are the coordinates for top left corner;
    // - what is the tile (x, y) containing this top-left corner point;
    // - the deltas that we should apply relative to the own tile's top-left
    //   corner.

    // Correct vx / vy to the top left corner
    vx -= CENTERX; vy -= CENTERY;

    // Find tile X and Y indices. Integer division by 128
    int16_t tilex, tiley;
    tilex = vx >> 7; tiley = vy >> 7;

    // Find pixel deltas for X and Y coordinates that are figuratively
    // subtracted from each tile's top left corner
    int16_t deltax, deltay;
    deltax = vx - (tilex << 7);
    deltay = vy - (tiley << 7);

    // We need to figure out the number of tiles we need to draw on X
    // and Y coordinates. This is because we may well be extrapolating
    // those tiles!
    int16_t num_tiles_x, num_tiles_y;
    num_tiles_x = (SCREEN_XRES >> 7) + 1;
    num_tiles_y = (SCREEN_YRES >> 7) + 1;

    // Clamp number of tiles
    int16_t max_tile_x, max_tile_y;
    max_tile_x = CLAMP_SUM(tilex, num_tiles_x, (int16_t)l->width);
    max_tile_y = CLAMP_SUM(tiley, num_tiles_y, (int16_t)l->height);

    // Now iterate over tiles and render them.
    uint16_t a = 0;
    for(int16_t iy = tiley; iy <= max_tile_y; iy++) {
        for(int16_t ix = tilex; ix <= max_tile_x; ix++) {
            a++;
            int16_t frame_idx = (iy * l->width) + ix;
            //printf("Chunk: %d @ %u (%u)\n", a, frame_idx, l->tiles[frame_idx]);
            _render_128(lvl, map128, map16,
                        ((ix - tilex) << 7) - deltax,
                        ((iy - tiley) << 7) - deltay,
                        otz,
                        l->tiles[frame_idx]);
        }
    }
}

void
render_lvl(
    LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
    int32_t cam_x, int32_t cam_y)
{
    int16_t cx = (cam_x >> 12),
        cy = (cam_y >> 12);
    _render_layer(lvl, map128, map16, cx, cy, 4, 0);
    _render_layer(lvl, map128, map16, cx, cy, 2, 1);

    DR_TPAGE *tpage = get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpage, 0, 1, getTPage(0, 1, lvl->prectx, lvl->precty));
    sort_prim(tpage, OT_LENGTH - 1);
}
