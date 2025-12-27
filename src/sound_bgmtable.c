#include "sound.h"

// References whether each BGM loops.
// See BGMOption definition for order of BGMs.

static uint8_t _bgm_loops[] = {
    0, // TITLESCREEN
    1, // SPEEDSHOES
    0, // LEVELSELECT
    1, // TESTLEVEL0
    1, // TESTLEVEL1
    1, // GREENHILL
    1, // SURELYWOOD
    1, // AMAZINGOCEAN
    1, // DAWNCANYON
    0, // LEVELCLEAR
    0, // CREDITS
    1, // BOSS
};

void
sound_bgm_play(BGMOption t)
{
    if(t >= BGM_NUM_SONGS) return;
    sound_cdda_play_track(t + 1, _bgm_loops[t]);
}
