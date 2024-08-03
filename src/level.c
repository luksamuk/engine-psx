#include "level.h"
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "render.h"

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
        mapping->collision[tile_id] = malloc(sizeof(Collision));
        Collision *collision = mapping->collision[tile_id];

        for(int j = 0; j < 8; j++)
            collision->floor[j] = get_byte(bytes, &b);
        for(int j = 0; j < 8; j++)
            collision->rwall[j] = get_byte(bytes, &b);
        for(int j = 0; j < 8; j++)
            collision->ceiling[j] = get_byte(bytes, &b);
        for(int j = 0; j < 8; j++)
            collision->lwall[j] = get_byte(bytes, &b);
    }

    free(bytes);
}

void
load_map(TileMapping *mapping, const char *filename, const char *collision_filename)
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

    if(!collision_filename) {
        mapping->collision = NULL;
        return;
    }

    // Load collision data
    mapping->collision = malloc((mapping->num_tiles + 1) * sizeof(Collision *));
    for(uint16_t i = 0; i < mapping->num_tiles; i++) {
        mapping->collision[i] = NULL;
    }
    _load_collision(mapping, collision_filename);
}

void
free_map(TileMapping *mapping)
{
    free(mapping->frames);
    for(uint16_t i = 0; i < mapping->num_tiles; i++) {
        if(mapping->collision[i] != NULL) {
            free(mapping->collision[i]);
        }
    }
    free(mapping->collision);
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
            //deltax = (idx % 2),
            deltax = (idx & 0x01),
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
            //deltax = (idx % 8),
            deltax = (idx & 0x07),
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
    for(int16_t iy = tiley; iy <= max_tile_y; iy++) {
        for(int16_t ix = tilex; ix <= max_tile_x; ix++) {
            int16_t frame_idx = (iy * l->width) + ix;
            if(l->tiles[frame_idx] == 0) continue;

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
    int16_t
        cx = (cam_x >> 12),
        cy = (cam_y >> 12);
    _render_layer(lvl, map128, map16, cx, cy, 4, 0);
    _render_layer(lvl, map128, map16, cx, cy, 2, 1);

    DR_TPAGE *tpage = get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpage, 0, 1, getTPage(0, 1, lvl->prectx, lvl->precty));
    sort_prim(tpage, OT_LENGTH - 1);
}

CollisionEvent
linecast(LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
         int32_t vx, int32_t vy, uint8_t direction, int32_t magnitude)
{
    // First of all, let's simplify this.
    // Do not tolerate linecasts with a magnitude greater than 16.
    // This way, we will always compare a maximum of two 128x128 tiles,
    // and also, a maximum of two 16x16 tiles.
    // We NEED to have this kind of constraint for performance reasons and,
    // let's be perfectly honest, it will not be missed.
    if(magnitude > 16 || magnitude < -16) return (CollisionEvent){ 0 };

    // From this point forward, for ease of definition, I'm gonna call
    // 128x128 tiles as chunks, and 16x16 tiles as pieces. 8x8 tiles will remain
    // just tiles.


    // 1. Calculate the X and Y of chunks on the map, for the start of
    // the linecast.
    int32_t
        tstartx = vx >> 7,
        tstarty = vy >> 7,
        lvlwidth = lvl->layers[0].width,
        lvlheight = lvl->layers[0].height;

    // 2. Calculate the X and Y of chunks on the map, for the end of
    // the linecast.
    int32_t tendx = vx, tendy = vy;
    if(direction == 0) tendx += magnitude; // horizontal
    else               tendy += magnitude; // vertical

    tendx = tendx >> 7;
    tendy = tendy >> 7;

    // End detection HERE if start AND end on any direction are out of bounds.
    if((tstartx < 0 && tendx < 0) ||
       (tstartx >= lvlwidth && tendx >= lvlwidth) ||
       (tstarty < 0 && tendy < 0) ||
       (tstarty >= lvlheight && tendy >= lvlheight))
        return (CollisionEvent){ 0 };

    // 3. We need to make sure both our chunks tiles are valid, and we have to
    // decide whether we are going to get pieces from one, two or none of them.
    // And while we're at it, let's also calculate the X and Y coordinates within
    // each chunk.
    
    uint8_t num_chunks = 0;
    int16_t chunks[2] = { -1, -1 };
    int32_t vchunkx[2];
    int32_t vchunky[2];

    if((tstartx >= 0) && (tstartx < lvlwidth) &&
       (tstarty >= 0) && (tstarty < lvlheight)) {
        chunks[0] = (tstarty * lvlwidth) + tstartx;
        vchunkx[0] = vx & 0x7f;
        vchunky[0] = vy & 0x7f;
        num_chunks++;
    }

    if((tendx >= 0) && (tendx < lvl->layers[0].width) &&
       (tendy >= 0) && (tendy < lvl->layers[0].height)) {
        chunks[1] = (tendy * lvlwidth) + tendx;
        if(direction == 0) {
            vchunkx[1] = vx + magnitude;
            vchunky[1] = vy;
        } else {
            vchunkx[1] = vx;
            vchunky[1] = vy + magnitude;
        }
        vchunkx[1] = vchunkx[1] & 0x7f;
        vchunky[1] = vchunky[1] & 0x7f;
        num_chunks++;
    }

    if(chunks[0] < 0 && chunks[1] < 0) return (CollisionEvent){ 0 };
    else if(chunks[0] == chunks[1])    num_chunks = 1;
    else if(chunks[0] < 0) {
        chunks[0]  = chunks[1];
        vchunkx[0] = vchunkx[1];
        vchunky[0] = vchunky[1];
        num_chunks = 1;
    }

    /* if(num_chunks > 1) { */
    /*     printf("chunks: %d+(%d,%d)=%d %d+(%d,%d)=%d\n", */
    /*            chunks[0], */
    /*            vchunkx[0], vchunky[0], */
    /*            lvl->layers[0].tiles[chunks[0]], */
    /*            chunks[1], */
    /*            vchunkx[1], vchunky[1], */
    /*            lvl->layers[0].tiles[chunks[1]]); */
    /* } else { */
    /*     printf("chunk: %d+(%d,%d)=%d\n", */
    /*            chunks[0], vchunkx[0], vchunky[0], */
    /*            lvl->layers[0].tiles[chunks[0]]); */
    /* } */

    // 4. Per definition, our linecast can at most intersect two chunks, and
    // will affect exactly ONE or TWO pieces. So since we know the chunk or
    // chunks are decided right now, let's figure the pieces out.
    // I know a few things:
    //   a. I am ALWAYS picking up a piece at the first chunk.
    //   b. If there is more than one chunk, this means that I am DEFINITELY
    //      picking up the second piece from the second chunk.
    // So we know for a fact where the first piece is, and the second piece can
    // either be at the first or the second chunk, depending on whether there
    // is a second chunk or not. Well, this should be easy enough!

    // Further, if these pieces are the same, I only need to look at one of
    // them.
    int16_t pieces[2];
    int32_t vpiecex[2];
    int32_t vpiecey[2];
    pieces[0]  = ((vchunky[0] >> 4) << 3) + (vchunkx[0] >> 4);
    pieces[1]  = ((vchunky[1] >> 4) << 3) + (vchunkx[1] >> 4);
    
    // mod 16 => AND with four least-significant bits
    vpiecex[0] = vchunkx[0] & 0x0f;
    vpiecey[0] = vchunky[0] & 0x0f;
    vpiecex[1] = vchunkx[1] & 0x0f;
    vpiecey[1] = vchunky[1] & 0x0f;

    // Get chunk address from level map
    chunks[0] = lvl->layers[0].tiles[chunks[0]];
    if(num_chunks > 1) chunks[1] = lvl->layers[0].tiles[chunks[1]];

    // Get piece address from chunk mapping
    pieces[0] = map128->frames[(chunks[0] << 6) + pieces[0]];
    pieces[1] = map128->frames[(chunks[(num_chunks > 1) ? 1 : 0] << 6) + pieces[1]];

    // Get actual piece collision data
    Collision *piece0 = map16->collision[pieces[0]];
    Collision *piece1 = map16->collision[pieces[1]];

    // Should both be null, there is no collision to check.
    if(!piece0 && !piece1) return (CollisionEvent){ 0 };

    // Now we need to use the direction and the signal of the magnitude
    // to determine which height mask we are supposed to use.
    uint8_t *mask0 = NULL, *mask1 = NULL;
    uint8_t index = 0;
    if(direction == 0) { // horizontal
        // index is related to y
        if(magnitude < 0) {
            mask0 = piece0 ? piece0->lwall : NULL;
            mask1 = piece1 ? piece1->lwall : NULL;
            // index starts at 0 from uppermost
            index = vpiecey[0]; // piece 1 or 0, doesn't matter
        } else {
            mask0 = piece0 ? piece0->rwall : NULL;
            mask1 = piece1 ? piece1->rwall : NULL;
            // index starts at 15 from uppermost
            index = 15 - vpiecey[0];
        }
    } else { // vertical
        // index is related to x
        if(magnitude >= 0) {
            mask0 = piece0 ? piece0->floor : NULL;
            mask1 = piece1 ? piece1->floor : NULL;
            // index starts at 0 from leftmost
            index = vpiecex[0];
        } else {
            mask0 = piece0 ? piece0->ceiling : NULL;
            mask1 = piece1 ? piece1->ceiling : NULL;
            // index starts at 15 from rightmost
            index = 15 - vpiecey[0];
        }
    }

    /* printf("piece: %u=(%d,%d)\n", pieces[0], vpiecex[0], vpiecey[0]); */

    // These pieces are supposed to be verified linearly. If piece 0 has
    // collision, identify the height on height mask and return the desired
    // x or y pushbacks.
    // But, if our height on heightmask has a 0 height, then we proceed.
    uint8_t mask_byte = 0;
    int16_t h;
    if(mask0) {
        mask_byte = mask0[index >> 1];
        mask_byte = mask_byte >> (((index & 0x1) ^ 0x1) << 2);
        mask_byte = mask_byte & 0xf;

        if(mask_byte > 0) {
            h = (direction == 0) ? vx : vy;
            h = ((magnitude < 0) ? 0 : 16) - (h & 0x0f);
        }
    }
    
    // Piece 1 is verified if and only if last piece had no collision or
    // if there was no collision on that spot.
    if((mask_byte == 0) && mask1) {
        mask_byte = mask1[index >> 1];
        mask_byte = mask_byte >> (((index & 0x1) ^ 0x1) << 2);
        mask_byte = mask_byte & 0xf;

        if(mask_byte > 0) {
            h = (direction == 0) ? vx : vy;
            h = ((magnitude < 0) ? 0 : 16) - ((h + abs(magnitude)) & 0xf);
        }
    }

    // Now mask_byte holds a number [0-15] telling the height on that tile.
    // Regardless, we need to calculate the proper X or Y value and leverage the
    // value of mask_byte to ensure we have our proper pushback value.
    CollisionEvent ev = { 0 };

    // It is not enough for us just to have a mask with a cute height value.
    // We need to know if, within our piece, the casted line reaches that
    // height.
    // We can deduce that by taking the mod of the magnitude on the direction
    // of the casted line.
    // Let's take a ground (top to bottom) collision for instance; here, our
    // magnitude is positive.
    // Take the Y position of the end by adding the magnitude to VY. Then take
    // the integer remainder (VY + Magnitude) % 16. This is how deep we are at
    // the LAST detected piece, with respect to its top.
    // We need to be careful here with this depth. If we have two pieces and

    // Now, there are two ways to calculate depth, depending on what piece
    // we found the collision at. On both these ways, we will calculate the
    // pushback with respect to the (VX, VY) point.

    // If we found a collision at piece 0, take h = (16 - (VY % 16)) to know
    // the start height inside our piece. If h > height, we have no
    // collision; otherwise, we do have a collision, and the pushback from
    // the tip of our line should be (height - h).

    // If we found a collision at piece 1, take h = (16 - ((VY + Magnitude) % 16)).
    // Perform the exact same check as above, except the retrieved height should
    // be from piece 1.
    
    ev.collided = (mask_byte > 0) && (h <= mask_byte);

    // The pushback is, with respect to the linecast's start X or Y position
    // (deducible by the direction), the amount that should be added or
    // subtracted to that coordinate so our collision object is properly
    // pushed out of the colliding object.
    
    if(ev.collided) {
        ev.direction = direction;
        ev.pushback = (int16_t)mask_byte - h - (magnitude < 0 ? 16 : 0);
        /* if(ev.direction == 0) { // horizontal */
        /*     // TODO */
        /*     ev.pushback = (int16_t)mask_byte - h - (magnitude < 0 ? 16 : 0); */
        /* } else { // vertical */
        /*     // TODO */
        /*     ev.pushback = (int16_t)mask_byte - h - (magnitude < 0 ? 16 : 0); */
        /* } */
    }
    
    return ev;
}

