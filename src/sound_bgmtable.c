#include "sound.h"

const BGMTableEntry bgm_table[] = {
    // Order MUST FOLLOW BGMTableEntry.
    // Filename - Channel - Loop sector - Stop sector
    {"\\BGM\\MNU001.XA;1",   0, 0, 1550}, // Level select
    {"\\BGM\\MNU001.XA;1",   1, 0, 450},  // Title
    {"\\BGM\\MNU001.XA;1",   2, 0, 4850}, // Credits

    {"\\BGM\\MNU002.XA;1",   0, 0, 1300}, // Speed shoes

    {"\\BGM\\EVENT001.XA;1", 0, 0, 300},  // Level clear

    {"\\BGM\\BGM001.XA;1",   0, 4000, 0}, // PZ1
    {"\\BGM\\BGM001.XA;1",   1, 4800, 0}, // PZ2
    {"\\BGM\\BGM001.XA;1",   2, 3230, 0}, // PZ3

    {"\\BGM\\BGM002.XA;1",   0, 4050, 0}, // PZ4
    {"\\BGM\\BGM002.XA;1",   1, 4050, 0}, // GHZ
    {"\\BGM\\BGM002.XA;1",   2, 3250, 0}, // SWZ

    {"\\BGM\\BGM003.XA;1",   0, 3900, 0}, // DCZ
    {"\\BGM\\BGM003.XA;1",   2, 7420, 0}, // A0Z
    {"\\BGM\\BGM003.XA;1",   1, 2830, 0}, // EZ
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

void
sound_bgm_check_stop(BGMOption t)
{
    if(sound_xa_requested_play() && (bgm_table[t].stop_sector != 0)) {
        uint32_t elapsed_sectors;
        sound_xa_get_elapsed_sectors(&elapsed_sectors);
        if(elapsed_sectors >= bgm_table[t].stop_sector) sound_stop_xa();
    }
}

const BGMTableEntry *
sound_bgm_get_data(BGMOption t)
{
    return &bgm_table[t];
}
