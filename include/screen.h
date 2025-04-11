#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

typedef enum {
    SCREEN_DISCLAIMER,
    SCREEN_LEVELSELECT,
    SCREEN_LEVEL,
    SCREEN_TITLE,
    SCREEN_MODELTEST,
    SCREEN_SLIDE,
    SCREEN_CREDITS,
    SCREEN_SPRITETEST,
    SCREEN_CHARSELECT,
} ScreenIndex;

void scene_change(ScreenIndex scr);

void scene_init();
void scene_load();
void scene_unload();
void scene_update();
void scene_draw();

void *screen_alloc(uint32_t size);
void screen_free();
void *screen_get_data();

void render_loading_logo();

#endif
