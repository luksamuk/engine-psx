#ifndef SCREENS_SLIDE_H
#define SCREENS_SLIDE_H

typedef enum {
    SLIDE_COMINGSOON = 0,
    SLIDE_THANKS = 1,
    SLIDE_SEGALOGO = 2,
    SLIDE_CREATEDBY = 3,
    SLIDE_SAGE2025 = 4,

    SLIDE_NUM_SLIDES = (SLIDE_SAGE2025 + 1),
} SlideOption;

void screen_slide_load();
void screen_slide_unload(void *);
void screen_slide_update(void *);
void screen_slide_draw(void *);

void screen_slide_set_next(SlideOption);
#endif
