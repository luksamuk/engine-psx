#include "sound.h"
#include "util.h"

#include <psxspu.h>
#include <psxapi.h>
#include <assert.h>

// First 4KB of SPU RAM are reserved for capture buffers.
// psxspu additionally uploads a dummy sample (16 bytes) at 0x1000
// by default, so the samples must be placed after those.
#define SPU_ALLOC_START_ADDR 0x01010
#define SPU_ALLOC_MAX_ADDR   0x80000

// So for the SPU, just like we're doing for primitives,
// we're going to have an arena allocator. In other words,
// we hold a pointer to the first available address and keep
// incrementing it; the only way to deallocate is by removing
// everything (in this case, resetting this pointer).
static uint32_t next_sample_addr = SPU_ALLOC_START_ADDR;

// Used by the CD handler callback as a temporary area for
// sectors read from the CD. Due to DMA limitations, it can't
// be allocated on the stack -- especially not in the interrupt
// callback's stack, whose size is very limited.
static XACDSector _sector;

// Current .XA audio data start location.
static CdlLOC   _xa_loc;
static int      _xa_should_play = 0;
static uint32_t _cd_status = 0;

void
sound_init(void)
{
    SpuInit();
    sound_reset_mem();
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
    if(_cd_status == 0 && _xa_should_play) {
        CdControl(CdlReadS, &_xa_loc, 0);
    }
}

uint32_t
sound_get_cd_status(void)
{
    return _cd_status;
}

uint32_t
sound_upload_sample(const uint32_t *data, uint32_t size)
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

void _xacd_event_callback(CdlIntrResult, uint8_t *);

void
sound_play_xa(const char *filename, int double_speed, uint8_t channel)
{
    CdlFILE file;
    CdlFILTER filter;
    
    if(!CdSearchFile(&file, filename)) {
        printf("Could not find .XA file %s.\n", filename);
        return;
    }

    int sectorn = CdPosToInt(&file.pos);
    printf("%s: Sector %d (size: %d).\n", filename, sectorn, file.size);

    // Save current file location
    _xa_loc = file.pos;

    // Hook .XA callback for auto stop/loop
    EnterCriticalSection();
    CdReadyCallback(_xacd_event_callback);
    ExitCriticalSection();

    // Set read mode for XA streaming (send XA to SPU, enable filter)
    uint32_t mode = CdlModeRT | CdlModeSF;
    if(double_speed) mode |= CdlModeSpeed;
    CdControl(CdlSetmode, &mode, 0);

    filter.file = 1;
    filter.chan = channel;

    CdControl(CdlSetfilter, &filter, 0);
    CdControl(CdlReadS, &_xa_loc, 0);
    _xa_should_play = 1;
}

// Callback for XA audio playback.
void
_xacd_event_callback(CdlIntrResult event, uint8_t * /* payload */)
{   
    // Only handle sector-ready events.
    //if(event != CdlDataReady) {
    //printf("Event: %d\n", event);
        //    return;
        //}

    /* // Fetch sector */
    /* CdGetSector(&_sector, sizeof(XACDSector) / 4); */

    /* // Check if it is part of the .XA audio file. */
    /* // If it isn't, restart playback. */
    /* // Be wary that the XA header has two copies of itself. */
    /* // See: https://problemkaputt.de/psx-spx.htm#cdromsectorencoding */
    /* if(!(_sector.xa_header[0].submode & XA_TYPE_AUDIO) && */
    /*    !(_sector.xa_header[1].submode & XA_TYPE_AUDIO)) { */
    /*     printf("Got submode %x / %x\n", _sector.xa_header[0].submode, _sector.xa_header[1].submode); */
    /*     //CdControlF(CdlReadS, &_xa_loc); */
    /* } */

    // End of playback is issuing an error, so...
    if(event != CdlDiskError) return;
    CdControlF(CdlReadS, &_xa_loc);
}


void
sound_stop_xa(void)
{
    // It is more desirable to use a pause command instead
    // of a full stop. Halting the playback completely also
    // stops CD from spinning and may increase time until
    // next playback
    _xa_should_play = 0;
    CdControl(CdlPause, 0, 0);
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
