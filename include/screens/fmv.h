#ifndef SCREENS_FMV_H
#define SCREENS_FMV_H

#include "screen.h"

void screen_fmv_load();
void screen_fmv_unload(void *);
void screen_fmv_update(void *);
void screen_fmv_draw(void *);

void screen_fmv_set_next(ScreenIndex next);
void screen_fmv_enqueue(const char *filepath);

#endif
