#ifndef SCREENS_SLIDE_H
#define SCREENS_SLIDE_H

typedef enum {
    SLIDE_COMINGSOON = 0,
    SLIDE_SEGALOGO = 1,
    SLIDE_CREATEDBY = 2,
    SLIDE_SAGE2025 = 3,

    SLIDE_NUM_SLIDES = (SLIDE_SAGE2025 + 1),
} SlideOption;

void screen_slide_load();
void screen_slide_unload(void *);
void screen_slide_update(void *);
void screen_slide_draw(void *);

void screen_slide_set_next(SlideOption);
#endif
