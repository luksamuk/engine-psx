#ifndef PLAYER_H
#define PLAYER_H

#include "chara.h"
#include <psxgte.h>

#define X_ACCEL            192
#define X_FRICTION         192
#define X_DECEL           2048
#define X_TOP_SPD        24576
#define Y_GRAVITY          896
#define Y_MIN_JUMP       16384
#define Y_JUMP_STRENGTH  26624

typedef struct {
    Chara     chara;

    CharaAnim *cur_anim;
    VECTOR    pos;
    VECTOR    vel; // vel.vz = ground speed
    uint8_t   anim_frame;
    uint8_t   anim_timer;
    int8_t    anim_dir;
    uint8_t   idle_timer;
    uint8_t   grnd;
    uint8_t   jmp;
} Player;

void load_player(Player *player, const char *chara_filename, TIM_IMAGE  *sprites);
void free_player(Player *player);

void      player_set_animation(Player *player, const char *name);
void      player_set_animation_precise(Player *player, const char *name);
void      player_set_animation_direct(Player *player, uint32_t sum);
CharaAnim *player_get_animation(Player *player, uint32_t sum);
CharaAnim *player_get_animation_by_name(Player *player, const char *name);

void player_update(Player *player);
void player_draw(Player *player, VECTOR *screen_pos);

#endif
