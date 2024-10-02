#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H

#include <stdint.h>

/* ======================== */
/*  OBJECT STATE STRUCTURE */
/* ======================== */

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
} ObjectAnimState;

typedef struct {
    uint16_t id;
    uint8_t flipmask;
    uint8_t props;
    int16_t rx, ry; // Positions relative to chunk top-left corner
    void    *extra;

    ObjectAnimState anim_state;
} ObjectState;

#endif
