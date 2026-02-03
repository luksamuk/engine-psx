// TIM format type definitions and constants

#ifndef TIM_TYPES_H
#define TIM_TYPES_H

#include <stdint.h>
#include "ps1_types.h"

// TIM signature (little-endian: 0x014C = ASCII 'L')
#define TIM_SIGNATURE 0x014C

// TIM image format codes
#define TIM_FORMAT_CLUT_RAW_16BIT 0x00
#define TIM_FORMAT_CLUT_RAW_4BIT  0x01
#define TIM_FORMAT_CLUT_RAW_8BIT  0x02
#define TIM_FORMAT_CLUT_RAW_1BIT  0x03
#define TIM_FORMAT_VQ_CLUT_16BIT  0x04
#define TIM_FORMAT_VQ_CLUT_4BIT   0x05
#define TIM_FORMAT_VQ_CLUT_8BIT   0x06
#define TIM_FORMAT_VQ_CLUT_1BIT   0x07

// TIM palette format codes
#define TIM_PALETTE_DIRECT 0x00
#define TIM_PALETTE_INDIRECT 0x10
#define TIM_PALETTE_COMPRESSED 0x20

// TIM compression structures
typedef struct {
    uint8_t code1;
    uint8_t code2;
    uint16_t offset;
    uint8_t clut_index;
} TIMVQCode;

// Complete TIM file structure
typedef struct {
    TIMFileHeader header;
    PS1CLUT* palette;
    uint8_t* image_data;
    uint32_t width;
    uint32_t height;
    TIMCompression compression;
    uint32_t file_size;
    uint8_t* file_data;
} TIMFile;

// Pixel data structure for different formats
typedef enum {
    PS1_PIXEL_FORMAT_1BIT,
    PS1_PIXEL_FORMAT_4BIT,
    PS1_PIXEL_FORMAT_8BIT,
    PS1_PIXEL_FORMAT_16BIT
} PS1PixelFormat;

// VQ codebook entry
typedef struct {
    uint8_t* block_data;
    uint8_t* codebook;
    int block_size;
} VQCodebookEntry;

#endif // TIM_TYPES_H