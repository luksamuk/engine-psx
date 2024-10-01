// -*- mode: c; -*-

enum ObjectType: s8 {
    RING_3V = 0xfe, // -2
    RING_3H = 0xff, // -1

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

struct ObjectPlacement {
    u8 is_level_specific;
    ObjectType type;
    FlipMask flipmask;
    be s32 vx;
    be s32 vy;

    // Properties, type-dependent
    if(type == ObjectType::MONITOR) {
        MonitorProperties properties;
    }
};

struct LevelObjectLayout {
    be u16 num_objects;
    ObjectPlacement placements[num_objects];
};

LevelObjectLayout data @ 0x00;
