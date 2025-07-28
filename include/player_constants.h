#ifndef PLAYER_CONSTANTS_H
#define PLAYER_CONSTANTS_H

#include <stdint.h>

#define MILES_GRAVITY_FLYDOWN 0x00000080
#define MILES_GRAVITY_FLYUP   0x00000200 // Fly up by SUBTRACTING this gravity

#define KNUX_GLIDE_X_ACCEL    0x00000040
#define KNUX_GLIDE_X_TOPSPD   0x00018000
#define KNUX_GLIDE_GRAVITY    0x00000200 // May be added or subtracted
#define KNUX_GLIDE_TURN_STEP  0x00000020 // 20.12 scale for a range [0.5 -- 0]
#define KNUX_GLIDE_FRICTION   0x00000200

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
        x_jump_away, // Knuckles only
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
        x_max_slip_spd,
        x_drpspd,
        x_drpmax,
        y_hurt_force,
        x_hurt_force,
        x_peelout_spd,
        y_dead_force;
} PlayerConstants;

// SEE PLAYER_CONSTANTS.C FOR CONSTANTS DEFINITIONS!!!!

#endif
