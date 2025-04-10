#ifndef PLAYER_H
#define PLAYER_H

#include "chara.h"
#include <psxgte.h>
#include "level.h"
#include "collision.h"
#include "input.h"
#include "player_constants.h"

// Constants for adjusting hitbox and sensors
#define WIDTH_RADIUS_NORMAL      9
#define HEIGHT_RADIUS_NORMAL    19
#define LEDGE_SENSOR_MAGNITUDE  26
#define WIDTH_RADIUS_ROLLING     7
#define HEIGHT_RADIUS_ROLLING   14
#define PUSH_RADIUS             10

#define PLAYER_HURT_IFRAMES     120
#define PLAYER_FLY_MAXFRAMES    480

typedef enum {
    CHARA_SONIC = 0,
    CHARA_MILES = 1,
    CHARA_KNUCKLES = 2,
} PlayerCharacter;

#define CHARA_MAX CHARA_KNUCKLES

typedef enum {
    ACTION_NONE,
    ACTION_SKIDDING,
    ACTION_LOOKUP,
    ACTION_CROUCHDOWN,
    ACTION_JUMPING,
    ACTION_ROLLING,
    ACTION_SPINDASH,
    ACTION_PEELOUT,
    ACTION_DROPDASH,
    ACTION_SPRING,
    ACTION_HURT,
    ACTION_GASP,
    ACTION_FLY,
    ACTION_GLIDE,
    ACTION_DROP,
} PlayerAction;

// Alias to make things look less weird
typedef LinecastDirection CollMode;

typedef enum {
    PC_DEFAULT,
    PC_UNDERWATER,
    PC_SPEEDSHOES,
} PlayerConstantType;

// declared in player_constants.c
PlayerConstants *getconstants(PlayerCharacter, PlayerConstantType);

typedef struct {
    InputState       input;
    PlayerConstants *cnst;
    Chara            chara;
    PlayerCharacter  character;

    CharaAnim *cur_anim;
    CharaAnim *tail_cur_anim;
    VECTOR    pos;
    VECTOR    vel; // vel.vz = ground speed
    int32_t   angle;
    uint8_t   anim_frame;
    uint8_t   anim_timer;
    uint8_t   tail_anim_frame;
    uint8_t   tail_anim_timer;
    uint8_t   frame_duration;
    uint8_t   loopback_frame;
    int8_t    anim_dir;
    uint8_t   idle_timer;
    uint8_t   grnd;
    uint8_t   ceil;
    uint8_t   push;
    uint32_t  spinrev;        // Also used as flight and glide direction toggle
    uint8_t   ctrllock;
    uint8_t   airdirlock;
    uint16_t  framecount;     // Used for many purposes incl. flight and glide
    uint8_t   holding_jump;
    uint16_t  iframes;
    uint8_t   shield;
    int32_t   speedshoes_frames;
    uint8_t   underwater;
    uint16_t  remaining_air_frames;
    int8_t    glide_turn_dir;

    // Collision modes
    CollMode  gsmode;
    CollMode  psmode;

    PlayerAction action;

    uint8_t        col_ledge;
    CollisionEvent ev_grnd1;
    CollisionEvent ev_grnd2;
    CollisionEvent ev_left;
    CollisionEvent ev_right;
    CollisionEvent ev_ceil1;
    CollisionEvent ev_ceil2;

    VECTOR startpos;
    VECTOR respawnpos;
} Player;

void load_player(Player *player, PlayerCharacter character, const char *chara_filename, TIM_IMAGE  *sprites);
void free_player(Player *player);

void      player_set_animation(Player *player, const char *name);
void      player_set_animation_precise(Player *player, const char *name);
void      player_set_animation_direct(Player *player, uint32_t sum);
void      player_set_frame_duration(Player *player, uint8_t duration);
uint32_t  player_get_current_animation_hash(Player *player);
CharaAnim *player_get_animation(Player *player, uint32_t sum);
CharaAnim *player_get_animation_by_name(Player *player, const char *name);
void      player_set_action(Player *player, PlayerAction action);

void player_update(Player *player);
void player_draw(Player *player, VECTOR *screen_pos);

void player_do_damage(Player *player, int32_t hazard_x);

#endif
