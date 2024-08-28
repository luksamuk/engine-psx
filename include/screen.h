#ifndef SCREEN_H
#define SCREEN_H

typedef enum {
    SCREEN_DISCLAIMER,
    SCREEN_LEVELSELECT,
    SCREEN_LEVEL,
} ScreenIndex;

void scene_change(ScreenIndex scr);

void scene_load();
void scene_unload();
void scene_update();
void scene_draw();

#endif
