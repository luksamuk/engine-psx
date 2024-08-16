#include "collision.h"
#include <stdlib.h>
#include <assert.h>

void
_move_point_linecast(uint8_t direction, int32_t vx, int32_t vy,
                     int32_t *lx, int32_t *ly,
                     uint8_t *lm)
{
    // Move simulated sensor position backwards within range
    switch(direction) {
    case CDIR_FLOOR: // Directed down
        (*ly) -= 16;
        if(*ly < vy) *ly = vy;
        goto adjusty;
    case CDIR_RWALL: // Directed left
        (*lx) += 16;
        if(*lx > vx) *lx = vx;
        goto adjustx;
    case CDIR_CEILING: // Directed up
        (*ly) += 16;
        if(*ly > vy) *ly = vy;
        goto adjusty;
    case CDIR_LWALL: // Directed right
        (*lx) -= 16;
        if(*lx < vx) *lx = vx;
        goto adjustx;
    }

adjustx:
    *lm = abs(*lx - vx);
    return;
adjusty:
    *lm = abs(*ly - vy);
}

uint8_t
_get_height_position(int32_t vx, int32_t vy, LinecastDirection direction)
{
    switch(direction) {
    case CDIR_FLOOR:   return (vx & 0x0f);
    case CDIR_RWALL:   return 16 - (vy & 0x0f);
    case CDIR_CEILING: return 16 - (vx & 0x0f);
    case CDIR_LWALL:   return (vy & 0x0f);
    }
    return 0;
}

void
_get_height_and_angle_from_mask(
    TileMap16 *map16, uint16_t piece, LinecastDirection direction, uint8_t hpos,
    uint8_t *out_h, int32_t *out_angle)
{
    Collision *collision = map16->collision[piece];
    uint8_t *mask;

    switch(direction) {
    case CDIR_FLOOR:
        *out_angle = collision->floor_angle;
        mask = collision->floor;
        break;
    case CDIR_RWALL:
        *out_angle = collision->rwall_angle;
        mask = collision->rwall;
        break;
    case CDIR_CEILING:
        *out_angle = collision->ceiling_angle;
        mask = collision->ceiling;
        break;
    case CDIR_LWALL:
        *out_angle = collision->lwall_angle;
        mask = collision->lwall;
        break;
    }

    *out_h = (mask[hpos >> 1] >> ((1 - (hpos & 0x1)) << 2)) & 0x0f;
}

int32_t
_get_new_position(uint8_t direction,
                  int32_t cx, int32_t cy, int32_t px, int32_t py,
                  uint8_t h)
{
    h = 16 - h;
    switch(direction) {
    case CDIR_FLOOR:
    case CDIR_CEILING:
        return (cy << 7) + (py << 4) + (int32_t)h;
    case CDIR_RWALL:
    case CDIR_LWALL:
        return (cx << 7) + (px << 4) + (int32_t)h;
    }
    return 0;
}

CollisionEvent
linecast(LevelData *lvl, TileMap128 *map128, TileMap16 *map16,
         int32_t vx, int32_t vy, LinecastDirection direction, uint8_t magnitude)
{
    const uint8_t layer = 0;
    assert(direction < 4);

    CollisionEvent ev = { 0 };

    // Linecast should start from bottom to top, so we start
    // at the end of the line
    int32_t lx = vx;
    int32_t ly = vy;
    uint8_t lm = magnitude;

    switch(direction) {
    case CDIR_FLOOR:   ly += magnitude; break; // Directed downwards, collides with top of tile
    case CDIR_CEILING: ly -= magnitude; break; // Directed upwards, collides with bottom of tile
    case CDIR_RWALL:   lx -= magnitude; break; // Directed left, collides with right of tile
    case CDIR_LWALL:   lx += magnitude; break; // Directed right, collides with left of tile
    }

    uint8_t n_max = (magnitude >> 4) + 1;
    
    for(uint8_t n = 0; n < n_max; n++) {
        // Chunk coordinates on the level.
        int32_t cx = lx >> 7;
        int32_t cy = ly >> 7;

        // Get chunk id
        int32_t chunk_pos;
        if((cx < 0) || (cx >= lvl->layers[layer].width)) chunk_pos = -1;
        else if((cy < 0) || (cy >= lvl->layers[layer].height)) chunk_pos = -1;
        else chunk_pos = (cy * lvl->layers[layer].width) + cx;

        int16_t chunk = lvl->layers[layer].tiles[chunk_pos];
        
        if(chunk >= 0) {
            // Piece coordinates within chunk
            int32_t px = (lx & 0x7f) >> 4;
            int32_t py = (ly & 0x7f) >> 4;
            uint16_t piece_pos = ((py << 3) + px) + (chunk << 6);
            uint16_t piece = map128->frames[piece_pos];

            if(piece > 0) {
                uint8_t hpos;
                uint8_t h;
                int32_t angle;

                hpos = _get_height_position(lx, ly, direction);

                _get_height_and_angle_from_mask(
                    map16, piece, direction, hpos, &h, &angle);

                if(h > 0) {
                    int32_t coord = _get_new_position(direction, cx, cy, px, py, h);
                    ev = (CollisionEvent) {
                        .collided = 1,
                        .coord = coord,
                        .angle = angle,
                    };
                }
            }
        }

        // If continuing, move linecast point forward
        _move_point_linecast(direction, vx, vy, &lx, &ly, &lm);
    }

    return ev;
}
