#ifndef BOSS_H
#define BOSS_H

#include <stdint.h>
#include <psxgte.h>

typedef struct {
    VECTOR anchor;
    uint8_t state;
    uint8_t health;
    uint8_t counter1;
    uint8_t counter2;
    int32_t counter3;
    int32_t counter4;
    int32_t counter5;
    uint8_t counter6;
    uint8_t hit_cooldown;
} BossState;

// Defined on object_state.c
uint8_t boss_hit_glowing();

#endif
