#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include <psxcd.h>

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

// .XA audio header
typedef struct {
    uint8_t file;
    uint8_t channel;
    uint8_t submode;
    uint8_t coding_info;
} XAHeader;

// .XA audio submode flags
typedef enum {
    XA_END_OF_RECORD = 0x01,
    XA_TYPE_VIDEO    = 0x02,
    XA_TYPE_AUDIO    = 0x04,
    XA_TYPE_DATA     = 0x08,
    XA_TRIGGER       = 0x10,
    XA_FORM2         = 0x20,
    XA_REAL_TIME     = 0x40,
    XA_END_OF_FILE   = 0x80,
} XASubmode;

// CD-ROM Sector encoding (for .XA playback)
typedef struct {
    CdlLOC    pos;
    XAHeader  xa_header[2];
    uint8_t   data[2048];
    uint32_t  edc;
    uint8_t   ecc[276];
} XACDSector;

void sound_init(void);
void sound_reset_mem(void);
void sound_update(void);

SoundEffect sound_load_vag(const char *filename);
uint32_t    sound_upload_vag(const uint32_t *data, uint32_t size);
void        sound_play_vag(SoundEffect sfx, uint8_t loops);

uint32_t sound_get_cd_status(void);
void     sound_play_xa(const char *filename, int double_speed,
                       uint8_t channel, uint32_t loopback_sector);
void     sound_stop_xa(void);
void     sound_xa_set_channel(uint8_t channel);
void     sound_xa_get_pos(uint8_t *minute, uint8_t *second, uint8_t *sector);
void     sound_xa_get_elapsed_sectors(uint32_t *out);

#endif
