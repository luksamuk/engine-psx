// -*- mode: c; -*-

struct FrameData<auto size> {
    be u16 frames[size];
};

struct MapData {
    be u16 tile_width;
    be u16 numtiles;
    be u16 frame_side;
    FrameData<frame_side * frame_side> tiles[numtiles];
};

MapData data @ 0x00;

