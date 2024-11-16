#include "sound.h"

const BGMTableEntry bgm_table[] = {
    // Order MUST FOLLOW BGMTableEntry.
    {"\\BGM\\MNU001.XA;1",   0, 0},    // Level select
    {"\\BGM\\MNU001.XA;1",   1, 0},    // Title
    {"\\BGM\\MNU001.XA;1",   2, 0},    // Credits
    {"\\BGM\\EVENT001.XA;1", 0, 0},    // Level clear
    {"\\BGM\\BGM001.XA;1",   0, 4000}, // PZ1
    {"\\BGM\\BGM001.XA;1",   1, 4800}, // PZ2
    {"\\BGM\\BGM001.XA;1",   2, 3230}, // PZ3
    {"\\BGM\\BGM002.XA;1",   0, 4050}, // PZ4
    {"\\BGM\\BGM002.XA;1",   1, 4050}, // GHZ
    {"\\BGM\\BGM002.XA;1",   2, 3250}, // SWZ
};

void
sound_bgm_play(BGMOption t)
{
    sound_play_xa(
        bgm_table[t].filename,
        0,
        bgm_table[t].channel,
        bgm_table[t].loopback_sector);
}


const BGMTableEntry *
sound_bgm_get_data(BGMOption t)
{
    return &bgm_table[t];
}
