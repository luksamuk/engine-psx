#include <cstdio>
#include <cstdlib>
#include "fixedpoint.hpp"

struct SVECTOR
{
    int16_t vx;
    int16_t vy;
    int16_t vz;
    //int16_t _unused;
};

struct CVECTOR
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

/* Face attributes */
#define TYPE_TRIANGLE 0x0  // ARGS: FLAG V0 V1 V2 __ N0 N1 N2 __
#define TYPE_QUAD     0x1  // ARGS: FLAG V0 V1 V2 V3 N0 N1 N2 N3
#define TYPE_LINE     0x2  // ARGS: FLAG V0 V1 __ __ __ __ __ __
#define TYPE_SPRITE   0x3  // ARGS: FLAG V0 W  H

union Face
{
    struct {
        uint8_t flag;
        uint16_t param[8];
    } gn; // Generic

    struct {
        uint8_t  flag; // flag == 0
        uint16_t iv0;
        uint16_t iv1;
        uint16_t iv2;
        uint16_t _unused0;
        uint16_t in0;
        uint16_t in1;
        uint16_t in2;
        uint16_t _unused1;
    } triangle;

    struct {
        uint8_t  flag; // flag == 1
        uint16_t iv0;
        uint16_t iv1;
        uint16_t iv2;
        uint16_t iv3;
        uint16_t in0;
        uint16_t in1;
        uint16_t in2;
        uint16_t in3;
    } quad;

    struct {
        uint8_t  flag; // flag == 2
        uint16_t iv0;
        uint16_t iv1;
        uint16_t __unused0;
        uint16_t __unused1;
        uint16_t __unused2;
        uint16_t __unused3;
        uint16_t __unused4;
        uint16_t __unused5;
    } line;

    struct {
        uint8_t  flag; // flag == 3
        uint16_t iv0;
        uint16_t iv1;
        uint16_t iw;
        uint16_t ih;
        uint16_t __unused2;
        uint16_t __unused3;
        uint16_t __unused4;
        uint16_t __unused5;
    } sprite;
};

// ========================

enum MaterialType
{
    MATERIAL_TYPE_C, // "C": Flat colored
    MATERIAL_TYPE_G, // "G": Gouraud shaded
    MATERIAL_TYPE_T, // "T": Textured no-color
    MATERIAL_TYPE_D, // "T": Textured, flat colored
    MATERIAL_TYPE_H, // "H": Textured, gouraud shaded

    // MATERIAL_TYPE_W, // "W": Repeating textures, no-color
    // MATERIAL_TYPE_S, // "S": Repeating textures, flat colored
    // MATERIAL_TYPE_N, // "N": Repeating textures, gouraud shaded
};

union MaterialInfo
{
    MaterialType type;

    struct {
        MaterialType type;
        uint8_t r0;
        uint8_t g0;
        uint8_t b0;
    } f;

    struct {
        MaterialType type;
        uint8_t r0;
        uint8_t g0;
        uint8_t b0;
        uint8_t r1;
        uint8_t g1;
        uint8_t b1;
        uint8_t r2;
        uint8_t g2;
        uint8_t b2;
        uint8_t r3;
        uint8_t g3;
        uint8_t b3;
    } g;

    struct {
        MaterialType type;
        uint8_t u0;
        uint8_t v0;
        uint8_t u1;
        uint8_t v1;
        uint8_t u2;
        uint8_t v2;
        uint8_t u3;
        uint8_t v3;
    } t;

    struct {
        MaterialType type;
        uint8_t u0;
        uint8_t v0;
        uint8_t u1;
        uint8_t v1;
        uint8_t u2;
        uint8_t v2;
        uint8_t u3;
        uint8_t v3;
        uint8_t r0;
        uint8_t g0;
        uint8_t b0;
    } ft;

    struct {
        MaterialType type;
        uint8_t u0;
        uint8_t v0;
        uint8_t u1;
        uint8_t v1;
        uint8_t u2;
        uint8_t v2;
        uint8_t u3;
        uint8_t v3;
        uint8_t r0;
        uint8_t g0;
        uint8_t b0;
        uint8_t r1;
        uint8_t g1;
        uint8_t b1;
        uint8_t r2;
        uint8_t g2;
        uint8_t b2;
        uint8_t r3;
        uint8_t g3;
        uint8_t b3;
    } gt;

    // Won't care about the rest
};

enum ShadingType
{
    SHADING_TYPE_FLAT,    // "F"
    SHADING_TYPE_GOURAUD, // "G"
};

struct Material
{
    // Can be in 0-5 format, specifies polygons 0 thru 5 use this material
    uint16_t     ipolygon;
    uint8_t      flag;
    ShadingType  shading;
    MaterialInfo info;
};

// ========================

struct PlyData
{
    char magic[11];
    uint32_t num_vertices;
    uint32_t num_normals;
    uint32_t num_faces;
    SVECTOR *vertices;
    SVECTOR *normals;
    Face    *faces;
};

struct MatData
{
    char     magic[11];
    uint16_t num_items;
    Material *data;
};

// ========================

struct RSDModel
{
    char    magic[11];
    PlyData ply;
    MatData mat;
    // TODO: NTEX and TEX[n] for textures
};

// ========================

void
parse_rsd(const char *filename, RSDModel &model)
{
}

void
parse_ply(const char *filename, PlyData &ply)
{
}

void
parse_mat(const char *filename, MatData &mat)
{
}

int
main(void)
{
    // Read and parse RSD file and its acessories
    // Convert RSD to actual engine model format (still don't know what to do here...)
    return 0;
}
