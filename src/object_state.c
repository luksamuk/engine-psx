#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

#include "object.h"
#include "object_state.h"
#include "util.h"
#include "memalloc.h"
#include "level.h"
#include "render.h"
#include "timer.h"
#include "camera.h"
#include "boss.h"
#include "screen.h"

extern uint8_t        paused;
extern Player         *player;
extern Camera         *camera;
extern uint8_t        level_fade;
extern uint8_t        level_has_boss;
extern BossState      *boss;
extern uint16_t       level_ring_max;

extern ObjectTable *obj_table_common;
extern ObjectTable *obj_table_level;



// Takes up to 12KB of object state table
// (8KB emplaceds, 4KB awaiting references, 4B num_awaiting)
#define MAX_LEVEL_OBJECTS 2000

typedef struct {
    ObjectState **emplaceds;
    uint16_t    *awaiting_parent; // References awaiting a parent
    uint32_t    num_awaiting;
} ObjectPlacementIndex;

static ObjectPlacementIndex *placement_idx = NULL;

// ---- Functions to manipulate this data structure ---
// Initialize placement index
void
init_placement_index(ObjectPlacementIndex *idx)
{
    idx->emplaceds = (ObjectState **)malloc(sizeof(ObjectState *) * MAX_LEVEL_OBJECTS);
    idx->awaiting_parent = (uint16_t *)malloc(sizeof(uint16_t) * MAX_LEVEL_OBJECTS);
    bzero(idx->emplaceds, sizeof(ObjectState *) * MAX_LEVEL_OBJECTS);
    bzero(idx->awaiting_parent, sizeof(uint16_t) * MAX_LEVEL_OBJECTS);
    idx->num_awaiting = 0;
}

// Destroy placement index
void
destroy_placement_index(ObjectPlacementIndex *idx)
{
    free(idx->emplaceds);
    free(idx->awaiting_parent);
    idx->num_awaiting = 0;
}

void
placement_index_add_emplaced(ObjectPlacementIndex *idx, ObjectState *st)
{
    if(st->unique_id == 0) return; // Ignore certain objects
    idx->emplaceds[st->unique_id] = st;
}

void
placement_index_add_awaiting(ObjectPlacementIndex *idx, ObjectState *st)
{
    idx->awaiting_parent[idx->num_awaiting++] = st->unique_id;
}

void
placement_index_remove_awaiting_at(ObjectPlacementIndex *idx, uint32_t n)
{
    if(n >= idx->num_awaiting) return;
    for(uint32_t i = n; i < idx->num_awaiting; i++) {
        idx->awaiting_parent[i] = idx->awaiting_parent[i+1];
    }
    idx->num_awaiting--;
}

// ---- entry points for placement index ----

void
placement_index_add_parent(ObjectPlacementIndex *idx, ObjectState *parent)
{
    if(parent->unique_id == 0) return;
    idx->emplaceds[parent->unique_id] = parent;
}

void
placement_index_find_child(ObjectPlacementIndex *idx, ObjectState *parent)
{
    // I am a parent that has been emplaced, and I'm looking for my only child!
    // Worst case scenario: I don't find any child
    for(uint32_t i = 0; i < idx->num_awaiting; i++) {
        ObjectState *orphan = idx->emplaceds[idx->awaiting_parent[i]];
        if(orphan->parent_id == parent->unique_id) {
            orphan->parent = parent;
            parent->child = orphan;
            placement_index_remove_awaiting_at(idx, i);
            return;
        }
    }
}

void
placement_index_try_find_parent(ObjectPlacementIndex *idx, ObjectState *orphan)
{
    // I have just been created, and I know my parent's ID.
    // If it exists, attribute it.
    // If it doesn't, send me to the orphanage.
    ObjectState *parent = idx->emplaceds[orphan->parent_id];
    if(parent != NULL) {
        orphan->parent = parent;
        parent->child = orphan;
        return;
    }
    placement_index_add_awaiting(idx, orphan);
}

// --------

void
_emplace_object(
    ChunkObjectData *data, int32_t tx, int32_t ty,
    uint8_t is_level_specific, uint8_t has_fragment,
    int8_t type, uint8_t flipmask, int32_t vx, int32_t vy,
    uint16_t unique_id, uint16_t parent_id,
    void *extra)
{
    if(data->num_objects + 1 >= MAX_OBJECTS_PER_CHUNK) {
        printf("WARNING: Not emplacing extra object. ID: %d, specific? %d\n",
               type, is_level_specific);
        return;
    }
    ObjectState *state = &data->objects[data->num_objects++];
    assert(data->num_objects < MAX_OBJECTS_PER_CHUNK);

    state->id = type + (is_level_specific ? MIN_LEVEL_OBJ_GID : 0);
    state->unique_id = unique_id; // TODO: Connect to children if possible
    state->parent_id = parent_id; // TODO: Connect to parent if possible
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

    // Parent and next entities is always NULL unless manually set.
    state->parent = NULL;
    state->next = NULL;

    // Initialize animation state if this object
    // has a fragment
    if(has_fragment) {
        state->frag_anim_state = screen_alloc(sizeof(ObjectAnimState));
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
        {
            uint16_t animation = (uint16_t)((MonitorExtra *)state->extra)->kind;
            if(animation == MONITOR_KIND_1UP) {
                // If this is a 1-UP monitor, change animation again with
                // respect to current character
                switch(player->character) {
                default:
                case CHARA_SONIC:    animation = 5; break;
                case CHARA_MILES:    animation = 7; break;
                case CHARA_KNUCKLES: animation = 8; break;
                case CHARA_AMY:      animation = 9; break;
                }
            }
            state->frag_anim_state->animation = animation;
        }
        break;
    case OBJ_GOAL_SIGN:
    case OBJ_END_CAPSULE:
        camera_set_right_bound(camera, (vx << 12) + ((CENTERX >> 1) << 12));
        break;
    };

    // Try and find parent and child
    state->parent = state->child = state->next = NULL;
    placement_index_add_parent(placement_idx, state);
    if(state->parent_id != 0) {
        placement_index_try_find_parent(placement_idx, state);
    }
    placement_index_find_child(placement_idx, state);
}

void
load_object_placement(const char *filename, void *lvl_data, uint8_t has_started)
{
    LevelData *lvl = (LevelData *)lvl_data;
    uint8_t *bytes;
    uint32_t b, length;

    // Slurp object placement file
    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading OTD file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    // Prepare object reference list and sentinels
    placement_idx = (ObjectPlacementIndex *)malloc(sizeof(ObjectPlacementIndex));
    init_placement_index(placement_idx);

    uint16_t created_objects = 0;
    uint16_t num_objects = get_short_be(bytes, &b);
    for(uint16_t i = 0; i < num_objects; i++) {
        uint8_t is_level_specific = get_byte(bytes, &b);
        int8_t type = get_byte(bytes, &b);
        uint16_t unique_id = get_short_be(bytes, &b); // Should be used to identify objects
        uint16_t parent_id = get_short_be(bytes, &b); // Identifies a parent if existing
        uint8_t flipmask = get_byte(bytes, &b);
        int32_t vx = get_long_be(bytes, &b);
        int32_t vy = get_long_be(bytes, &b);
        void *extra = NULL;

        switch(type) {
        default: break;
        case OBJ_MONITOR:
            extra = screen_alloc(sizeof(MonitorExtra));
            ((MonitorExtra *)extra)->kind = get_byte(bytes, &b);
            break;
        case OBJ_BUBBLE_PATCH:
            extra = screen_alloc(sizeof(BubblePatchExtra));
            ((BubblePatchExtra *)extra)->frequency = get_byte(bytes, &b);

            // Start timer at a low timer so we start with idle instead of producing
            ((BubblePatchExtra *)extra)->timer = 8;
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
            data = screen_alloc(sizeof(ChunkObjectData));
            lvl->objects[chunk_pos] = data;
            *data = (ChunkObjectData){ 0 };
        }

        if(type < 0) {
            // This is a dummy object, so create others in its place.
            switch(type) {
            case OBJ_DUMMY_RINGS_3V:
                _emplace_object(data, cx, cy, 0, 0, OBJ_RING, 0, vx, vy - 24, 0, 0, NULL);
                _emplace_object(data, cx, cy, 0, 0, OBJ_RING, 0, vx, vy, 0, 0, NULL);
                _emplace_object(data, cx, cy, 0, 0, OBJ_RING, 0, vx, vy + 24, 0, 0, NULL);
                created_objects += 3;
                break;
            case OBJ_DUMMY_RINGS_3H:
                _emplace_object(data, cx, cy, 0, 0, OBJ_RING, 0, vx - 24, vy, 0, 0, NULL);
                _emplace_object(data, cx, cy, 0, 0, OBJ_RING, 0, vx, vy, 0, 0, NULL);
                _emplace_object(data, cx, cy, 0, 0, OBJ_RING, 0, vx + 24, vy, 0, 0, NULL);
                created_objects += 3;
                break;
            case OBJ_DUMMY_STARTPOS:
                // Initialize player at position. But only if not reloading
                // objects after a respawn
                if(!has_started) {
                    player->startpos = (VECTOR){ .vx = vx << 12, .vy = (vy - 8) << 12, .vz = 0 };
                    player->pos = player->respawnpos = player->startpos;
                }
                break;
            default: break;
            }
        } else {
            // Exception: If the level has started, this is a soft reset
            // situation. We'd be in trouble if we were recreating any
            // checkpoints, since they are never reset not recreated --
            // generally they're just moved to the beginning of the object array
            if(!(has_started && type == OBJ_CHECKPOINT)) {
                ObjectTableEntry *entry = (type >= MIN_LEVEL_OBJ_GID)
                    ? &obj_table_level->entries[type - MIN_LEVEL_OBJ_GID]
                    : &obj_table_common->entries[type];

                _emplace_object(data, cx, cy,
                                is_level_specific, entry->has_fragment,
                                type, flipmask, vx, vy, unique_id, parent_id,
                                extra);
                created_objects++;
            }
        }
    }

    printf("Loaded %d objects.\n", created_objects);

    // Destroy object placement index
    destroy_placement_index(placement_idx);
    free(placement_idx);
    placement_idx = NULL;

    // Free slurped file
    free(bytes);
}

void
unload_object_placements(void *lvl_data)
{
    LevelData *lvl = (LevelData *)lvl_data;

    // Levels are 255 x 31, so we have this amount of objects
    // to unload
    for(uint32_t i = 0; i < LEVEL_MAX_X_CHUNKS * LEVEL_MAX_Y_CHUNKS; i++) {
        ChunkObjectData *cnk = lvl->objects[i];
        if(cnk != NULL) {
            if(cnk->num_objects == 0) continue;

            uint8_t orig_num_objs = cnk->num_objects;
            cnk->num_objects = 0;
            for(uint8_t j = 0; j < orig_num_objs; j++) {
                ObjectState *obj = &cnk->objects[j];
                // We never destroy checkpoints.
                // That's because object unloading is supposed to be used
                // on respawns.
                // Instead, move checkpoints to beginning of vector.
                if(obj->id == OBJ_CHECKPOINT) {
                    // Hey, look, IT'S BUBBLESORT!!!!
                    if(cnk->num_objects != j) {
                        // Notice that ANY REFERENCE ON PARENT/CHILD OBJECTS
                        // WILL BE LOST ON THIS PROCESS. SO >>DO NOT<< USE
                        // OBJECT REFERENCES WITHIN CHECKPOINTS, PERIOD.
                        obj->parent_id = 0;
                        obj->parent = obj->child = obj->next = NULL;

                        memcpy(&cnk->objects[cnk->num_objects], obj, sizeof(ObjectState));
                        obj->props |= OBJ_FLAG_DESTROYED;
                        cnk->objects[j].props |= OBJ_FLAG_DESTROYED;
                    }
                    cnk->num_objects++;
                } else cnk->objects[j].props |= OBJ_FLAG_DESTROYED;
            }
            // Clean other object placements
            uint8_t num_empty_objs = orig_num_objs - cnk->num_objects;
            ObjectState *empty_start = &cnk->objects[cnk->num_objects];
            bzero(empty_start, num_empty_objs * sizeof(ObjectState));
        }
    }
}

uint16_t
count_emplaced_rings(void *lvl_data)
{
    uint16_t result = 0;
    LevelData *lvl = (LevelData *)lvl_data;
    for(uint32_t i = 0; i < LEVEL_MAX_X_CHUNKS * LEVEL_MAX_Y_CHUNKS; i++) {
        ChunkObjectData *cnk = lvl->objects[i];
        if(cnk != NULL) {
            for(uint8_t j = 0; j < cnk->num_objects; j++) {
                ObjectState *obj = &cnk->objects[j];
                if(obj->id == OBJ_RING && !(obj->props & OBJ_FLAG_DESTROYED))
                    result++;
            }
        }
    }
    return result;
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
        // This is a weird fix for animation locks when the level
        // timer is frozen, but hey, it works.
        uint32_t frame = paused ? get_elapsed_frames() : get_global_frames();

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
    uint8_t flipmask = state->flipmask ^ frame->flipmask;

    // Some rules:
    // 1. If an object is flipped, it cannot be rotated.
    // 2. ct and cw rotations cancel out.
    // 3. As a rule of thumb... objects with fragments
    //    will have their fragment flipped or rotated through
    //    their entire flip state.
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
            vx -= (frame->w >> 1);
            if(flipmask & MASK_FLIP_FLIPY) vy -= 64 - frame->h;
            vy -= frame->h;
        }
        break;
    }

    // Clip object if not within screen
    if((vx < -64) || (vx > SCREEN_XRES + 64)) goto after_render;
    if((vy < -64) || (vy > SCREEN_YRES + 64)) goto after_render;

    // If this is a shield, do not render it if current frame is the
    // 3rd and 4th frame of an animation (note: shield has 3 frames with duration 4)
    if((state->id == OBJ_SHIELD) && ((anim->counter >> 1) % 2))
        goto after_render;

    POLY_FT4 *poly = (POLY_FT4 *)get_next_prim();
    increment_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, level_fade, level_fade, level_fade);

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

    if(!typedata->is_level_specific) {
        // COMMON OBJECTS have the following VRAM coords:
        // Sprites: 576x0
        // CLUT: 0x481
        poly->tpage = getTPage(1, 0, 576, frame->tpage ? 256 : 0);
        poly->clut = getClut(0, 481);
    } else {
        // LEVEL OBJECTS have these VRAM coords:
        // Sprites: 704x0
        // CLUT: 0x485
        // Boss CLUT: 0x486 (normal); 0x487 (when hit)
        poly->tpage = getTPage(1, 0, 704, frame->tpage ? 256 : 0);
        poly->clut = getClut(
            0,
            (frame->tpage && level_has_boss)
            ? (boss_hit_glowing() ? 487 : 486)
            : 485);
    }

    uint32_t layer = ((state->id == OBJ_RING)
                      || (state->id == OBJ_SHIELD)
                      || (state->id == OBJ_EXPLOSION)
                      || (state->id == OBJ_BUBBLE))
        ? OTZ_LAYER_OBJECTS
        : OTZ_LAYER_UNDER_PLAYER;

    // NOTABLE EXCEPTION: if this is a bubble object which animation has gone
    // beyond the first digit display, it should be rendered on text layer
    if((state->id == OBJ_BUBBLE)
       && (state->anim_state.animation >= 3)
       && (state->anim_state.frame >= 5))
        layer = OTZ_LAYER_HUD;

    sort_prim(poly, layer);

after_render:

    if(!in_fragment && typedata->has_fragment) {
        in_fragment = 1;
        anim = state->frag_anim_state;
        if(anim->animation >= typedata->fragment->num_animations) return;
        an = &typedata->fragment->animations[anim->animation];
        if(flipmask & MASK_FLIP_FLIPX)
            ovx -= typedata->fragment->offsetx;
        else ovx += typedata->fragment->offsetx;
        if(flipmask & MASK_FLIP_FLIPY)
            ovy -= typedata->fragment->offsety;
        else ovy += typedata->fragment->offsety;
        
        goto begin_render_routine;
    }
}

uint8_t
boss_hit_glowing()
{
    // Glow on odd intervals.
    // Being odd ensures that a hit cooldown equals 0 uses original palette
    return (boss->health > 0) && (boss->hit_cooldown >> 1) % 2 != 0;
}
