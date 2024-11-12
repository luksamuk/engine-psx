// -*- mode: c; -*-

enum CommonId: s8 {
    RING = 0,
    MONITOR = 1,
    SPIKES = 2,
    CHECKPOINT = 3,
    SPRING_YELLOW = 4,
    SPRING_RED = 5,
    SPRING_YELLOW_DIAGONAL = 6,
    SPRING_RED_DIAGONAL = 7,
    GOAL_SIGN = 8,
    SWITCH = 9,
    EXPLOSION = 10,
};

bitfield FlipMask {
    padding     : 6;
    bool flip_y : 1;
    bool flip_x : 1;
};

struct Frame {
    u8 u0;
    u8 v0;
    u8 width;
    u8 height;
    FlipMask flipmask;
};

struct ObjectAnimation {
    be u16 num_frames;
    s8 loopback_frame;
    u8 duration;
    Frame frames[num_frames];
};

struct ObjectFragment {
    be s16 offset_x;
    be s16 offset_y;
    be u16 num_animations;
    ObjectAnimation animations[num_animations];
};

struct ObjectClass {
    CommonId id;
    u8 has_fragment;
    be u16 num_animations;
    ObjectAnimation animations[num_animations];

    if(has_fragment == 1) {
        ObjectFragment fragment;
    }
};

struct OdtData {
    u8 is_level_specific;
    be u16 num_classes;
    ObjectClass classes[num_classes];
};

OdtData data @ 0x00;
