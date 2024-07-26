// -*- mode: c; -*-

struct LayerData {
    u8 width;
    u8 height;
    be u16 tiles[width * height];
};

struct LvlData {
    u8 num_layers;
    padding[1];
    LayerData layers[num_layers];
};

LvlData level @ 0x00;
