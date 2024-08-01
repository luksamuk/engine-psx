// -*- mode: c; -*-

bitfield HeightMask {
    unsigned c0 : 4;
    unsigned c1 : 4;
    unsigned c2 : 4;
    unsigned c3 : 4;
    unsigned c4 : 4;
    unsigned c5 : 4;
    unsigned c6 : 4;
    unsigned c7 : 4;
    unsigned c8 : 4;
    unsigned c9 : 4;
    unsigned cA : 4;
    unsigned cB : 4;
    unsigned cC : 4;
    unsigned cD : 4;
    unsigned cE : 4;
    unsigned cF : 4;
};

struct Collision {
    be u16 tile_id;
    // Height mask downwards (left to right)
    be HeightMask floor;
    // Height mask to right (bottom to top)
    be HeightMask rwall;
    // Height mask upwards (right to left)
    be HeightMask ceiling;
    // Height mask to left (top to bottom)
    be HeightMask lwall;
};

struct TileData {
    be u16 num_tiles;
    Collision tiles[num_tiles];
};

TileData tiles @ 0x0;
