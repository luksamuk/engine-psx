#ifndef SCREENS_LEVEL_H
#define SCREENS_LEVEL_H

#include <stdint.h>

typedef enum {
    LEVEL_MODE_NORMAL,
    LEVEL_MODE_RECORD,
    LEVEL_MODE_DEMO,
} LEVELMODE;

void screen_level_load();
void screen_level_unload(void *);
void screen_level_update(void *);
void screen_level_draw(void *);

void    screen_level_setlevel(uint8_t menuchoice);
uint8_t screen_level_getlevel(void);
void    screen_level_setstate(uint8_t state);
uint8_t screen_level_getstate();
void    screen_level_setmode(LEVELMODE mode);

#endif
