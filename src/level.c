#include "level.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "render.h"
#include "memalloc.h"

#include "object.h"

#define MAX_TILES 1400

ArenaAllocator _level_arena = { 0 };
static uint8_t _arena_mem[LEVEL_ARENA_SIZE];

extern int debug_mode;
extern uint8_t level_fade;

void
level_init()
{
    alloc_arena_init(&_level_arena, &_arena_mem, LEVEL_ARENA_SIZE);
}

void
level_reset()
{
    alloc_arena_free(&_level_arena);
}

void
level_debrief()
{
    printf("Arena start address: 0x%08x\n"
           "Arena end address:   0x%08x\n"
           "Arena bytes size:    %u\n"
           "Arena bytes used:    %u\n"
           "Arena bytes free:    %u\n",
           _level_arena.start,
           _level_arena.start + _level_arena.size,
           _level_arena.size,
           alloc_arena_bytes_used(&_level_arena),
           alloc_arena_bytes_free(&_level_arena));
}

#define TILECLIP(sz) \
    { \
        if(vx <= -sz || vy <= -sz || vx >= SCREEN_XRES + sz || vy >= SCREEN_YRES + sz) \
            return;\
    }

void
_load_collision(TileMap16 *mapping, const char *filename)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading COLLISION file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    uint16_t num_tiles = get_short_be(bytes, &b);

    for(uint16_t i = 0; i < num_tiles; i++) {
        uint16_t tile_id = get_short_be(bytes, &b);
        mapping->collision[tile_id] = alloc_arena_malloc(&_level_arena, sizeof(Collision));
        Collision *collision = mapping->collision[tile_id];

        collision->floor_angle = (int32_t)get_long_be(bytes, &b);
        for(int j = 0; j < 8; j++)
            collision->floor[j] = get_byte(bytes, &b);

        collision->rwall_angle = (int32_t)get_long_be(bytes, &b);
        for(int j = 0; j < 8; j++)
            collision->rwall[j] = get_byte(bytes, &b);

        collision->ceiling_angle = (int32_t)get_long_be(bytes, &b);
        for(int j = 0; j < 8; j++)
            collision->ceiling[j] = get_byte(bytes, &b);

        collision->lwall_angle = (int32_t)get_long_be(bytes, &b);
        for(int j = 0; j < 8; j++)
            collision->lwall[j] = get_byte(bytes, &b);
    }

    free(bytes);
}

void
load_map16(TileMap16 *mapping, const char *filename, const char *collision_filename)
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

    mapping->frames = alloc_arena_malloc(&_level_arena, total_frames * sizeof(uint16_t));
    for(uint32_t i = 0; i < total_frames; i++) {
        mapping->frames[i] = get_short_be(bytes, &b);
    }

    free(bytes);

    if(!collision_filename) {
        mapping->collision = NULL;
        return;
    }

    // Load collision data
    mapping->collision = alloc_arena_malloc(&_level_arena, (mapping->num_tiles + 1) * sizeof(Collision *));
    for(uint16_t i = 0; i < mapping->num_tiles; i++) {
        mapping->collision[i] = NULL;
    }
    _load_collision(mapping, collision_filename);
}

void
load_map128(TileMap128 *mapping, const char *filename)
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

    mapping->frames = alloc_arena_malloc(&_level_arena, total_frames * sizeof(Frame128));
    for(uint32_t i = 0; i < total_frames; i++) {
        mapping->frames[i].index = get_short_be(bytes, &b);
        mapping->frames[i].props = get_byte(bytes, &b);
        mapping->frames[i]._unused = 0;
    }

    free(bytes);
}

void
load_lvl(LevelData *lvl, const char *filename)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        return;
    }

    b = 0;

    lvl->num_layers = get_byte(bytes, &b);
    lvl->_unused0 = 0; b += 1;

    uint16_t max_tiles = 0;

    lvl->layers = alloc_arena_malloc(&_level_arena, lvl->num_layers * sizeof(LevelLayerData));
    for(uint8_t n_layer = 0; n_layer < lvl->num_layers; n_layer++) {
        LevelLayerData *layer = &lvl->layers[n_layer];
        layer->width = get_byte(bytes, &b);
        layer->height = get_byte(bytes, &b);
        uint16_t num_tiles = (uint16_t)layer->width * (uint16_t)layer->height;
        if(num_tiles > max_tiles) max_tiles = num_tiles;
        layer->tiles = alloc_arena_malloc(&_level_arena, num_tiles * sizeof(uint16_t));
        for(uint16_t i = 0; i < num_tiles; i++) {
            layer->tiles[i] = get_short_be(bytes, &b);
        }
    }

    free(bytes);

    // These values are static because we're always using the same
    // coordinates for tpage and clut info. For some reason they're
    // not loading correctly, but we don't really need them
    lvl->prectx   = 448;
    lvl->precty   = 0;
    lvl->crectx   = 0;
    lvl->crecty   = 482;
    //lvl->clutmode = 0; // NOTE: This was set to tim->mode previously.
    lvl->_unused1 = 0;

    // Initialize object state array within level map
    printf("Allocating object array\n");
    lvl->objects = alloc_arena_malloc(&_level_arena, max_tiles * sizeof(ChunkObjectData *));
    for(uint16_t i = 0; i < max_tiles; i++) {
        lvl->objects[i] = NULL;
    }
}

// Level sprite buffer.
// We simply cannot afford to have so much information passing
// around all the time, so we pre-configure 1300 8x8 sprites to be used
// as level tiles and that's it.
static uint16_t _numsprites = 0;
static uint8_t  _current_spritebuf = 0;
static SPRT_8   _sprites[2][MAX_TILES];

void
_render_8(
    int16_t vx, int16_t vy, int16_t otz,
    uint16_t frame)
{
    // Clipping
    TILECLIP(8);

    // Calculate whether we want to use upper or lower texture page.
    // First page goes 0 ~ 1023. Anything above that is on page 1
    /* uint16_t page = frame >> 10; */
    /* frame &= 0x3ff; */

    uint16_t
        v0_idx = frame >> 5,
        u0_idx = frame - (v0_idx << 5);
    uint8_t
        u0 = (u0_idx << 3),
        v0 = (v0_idx << 3);

    assert(_numsprites < MAX_TILES);
    SPRT_8 *sprt = &_sprites[_current_spritebuf ^ 1][_numsprites++];
    setXY0(sprt, vx, vy);
    setUV0(sprt, u0, v0);
    if(sprt->r0 != level_fade) setRGB0(sprt, level_fade, level_fade, level_fade);
    sort_prim(sprt, otz);
}

void
_render_16(
    TileMap16 *map16,
    int16_t vx, int16_t vy, int16_t otz,
    uint16_t frame)
{
    // Clipping
    TILECLIP(16);

    // Frames per tile: 2 * 2 = 4
    uint16_t *tileframes = &map16->frames[frame << 2];
    for(int16_t idx = 0; idx < 4; idx++) {
        if(tileframes[idx] == 0) continue;

        int16_t
            //deltax = (idx % 2),
            deltax = (idx & 0x01),
            deltay = (idx >> 1);

        _render_8(
            vx + (deltax << 3),
            vy + (deltay << 3),
            otz,
            tileframes[idx]);
    }
}

void
_render_128(
    TileMap128 *map128, TileMap16 *map16,
    int16_t vx, int16_t vy, int16_t otz,
    uint16_t frame)
{
    // Clipping
    TILECLIP(128);

    // 128x128 has 8x8 tiles of 16x16.
    // Since this is a constant, we will then write the optimized code
    // just like _render_16.
    // Frames per tile: 8 * 8 = 64 => rshift << 6
    Frame128 *tileframes = &map128->frames[frame << 6];
    for(int16_t idx = 0; idx < 64; idx++) {
        if(tileframes[idx].index == 0) continue;

        int16_t
            //deltax = (idx % 8),
            deltax = (idx & 0x07),
            deltay = (idx >> 3);
        _render_16(
            map16,
            vx + (deltax << 4),
            vy + (deltay << 4),
            otz,
            tileframes[idx].index);
    }
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
    for(int16_t iy = tiley; iy <= max_tile_y; iy++) {
        for(int16_t ix = tilex; ix <= max_tile_x; ix++) {
            int16_t frame_idx = (iy * l->width) + ix;
            if(l->tiles[frame_idx] == 0) continue;

            _render_128(//lvl,
                        map128, map16,
                        ((ix - tilex) << 7) - deltax,
                        ((iy - tiley) << 7) - deltay,
                        otz,
                        l->tiles[frame_idx]);
        }
    }
}

void
prepare_renderer(LevelData *lvl)
{
    for(int i = 0; i < MAX_TILES; i++) {
        SPRT_8 *sprt = &_sprites[0][i];
        setSprt8(sprt);
        setRGB0(sprt, level_fade, level_fade, level_fade);
        setClut(sprt, lvl->crectx, lvl->crecty);
    }
    for(int i = 0; i < MAX_TILES; i++) {
        SPRT_8 *sprt = &_sprites[1][i];
        setSprt8(sprt);
        setRGB0(sprt, level_fade, level_fade, level_fade);
        setClut(sprt, lvl->crectx, lvl->crecty);
    }
    _current_spritebuf = 0;
}

inline int32_t
get_chunk_pos(int32_t vx, int32_t vy, int32_t lvlwidth, int32_t lvlheight)
{
    int32_t cx = vx >> 7;
    int32_t cy = vy >> 7;
    int32_t chunk_pos;
    if((cx < 0) || (cx >= lvlwidth)) chunk_pos = -1;
    else if((cy < 0) || (cy >= lvlheight)) chunk_pos = -1;
    else chunk_pos = (cy * lvlwidth) + cx;

    return chunk_pos;
}

void
_render_obj(ObjectState *obj, ObjectTableEntry *typedata,
            int32_t cx, int32_t cy, int32_t tx, int32_t ty)
{
    if((obj->props & OBJ_FLAG_DESTROYED) || (obj->props & OBJ_FLAG_INVISIBLE))
        return;

    // cx = camera center x
    // cy = camera center y
    // tx = chunk x index on map
    // ty = chunk y index on map

    // Calculate object center/bottom
    int16_t vx = (int16_t)(tx << 7) + obj->rx;
    int16_t vy = (int16_t)(ty << 7) + obj->ry;

    // Now that vx and vy are the world's absolute world positions,
    // all we need to do is subtrack the camera position from it

    // Calculate screen position
    cx -= CENTERX; cy -= CENTERY;
    int16_t px = vx - cx;
    int16_t py = vy - cy;

    // Debug render
    if(debug_mode > 1) {
        POLY_F4 *poly = (POLY_F4 *)get_next_prim();
        setPolyF4(poly);
        increment_prim(sizeof(POLY_F4));
        setSemiTrans(poly, 1);
        setRGB0(poly, 128, 0, 0);
        setXYWH(poly, px - 4, py - 4, 8, 8);
        sort_prim(poly, OTZ_LAYER_OBJECTS);
    }

    object_render(obj, typedata, px, py);
}

extern int player_hitbox_shown;

void
update_obj_window(LevelData *lvl, ObjectTable *tbl, int32_t cam_x, int32_t cam_y)
{
    cam_x = cam_x >> 12;
    cam_y = cam_y >> 12;
    player_hitbox_shown = 0;
    
    for(int32_t i = 0; i < 5; i++) {
        int32_t new_cam_x = cam_x - 256 + (i * 128);

        for(int32_t j = 0; j < 5; j++) {
            int32_t new_cam_y = cam_y - 256 + (j * 128);

            int32_t cx = new_cam_x >> 7;
            int32_t cy = new_cam_y >> 7;
            int32_t chunk_pos;
            if((cx < 0) || (cx >= lvl->layers[0].width)) chunk_pos = -1;
            else if((cy < 0) || (cy >= lvl->layers[0].height)) chunk_pos = -1;
            else chunk_pos = (cy * lvl->layers[0].width) + cx;

            if(chunk_pos > 0) {
                ChunkObjectData *objdata = lvl->objects[chunk_pos];
                if(objdata) {
                    for(uint8_t k = 0; k < objdata->num_objects; k++) {
                        ObjectState *obj = &objdata->objects[k];
                        ObjectTableEntry *typedata = &tbl->entries[obj->id];
                        VECTOR pos = {
                            .vx = (int32_t)(cx << 7) + (int32_t)obj->rx,
                            .vy = (int32_t)(cy << 7) + (int32_t)obj->ry,
                            .vz = 0
                        };
                        object_update(obj, typedata, &pos);
                    }
                }
            }
        }
    }
}

void
_render_obj_window(LevelData *lvl, ObjectTable *tbl, int32_t cx, int32_t cy)
{
    int32_t chunk;
    int32_t w = lvl->layers[0].width;
    int32_t h = lvl->layers[0].height;

#define _DO_RENDER(x, y)                                                 \
    chunk = get_chunk_pos(x, y, w, h);                                   \
    if(chunk > 0) {                                                      \
        ChunkObjectData *objdata = lvl->objects[chunk];                  \
        if(objdata) {                                                    \
            for(uint8_t i = 0; i < objdata->num_objects; i++) {          \
                ObjectState *obj = &objdata->objects[i];                 \
                ObjectTableEntry *typedata = &tbl->entries[obj->id];     \
                _render_obj(obj, typedata, cx, cy, (x) >> 7, (y) >> 7);  \
            }                                                            \
        }                                                                \
    }

    // Render a 5x5 grid of objects.
    for(int32_t i = 0; i < 5; i++) {
        int32_t new_cx = cx - 256 + (i * 128);
        for(int32_t j = 0; j < 5; j++) {
            int32_t new_cy = cy - 256 + (j * 128);
            _DO_RENDER(new_cx, new_cy);
        }
    }
}

void
render_lvl(
    LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
    ObjectTable *tbl,
    int32_t cam_x, int32_t cam_y)
{
    _numsprites = 0;
    _current_spritebuf = !_current_spritebuf;

    int16_t
        cx = (cam_x >> 12),
        cy = (cam_y >> 12);

    if(lvl->num_layers > 0)
        _render_layer(lvl, map128, map16, cx, cy, OTZ_LAYER_LEVEL_FG_BACK, 0);

    // TODO: Add front layer rendered at OTZ_LAYER_LEVEL_FG_FRONT
    // TODO: Fix level mapping tool to consider only mapping layers
    /* if(lvl->num_layers > 1) */
    /*     _render_layer(lvl, map128, map16, cx, cy, OTZ_LAYER_LEVEL_FG_FRONT, 1); */

    // Texture TPAGE info for level foreground (back tiles)
    DR_TPAGE *tpage = get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpage, 0, 1, getTPage(lvl->clutmode & 0x3, 1, lvl->prectx, lvl->precty));
    sort_prim(tpage, OTZ_LAYER_LEVEL_FG_BACK);

    // Texture TPAGE info for level foreground (front tiles)
    /* tpage = get_next_prim(); */
    /* increment_prim(sizeof(DR_TPAGE)); */
    /* setDrawTPage(tpage, 0, 1, getTPage(lvl->clutmode & 0x3, 1, lvl->prectx, lvl->precty)); */
    /* sort_prim(tpage, OTZ_LAYER_LEVEL_FG_FRONT); */

    // Render objects on nearest window
    _render_obj_window(lvl, tbl, cx, cy);
}

