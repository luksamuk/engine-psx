#ifndef OBJECT_STATE_H
#define OBJECT_STATE_H

#include <stdint.h>

/* ======================== */
/*  OBJECT STATE STRUCTURE */
/* ======================== */

typedef struct {
    uint8_t kind;
} MonitorExtra;

typedef struct {
    uint16_t id;
    uint8_t flipmask;
    uint8_t props;
    int16_t rx, ry; // Positions relative to chunk top-left corner
    void    *extra;
} ObjectState;

#endif
