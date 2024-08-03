#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "player.h"
#include "util.h"
#include "input.h"
#include "render.h"
#include "sound.h"

#define TMP_ANIM_SPD          7
#define ANIM_IDLE_TIMER_MAX 180
#define DUMMY_GROUND        ((432 - 64 - 8) << 12)

// Adler32 sums of animation names for ease of use
#define ANIM_STOPPED    0x08cd0220
#define ANIM_IDLE       0x02d1011f
#define ANIM_WALKING    0x0854020e
#define ANIM_RUNNING    0x08bf0222
#define ANIM_ROLLING    0x08890218
#define ANIM_SKIDDING   0x0a85024e
#define ANIM_PEELOUT    0x0849021f
#define ANIM_PUSHING    0x08b2021f
#define ANIM_CROUCHDOWN 0x104802fd
#define ANIM_LOOKUP     0x067001db

SoundEffect sfx_jump = { 0 };

void
load_player(Player *player,
            const char *chara_filename,
            TIM_IMAGE  *sprites)
{
    load_chara(&player->chara, chara_filename, sprites);
    player->cur_anim = NULL;
    player->pos = (VECTOR){ 0 };
    player->vel = (VECTOR){ 0 };
    player_set_animation_direct(player, ANIM_STOPPED);
    player->anim_frame = player->anim_timer = 0;
    player->anim_dir = 1;
    player->idle_timer = ANIM_IDLE_TIMER_MAX;
    player->grnd = player->jmp = 0;

    if(sfx_jump.addr == 0) sfx_jump = sound_load_vag("\\SFX\\JUMP.VAG;1");
}

void
free_player(Player *player)
{
    free_chara(&player->chara);
}

CharaAnim *
player_get_animation(Player *player, uint32_t sum)
{
    for(uint16_t i = 0; i < player->chara.numanims; i++) {
        if(player->chara.anims[i].hname == sum)
            return &player->chara.anims[i];
    }
    return NULL;
}

CharaAnim *
player_get_animation_by_name(Player *player, const char *name)
{
    for(uint16_t i = 0; i < player->chara.numanims; i++) {
        if(strncmp(player->chara.anims[i].name, name, 16) == 0)
            return &player->chara.anims[i];
    }
    return NULL;
}

void
_set_animation_underlying(Player *player, CharaAnim *anim)
{
    if(player->cur_anim == anim) return;
    player->cur_anim = anim;
    player->anim_frame = player->anim_timer = 0;
    if(anim) {
        player->anim_frame = anim->start;
        player->anim_timer = TMP_ANIM_SPD; // TODO: Use actual animation fps
    }
}

void
player_set_animation(Player *player, const char *name)
{
    _set_animation_underlying(player, player_get_animation(player, adler32(name)));
}

void
player_set_animation_precise(Player *player, const char *name)
{
    _set_animation_underlying(player, player_get_animation_by_name(player, name));
}

void
player_set_animation_direct(Player *player, uint32_t sum)
{
    _set_animation_underlying(player, player_get_animation(player, sum));
}

void
player_update(Player *player)
{
    if(player->cur_anim) {
        if(player->cur_anim->start == player->cur_anim->end)
            player->anim_frame = player->cur_anim->start;
        else if(player->anim_timer == 0) {
            player->anim_timer = TMP_ANIM_SPD; // TODO: Use actual animation fps
            player->anim_frame++;
            if(player->anim_frame > player->cur_anim->end)
                player->anim_frame = player->cur_anim->start;
        } else player->anim_timer--;
    }

    // X movement
    if(pad_pressing(PAD_RIGHT)) {
        if(player->grnd && (player->vel.vx < 0)) {
            player->vel.vx += X_DECEL;
        } else {
            player->vel.vx += X_ACCEL;
            player->anim_dir = 1;
        }
    } else if(pad_pressing(PAD_LEFT)) {
        if(player->grnd && (player->vel.vx > 0)) {
            player->vel.vx -= X_DECEL;
        } else {
            player->vel.vx -= X_ACCEL;
            player->anim_dir = -1;
        }
    } else {
        player->vel.vx -= (player->vel.vx > 0 ? X_FRICTION : -X_FRICTION);
        if(abs(player->vel.vx) <= X_FRICTION) player->vel.vx = 0;
    }

    if(player->vel.vx > X_TOP_SPD) player->vel.vx = X_TOP_SPD;
    else if(player->vel.vx < -X_TOP_SPD) player->vel.vx = -X_TOP_SPD;

    // Y movement
    if(!player->grnd) {
        player->vel.vy += Y_GRAVITY;

        if(player->pos.vy >= DUMMY_GROUND) {
            player->vel.vy = 0;
            player->pos.vy = DUMMY_GROUND;
            player->grnd = 1;
            player->jmp = 0;
        }

        if(player->jmp
           && !pad_pressing(PAD_CROSS)
           && player->vel.vy < -Y_MIN_JUMP) {
            player->vel.vy = -Y_MIN_JUMP;
        }
    } else {
        if(pad_pressed(PAD_CROSS)) {
            player->vel.vy = -Y_JUMP_STRENGTH;
            player->grnd = 0;
            player->jmp = 1;
            player_set_animation_direct(player, ANIM_ROLLING);
            sound_play_vag(sfx_jump);
        }
    }

    // Animation
    if(player->grnd) {
        if(player->vel.vx == 0) {
            if(pad_pressing(PAD_UP)) {
                player_set_animation_direct(player, ANIM_LOOKUP);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
            } else if(pad_pressing(PAD_DOWN)) {
                player_set_animation_direct(player, ANIM_CROUCHDOWN);
                player->idle_timer = ANIM_IDLE_TIMER_MAX;
            } else if(player->idle_timer == 0) {
                player_set_animation_direct(player, ANIM_IDLE);
            } else if (!pad_pressing(PAD_LEFT) && !pad_pressing(PAD_RIGHT)) {
                player_set_animation_direct(player, ANIM_STOPPED);
                if(player->idle_timer > 0) player->idle_timer--;
            }
        } else {
            player->idle_timer = ANIM_IDLE_TIMER_MAX;
            if(abs(player->vel.vx) > (5 * ONE))
                player_set_animation_direct(player, ANIM_RUNNING);
            else player_set_animation_direct(player, ANIM_WALKING);
        }
    }

    // Screen transform
    player->pos.vx += player->vel.vx;
    player->pos.vy += player->vel.vy;
}

void
player_draw(Player *player, VECTOR *pos)
{
    if(player->cur_anim)
        chara_render_frame(&player->chara,
                           player->anim_frame,
                           (int16_t)(pos->vx >> 12),
                           (int16_t)(pos->vy >> 12),
                           /* (int16_t)(player->pos.vx >> 12), */
                           /* (int16_t)(player->pos.vy >> 12), */
                           player->anim_dir < 0);
}
