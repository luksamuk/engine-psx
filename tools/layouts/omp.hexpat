// -*- mode: c; -*-

enum ObjectType: s8 {
    RING_3V = 0xfe, // -2
    RING_3H = 0xff, // -1

    RING = 0x0,
    MONITOR = 0x1,
    SPIKES = 0x2,
    CHECKPOINT = 0x3,
    SPRING_YELLOW = 0x4,
    SPRING_RED = 0x5,
    SPRING_YELLOW_DIAGONAL = 0x6,
    SPRING_RED_DIAGONAL = 0x7,
    SWITCH = 0x8,
    GOAL_SIGN = 0x9,
    EXPLOSION = 0xa,
    MONITOR_IMAGE = 0xb,
    SHIELD = 0xc,
    BUBBLE_PATCH = 0xd,
    BUBBLE = 0xe
};

enum MonitorKind: u8 {
     NONE = 0,
     RING = 1,
     SPEEDSHOES = 2,
     SHIELD = 3,
     INVINCIBILITY = 4,
     LIFE = 5, // a.k.a. 1UP
     SUPER = 6,
};

bitfield FlipMask {
    padding                   : 4;
    bool rot_counterclockwise : 1;
    bool rot_clockwise        : 1;
    bool flip_y               : 1;
    bool flip_x               : 1;
};

struct MonitorProperties {
    MonitorKind kind;
};

struct BubblePatchProperties {
    u8 frequency;
};

struct ObjectPlacement {
    u8 is_level_specific;
    ObjectType type;
    FlipMask flipmask;
    be s32 vx;
    be s32 vy;

    // Properties, type-dependent
    if(type == ObjectType::MONITOR) {
        MonitorProperties properties;
    } else if(type == ObjectType::BUBBLE_PATCH) {
        BubblePatchProperties properties;
    }
};

struct LevelObjectLayout {
    be u16 num_objects;
    ObjectPlacement placements[num_objects];
};

LevelObjectLayout data @ 0x00;
