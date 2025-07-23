#include "sound.h"
#include "util.h"

#include <psxspu.h>
#include <psxapi.h>
#include <assert.h>
#include <stdlib.h>


// First 4KB of SPU RAM are reserved for capture buffers.
// psxspu additionally uploads a dummy sample (16 bytes) at 0x1000
// by default, so the samples must be placed after those.
#define SPU_ALLOC_START_ADDR 0x01010
#define SPU_ALLOC_MAX_ADDR   0x80000
#define SPU_DUMMY_BLOCK_ADDR 0x01000

// So for the SPU, just like we're doing for primitives,
// we're going to have an arena allocator. In other words,
// we hold a pointer to the first available address and keep
// incrementing it; the only way to deallocate is by removing
// everything (in this case, resetting this pointer).
static uint32_t next_sample_addr = SPU_ALLOC_START_ADDR;

// Address of next channel for playing .VAG samples.
// Channels are always cycling.
#define MAX_CHANNELS 24
static int32_t next_channel = 0;

// Table of contents for playback of CD-DA audio
#define MAX_TOC_TRACKS 25
static volatile int32_t cdda_toc_size;
static volatile CdlLOC  cdda_toc[MAX_TOC_TRACKS];
static volatile uint8_t cdda_current_track;
static volatile uint8_t cdda_track_loops;

// Volume levels
static volatile uint16_t volume_master = 0;
static volatile uint16_t volume_cdda   = 0;
static volatile uint16_t volume_vag    = VAG_DEFAULT_VOLUME;

void
_sound_reset_channels(void)
{
    SpuSetKey(0, 0x00ffffff);

    for (int i = 0; i < 24; i++) {
        SPU_CH_ADDR(i) = getSPUAddr(SPU_DUMMY_BLOCK_ADDR);
        SPU_CH_FREQ(i) = 0x1000;
    }

    SpuSetKey(1, 0x00ffffff);
}

void
sound_init(void)
{
    SpuInit();
    sound_reset_mem();
    _sound_reset_channels();
}

void
sound_reset_mem(void)
{
    next_sample_addr = SPU_ALLOC_START_ADDR;
}


uint32_t
sound_upload_vag(const uint32_t *data, uint32_t size)
{
    uint32_t addr = next_sample_addr;
    // Round size up to a multiple of 64, since DMA transfers
    // 64-byte packages at once.
    size = (size + 63) & ~63;

    // Crash if we're going beyond allowed!
    assert(addr + size <= SPU_ALLOC_MAX_ADDR);

    SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
    SpuSetTransferStartAddr(addr);

    SpuWrite(data, size);
    SpuIsTransferCompleted(SPU_TRANSFER_WAIT);

    next_sample_addr = addr + size;
    return addr;
}

SoundEffect
sound_load_vag(const char *filename)
{
    uint8_t *bytes;
    uint32_t length;
    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading VAG file %s from the CD.\n", filename);
        return (SoundEffect){ 0 };
    }

    const VAGHeader *hdr = (const VAGHeader *)bytes;
    const uint32_t *data = (const uint32_t *)(bytes + sizeof(VAGHeader));
    uint32_t sample_rate = __builtin_bswap32(hdr->sample_rate);
    uint32_t addr = sound_upload_vag(data, __builtin_bswap32(hdr->size));
    free(bytes);
    return (SoundEffect) { addr, sample_rate };
}

int32_t
_get_next_channel()
{
    int32_t ch = next_channel;
    next_channel = (ch + 1) % MAX_CHANNELS;
    return ch;
}

void
sound_play_vag(SoundEffect sfx, uint8_t loops)
{
    int ch = _get_next_channel();
    SpuSetKey(0, 1 << ch);

    // SPU expects sample rate to be in 4.12 fixed-point format
    // (with 1.0 = 44100 Hz), and the address must be in 8-byte
    // units.
    SPU_CH_FREQ(ch) = getSPUSampleRate(sfx.sample_rate);
    SPU_CH_ADDR(ch) = getSPUAddr(sfx.addr);

    // Set channel volume and ADSR parameters.
    // 0x80ff and 0x1fee are dummy values that disable ADSR envelope entirely.
    SPU_CH_VOL_L(ch) = volume_vag;
    SPU_CH_VOL_R(ch) = volume_vag;
    SPU_CH_ADSR1(ch) = 0x00ff;
    SPU_CH_ADSR2(ch) = 0x0000;

    // Looping threshold
    if(loops) SPU_CH_LOOP_ADDR(ch) = getSPUAddr(sfx.addr);

    // Start playback
    SpuSetKey(1, 1 << ch);
}


void
_cdda_loop_callback()
{
    if(!cdda_track_loops || (cdda_current_track > cdda_toc_size)) {
        CdControlF(CdlPause, 0);
        return;
    }
    CdControlF(CdlSetloc, (CdlLOC *)&cdda_toc[cdda_current_track]);
    CdControlF(CdlPlay, 0);
}

void
sound_master_set_volume(uint16_t volume)
{
    volume_master = volume;
    SpuSetCommonMasterVolume(volume, volume);
}

void
sound_cdda_set_volume(uint16_t volume)
{
    volume_cdda = volume;
    SpuSetCommonCDVolume(volume, volume);
}

void
sound_vag_set_volume(uint16_t volume)
{
    volume_vag = volume;
}

void
sound_cdda_set_mute(uint8_t state)
{
    CdControl(state ? CdlMute : CdlDemute, 0, 0);
    
}

void
sound_cdda_init()
{
    CdAutoPauseCallback(_cdda_loop_callback);

    sound_master_set_volume(BGM_DEFAULT_VOLUME);
    sound_cdda_set_volume(BGM_DEFAULT_VOLUME);

    while((cdda_toc_size = CdGetToc((CdlLOC *)cdda_toc)) == 0) {
        printf("Warning: CD TOC not found, retrying. Please insert a CD-DA disc..\n");
    }

    printf("Number of TOC entries: %d\n", cdda_toc_size);

    // Align locations
    for(uint8_t i = 0; i < cdda_toc_size; i++) {
        CdIntToPos(CdPosToInt((CdlLOC *)&cdda_toc[i]) - 74,
                   (CdlLOC *)&cdda_toc[i]);
    }
}

void
sound_cdda_play_track(uint8_t track, uint8_t loops)
{
    if(track > cdda_toc_size) {
        printf("Error: Cannot set audio track %02d\n", track);
        return;
    }

    // Mode: Report + CD-DA + Auto-pause callback
    uint8_t mode = CdlModeRept | CdlModeDA | CdlModeAP;

    cdda_current_track = track;
    cdda_track_loops = loops;
    CdSync(0, 0);
    CdControl(CdlSetmode, &mode, 0);
    CdControl(CdlSetloc, (CdlLOC *)&cdda_toc[cdda_current_track], 0);
    CdControl(CdlPlay, 0, 0);
}

void
sound_cdda_stop()
{
    CdControl(CdlPause, 0, 0);
    CdSync(0, 0);
    CdControl(CdlDemute, 0, 0);
    cdda_current_track = 0xff;
    cdda_track_loops = 0;
}

uint8_t
sound_cdda_get_num_tracks()
{
    return cdda_toc_size;
}

uint16_t
sound_master_get_volume()
{
    return volume_master;
}

uint16_t
sound_cdda_get_volume()
{
    return volume_cdda;
}

uint16_t
sound_vag_get_volume()
{
    return volume_vag;
}
