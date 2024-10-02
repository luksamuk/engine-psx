#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "object.h"
#include "object_state.h"
#include "util.h"
#include "memalloc.h"
#include "level.h"
#include "render.h"

extern ArenaAllocator _level_arena;

void
_emplace_object(
    ChunkObjectData *data, int32_t tx, int32_t ty,
    uint8_t is_level_specific,
    int8_t type, uint8_t flipmask, int32_t vx, int32_t vy, void *extra)
{
    ObjectState *state = &data->objects[data->num_objects++];
    assert(data->num_objects < MAX_OBJECTS_PER_CHUNK);

    state->id = type + (is_level_specific ? 100 : 0);
    state->flipmask = flipmask;

    state->rx = vx - (tx << 7);
    state->ry = vy - (ty << 7);

    state->extra = extra;
    state->props = 0;

    // Some very specific features that are object-dependent.
    switch(type) {
    default: break;
    case OBJ_RING:
        state->props |= OBJ_FLAG_ANIM_LOCK;
        break;
    };

    state->anim_state = (ObjectAnimState){ 0 };
}

void
load_object_placement(const char *filename, void *lvl_data)
{
    LevelData *lvl = (LevelData *)lvl_data;
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading OTD file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    uint16_t created_objects = 0;
    uint16_t num_objects = get_short_be(bytes, &b);
    for(uint16_t i = 0; i < num_objects; i++) {
        uint8_t is_level_specific = get_byte(bytes, &b);
        int8_t type = get_byte(bytes, &b);
        uint8_t flipmask = get_byte(bytes, &b);
        int32_t vx = get_long_be(bytes, &b);
        int32_t vy = get_long_be(bytes, &b);
        void *extra = NULL;

        switch(type) {
        default: break;
        case OBJ_MONITOR:
            extra = alloc_arena_malloc(&_level_arena, sizeof(MonitorExtra));
            ((MonitorExtra *)extra)->kind = get_byte(bytes, &b);
            break;
        }

        // Get chunk at position
        int32_t cx = vx >> 7;
        int32_t cy = vy >> 7;
        int32_t chunk_pos;
        if((cx < 0) || (cx >= lvl->layers[0].width)) chunk_pos = -1;
        else if((cy < 0) || (cy >= lvl->layers[0].height)) chunk_pos = -1;
        else chunk_pos = (cy * lvl->layers[0].width) + cx;

        ChunkObjectData *data = lvl->objects[chunk_pos];
        if(!data) {
            data = alloc_arena_malloc(&_level_arena, sizeof(ChunkObjectData));
            lvl->objects[chunk_pos] = data;
            *data = (ChunkObjectData){ 0 };
        }

        if(type < 0) {
            // This is a dummy object, so create others in its place.
            switch(type) {
            case OBJ_DUMMY_RINGS_3V:
                _emplace_object(data, cx, cy, 0, OBJ_RING, 0, vx, vy - 24, NULL);
                _emplace_object(data, cx, cy, 0, OBJ_RING, 0, vx, vy, NULL);
                _emplace_object(data, cx, cy, 0, OBJ_RING, 0, vx, vy + 24, NULL);
                created_objects += 3;
                break;
            case OBJ_DUMMY_RINGS_3H:
                _emplace_object(data, cx, cy, 0, OBJ_RING, 0, vx - 24, vy, NULL);
                _emplace_object(data, cx, cy, 0, OBJ_RING, 0, vx, vy, NULL);
                _emplace_object(data, cx, cy, 0, OBJ_RING, 0, vx + 24, vy, NULL);
                created_objects += 3;
                break;
            default: break;
            }
        } else {
            _emplace_object(data, cx, cy, is_level_specific, type, flipmask, vx, vy, extra);
            created_objects++;
        }
    }

    printf("Loaded %d objects.\n", created_objects);

    free(bytes);
}


void object_render(ObjectState *state, ObjectTableEntry *typedata,
                   int16_t vx, int16_t vy)
{
    // Only show rings for now
    if(state->id != OBJ_RING) return;

    // TODO: This should be part of an update
    ObjectAnimState *anim = &state->anim_state;
    if(anim->animation >= typedata->num_animations) return;

    ObjectAnim *an = &typedata->animations[anim->animation];
    // TODO: check if animation is locked
    if(anim->counter == 0) {
        anim->frame++;
        anim->counter = 6; // TODO: Set to duration
    } else anim->counter--;
    if(anim->frame >= an->num_frames) {
        if(an->loopback >= 0) anim->frame = an->loopback;
        else {
            anim->frame = 0;
            anim->animation = 0xff; // 255 = no animation
            state->props |= OBJ_FLAG_DESTROYED;
            return;
        }
    }

    ObjectAnimFrame *frame = &an->frames[anim->frame];

    // Calculate actual position from width and height
    vx -= frame->w >> 1;
    vy -= 48 - (frame->h >> 1);

    // COMMON OBJECTS have the following VRAM coords:
    // Sprites: 576x0
    // CLUT: 576x256

    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, 128, 128, 128);

    poly->tpage = getTPage(0, 0, 576, 0);
    poly->clut = getClut(576, 256);

    setXYWH(poly, vx, vy, frame->w, frame->h);
    setUVWH(poly, frame->u0, frame->v0, frame->w, frame->h);
    sort_prim(poly, 3);
}
