#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <psxgpu.h>
#include <psxgte.h>

typedef enum {
    TYPE_F3  = 0x0,
    TYPE_G3  = 0x1,
    TYPE_F4  = 0x2,
    TYPE_G4  = 0x3,
    TYPE_FT3 = 0x4,
    TYPE_GT3 = 0x5,
    TYPE_FT4 = 0x6,
    TYPE_GT4 = 0x7,
} ObjPolyType;

typedef struct {
    uint8_t ftype;
    uint8_t *info;
} ObjPolygon;

typedef struct {
    uint8_t  r0, b0, g0;
    uint16_t iv0, iv1, iv2;
    uint16_t in0;
} OBJF3;

typedef struct {
    uint8_t  r0, b0, g0;
    uint8_t  r1, b1, g1;
    uint8_t  r2, b2, g2;
    uint16_t iv0, iv1, iv2;
    uint16_t in0, in1, in2;
} OBJG3;

typedef struct {
    uint8_t  r0, b0, g0;
    uint16_t iv0, iv1, iv2, iv3;
    uint16_t in0;
} OBJF4;

typedef struct {
    uint8_t  r0, b0, g0;
    uint8_t  r1, b1, g1;
    uint8_t  r2, b2, g2;
    uint8_t  r3, b3, g3;
    uint16_t iv0, iv1, iv2, iv3;
    uint16_t in0, in1, in2, in3;
} OBJG4;

typedef struct {
    uint8_t  u0, v0;
    uint8_t  u1, v1;
    uint8_t  u2, v2;
    uint8_t  r0, b0, g0;
    uint16_t iv0, iv1, iv2;
    uint16_t in0;
} OBJFT3;

typedef struct {
    uint8_t  u0, v0;
    uint8_t  u1, v1;
    uint8_t  u2, v2;
    uint8_t  r0, b0, g0;
    uint8_t  r1, b1, g1;
    uint8_t  r2, b2, g2;
    uint16_t iv0, iv1, iv2;
    uint16_t in0, in1, in2;
} OBJGT3;

typedef struct {
    uint8_t  u0, v0;
    uint8_t  u1, v1;
    uint8_t  u2, v2;
    uint8_t  u3, v3;
    uint8_t  r0, b0, g0;
    uint16_t iv0, iv1, iv2, iv3;
    uint16_t in0;
} OBJFT4;

typedef struct {
    uint8_t  u0, v0;
    uint8_t  u1, v1;
    uint8_t  u2, v2;
    uint8_t  u3, v3;
    uint8_t  r0, b0, g0;
    uint8_t  r1, b1, g1;
    uint8_t  r2, b2, g2;
    uint8_t  r3, b3, g3;
    uint16_t iv0, iv1, iv2, iv3;
    uint16_t in0, in1, in2, in3;
} OBJGT4;

typedef struct {
    VECTOR  pos;
    VECTOR  scl;
    SVECTOR rot;
    MATRIX  world;

    uint16_t num_vertices, num_normals, num_polygons;
    SVECTOR    *vertices;
    SVECTOR    *normals;
    ObjPolygon *polygons;
} Model;



void load_model(Model *m, const char *filename);
void render_model(Model *m);

#endif
