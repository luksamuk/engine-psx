#ifndef PLAYER_CONSTANTS_H
#define PLAYER_CONSTANTS_H

#include <stdint.h>

#define MILES_GRAVITY_FLYDOWN 0x00000080
#define MILES_GRAVITY_FLYUP   0x00000200 // Fly up by SUBTRACTING this gravity

// Constants for running the game at a fixed 60 FPS.
// These constants are also in a 12-scale format for fixed point math.
typedef struct {
    int32_t
        x_accel,
        x_air_accel,
        x_friction,
        x_decel,
        x_top_spd,
        y_gravity,
        y_hurt_gravity,
        y_min_jump,
        y_jump_strength,
        x_min_roll_spd,
        x_min_uncurl_spd,
        x_roll_friction,
        x_roll_decel,
        x_slope_min_spd,
        x_slope_normal,
        x_slope_rollup,
        x_slope_rolldown,
        x_max_spd,
        x_map_slip_spd,
        x_drpspd,
        x_drpmax,
        y_hurt_force,
        x_hurt_force,
        x_peelout_spd;
} PlayerConstants;

// SEE PLAYER_CONSTANTS.C FOR CONSTANTS DEFINITIONS!!!!

#endif
