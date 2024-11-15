#include "sound.h"
#include "util.h"
#include "cd_callback.h"

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

// Used by the CD handler callback as a temporary area for
// sectors read from the CD. Due to DMA limitations, it can't
// be allocated on the stack -- especially not in the interrupt
// callback's stack, whose size is very limited.
/* static XACDSector _sector; */

// Current .XA audio data start location.
static volatile CdlLOC   _xa_loc;
static volatile int      _xa_should_play = 0;
static volatile uint32_t _cd_status = 0;
static volatile uint32_t _xa_loopback_sector = 0;

// Read error threshold. If surpasses the limit, restart the music.
#define CD_MAX_ERR_THRESHOLD 10
static uint8_t _cd_err_threshold = 0;

// Elapsed CD sectors from start of .XA playback
static uint32_t _cd_elapsed_sectors = 0;

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
    sound_xa_set_volume(XA_DEFAULT_VOLUME);
}

void
sound_reset_mem(void)
{
    next_sample_addr = SPU_ALLOC_START_ADDR;
}

void
sound_update(void)
{
    CdControl(CdlNop, 0, 0);
    _cd_status = CdStatus();
}

uint32_t
sound_get_cd_status(void)
{
    return _cd_status;
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
    SPU_CH_VOL_L(ch) = 0x3fff;
    SPU_CH_VOL_R(ch) = 0x3fff;
    SPU_CH_ADSR1(ch) = 0x00ff;
    SPU_CH_ADSR2(ch) = 0x0000;

    // Looping threshold
    if(loops) SPU_CH_LOOP_ADDR(ch) = getSPUAddr(sfx.addr);

    // Start playback
    SpuSetKey(1, 1 << ch);
}


void _xacd_event_callback(CdlIntrResult, uint8_t *);

void
sound_play_xa(const char *filename, int double_speed,
              uint8_t channel, uint32_t loopback_sector)
{
    CdlFILE file;
    CdlFILTER filter;

    // Stop sound if playing. We'll need the CD right now
    sound_stop_xa();
    
    if(!CdSearchFile(&file, filename)) {
        printf("Could not find .XA file %s.\n", filename);
        return;
    }

    int sectorn = CdPosToInt(&file.pos);
    int numsectors = (file.size + 2047) / 2048;
    printf("%s: Sector %d (size: %d => %d sectors).\n",
           filename, sectorn, file.size, numsectors);

    // Save current file location
    _xa_loc = file.pos;

    // Hook .XA callback for auto stop/loop
    cd_set_callbacks(PLAYBACK_XA);

    // Set read mode for XA streaming (send XA to SPU, enable filter)
    uint32_t mode = CdlModeRT | CdlModeSF | CdlModeAP;
    if(double_speed) mode |= CdlModeSpeed;
    CdControl(CdlSetmode, &mode, 0);

    filter.file = 1;
    filter.chan = channel;

    // Set CD volume to 50%
    sound_xa_set_volume(XA_DEFAULT_VOLUME);

    _cd_elapsed_sectors = 0;
    _xa_loopback_sector = loopback_sector;
    CdControl(CdlSetfilter, &filter, 0);
    CdControlF(CdlReadS, (const void *)&_xa_loc);
    _xa_should_play = 1;
}

// Callback for XA audio playback.
// NOTE: This is leveraged from cd_callback.h, but is declared here
void
_xa_cd_event_callback(CdlIntrResult event, uint8_t * /* payload */)
{   
    switch(event) {
    case CdlDataReady:
        _cd_err_threshold = 0; // Reset error threshold
        _cd_elapsed_sectors++;
        if((_xa_loopback_sector > 0) && (_cd_elapsed_sectors > _xa_loopback_sector)) {
            // Loop back to beginning
            _cd_err_threshold = 0;
            _cd_elapsed_sectors = 0;
            CdControlF(CdlReadS, (const void *)&_xa_loc);
        }
        break;
    case CdlDiskError:
        printf("Caught CD error\n");
        _cd_err_threshold++;
        if(_cd_err_threshold > CD_MAX_ERR_THRESHOLD) {
            // Stop music if too many errs
            _cd_err_threshold = 0;
            _cd_elapsed_sectors = 0;
            printf("Too many CD errors -- stop playback!\n");
            CdControlF(CdlPause, 0);
            _xa_should_play = 0;
            _xa_loopback_sector = 0;
        }
        break;
    default:
        printf("Event: %d\n", event);
        break;
    };
}


void
sound_stop_xa(void)
{
    // It is more desirable to use a pause command instead
    // of a full stop. Halting the playback completely also
    // stops CD from spinning and may increase time until
    // next playback
    if(_xa_should_play) CdControl(CdlPause, 0, 0);
    _xa_should_play = 0;
    _xa_loopback_sector = 0;
}

void
sound_xa_set_channel(uint8_t channel)
{
    // Seamlessly change channel
    CdlFILTER filter;
    filter.file = 1;
    filter.chan = channel;
    CdControl(CdlSetfilter, &filter, 0);
}

void
sound_xa_get_pos(uint8_t *minute, uint8_t *second, uint8_t *sector)
{
    static CdlLOCINFOL info;
    if(_cd_status != 0x22) {
        if(minute) *minute = 0;
        if(second) *second = 0;
        if(sector) *sector = 0;
        return;
    }

    CdControl(CdlGetlocL, 0, (uint8_t *)&info);
    CdSync(0, 0);
    if(minute) *minute = BCD_TO_DEC(info.minute);
    if(second) *second = BCD_TO_DEC(info.second);
    if(sector) *sector = info.sector;
}

void
sound_xa_get_elapsed_sectors(uint32_t *out)
{
    *out = _cd_elapsed_sectors;
}

void
sound_xa_set_volume(uint8_t vol)
{
    CdlATV v;
    // Stereo
    /* v.val0 = v.val3 = vol; // L->L and R->R volumes */
    /* v.val1 = v.val2 = 0x00; // L->R and R->L volumes */

    // Reversed stereo
    /* v.val0 = v.val3 = 0x00; // L->L and R->R volumes */
    /* v.val1 = v.val2 = vol; // L->R and R->L volumes */

    // Mono
    v.val0 = v.val3 = vol >> 1; // L->L and R->R volumes
    v.val1 = v.val2 = vol >> 1; // L->R and R->L volume
    
    CdMix(&v);
    CdSync(0, 0);
}
