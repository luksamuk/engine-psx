// -*- mode: c; -*-

#include <type/color.pat>

enum PolygonType: u8 {
    F3  = 0,
    G3  = 1,
    F4  = 2,
    G4  = 3,
    FT3 = 4,
    GT3 = 5,
    FT4 = 6,
    GT4 = 7,
};

struct Polygon {
    PolygonType ftype;
    if(ftype == PolygonType::F3) {
        le type::RGB8 rgb0;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 in0;
    } else if(ftype == PolygonType::G3) {
        le type::RGB8 rgb0;
        le type::RGB8 rgb1;
        le type::RGB8 rgb2;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 in0;
        be u16 in1;
        be u16 in2;
    } else if(ftype == PolygonType::F4) {
        le type::RGB8 rgb0;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 iv3;
        be u16 in0;
    } else if(ftype == PolygonType::G4) {
        le type::RGB8 rgb0;
        le type::RGB8 rgb1;
        le type::RGB8 rgb2;
        le type::RGB8 rgb3;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 iv3;
        be u16 in0;
        be u16 in1;
        be u16 in2;
        be u16 in3;
    } else if(ftype == PolygonType::FT3) {
        u8 u0; u8 v0;
        u8 u1; u8 v1;
        u8 u2; u8 v2;
        le type::RGB8 rgb0;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 in0;
    } else if(ftype == PolygonType::GT3) {
        u8 u0; u8 v0;
        u8 u1; u8 v1;
        u8 u2; u8 v2;
        le type::RGB8 rgb0;
        le type::RGB8 rgb1;
        le type::RGB8 rgb2;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 in0;
        be u16 in1;
        be u16 in2;
    } else if(ftype == PolygonType::FT4) {
        u8 u0; u8 v0;
        u8 u1; u8 v1;
        u8 u2; u8 v2;
        u8 u3; u8 v3;
        le type::RGB8 rgb0;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 iv3;
        be u16 in0;
    } else if(ftype == PolygonType::GT4) {
        u8 u0; u8 v0;
        u8 u1; u8 v1;
        u8 u2; u8 v2;
        u8 u3; u8 v3;
        le type::RGB8 rgb0;
        le type::RGB8 rgb1;
        le type::RGB8 rgb2;
        le type::RGB8 rgb3;
        be u16 iv0;
        be u16 iv1;
        be u16 iv2;
        be u16 iv3;
        be u16 in0;
        be u16 in1;
        be u16 in2;
        be u16 in3;
    } else {
        std::error(std::format("Undefined polygon type: {}\n", ftype));
    }
};

struct VECTOR {
    be s32 vx;
    be s32 vy;
    be s32 vz;
};

struct Model {
    be u16 num_vertices;
    be u16 num_normals;
    be u16 num_polys;
    VECTOR vertices[num_vertices];
    VECTOR normals[num_normals];
    Polygon polygons[num_polys];
};

Model data @ 0x00;