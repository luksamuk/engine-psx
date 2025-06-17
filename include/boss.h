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
} BossState;

#endif
