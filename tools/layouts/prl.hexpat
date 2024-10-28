// -*- mode: c; -*-

struct ParallaxStrip {
       u8 u0;
       u8 v0;
       be u16 width;
       be u16 height;
       u8 tex_idx;
       u8 is_single;
       be s32 scrollx;
       be s32 speedx;
       be s16 y0;
};

struct Parallax {
       u8 num_strips;
       ParallaxStrip strips[num_strips];
};

Parallax parallax @ 0x0;
