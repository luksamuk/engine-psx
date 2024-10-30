#include "mdec.h"
#include <psxpress.h>
#include <psxapi.h>
#include <psxetc.h>
#include <psxcd.h>
#include <stdlib.h>
#include <stdio.h>

#include "input.h"

#include "cd_callback.h"
#include "memalloc.h"

#define VRAM_X_COORD(x) ((x) * BLOCK_SIZE / 16)

// Since the stream context in this case can use approx. 320K of RAM,
// it seems like a better choice to let it live on the heap.
// No need to manage this using an arena allocator, though.
static volatile StreamContext *str_ctx = NULL;

// MDEC lookup table, generally lives on the scratchpad
// Using v2 because it looks much better after AVI conversion!
static volatile VLC_TableV2   *lookup_table;

// Temporary area for CD sectors read from CD.
// Accessed by DMA, therefore must not be on heap
static volatile STR_Header    sector_header;

// STR file location on CD
static volatile CdlFILE       file;

static int decode_errors;
static int frame_time;

// Extern structure and functions related to render buffers
extern RenderContext ctx;
extern int debug_mode;
extern void render_mdec_prepare(void);
extern void render_mdec_dispose(void);

// Forward declarations
void         _mdec_dma_callback(void);
void         _mdec_cd_event_callback(CdlIntrResult, uint8_t *);
void         _mdec_cd_sector_handler(void);
StreamBuffer *_mdec_get_next_frame(void);
int          _mdec_should_abort();
void         _mdec_swap_buffers();

void
mdec_play(const char *filepath)
{
    mdec_start(filepath);
    mdec_loop();
    mdec_stop();
}

void
mdec_start(const char *filepath)
{
    printf("Preparing MDEC playback.\n");
    DecDCTReset(0);

    render_mdec_prepare();

    // Hook MDEC callbacks
    cd_set_callbacks(PLAYBACK_STR);

    // Copy MDEC lookup table to scratchpad
    lookup_table = fastalloc_malloc(sizeof(VLC_TableV2));
    DecDCTvlcCopyTableV2((VLC_TableV2 *)lookup_table);

    // Setup stream context
    if(!str_ctx) str_ctx = malloc(sizeof(StreamContext));
    str_ctx->cur_frame = 0;
    str_ctx->cur_slice = 0;

    decode_errors = 0;
    frame_time    = 1;
    
    // Find file on CD.
    // We won't be using the util.h library since we need to find
    // the actual file position to stream
    if(!CdSearchFile((CdlFILE *)&file, filepath)) {
        printf("Could not find .STR file %s.\n", filepath);
        // TODO: Halt forever?
        return;
    }

    // Prepare context
    str_ctx->frame_id       = -1;
    str_ctx->dropped_frames = 0;
    str_ctx->sector_pending = 0;
    str_ctx->frame_ready    = 0;

    CdSync(0, 0);

    // Read at 2x speed to play any XA-ADPCM sectors that could be
    // interleaved with the data
    // Start reading in real-time mode (doesn't retry in case of errors).
    uint8_t mode = CdlModeRT | CdlModeSpeed;
    CdControl(CdlSetmode, (const uint8_t *)&mode, 0);
    CdControl(CdlReadS, (const void *)&file.pos, 0);

    // Wait for first frame to be buffered.
    _mdec_get_next_frame();
}

void
mdec_stop()
{
    // Force playback end on context
    cd_detach_callbacks();
    str_ctx->frame_ready = -1;
    CdControlB(CdlPause, 0, 0);

    if(str_ctx) {
        free((void *)str_ctx);
        str_ctx = NULL;
    }
    fastalloc_free();
    render_mdec_dispose();
}

void
mdec_loop()
{
    int frame_start, decode_time, cpu_usage;
    frame_start = decode_time = cpu_usage = 0;

    printf("Starting MDEC playback.\n");
    while(1) {
        if(debug_mode) frame_start = TIMER_VALUE(1);

        // Wait for a full frame read from disc
        StreamBuffer *frame = _mdec_get_next_frame();
        if(!frame) {
            printf("MDEC playback ended\n");
            return;
        }

        if(_mdec_should_abort()) {
            printf("MDEC playback aborted\n");
            return;
        }

        if((pad_pressing(PAD_L1) && pad_pressed(PAD_R1)) ||
           (pad_pressed(PAD_L1) && pad_pressing(PAD_R1))) {
            debug_mode = (debug_mode + 1) % 3;
        }

        if(debug_mode) decode_time = TIMER_VALUE(1);

        // Decompress bitstream into a format expected by the MDEC
        VLC_Context vlc_ctx;
        if(DecDCTvlcStart(
               &vlc_ctx,
               frame->mdec_data,
               sizeof(frame->mdec_data) / 4,
               frame->bs_data))
        {
            decode_errors++;
            printf("Error decoding FMV! (#%d)\n", decode_errors);
            continue;
        }

        if(debug_mode) {
            decode_time = (TIMER_VALUE(1) - decode_time) & 0xffff;
            cpu_usage   = decode_time * 100 / frame_time;
        }

        // Wait for MDEC to finish decoding the previous frame
        // and manually flip the framebuffers
        VSync(0);
        DecDCTinSync(0);
        DecDCToutSync(0);

        if(debug_mode) {
            // Draw overlay
            FntPrint(-1, "FRAME:%6d      READ ERRORS:  %6d\n",
                     str_ctx->frame_id, str_ctx->dropped_frames);
            FntPrint(-1, "CPU:  %6d%%     DECODE ERRORS:%6d\n",
                     cpu_usage, decode_errors);
            FntFlush(-1);
        }

        // Manual buffer swap
        _mdec_swap_buffers();

        // Feed newly compressed frame to the MDEC.
        // The MDEC will not actually start decoding it until an output
        // buffer is also configured by calling DecDCTout().
        //DecDCTin(frame->mdec_data, DECDCT_MODE_24BPP);
        DecDCTin(frame->mdec_data, DECDCT_MODE_16BPP);

        // Place frame at the center of the currently active framebuffer,
        // then start decoding the first slice.
        // Decoded slices will be uploaded to VRAM in the background
        // by _mdec_dma_callback().
        RECT *clip = render_get_buffer_clip();
        int offsetx = (clip->w - frame->width) >> 1;
        int offsety = (clip->h - frame->height) >> 1;

        str_ctx->slice_pos.x = clip->x + VRAM_X_COORD(offsetx);
        str_ctx->slice_pos.y = clip->y + offsety;
        str_ctx->slice_pos.w = BLOCK_SIZE;
        str_ctx->slice_pos.h = frame->height;
        str_ctx->frame_width = VRAM_X_COORD(frame->width);

        DecDCTout(
            (uint32_t *)str_ctx->slices[str_ctx->cur_slice],
            BLOCK_SIZE * str_ctx->slice_pos.h / 2);

        if(debug_mode) frame_time = (TIMER_VALUE(1) - frame_start) & 0xffff;
    }
}

/* ========================================== */

// DMA callback for MDEC transfers.
// NOTE: This is leveraged from cd_callback.h
void
_mdec_dma_callback(void)
{
    // Handle sectors that were not processed by _mdec_cd_sector_handler
    // while a DMA transfer is in progress.
    // As the MDEC has just finished decoding a slice, they can be
    // safely handled now
    if(str_ctx->sector_pending) {
        _mdec_cd_sector_handler();
        str_ctx->sector_pending = 0;
    }

    // Upload the decoded slice to VRAM and start decoding
    // the next slice into another buffer, if any
    LoadImage((const RECT *)&str_ctx->slice_pos, (const uint32_t *)str_ctx->slices[str_ctx->cur_slice]);

    str_ctx->cur_slice ^= 0x1;
    str_ctx->slice_pos.x += BLOCK_SIZE;

    if(str_ctx->slice_pos.x < str_ctx->frame_width) {
        DecDCTout(
            (uint32_t *)str_ctx->slices[str_ctx->cur_slice],
            BLOCK_SIZE * str_ctx->slice_pos.h / 2);
    }
}

// CD event callback for FMV reading.
// NOTE: This is leveraged from cd_callback.h
void
_mdec_cd_event_callback(CdlIntrResult event, uint8_t *)
{
    // Ignore events that are not that of a sector being ready
    if(event != CdlDataReady) return;

    // Only handle sectors immediately if the MDEC is not decoding
    // a frame, otherwise defer handling to _mdec_dma_callback.
    // This is a workaround for a hardware conflict between the
    // DMA channels used for the CD drive and MDEC output, which
    // do not run simultaneously.
    if(DecDCTinSync(1)) str_ctx->sector_pending = 1;
    else _mdec_cd_sector_handler();
    
}

// CD sector handler for processing sectors that were read from the CD.
void
_mdec_cd_sector_handler(void)
{
    volatile StreamBuffer *frame = &str_ctx->frames[str_ctx->cur_frame];

    // Fetch .STR header of the sector that has been read and
    // make sure it is valid. If not, assume the file has ended
    // and set frame_ready as a signal for the playback loop to
    // stop playback.
    CdGetSector((void *)&sector_header, sizeof(STR_Header) >> 2);
    if(sector_header.magic != 0x0160) {
        str_ctx->frame_ready = -1;
        return;
    }

    // Ignore any non-MDEC sectors.
    if(sector_header.type != 0x8001) return;

    // If this sector is part of a new frame, validate the sectors
    // that have been read so far and flip the bistream data buffers.
    // If the frame number is lower than the current one, assume the
    // drive has started another .STR file and stop playback.
    if((int)sector_header.frame_id < str_ctx->frame_id) {
        str_ctx->frame_ready = -1;
        return;
    }

    if((int)sector_header.frame_id > str_ctx->frame_id) {
        // Do not set ready flag if a sector has been missed.
        if(str_ctx->sector_count) str_ctx->dropped_frames++;
        else str_ctx->frame_ready = 1;

        str_ctx->frame_id     = sector_header.frame_id;
        str_ctx->sector_count = sector_header.sector_count;
        str_ctx->cur_frame ^= 0x1;

        frame = &str_ctx->frames[str_ctx->cur_frame];

        // Initialize the next frame; round up dimensions
        // to the nearest multiple of 16 since the MDEC
        // operates on 16x16 blocks
        frame->width  = (sector_header.width  + 15) & 0xfff0;
        frame->height = (sector_header.height + 15) & 0xfff0;
    }

    // Append payload contained in this sector to the current buffer
    str_ctx->sector_count--;
    CdGetSector(
        (void *)&(frame->bs_data[2016 / 4 * sector_header.sector_id]),
        2016 / 4);
}

StreamBuffer *
_mdec_get_next_frame(void)
{
    while(!str_ctx->frame_ready) {
        __asm__ volatile("");
        if(_mdec_should_abort())
            return NULL;
    }

    if(str_ctx->frame_ready < 0) {
        return NULL;
    }

    str_ctx->frame_ready = 0;
    return (StreamBuffer *)&str_ctx->frames[str_ctx->cur_frame ^ 0x1];
}


int
_mdec_should_abort()
{
    // Call this when the CPU isn't doing anything or
    // should check for user input.
    pad_update();
    return pad_pressed(PAD_CROSS) || pad_pressed(PAD_START);
}

void
_mdec_swap_buffers()
{
    ctx.active_buffer ^= 1;
    DrawSync(0);
    PutDrawEnv(&ctx.buffers[ctx.active_buffer].draw_env);
    PutDispEnv(&ctx.buffers[ctx.active_buffer ^ 1].disp_env);
    SetDispMask(1);
}
