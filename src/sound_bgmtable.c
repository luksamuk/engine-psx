#include "sound.h"

// References whether each BGM loops.
// See BGMOption definition for order of BGMs.
static uint8_t _bgm_loops[] = {
    0,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    1,
};

void
sound_bgm_play(BGMOption t)
{
    if(t >= BGM_NUM_SONGS) return;
    sound_cdda_play_track(t + 1, _bgm_loops[t]);
}
