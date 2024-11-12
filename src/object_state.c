#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "object.h"
#include "object_state.h"
#include "util.h"
#include "memalloc.h"
#include "level.h"
#include "render.h"
#include "timer.h"
#include "camera.h"

extern ArenaAllocator _level_arena;
extern uint8_t        paused;
extern Player         player;
extern Camera         camera;
extern uint8_t        level_fade;

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

    state->anim_state = (ObjectAnimState){ 0 };
    state->frag_anim_state = NULL;

    // None of these objects have FREE positioning, so none of them have
    // a "freepos" field either
    state->freepos = NULL;

    // Initialize animation state if this object
    // has a fragment
    if(state->id == OBJ_MONITOR || state->id == OBJ_CHECKPOINT) {
        state->frag_anim_state = alloc_arena_malloc(
            &_level_arena,
            sizeof(ObjectAnimState));
        *state->frag_anim_state = (ObjectAnimState){ 0 };
    }

    // Some very specific features that are object-dependent.
    switch(type) {
    default: break;
    case OBJ_RING:
        state->props |= OBJ_FLAG_ANIM_LOCK;
        break;
    case OBJ_MONITOR:
        state->props |= OBJ_FLAG_ANIM_LOCK;
        // Set initial animation with respect to kind
        state->frag_anim_state->animation = (uint16_t)((MonitorExtra *)state->extra)->kind;
        break;
    case OBJ_GOAL_SIGN:
        camera_set_right_bound(&camera, (vx << 12) + ((CENTERX >> 1) << 12));
        break;
    };
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
            case OBJ_DUMMY_STARTPOS:
                // Initialize player at position
                player.startpos = (VECTOR){ .vx = vx << 12, .vy = (vy - 8) << 12, .vz = 0 };
                player.pos = player.startpos;
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

void
object_render(ObjectState *state, ObjectTableEntry *typedata,
              int16_t ovx, int16_t ovy)
{
    ObjectAnimState *anim = &state->anim_state;
    if(anim->animation >= typedata->num_animations) return;
    ObjectAnim *an = &typedata->animations[anim->animation];
    ObjectAnimFrame *frame = NULL;

    uint8_t in_fragment = 0;

begin_render_routine:

    if(state->props & OBJ_FLAG_ANIM_LOCK) {
        uint32_t frame = get_elapsed_frames();
        if(an->duration > 0) {
            frame = (frame / an->duration);
            if(an->loopback >= 0) frame %= an->num_frames;
        }
        anim->frame = (uint8_t)frame;
    } else if(!paused) {
        if(anim->counter == 0) {
            anim->frame++;
            anim->counter = an->duration;
        } else anim->counter--;
    }

    if(anim->frame >= an->num_frames) {
        if(an->loopback >= 0) anim->frame = an->loopback;
        // If animation does not loop back, set animation to none
        else {
            anim->frame = 0;
            anim->animation = OBJ_ANIMATION_NO_ANIMATION;
            return;
        }
    }

    frame = &an->frames[anim->frame];

    // Leverage "double flipping"
    uint8_t flipmask = state->frag_anim_state ? 0 : (state->flipmask ^ frame->flipmask);

    // Some rules:
    // 1. If an object is flipped, it cannot be rotated.
    // 2. ct and cw rotations cancel out.
    // 3. As a rule of thumb... objects with fragments
    //    cannot be flipped or rotated.
    // We'll expect these values to work as they should.

    int16_t vx = ovx;
    int16_t vy = ovy;

    // Calculate actual position from width and height
    switch(state->id) {
    case OBJ_RING:
        vx -= frame->w >> 1;
        vy -= 48 - (frame->h >> 1) - 1;
        break;
    default:
        if(flipmask & MASK_FLIP_ROTCW) {
            vx -= 32 - frame->h - 1;
            vy += (frame->w >> 1);
        } else if(flipmask & MASK_FLIP_ROTCT) {
            vx -= 32 + frame->h;
            vy -= (frame->w >> 1);
        } else {
            vx -= frame->w >> 1;
            if(flipmask & MASK_FLIP_FLIPY) vy -= 64 - frame->h;
            vy -= frame->h;
        }
        break;
    }

    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, level_fade, level_fade, level_fade);

    // Clip object if not within screen
    if((vx < -64) || (vx > SCREEN_XRES + 64)) goto after_render;
    if((vy < -64) || (vy > SCREEN_YRES + 64)) goto after_render;
    
    if((flipmask & MASK_FLIP_FLIPX) && (flipmask & MASK_FLIP_FLIPY)) {
        setXYWH(poly, vx, vy, frame->w, frame->h);
        setUV4(poly,
               frame->u0 + frame->w - 1,  frame->v0 + frame->h - 1,
               frame->u0,                 frame->v0 + frame->h - 1,
               frame->u0 + frame->w - 1,  frame->v0,
               frame->u0,                 frame->v0);
    } else if(flipmask & MASK_FLIP_FLIPX) {
        setXYWH(poly, vx, vy, frame->w, frame->h);
        setUV4(poly,
               frame->u0 + frame->w - 1,  frame->v0,
               frame->u0,                 frame->v0,
               frame->u0 + frame->w - 1,  frame->v0 + frame->h,
               frame->u0,                 frame->v0 + frame->h);
    } else if(flipmask & MASK_FLIP_FLIPY) {
        setXYWH(poly, vx, vy, frame->w, frame->h);
        setUV4(poly,
               frame->u0,                 frame->v0 + frame->h - 1,
               frame->u0 + frame->w - 1,  frame->v0 + frame->h - 1,
               frame->u0,                 frame->v0,
               frame->u0 + frame->w - 1,  frame->v0);
    } else if(flipmask & MASK_FLIP_ROTCW) {
        setXY4(poly,
               vx,                 vy,
               vx,                 vy + frame->w,
               vx - frame->h - 1,  vy,
               vx - frame->h - 1,  vy + frame->w);
        setUVWH(poly, frame->u0, frame->v0, frame->w - 1, frame->h - 1);
    } else if(flipmask & MASK_FLIP_ROTCT) {
        setXY4(poly,
               vx,                 vy,
               vx,                 vy - frame->w,
               vx + frame->h,      vy,
               vx + frame->h,      vy - frame->w);
        setUVWH(poly, frame->u0, frame->v0, frame->w - 1, frame->h);
    } else {
        setXYWH(poly, vx, vy, frame->w, frame->h);
        setUVWH(poly, frame->u0, frame->v0, frame->w, frame->h);
    }

    // COMMON OBJECTS have the following VRAM coords:
    // Sprites: 576x0
    // CLUT: 0x481
    
    poly->tpage = getTPage(1, 0, 576, 0);
    poly->clut = getClut(0, 481);

    setSemiTrans(poly, state->id == OBJ_SHIELD ? 1 : 0);

    sort_prim(poly,
              ((state->id == OBJ_RING) || (state->id == OBJ_SHIELD))
              ? OTZ_LAYER_OBJECTS
              : OTZ_LAYER_PLAYER);

after_render:

    if(!in_fragment && (typedata->fragment != NULL)) {
        in_fragment = 1;
        anim = state->frag_anim_state;
        if(anim->animation >= typedata->fragment->num_animations) return;
        an = &typedata->fragment->animations[anim->animation];
        ovx += typedata->fragment->offsetx;
        ovy += typedata->fragment->offsety;
        
        goto begin_render_routine;
    }
}
