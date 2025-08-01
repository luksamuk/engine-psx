#ifndef SCREENS_LEVEL_H
#define SCREENS_LEVEL_H

#include "player.h"

typedef enum {
    LEVEL_MODE_NORMAL,
    LEVEL_MODE_RECORD,
    LEVEL_MODE_DEMO,
    LEVEL_MODE_FINISHED,
    LEVEL_MODE_FINISHED2,
} LEVELMODE;

typedef enum {
    LEVEL_TRANS_TITLECARD = 0,
    LEVEL_TRANS_FADEIN = 1,
    LEVEL_TRANS_GAMEPLAY = 2,
    LEVEL_TRANS_SCORE_IN = 3,
    LEVEL_TRANS_SCORE_COUNT = 4,
    LEVEL_TRANS_SCORE_OUT = 5,
    LEVEL_TRANS_FADEOUT = 6,
    LEVEL_TRANS_NEXT_LEVEL = 7,
    LEVEL_TRANS_DEATH_WAIT = 8,
    LEVEL_TRANS_DEATH_FADEOUT = 9,
} LEVEL_TRANSITION;

void screen_level_load();
void screen_level_unload(void *);
void screen_level_update(void *);
void screen_level_draw(void *);

void      screen_level_setlevel(uint8_t menuchoice);
uint8_t   screen_level_getlevel(void);

LEVEL_TRANSITION screen_level_getstate();

uint16_t  screen_level_get_counter();

void      screen_level_setmode(LEVELMODE mode);
LEVELMODE screen_level_getmode();

void            screen_level_setcharacter(PlayerCharacter character);
PlayerCharacter screen_level_getcharacter();

void            screen_level_boss_lock(uint8_t state);
void            screen_level_play_music(uint8_t round, uint8_t act);
uint8_t         screen_level_give_1up(int8_t ring_cent);
void            screen_level_give_rings(uint16_t amount);

void screen_level_transition_start_timer();
void screen_level_transition_to_next();
void screen_level_transition_death();

#endif
