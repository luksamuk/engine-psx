#ifndef MDEC_H
#define MDEC_H

#include "render.h"
#include <psxcd.h>

//#define BLOCK_SIZE 24
#define BLOCK_SIZE 16

typedef struct {
    uint16_t width, height;
    uint32_t bs_data[0x2000];   // Bitstream data read from the disc
    uint32_t mdec_data[0x8000]; // Decompressed data to be fed to the MDEC
} StreamBuffer;

typedef struct {
    StreamBuffer frames[2];
    uint32_t     slices[2][BLOCK_SIZE * SCREEN_YRES / 2];

    int  frame_id, sector_count;
    int  dropped_frames;
    RECT slice_pos;
    int  frame_width;

    volatile int8_t sector_pending, frame_ready;
    volatile int8_t cur_frame, cur_slice;
} StreamContext;


typedef struct {
    uint16_t magic;        // Always 0x0160
    uint16_t type;         // 0x8001 for MDEC
    uint16_t sector_id;    // Chunk number (0 = first chunk of this frame)
    uint16_t sector_count; // Total number of chunks for this frame
    uint32_t frame_id;	   // Frame number
    uint32_t bs_length;    // Total length of this frame in bytes

    uint16_t width, height;
    uint8_t  bs_header[8];
    uint32_t _reserved;
} STR_Header;

typedef enum {
    FMV_SONICTEAM  = 0,
    FMV_PS30YRS    = 1,
    FMV_NUM_VIDEOS = FMV_PS30YRS + 1,
} FMVOption;

// Load all locations of FMVs on disc
void mdec_fmv_init();

// Use this function as main playback entrypoint.
// It will override the game loop and be stuck in a playback loop.
// Do not call the other functions directly unless you know what you're doing.
void mdec_play(FMVOption);

void mdec_start(volatile CdlLOC *loc);
void mdec_stop();
void mdec_loop();

#endif
