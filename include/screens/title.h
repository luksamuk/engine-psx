#ifndef SCREENS_TITLE_H
#define SCREENS_TITLE_H

void screen_title_load();
void screen_title_unload(void *);
void screen_title_update(void *);
void screen_title_draw(void *);

void screen_title_reset_demo();
void screen_title_cycle_demo();

#endif
