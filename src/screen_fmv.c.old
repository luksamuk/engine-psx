#include "screens/fmv.h"

#include <stdlib.h>
#include <psxcd.h>

#include "screen.h"
#include "mdec.h"

#define FMV_QUEUE_MAX 3

// Default screen to level select.
// Can't wait for hackers using ACE to manipulate this someday :)
static ScreenIndex next_screen = SCREEN_LEVELSELECT;

static FMVOption fmv_queue[FMV_QUEUE_MAX];
static uint8_t fmv_count = 0;

void screen_fmv_load() {}

void
screen_fmv_unload(void *)
{
    fmv_count = 0;
    // Since screen allocations are used for MDEC playback,
    // dispose of any used memory
    screen_free();
}

void
screen_fmv_update(void *)
{
    for(uint8_t i = 0; i < fmv_count; i++) {
        mdec_play(fmv_queue[i]);
    }
    scene_change(next_screen);
}

void screen_fmv_draw(void *) {}


void
screen_fmv_set_next(ScreenIndex next)
{
    next_screen = next;
}

void
screen_fmv_enqueue(FMVOption option)
{
    if(fmv_count < FMV_QUEUE_MAX) {
        fmv_queue[fmv_count] = option;
        fmv_count++;
    }
}

