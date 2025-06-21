#include <psxgpu.h>
#include <assert.h>
#include "object_state.h"
#include "memalloc.h"
#include "render.h"

extern ArenaAllocator _level_arena;

// Pointer to object pool.
// Notice that we still use our level allocator
static PoolObject *_object_pool;
static uint32_t   _pool_count = 0;

extern ObjectTable obj_table_common;
extern ObjectTable obj_table_level;

// Defined in level.c
extern void _render_obj(
    ObjectState *obj, ObjectTableEntry *typedata,
    int32_t cx, int32_t cy, int32_t tx, int32_t ty);

void
object_pool_init()
{
    _object_pool = alloc_arena_malloc(
        &_level_arena,
        sizeof(PoolObject) * OBJECT_POOL_SIZE);

    // Zero-initialize all objects and set them as free, destroyed objects.
    for(uint32_t i = 0; i < OBJECT_POOL_SIZE; i++) {
        _object_pool[i] = (PoolObject){ 0 };
        _object_pool[i].props |= (OBJ_FLAG_DESTROYED | OBJ_FLAG_FREE_OBJECT);
    }

    _pool_count = 0;
    printf("Initialized object pool\n");
}

void
object_pool_update(uint8_t round)
{
    _pool_count = 0;
    for(uint32_t i = 0; i < OBJECT_POOL_SIZE; i++) {
        PoolObject *obj = &_object_pool[i];
        if(!(obj->props & OBJ_FLAG_DESTROYED)) {
            VECTOR pos = { obj->freepos.vx >> 12, obj->freepos.vy >> 12, 0 };
            object_update((ObjectState *)&obj->state,
                          (obj->state.id >= MIN_LEVEL_OBJ_GID)
                          ? &obj_table_level.entries[obj->state.id - MIN_LEVEL_OBJ_GID]
                          : &obj_table_common.entries[obj->state.id],
                          &pos,
                          round);
            _pool_count += !(obj->props & OBJ_FLAG_DESTROYED);
        }
    }
}

void
object_pool_render(int32_t camera_x, int32_t camera_y)
{
    camera_x = (camera_x >> 12) - CENTERX;
    camera_y = (camera_y >> 12) - CENTERY;

    for(uint32_t i = 0; i < OBJECT_POOL_SIZE; i++) {
        PoolObject *obj = &_object_pool[i];

        // Inactive objects are discarded
        if(obj->props & OBJ_FLAG_DESTROYED) continue;

        // Calculate screen position
        int16_t px = (obj->freepos.vx >> 12) - camera_x;
        int16_t py = (obj->freepos.vy >> 12) - camera_y;

        object_render(&obj->state,
                      (obj->state.id >= MIN_LEVEL_OBJ_GID)
                      ? &obj_table_level.entries[obj->state.id - MIN_LEVEL_OBJ_GID]
                      : &obj_table_common.entries[obj->state.id],
                      px, py);
    }
}

PoolObject *
object_pool_create(ObjectType t)
{
    for(uint32_t i = 0; i < OBJECT_POOL_SIZE; i++) {
        if(_object_pool[i].props & OBJ_FLAG_DESTROYED) {
            /* ObjectTableEntry *entry = (t >= MIN_LEVEL_OBJ_GID) */
            /*     ? &obj_table_level.entries[t - MIN_LEVEL_OBJ_GID] */
            /*     : &obj_table_common.entries[t]; */
            
            // Prepare object for usage
            _object_pool[i] = (PoolObject){ 0 };
            _object_pool[i].props |= OBJ_FLAG_FREE_OBJECT;
            _object_pool[i].state.id = t;

            // Fragment animation data is ALWAYS initialized since we cannot
            // be certain if an object has a fragment or not. If it does,
            // the space will already be available
            _object_pool[i].state.frag_anim_state = &_object_pool[i].frag_state;

            // A little pointer for the actual object position in the world
            _object_pool[i].state.freepos = (ObjectFreePos *)&_object_pool[i].freepos;
            return (PoolObject *) &_object_pool[i];
        }
    }

    // Could not allocate from object pool!!!!!!
    // For now, hang the application to get the programmer's attention.
    assert(0);

    return NULL;
}

uint32_t
object_pool_get_count()
{
    return _pool_count;
}
