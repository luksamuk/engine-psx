#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H

#include <stdint.h>
#include <psxgte.h>

#include "object.h"

/* ======================== */
/*  OBJECT STATE STRUCTURE */
/* ======================== */

#define MAX_OBJECTS_PER_CHUNK 15

// Enumeration describing useful flags for objects.
// Notice that these flags may overlap for different objects.
// LSB is always reserved for object system flags; MSB is for object-specific values.
// ...this might enable arbitrary code execution somewhere...
typedef enum {
    OBJ_FLAG_DESTROYED   = 0x01, // Object is destroyed
    OBJ_FLAG_INVISIBLE   = 0x02, // Object is visible but doesn't render
    OBJ_FLAG_ANIM_LOCK   = 0x04, // Object animation is locked with level timer
    OBJ_FLAG_FREE_OBJECT = 0x08, // Object has free position (is from object pool)

    // Checkpoint-only flags
    OBJ_FLAG_CHECKPOINT_ACTIVE = 0x10, // Checkpoint is active

    // Rings-only flags
    OBJ_FLAG_RING_MOVING = 0x10, // Ring is moving (ring loss)

    // Switch-only flags
    OBJ_FLAG_SWITCH_PRESSED = 0x10,
} ObjectGeneralFlag;

typedef struct {
    uint8_t kind;
} MonitorExtra;

typedef struct {
    // Defined during design
    uint8_t frequency;

    // Runtime variables
    uint8_t timer; // Decrementing random timer. idle=[128, 255]; producing=[0, 31]
    uint8_t state; // 0 = idle, 1 = producing
    uint8_t num_bubbles; // num=[1, 6] at every producing delay end
    uint8_t bubble_set; // bubble set picked at random=[0, 3]
    uint8_t cycle; // large bubble cycle counter
    uint8_t bubble_idx;
    uint8_t produced_big; // Whether a big bubble was produced this cycle
} BubblePatchExtra;

typedef struct {
    uint16_t animation;
    uint8_t  frame;
    uint8_t  counter;
} ObjectAnimState;

typedef struct {
    // World XY position
    int32_t vx;
    int32_t vy;

    // XY velocity
    int32_t spdx;
    int32_t spdy;

    // Relative XY position (if needed)
    int32_t rx;
    int32_t ry;
} ObjectFreePos;

typedef struct {
    uint8_t props; // IMPORTANT: DO NOT MOVE THIS FIELD.
    uint8_t flipmask;
    uint16_t id;
    int16_t rx, ry; // Positions relative to chunk top-left corner
    void    *extra;
    int16_t timer;

    ObjectAnimState anim_state;
    ObjectAnimState *frag_anim_state; // Only exists if fragment also exists
    ObjectFreePos *freepos; // Only exists if object lives in object pool
} ObjectState;

typedef struct {
    uint8_t num_objects;
    ObjectState objects[MAX_OBJECTS_PER_CHUNK];
} ChunkObjectData;

// ATTENTION: Coordinates used are already the hotspot coordinates
// for the object on the screen, so they must be from after camera
// transformation!
void object_render(ObjectState *state, ObjectTableEntry *typedata,
                   int16_t vx, int16_t vy);

// ATTENTION: "pos" does not influence in object position, ever.
// If the current object lives in object pool and can freely be moved, alter
// its position and speed by using the 'freepos' field.
void object_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos);


/* ============================== */
/* OBJECTS OWNED BY OBJECT POOL */
/* ============================== */

// A single ring loss is at least 32 rings, so this is our minimum!
// Also, notice that, even if we don't have any objects on the screen,
// object initialization, update and even creation is O(n), so it is
// inefficient by design. Be careful here.
#define OBJECT_POOL_SIZE 128

// This represents an object owned by the memory pool.
// If state.props & OBJ_FLAG_DESTROYED, the object is never
// updated.
typedef struct {
    union {
        uint8_t props; // IMPORTANT: DO NOT MOVE THIS FIELD.
        ObjectState state;
    };
    ObjectFreePos freepos;
} PoolObject;

void       object_pool_init();
void       object_pool_update(ObjectTable *tbl, ObjectTable *ltbl);
void       object_pool_render(ObjectTable *tbl, ObjectTable *ltbl, int32_t camera_x, int32_t camera_y);
PoolObject *object_pool_create(ObjectType t); // ...you should probably not create objects with extra data.
uint32_t   object_pool_get_count();

#endif
