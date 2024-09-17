// -*- mode: c; -*-

struct Frame16 {
    be u16 index;
    u8 props;
};

struct FrameData<auto size> {
    Frame16 frames[size];
};

struct MapData {
    be u16 tile_width;
    be u16 numtiles;
    be u16 frame_side;
    FrameData<frame_side * frame_side> tiles[numtiles];
};

MapData data @ 0x00;

