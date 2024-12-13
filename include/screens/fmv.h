#ifndef SCREENS_FMV_H
#define SCREENS_FMV_H

#include "screen.h"

typedef enum {
    FMV_SONICTEAM  = 0,
    FMV_PS30YRS    = 1,
    FMV_NUM_VIDEOS = FMV_PS30YRS + 1,
} FMVOption;

void screen_fmv_load();
void screen_fmv_unload(void *);
void screen_fmv_update(void *);
void screen_fmv_draw(void *);

void screen_fmv_set_next(ScreenIndex next);
void screen_fmv_enqueue(FMVOption);

#endif
