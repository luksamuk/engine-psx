#ifndef PLAYER_H
#define PLAYER_H

#include "chara.h"
#include <psxgte.h>
#include "level.h"
#include "collision.h"

// Constants for running the game at a fixed 60 FPS.
// These constants are also in a 12-scale format for fixed point math.
#define X_ACCEL          0x000c0
#define X_AIR_ACCEL      0x00180
#define X_FRICTION       0x000c0
#define X_DECEL          0x00800
#define X_TOP_SPD        0x06000
#define Y_GRAVITY        0x00380
#define Y_MIN_JUMP       0x04000
#define Y_JUMP_STRENGTH  0x06800
#define X_MIN_ROLL_SPD   0x01000
#define X_MIN_UNCURL_SPD 0x00800
#define X_ROLL_FRICTION  0x00060
#define X_ROLL_DECEL     0x00200
#define X_SLOPE_MIN_SPD  0x000d0
#define X_SLOPE_NORMAL   0x00200
#define X_SLOPE_ROLLUP   0x00140
#define X_SLOPE_ROLLDOWN 0x00500
#define X_MAX_SPD        0x10000
#define X_MAX_SLIP_SPD   0x02800
#define X_DRPSPD         0x08000
#define X_DRPMAX         0x0c000

// Constants for adjusting hitbox and sensors
#define WIDTH_RADIUS_NORMAL      8
#define HEIGHT_RADIUS_NORMAL    19
#define WIDTH_RADIUS_ROLLING     7
#define HEIGHT_RADIUS_ROLLING   14
#define PUSH_RADIUS             14

typedef enum {
    ACTION_NONE,
    ACTION_SKIDDING,
    ACTION_LOOKUP,
    ACTION_CROUCHDOWN,
    ACTION_JUMPING,
    ACTION_ROLLING,
    ACTION_SPINDASH,
    ACTION_DROPDASH
} PlayerAction;

typedef struct {
    Chara     chara;

    CharaAnim *cur_anim;
    VECTOR    pos;
    VECTOR    vel; // vel.vz = ground speed
    int32_t   angle;
    uint8_t   anim_frame;
    uint8_t   anim_timer;
    uint8_t   frame_duration;
    uint8_t   loopback_frame;
    int8_t    anim_dir;
    uint8_t   idle_timer;
    uint8_t   grnd;
    uint8_t   push;
    uint32_t  spinrev;
    uint8_t   ctrllock;
    uint8_t   framecount;
    uint8_t   holding_jump;

    PlayerAction action;

    CollisionEvent ev_grnd1;
    CollisionEvent ev_grnd2;
    CollisionEvent ev_left;
    CollisionEvent ev_right;
    CollisionEvent ev_ceil1;
    CollisionEvent ev_ceil2;
} Player;

void load_player(Player *player, const char *chara_filename, TIM_IMAGE  *sprites);
void free_player(Player *player);

void      player_set_animation(Player *player, const char *name);
void      player_set_animation_precise(Player *player, const char *name);
void      player_set_animation_direct(Player *player, uint32_t sum);
void      player_set_frame_duration(Player *player, uint8_t duration);
uint32_t  player_get_current_animation_hash(Player *player);
CharaAnim *player_get_animation(Player *player, uint32_t sum);
CharaAnim *player_get_animation_by_name(Player *player, const char *name);

void player_update(Player *player);
void player_draw(Player *player, VECTOR *screen_pos);

#endif
