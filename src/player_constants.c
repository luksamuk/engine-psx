#include "player_constants.h"

// Default player values
PlayerConstants CNST_DEFAULT = {
    .x_accel          = 0x000c0,
    .x_air_accel      = 0x00180,
    .x_friction       = 0x000c0,
    .x_decel          = 0x00800,
    .x_top_spd        = 0x06000,
    .y_gravity        = 0x00380,
    .y_hurt_gravity   = 0x00300,
    .y_min_jump       = 0x04000,
    .y_jump_strength  = 0x06800,
    .x_min_roll_spd   = 0x01000,
    .x_min_uncurl_spd = 0x00800,
    .x_roll_friction  = 0x00060,
    .x_roll_decel     = 0x00200,
    .x_slope_min_spd  = 0x000d0,
    .x_slope_normal   = 0x00200,
    .x_slope_rollup   = 0x00140,
    .x_slope_rolldown = 0x00500,
    .x_max_spd        = 0x10000,
    .x_map_slip_spd   = 0x02800,
    .x_drpspd         = 0x08000,
    .x_drpmax         = 0x0c000,
    .y_hurt_force     = 0x04000,
    .x_hurt_force     = 0x02000
};

// Underwater player values.
// Most values are simply halved from base defaults
PlayerConstants CNST_UNDERWATER = {
    .x_accel          = 0x00060, // Changed
    .x_air_accel      = 0x000c0, // Changed
    .x_friction       = 0x00060, // Changed
    .x_decel          = 0x00400, // Changed
    .x_top_spd        = 0x03000, // Changed
    .y_gravity        = 0x00100, // Changed
    .y_hurt_gravity   = 0x000c0, // Changed, not documented, y_gravity - half diff on default
    .y_min_jump       = 0x02000, // Changed
    .y_jump_strength  = 0x03800, // Changed
    .x_min_roll_spd   = 0x01000,
    .x_min_uncurl_spd = 0x00800,
    .x_roll_friction  = 0x00030, // Changed
    .x_roll_decel     = 0x00200,
    .x_slope_min_spd  = 0x000d0,
    .x_slope_normal   = 0x00200,
    .x_slope_rollup   = 0x00140,
    .x_slope_rolldown = 0x00500,
    .x_max_spd        = 0x10000,
    .x_map_slip_spd   = 0x02800,
    .x_drpspd         = 0x08000,
    .x_drpmax         = 0x0c000,
    .y_hurt_force     = 0x02000, // Changed
    .x_hurt_force     = 0x01000  // Changed
};

// Speed shoes (only used outside water)
PlayerConstants CNST_SPEEDSHOES = {
    .x_accel          = 0x00180, // Changed
    .x_air_accel      = 0x00300, // Changed
    .x_friction       = 0x00180, // Changed
    .x_decel          = 0x00800,
    .x_top_spd        = 0x0c000, // Changed
    .y_gravity        = 0x00380,
    .y_hurt_gravity   = 0x00300,
    .y_min_jump       = 0x04000,
    .y_jump_strength  = 0x06800,
    .x_min_roll_spd   = 0x01000,
    .x_min_uncurl_spd = 0x00800,
    .x_roll_friction  = 0x000c0, // Changed
    .x_roll_decel     = 0x00200,
    .x_slope_min_spd  = 0x000d0,
    .x_slope_normal   = 0x00200,
    .x_slope_rollup   = 0x00140,
    .x_slope_rolldown = 0x00500,
    .x_max_spd        = 0x10000,
    .x_map_slip_spd   = 0x02800,
    .x_drpspd         = 0x08000,
    .x_drpmax         = 0x0c000,
    .y_hurt_force     = 0x04000,
    .x_hurt_force     = 0x02000
};
