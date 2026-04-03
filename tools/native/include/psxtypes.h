/*
 * psxtypes.h - PlayStation 1 style types for asset tools
 * Big-endian binary output helpers
 */

#ifndef PSXTYPES_H
#define PSXTYPES_H

#include <stdint.h>
#include <stdio.h>

/* Fixed-width types */
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;

/* PSX angle type (20.12 fixed point, 0-4096 for 360 degrees) */
typedef s32 psx_angle_t;

/* Fixed point conversion */
#define FIXED12_SHIFT 12
#define to_fixed12(f) ((s32)((f) * (1 << FIXED12_SHIFT)))
#define from_fixed12(i) ((float)(i) / (1 << FIXED12_SHIFT))

/* Big-endian write helpers */
static inline void write_u8(FILE *f, u8 val) {
    fwrite(&val, 1, 1, f);
}

static inline void write_s8(FILE *f, s8 val) {
    fwrite(&val, 1, 1, f);
}

static inline void write_u16_be(FILE *f, u16 val) {
    u8 buf[2] = { (u8)((val >> 8) & 0xFF), (u8)(val & 0xFF) };
    fwrite(buf, 1, 2, f);
}

static inline void write_s16_be(FILE *f, s16 val) {
    write_u16_be(f, (u16)val);
}

static inline void write_u32_be(FILE *f, u32 val) {
    u8 buf[4] = { 
        (u8)((val >> 24) & 0xFF),
        (u8)((val >> 16) & 0xFF),
        (u8)((val >> 8) & 0xFF),
        (u8)(val & 0xFF)
    };
    fwrite(buf, 1, 4, f);
}

static inline void write_s32_be(FILE *f, s32 val) {
    write_u32_be(f, (u32)val);
}

/* Angle conversion: degrees to PSX angle (0-360 = 0-4096) */
static inline psx_angle_t degrees_to_psx_angle(float degrees) {
    float ratio = degrees / 360.0f;
    return (psx_angle_t)(ratio * 4096.0f);
}

/* Vector type matching PSX GTE format */
typedef struct {
    s32 vx, vy, vz;
} VECTOR;

static inline void write_vector_be(FILE *f, const VECTOR *v) {
    write_s32_be(f, v->vx);
    write_s32_be(f, v->vy);
    write_s32_be(f, v->vz);
}

#endif /* PSXTYPES_H */
