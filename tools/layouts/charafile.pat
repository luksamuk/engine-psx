struct FrameData {
    u8 x;
    u8 y;
    u8 columns;
    u8 rows;
    be u16 width;
    be u16 height;
    be u16 tiles[rows * columns];
};

struct AnimData {
    u8 name[16];
    u8 start;
    u8 end;
};

struct CharaData {
    be u16 width;
    be u16 height;
    be u16 numframes;
    be u16 numanims;
    FrameData frames[numframes];
    AnimData anims[numanims];
};

CharaData data @ 0x00;
