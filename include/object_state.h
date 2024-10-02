#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H

#include <stdint.h>

#include "object.h"

/* ======================== */
/*  OBJECT STATE STRUCTURE */
/* ======================== */

#define MAX_OBJECTS_PER_CHUNK 15

typedef enum {
    OBJ_FLAG_DESTROYED = 0x1,
    OBJ_FLAG_INVISIBLE = 0x2,
    OBJ_FLAG_ANIM_LOCK = 0x4,
} ObjectGeneralFlag;

typedef struct {
    uint8_t kind;
} MonitorExtra;

typedef struct {
    uint16_t animation;
    uint8_t  frame;
    uint8_t  counter;
} ObjectAnimState;

typedef struct {
    uint16_t id;
    uint8_t flipmask;
    uint8_t props;
    int16_t rx, ry; // Positions relative to chunk top-left corner
    void    *extra;

    ObjectAnimState anim_state;
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

void object_update(ObjectState *state, ObjectTableEntry *typedata);

#endif
