// TIM format type definitions and constants

#ifndef TIM_TYPES_H
#define TIM_TYPES_H

#include <stdint.h>
#include "ps1_types.h"

// TIM magic number
#define TIM_MAGIC 0x00000010

// TIM flags - BPP mode (bits 0-1)
#define TIM_BPP_4BIT   0  // 4-bit indexed (16 colors)
#define TIM_BPP_8BIT   1  // 8-bit indexed (256 colors)
#define TIM_BPP_16BIT  2  // 16-bit direct (RGB555)
#define TIM_BPP_24BIT  3  // 24-bit direct (rare)

// TIM flags - CLUT flag (bit 3)
#define TIM_FLAG_HAS_CLUT 0x08

// Helper macros
#define TIM_GET_BPP(flags)    ((flags) & 0x03)
#define TIM_HAS_CLUT(flags)  (((flags) & TIM_FLAG_HAS_CLUT) != 0)

// Pixel data structure for different formats
typedef enum {
    PS1_PIXEL_FORMAT_1BIT,
    PS1_PIXEL_FORMAT_4BIT,
    PS1_PIXEL_FORMAT_8BIT,
    PS1_PIXEL_FORMAT_16BIT
} PS1PixelFormat;

// Complete TIM file structure
typedef struct {
    TIMFileHeader header;
    TIMBlockHeader clut_rect;      // CLUT position/dimensions (if present)
    PS1CLUT* palette;              // Parsed CLUT data
    TIMBlockHeader image_rect;     // Image position/dimensions
    uint8_t* image_data;           // Raw pixel/index data
    uint32_t width;                // Image width in pixels
    uint32_t height;               // Image height in pixels
    uint32_t bpp;                  // Bits per pixel
    bool has_clut;                 // Has CLUT flag
    uint32_t file_size;
    uint8_t* file_data;
} TIMFile;

#endif // TIM_TYPES_H