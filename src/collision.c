#include "collision.h"
#include "player.h"
#include <stdlib.h>
#include <assert.h>

// Debug-only stuff
#include "render.h"
#include "camera.h"
extern int debug_mode;
extern Camera *camera;

// Level data
extern TileMap16   *map16;
extern TileMap128  *map128;
extern LevelData   *leveldata;
extern Player      *player;

void
_move_point_linecast(uint8_t direction, int32_t vx, int32_t vy,
                     int32_t *lx, int32_t *ly,
                     uint8_t *lm)
{
    const int32_t step = 16;
    //const int32_t step = 1;
    // Move simulated sensor position backwards within range
    switch(direction) {
    case CDIR_FLOOR: // Directed down
        (*ly) -= step;
        if(*ly < vy) *ly = vy;
        goto adjusty;
    case CDIR_RWALL: // Directed left
        (*lx) -= step; // TODO
        if(*lx > vx) *lx = vx;
        goto adjustx;
    case CDIR_CEILING: // Directed up
        (*ly) += step;
        if(*ly > vy) *ly = vy;
        goto adjusty;
    case CDIR_LWALL: // Directed right
        (*lx) += step; // TODO
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
    uint16_t piece, LinecastDirection direction, uint8_t hpos,
    uint8_t *out_h, int32_t *out_angle)
{
    Collision *collision = map16->collision[piece];

    if(collision == NULL) {
    abort_retrieve:
        *out_h = 0;
        *out_angle = 0;
        return;
    }

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
    default: goto abort_retrieve;
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

int32_t
_get_tip_height(LinecastDirection direction,
                int32_t lx, int32_t ly)
{
    switch(direction) {
    case CDIR_FLOOR:
        return 15 - (ly & 0x0f);
        break;
    case CDIR_CEILING:
        return ly & 0x0f;
        break;
    case CDIR_LWALL:
        return (lx & 0x0f);
        break;
    case CDIR_RWALL:
        return 15 - (lx & 0x0f);
        break;
    };
    return 0;
}

CollisionEvent
linecast(int32_t vx, int32_t vy, LinecastDirection direction,
         uint8_t magnitude, LinecastDirection floor_direction)
{
    // No level data? No collision.
    if(leveldata->num_layers < 1) return (CollisionEvent){ 0 };

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
    case CDIR_RWALL:   lx += magnitude; break; // Directed right, collides with left of tile
    case CDIR_LWALL:   lx -= magnitude; break; // Directed left, collides with right of tile
    }

    uint8_t n_max = (magnitude >> 4) + 1;
    
    for(uint8_t n = 0; n < n_max; n++) {
        // Chunk coordinates on the level.
        int32_t cx = lx >> 7;
        int32_t cy = ly >> 7;

        // Get chunk id
        int32_t chunk_pos;
        if((cx < 0) || (cx >= leveldata->layers[layer].width)) chunk_pos = -1;
        else if((cy < 0) || (cy >= leveldata->layers[layer].height)) chunk_pos = -1;
        else chunk_pos = (cy * leveldata->layers[layer].width) + cx;

        int16_t chunk = leveldata->layers[layer].tiles[chunk_pos];
        
        if(chunk >= 0) {
            // Piece coordinates within chunk
            int32_t px = (lx & 0x7f) >> 4;
            int32_t py = (ly & 0x7f) >> 4;
            uint16_t piece_pos = ((py << 3) + px) + (chunk << 6);
            uint16_t piece = map128->frames[piece_pos].index;
            uint8_t piece_props = map128->frames[piece_pos].props;

            if((piece > 0) &&
               (piece_props != MAP128_PROP_NONE) &&
               !(piece_props & MAP128_PROP_FRONT) &&
               !((direction != floor_direction) && (piece_props & MAP128_PROP_ONEWAY))) {
                uint8_t hpos;
                uint8_t h;
                int32_t angle;

                hpos = _get_height_position(lx, ly, direction) & 0x0f;

                _get_height_and_angle_from_mask(piece, direction, hpos, &h, &angle);

                if(h > 0) {
                    int32_t tip_height = _get_tip_height(direction, lx, ly);
                    
                    if(direction == floor_direction || (h >= tip_height)) {
                        int32_t coord = _get_new_position(direction, cx, cy, px, py, h);
                        ev = (CollisionEvent) {
                            .collided = 1,
                            .coord = coord,
                            .angle = angle,
                        };
                    }
                }
            }
        }

        // If continuing, move linecast point forward
        _move_point_linecast(direction, vx, vy, &lx, &ly, &lm);
    }
    return ev;
}

void
draw_collision_hitbox(int32_t vx, int32_t vy, int32_t w, int32_t h)
{
    uint16_t
        rel_vx = vx - (camera->pos.vx >> 12) + CENTERX,
        rel_vy = vy - (camera->pos.vy >> 12) + CENTERY;
    POLY_F4 *hitbox = get_next_prim();
    increment_prim(sizeof(POLY_F4));
    setPolyF4(hitbox);
    setSemiTrans(hitbox, 1);
    setXYWH(hitbox, rel_vx, rel_vy, w, h);
    setRGB0(hitbox, 0x40, 0xd2, 0x25);
    sort_prim(hitbox, OTZ_LAYER_OBJECTS);
}

int
aabb_intersects(int32_t a_vx, int32_t a_vy, int32_t aw, int32_t ah,
                int32_t b_vx, int32_t b_vy, int32_t bw, int32_t bh)
{
    if(debug_mode > 1) draw_collision_hitbox(b_vx, b_vy, bw, bh);

    if(player->death_type > 0) {
        return OBJECT_DO_NOTHING;
    }

    int32_t a_right  = a_vx + aw;
    int32_t a_bottom = a_vy + ah;
    int32_t b_right  = b_vx + bw;
    int32_t b_bottom = b_vy + bh;

    return !((a_vx > b_right) || (a_right < b_vx) ||
             (a_bottom < b_vy) || (a_vy > b_bottom));
}

