// -*- mode: c; -*-

struct ParallaxPart {
       u8 u0;
       u8 v0;
       u8 tex_idx;
       be u16 width;
       be u16 height;
};

struct ParallaxStrip {
       u8 num_parts;
       u8 is_single;
       be s16 scrollx;
       be s16 y0;
       ParallaxPart parts[num_parts];
};

struct Parallax {
       u8 num_strips;
       ParallaxStrip strips[num_strips];
};

Parallax parallax @ 0x0;
