#include "screens/fmv.h"

#include <stdlib.h>

#include "screen.h"
#include "mdec.h"

// Default screen to level select.
// Can't wait for hackers using ACE to manipulate this someday :)
static ScreenIndex next_screen = SCREEN_LEVELSELECT;
static const char *fmv_path = NULL;

void screen_fmv_load() {}

void screen_fmv_unload() {}

void screen_fmv_update()
{
    mdec_play(fmv_path); // TODO: Interrupt playback?
    scene_change(next_screen);
}

void screen_fmv_draw() {}


void screen_fmv_set_next(ScreenIndex next)
{
    next_screen = next;
}

void screen_fmv_set_path(const char *filepath)
{
    fmv_path = filepath;
}


