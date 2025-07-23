#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include <psxcd.h>

#define BGM_MAX_VOLUME      0x3fff
#define VAG_MAX_VOLUME      0x3fff
#define VAG_DEFAULT_VOLUME  0x1fff
#define BGM_DEFAULT_VOLUME  BGM_MAX_VOLUME
#define BGM_DEFAULT_IS_STEREO 1

// .VAG audio header
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t interleave;    // Unused in mono
    uint32_t size;          // Big-endian, bytes
    uint32_t sample_rate;   // Big-endian, Hz
    uint16_t _reserved[5];
    uint16_t channels;      // Unused in mono
    char     name[16];
} VAGHeader;

// Struct holding information on an uploaded sample.
typedef struct {
    uint32_t addr;
    uint32_t sample_rate;
} SoundEffect;

// "VAGp" text
#define VAG_MAGIC 0x70474156

void sound_init(void);
void sound_reset_mem(void);

SoundEffect sound_load_vag(const char *filename);
uint32_t    sound_upload_vag(const uint32_t *data, uint32_t size);
void        sound_play_vag(SoundEffect sfx, uint8_t loops);

/* CD-DA direct playback functions */
void    sound_cdda_init();
void    sound_cdda_play_track(uint8_t track, uint8_t loops);
void    sound_cdda_stop();
void    sound_cdda_set_mute(uint8_t state);
uint8_t sound_cdda_get_num_tracks();

/* Volume */
void    sound_master_set_volume(uint16_t volume);
void    sound_cdda_set_volume(uint16_t volume);
void    sound_vag_set_volume(uint16_t volume);

void    sound_cdda_set_stereo(uint8_t);
uint8_t sound_cdda_get_stereo();

uint16_t sound_master_get_volume();
uint16_t sound_cdda_get_volume();
uint16_t sound_vag_get_volume();

/* BGM audio table */
/* typedef enum { */
/*     BGM_TITLESCREEN  = 0, */
/*     BGM_SPEEDSHOES   = 1, */
/*     BGM_LEVELSELECT  = 2, */
/*     BGM_PLAYGROUND1  = 3, */
/*     BGM_PLAYGROUND2  = 4, */
/*     BGM_PLAYGROUND3  = 5, */
/*     BGM_PLAYGROUND4  = 6, */
/*     BGM_GREENHILL    = 7, */
/*     BGM_SURELYWOOD   = 8, */
/*     BGM_DAWNCANYON   = 9, */
/*     BGM_EGGMANLAND   = 10, */
/*     BGM_AMAZINGOCEAN = 11, */
/*     BGM_WINDMILLISLE = 12, */
/*     BGM_LEVELCLEAR   = 13, */
/*     BGM_CREDITS      = 14, */
/*     BGM_BOSS         = 15, */

/*     BGM_NUM_SONGS    = BGM_BOSS + 1, */
/* } BGMOption; */

typedef enum {
    BGM_TITLESCREEN  = 0,
    BGM_SPEEDSHOES   = 1,
    BGM_LEVELSELECT  = 2,
    BGM_PLAYGROUND1  = 3,
    BGM_PLAYGROUND2  = 4,
    BGM_PLAYGROUND3  = 5,
    BGM_PLAYGROUND4  = 6,
    BGM_GREENHILL    = 7,
    BGM_SURELYWOOD   = 8,
    BGM_AMAZINGOCEAN = 9,
    BGM_LEVELCLEAR   = 10,
    BGM_CREDITS      = 11,
    BGM_BOSS         = 12,

    BGM_NUM_SONGS    = BGM_BOSS + 1,
} BGMOption;

void                sound_bgm_play(BGMOption);


/* SFX audio */
void  sound_sfx_init();

#endif
