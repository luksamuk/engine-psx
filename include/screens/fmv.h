#ifndef SCREENS_FMV_H
#define SCREENS_FMV_H

#include "screen.h"

void screen_fmv_load();
void screen_fmv_unload();
void screen_fmv_update();
void screen_fmv_draw();

void screen_fmv_set_next(ScreenIndex next);
void screen_fmv_set_path(const char *filepath);

#endif
