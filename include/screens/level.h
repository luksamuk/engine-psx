#ifndef SCREENS_LEVEL_H
#define SCREENS_LEVEL_H

#include <stdint.h>

void screen_level_load();
void screen_level_unload();
void screen_level_update();
void screen_level_draw();

void screen_level_setlevel(uint8_t menuchoice);

#endif
