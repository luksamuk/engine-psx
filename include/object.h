#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

/* ======================== */
/*  OBJECT PROPERTY DATA   */
/* ======================== */

typedef enum {
    // Dummy objects
    OBJ_DUMMY_RINGS_3V         = -2,
    OBJ_DUMMY_RINGS_3H         = -1,

    // Actual objects
    OBJ_RING                   = 0x00,
    OBJ_MONITOR                = 0x01,
    OBJ_SPIKES                 = 0x02,
    OBJ_CHECKPOINT             = 0x03,
    OBJ_SPRING_YELLOW          = 0x04,
    OBJ_SPRING_RED             = 0x05,
    OBJ_SPRING_YELLOW_DIAGONAL = 0x06,
    OBJ_SPRING_RED_DIAGONAL    = 0x07,
    OBJ_GOAL_SIGN              = 0x08,
    OBJ_SWITCH                 = 0x09,
} ObjectType;

#define MASK_FLIP_FLIPX  0x1 // Flip on X axis
#define MASK_FLIP_FLIPY  0x2 // Flip on Y axis
#define MASK_FLIP_ROTCW  0x4 // Rotated clockwise
#define MASK_FLIP_ROTCT  0x8 // Rotated counterclockwise

#define OBJ_ANIMATION_NO_ANIMATION  0xff

typedef enum {
    MONITOR_KIND_NONE          = 0,
    MONITOR_KIND_RING          = 1,
    MONITOR_KIND_SPEEDSHOES    = 2,
    MONITOR_KIND_SHIELD        = 3,
    MONITOR_KIND_INVINCIBILITY = 4,
    MONITOR_KIND_1UP           = 5,
    MONITOR_KIND_SUPER         = 6,
} ObjectMonitorKind;


/* ======================== */
/*  OBJECT TABLE STRUCTURE */
/* ======================== */

typedef struct {
    uint8_t u0, v0;
    uint8_t w, h;
    uint8_t flipmask;
} ObjectAnimFrame;

typedef struct {
    ObjectAnimFrame *frames;
    uint16_t        num_frames;
    int8_t          loopback;
    // TODO: Animation speed (animation frames per game frame)
} ObjectAnim;

typedef struct {
    int16_t    offsetx, offsety;
    ObjectAnim *animations;
    uint16_t   num_animations;
} ObjectFrag;

typedef struct {
    ObjectAnim *animations;
    ObjectFrag *fragment;
    uint16_t num_animations;
} ObjectTableEntry;

typedef struct {
    uint8_t          is_level_specific;
    uint16_t         num_entries;
    ObjectTableEntry *entries;
} ObjectTable;


void load_object_table(const char *filename, ObjectTable *tbl);


#endif
